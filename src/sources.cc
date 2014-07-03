/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (radkovo@centrum.cz) 1998 - 2002

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   sources.cc

   Sources of events
   
   Last update 3.7.2002
  =========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <linux/ax25.h>

#include <utmp.h>

#include "version.h"
#include "calls.h"
#include "data.h"
#include "sources.h"
#include "tools.h"
#include "comp.h"

//uncomment this for event gate debugging
//#define GATE_DEBUG
//uncomment this for debugging events going through event gate
//#define GATE_EVENT_DEBUG

//This turns on experimental features
#define EXPERIMENTAL

#ifdef NO_SOCKLEN_T
typedef int socklen_t;
#endif

//Global input semaphore
//Any of these flags disables input on ALL channels
bool input_enabled[MAX_CHN+1];

//Waiting for application start
//Any of these flags disables input on ALL channels
//Activated by the application W flag (currently non documented function)
bool wait_app[MAX_CHN+1];

#ifdef GATE_DEBUG
char *strevent(const Event &ev)
{
  static char s[MAX_EVENT_DATA+64];
  sprintf(s, "Event %i chn=%i x=%i ", ev.type, ev.chn, ev.x);
  if (ev.type > 100 && ev.type < 200) strcat(s, (char *)ev.data);
  if (ev.type > 200 && ev.type < 300) strncat(s, (char *)ev.data, ev.x);
  return s;
}
#endif


//--------------------------------------------------------------------------
// Class Ax25io
//--------------------------------------------------------------------------
Ax25io::Ax25io(int num_chns)
{
  int i, j;

  strcpy(class_name, "Ax25io");
  if (get_port_data() == -1)
  {
    Error(errno, "Cannot open %s", AXPORTS);
    Message(errno, "Cannot open %s", AXPORTS);
    exit(1);
  }
  maxchn = num_chns;
  list = new int*[num_ports];
  for (i=0; i<num_ports; i++)
  {
    list[i] = new int[maxchn+1];
    for (j=0; j<=maxchn; j++) list[i][j] = -1;
  }

  for (i=0; i<=maxchn; i++)
  {
    setSConfig(i, "call", "");
    setSConfig(i, "cwit", "");
    setSConfig(i, "cphy", "");
    setIConfig(i, "port", 0);
    sock[i] = -1;
    in_cnt[i] = 0;
    out_cnt[i] = 0;
    flush[i] = false;
    strcpy(line[i], "");
    strcpy(orig_call[i], "");
    input_enabled[i] = true;
    wait_app[i] = false;
    wait_cnt[i] = 0;
    waiting_cnt[i] = false;
  }

  setSConfig("def_port", pname[0]); //1st port is default port
}

Ax25io::~Ax25io()
{
  int i;
  for (i=0; i<=maxchn; i++)
    if (sock[i] != -1) close(sock[i]);
}

void Ax25io::set_channel_call(int chn, char *callsign)
{
  int i, j, len;
  char path[30];
  struct full_sockaddr_ax25 addr;

  if (chn <= maxchn)
  {
    //Search for the same callsign in other channel
    for (i=maxchn, j=0; i>0; i--)
      if (i != chn && sconfig(i, "call") != NULL &&
          strcmp(sconfig(i, "call"), callsign) == 0 && list[0][i] != -1) j=i;
    if (bconfig("listen"))
      for (i=0; i<num_ports; i++)
        if (j==0) //no sockets for this callsign, create new ones
        {
          if (list[i][chn] != -1) close(list[i][chn]);
          list[i][chn] = socket(AF_AX25, SOCK_SEQPACKET, PF_AX25);
          sprintf(path, "%s %s", callsign, bcall[i]);
          len = convert_path(path, &addr);
          if (len == -1)
            Error(errno, "Ax25io: invalid path %s %s", callsign, bcall[i]);
          if (bind(list[i][chn], (struct sockaddr *)&addr, len) == -1)
            Error(errno, "Ax25io: cannot bind()");
          listen(list[i][chn], 5);
          //int en_digi = 1;
          //setsockopt(list[i][chn], SOL_AX25, AX25_IAMDIGI, &en_digi, sizeof(int));
        }
        else
        {
          if (list[i][chn] != -1) close(list[i][chn]);
          list[i][chn] = dup(list[i][j]); //duplicate only
        }
    setSConfig(chn, "call", callsign);
  }
}


int Ax25io::get_port_data()
{
  char buf[256];
  FILE *f;
  int i=0;

  f = fopen(AXPORTS, "r");
  if (f==NULL) return -1;
  while (!feof(f))
  {
    strcpy(buf, "");
    fgets(buf, 255, f);
    if (strlen(buf) != 0 && buf[0] != '#')
    {
      pname[i] = new char[16];
      bcall[i] = new char[10];
      int n = sscanf(buf, "%s %s %i %i %i", pname[i], bcall[i], &speed[i],
                                      &paclen[i], &win[i]);
      if (n == 5) i++;
    }
  }
  num_ports = i;
  setSConfig("unportname", pname[0]);
  setIConfig("unport", 0);
  fclose(f);
  return 0;
}

int Ax25io::socket_state(int s)
{
  ax25_info_struct info;
  if (ioctl(s, SIOCAX25GETINFO, &info) != -1) return info.state;
  return -1;
}

void Ax25io::handle_event(const Event &ev)
{
  static int count;

  if (ev.type == EV_NONE) //only when void-loop
  {
    count++;
    if (count > AX25_SCAN_CYCLES)
    {
      count=0;
      int chn;
      for(chn=maxchn; chn>0; chn--)
      {
        switch (iconfig(chn, "state"))
        {
          case ST_CONN:
            if (!CheckIncomming(chn))
              if (iconfig(chn, "state") != ST_CONN)
              {
                if (iconfig(chn, "state") == ST_TIME)
                  emit(chn, EV_DISC_TIME, 0, sconfig(chn, "cphy"));
                else
                  emit(chn, EV_DISC_FM, 0, sconfig(chn, "cphy"));
                ChangeStatus(chn, ST_DISC);
              }
            break;
          case ST_CONP:
            if (!CheckConnected(chn))
              if (iconfig(chn, "state") == ST_DISC)
                emit(chn, EV_FAILURE, 0, sconfig(chn, "cphy"));
            break;
          /*case ST_DISP:
          case ST_TIME: fprintf(stderr, "!"); break;*/
        }
      }
      CheckRequests(); //check connect requests on all channels
    }
  }

  if (ev.type == EV_CALL_CHANGE)
  {
    set_channel_call(ev.chn, reinterpret_cast<char *>(ev.data));
  }

  if (ev.type == EV_TEXT_COOKED        //transform TEXT to outgoing data
      && ((char *)ev.data)[0] != ':'   //do not print commands
      && ev.x != FLAG_FM_MACRO)          //these are redundant
  {
    //local copy of data for recoding
    char *data_line = new char[strlen((char *)ev.data) + 1];
    strcpy(data_line, (char *)ev.data);
    char *p = data_line;
    while (*p) {if (*p == '\n') *p = '\r'; p++;} //convert LF->CR

    if (ev.chn == 0) //channel 0 sends unproto frames
    {
       send_unproto(sconfig(0, "call"), sconfig(0, "cwit"), data_line);
    }
    else
    {
      int chn = ev.chn;
      int plen;
      if (sock[chn] == -1) plen = 256;
                      else plen = paclen[iconfig(chn, "port")];
      if (plen > 256) plen = 256;
      p = data_line;
      while (*p) //copy to output buffer
      {
        obuffer[chn][out_cnt[chn]] = *p;
        out_cnt[chn]++; p++;
        if (out_cnt[chn] >= plen) //output buffer is full ?
        {
          memcpy(s, obuffer[chn], plen);
          if (sock[chn] == -1) emit(chn, EV_LOCAL_MSG, plen, s);
                          else emit(chn, EV_DATA_OUTPUT, plen, s);
          out_cnt[chn] = 0;
        }
      }
      if (flush[chn]) {FlushBuffer(chn); flush[chn] = false;}
    }
    delete[] data_line;
  }

  if (ev.type == EV_TEXT_FLUSH)
  {
    if (ev.x == FLAG_FLUSH_IMMEDIATE) FlushBuffer(ev.chn);
    else flush[ev.chn] = true;
  }

  if (ev.type == EV_DATA_OUTPUT)
  {
     char *buf = new char[ev.x];
     memcpy(buf, ev.data, ev.x);
     int len = ev.x;

     if (huffman_comp[ev.chn])
     {
       char *cbuf = new char[len];
       int clen;
       if (encstathuf(buf, len, cbuf, &clen) == 0)
       {
         memcpy(buf, cbuf, clen);
         len = clen;
       }
       delete[] cbuf;
     }

     if (sock[ev.chn] != -1)
        write(sock[ev.chn], buf, len);

     delete[] buf;
  }

  if (ev.type == EV_CONN_LOC)
  {
     if (ev.chn == 0)
     {
       strcpy(s, "Connection not allowed on this channel");
       emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, s);
     }
     else if (sock[ev.chn] != -1)
     {
       strcpy(s, "Channel already connected");
       emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, s);
     }
     else
     {
       char path[30];
       struct full_sockaddr_ax25 addr, addrp;
       int len, lenp, pnum;
       int chn = ev.chn;
       char port[256];
       char *p;

       p = strchr((char *)ev.data, ':');
       if (p == NULL) {p = (char *)ev.data; strcpy(port, "");}
                 else {*p = '\0'; strcopy(port, (char *)ev.data, 256); *p = ':'; p++;}

       //look for a port
       pnum = -1;
       for (int i=0; i < num_ports; i++)
         if (pname[i] != NULL && strcmp(pname[i], port) == 0) pnum = i;
       if (pnum == -1)
       {
         sprintf(s, "Unknown port (%s)", port);
         emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, s);
         return;
       }

       //Try to find another connection with the same source and physical destination
       int i, j=0;
       for (i=1; i<=MAX_CHN; i++)
         if (i != chn && iconfig(i, "state") != ST_DISC
                      && call_match(sconfig(i, "call"), sconfig(chn, "call"))
                      && call_match(sconfig(i, "cphy"), p)) j=i;
     
       //If something has been found, change source SSID
       if (j != 0)
       {
         strcopy(orig_call[chn], sconfig(chn, "call"), 10);
         char newcall[10];
         sprintf(newcall, "%s-%i", call_call(sconfig(chn, "call")), chn);
         set_channel_call(chn, newcall);
       }

       //create new socket
       sock[ev.chn] = socket(AF_AX25, SOCK_SEQPACKET, PF_AX25);
       if (sock[ev.chn] == -1)
         Error(errno, "Ax25io: cannot create new socket");
       fcntl(sock[ev.chn], F_SETFL, O_NONBLOCK);
       sprintf(path, "%s %s", sconfig(ev.chn, "call"), bcall[pnum]);
       len = convert_path(path, &addr);
       if (len == -1)
         Error(errno, "Ax25io: invalid path %s %s", sconfig(ev.chn, "call"), bcall[pnum]);
       if (bind(sock[ev.chn], (struct sockaddr *)&addr, len) == -1)
         Error(errno, "Ax25io: cannot bind()");
       //connect new socket
       lenp = convert_path(p, &addrp);
       if (connect(sock[ev.chn], (struct sockaddr *)&addrp, lenp) == -1)
         if (errno != EINPROGRESS)
         {
           Error(errno, "Ax25io: cannot connect()");
           Message(errno, "Ax25io: cannot connect()");
         }
       //update status
       char s[256];
       norm_call(s, &addrp.fsa_ax25.sax25_call);
       setSConfig(ev.chn, "cwit", s);
       setSConfig(ev.chn, "cphy", s);
       ChangeStatus(ev.chn, ST_CONP);
     }
  }

  if (ev.type == EV_DISC_LOC)
  {
     if (sock[ev.chn] != -1)
     {
       if (out_cnt[ev.chn] > 0) //flush output buffer
       {
         write(sock[ev.chn], obuffer[ev.chn], out_cnt[ev.chn]);
         out_cnt[ev.chn] = 0;
       }
       close(sock[ev.chn]);
     }
  }

  if ((ev.type == EV_DISC_FM || ev.type == EV_DISC_TIME || ev.type == EV_FAILURE)
      && strlen(orig_call[ev.chn]) != 0)
  {
    set_channel_call(ev.chn, orig_call[ev.chn]);
    strcpy(orig_call[ev.chn], "");
  }

  if (ev.type == EV_UNPROTO_SRC)
  {
     setSConfig(0, "call", reinterpret_cast<char *>(ev.data));
  }

  if (ev.type == EV_UNPROTO_DEST)
  {
     setSConfig(0, "cwit", reinterpret_cast<char *>(ev.data));
     setSConfig(0, "cphy", reinterpret_cast<char *>(ev.data));
  }

  if (ev.type == EV_UNPROTO_PORT)
  {
    char *name = reinterpret_cast<char *>(ev.data);
    int i;
    for (i = 0; i < num_ports; i++)
      if (strcasecmp(name, pname[i]) == 0) break;
    if (i < num_ports)
    {
      setSConfig("unportname", name);
      setIConfig("unport", i);
    }
    else
    {
      char s[32];
      strcpy(s, "Unknown port name");
      emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, s);
    }
  }

  if (ev.type == EV_LISTEN_ON)
  {
    char s[10];
    for (int i = 1; i <= maxchn; i++)
    {
      strcpy(s, sconfig(i, "call"));
      set_channel_call(i, s);
    }
  }
  if (ev.type == EV_LISTEN_OFF)
  {
    for (int i = 0; i < num_ports; i++)
      for (int j = 1; j <= maxchn; j++)
        if (list[i][j] != -1)
        {
          close(list[i][j]);
          list[i][j] = -1;
        }
  }

  if (ev.type == EV_ENABLE_CHN) ch_disabled[ev.chn] = false;
  if (ev.type == EV_DISABLE_CHN) ch_disabled[ev.chn] = true;
  if (ev.type == EV_RX_CTL)
  {
      bool newval = ev.x;
      //output buffered data
      if (!input_enabled[ev.chn] && newval)
          OutputBuffer(ev.chn);
      input_enabled[ev.chn] = newval;
  }

  //Wait a moment after reconnect
  #ifndef EXPERIMENTAL
  if (ev.type == EV_RECONN_TO) wait_cnt[ev.chn] = CONN_WAIT_CYCLES;
  #else
  if (ev.type == EV_RECONN_TO) input_enabled[ev.chn] = false; //!!!
  #endif
}

void Ax25io::FlushBuffer(int chn)
{
  if (out_cnt[chn] > 0)
  {
    memcpy(s, obuffer[chn], out_cnt[chn]);
    if (sock[chn] == -1) emit(chn, EV_LOCAL_MSG, out_cnt[chn], s);
                    else emit(chn, EV_DATA_OUTPUT, out_cnt[chn], s);
    out_cnt[chn] = 0;
  }
}

void Ax25io::ChangeStatus(int chn, int status)
{
   setIConfig(chn, "state", status);
   emit(chn, EV_STATUS_CHANGE, status, NULL);
}

bool Ax25io::WaitingApp()
{
  for (int i = 0; i <= maxchn; i++)
    if (wait_app[i]) return true;
  return false;
}

bool Ax25io::InputEnabled()
{
  for (int i = 0; i <= maxchn; i++)
    if (!input_enabled[i]) return false;
  return true;
}

bool Ax25io::CheckRequests()
{
    fd_set list_rfds;
    struct timeval tv;
    int    rc, i, chn;

    int n  = 0;
    FD_ZERO(&list_rfds);
    for (chn = 1; chn <= maxchn; chn++)
      for (i = 0; i < num_ports; i++)
      {
        if (list[i][chn] != -1) FD_SET(list[i][chn], &list_rfds);
        if (list[i][chn] > n) n = list[i][chn];
      }

    if (n == 0) return false;

    tv.tv_sec = 0;
    tv.tv_usec = SELECT_TIME;
    rc = select(n+1, &list_rfds, NULL, NULL, &tv);
    
    if (rc > 0)
      for (chn = 1; chn <= maxchn; chn++)
        for (i=0; i<num_ports; i++)
          if (list[i][chn] != -1 && FD_ISSET(list[i][chn], &list_rfds))
          {
            struct full_sockaddr_ax25 addr;
            int ii=1; ioctl(list[i][chn], FIONBIO, &ii);
            socklen_t addr_len = sizeof(addr);
            int newsock = accept(list[i][chn], (struct sockaddr *)&addr, &addr_len);
            ii=0; ioctl(list[i][chn], FIONBIO, &ii);
            if (newsock > 0)
            {
              //some station connected
              char newcall[256];
              norm_call(newcall, &addr.fsa_ax25.sax25_call);
              fcntl(newsock, F_SETFL, O_NONBLOCK);
              //find first free channel with this call
              int j = 0;
              for (int ii = maxchn; ii > 0; ii--)
                if (sock[ii] == -1
                    && strcmp(sconfig(ii, "call"), sconfig(chn, "call")) == 0
                    && !ch_disabled[ii]) j = ii;
              if (j == 0)
              {
                 char str[256];
                 sprintf(str, "Hello, this is %s version %s\r"
                              "There are no free channels for the callsign %s.\r"
                              "Please try to connect later.\r",
                              PACKAGE, VERSION, sconfig(chn, "call"));
                 write(newsock, str, strlen(str));
                 norm_call(s, &addr.fsa_ax25.sax25_call);
                 emit(chn, EV_CONN_REQ, 0, s);
                 close(newsock);
              }
              else
              {
                 //int newchn = (j == 0) ? chn : j;
                 int newchn = j; //free channel found
                 if (iconfig(chn, "state") == ST_CONP) newchn = chn;
                 sock[newchn] = newsock;
                 setSConfig(newchn, "cwit", newcall);
                 setSConfig(newchn, "cphy", newcall);
                 ChangeStatus(newchn, ST_CONN);
                 setIConfig(newchn, "port", i);
                 emit(newchn, EV_CONN_TO, 1, sconfig(newchn, "cwit"));
                 #ifndef EXPERIMENRAL
                 wait_cnt[newchn] = CONN_WAIT_CYCLES;
                 #else
                 input_enabled[newchn] = false; //!!!
                 #endif
                 in_cnt[newchn] = 0;
                 return true;
              }
            }
          }
    return false;
}

bool Ax25io::CheckIncomming(int chn)
{
  if (sock[chn] == -1 || !InputEnabled() || WaitingApp()) return false;
  if (wait_cnt[chn] > 0)
  {
    wait_cnt[chn]--;
    waiting_cnt[chn] = true;
    return false;
  }
  else
    if (waiting_cnt[chn]) //output the data read after connection
    {
      waiting_cnt[chn] = false;
      OutputBuffer(chn);
    }
    
  int rc = read(sock[chn], buffer[chn], 256);
  if (rc == -1)
  {
     if (errno==EAGAIN) return false;
     if (errno==ETIMEDOUT) ChangeStatus(chn, ST_TIME);
                      else ChangeStatus(chn, ST_DISP);
     close(sock[chn]);
     sock[chn] = -1;
     return false;
  }
  if (rc == 0)
  {
     close(sock[chn]);
     sock[chn] = -1;
     ChangeStatus(chn, ST_DISP);
     return false;
  }

  //decompress huffman
  if (huffman_comp[chn] && rc > 0)
  {
    char comp_src[257];
    char comp_dest[257];
    memcpy(&comp_src[1], buffer[chn], rc);
    comp_src[0] = (char) rc - 1;
    if (decstathuf(comp_src, comp_dest) == 0)
    {
      rc = (int) comp_dest[0] + 1;
      memcpy(buffer[chn], &comp_dest[1], rc);
    }
    //else Error(0, "huffman failed");
  }

  in_cnt[chn] = rc;
  OutputBuffer(chn);
  return true;
}

bool Ax25io::CheckConnected(int chn)
{
  if (sock[chn] == -1) return false;
  int rc = read(sock[chn], buffer[chn], 256);
  if (rc == -1)
  {
    if (errno == ENOTCONN)
    {
       ax25_info_struct ax25_info;
       int r = ioctl(sock[chn], SIOCAX25GETINFO, &ax25_info);
       if ( r != 0 || ax25_info.state != 0) return false;
       else //connection timeout
       {
          ChangeStatus(chn, ST_DISC);
          close(sock[chn]);
          sock[chn] = -1;
          return false;
       }
    }
    else if (errno == EAGAIN) rc = 0; //connected but no data
    else
    {
      ChangeStatus(chn, ST_DISC);
      close(sock[chn]);
      sock[chn] = -1;
      return false;
    }
  }

  //decompress huffman
  if (huffman_comp[chn] && rc > 0)
  {
    char comp_src[257];
    char comp_dest[257];
    memcpy(&comp_src[1], buffer[chn], rc);
    comp_src[0] = (char) rc - 1;
    if (decstathuf(comp_src, comp_dest) == 0)
    {
      rc = (int) comp_dest[0] + 1;
      memcpy(buffer[chn], &comp_dest[1], rc);
    }
  }

  ChangeStatus(chn, ST_CONN);
  emit(chn, EV_CONN_TO, 0, sconfig(chn, "cwit"));
  #ifndef EXPERIMENTAL
  wait_cnt[chn] = CONN_WAIT_CYCLES; //wait after connect
  #else
  input_enabled[chn] = false; //!!!
  #endif
  //the buffer will be send out after waiting
  in_cnt[chn] = rc;
  return true;
}

void Ax25io::OutputBuffer(int chn)
{
  if (in_cnt[chn] > 0)
  {
    //Recieve lines
    int i;
    for(i = 0; i < in_cnt[chn]; i++)
      if (buffer[chn][i] == '\r')
      {
        strcpy(sline[chn], line[chn]);
        strcpy(line[chn], "");
        emit(chn, EV_LINE_RECV, 0, sline[chn]);
      }
      else
        if (strlen(line[chn]) < 255)
          strncat(line[chn], &buffer[chn][i], 1);
    //Emit the whole buffer
    emit(chn, EV_DATA_INPUT, in_cnt[chn], buffer[chn]);
    in_cnt[chn] = 0;
  }
}

void Ax25io::send_unproto(char *src, char *dest, char *msg)
{
  int s;
  char path[30];
  struct full_sockaddr_ax25 addr;
  int len;

  if ((s = socket(AF_AX25, SOCK_DGRAM, 0)) == -1)
     Error(errno, "Cannot create socekt for unproto");
  sprintf(path, "%s %s", src, bcall[iconfig("unport")]);
  len = convert_path(path, &addr);
  if (len == -1)
    Error(errno, "Ax25io: invalid path %s %s", src, bcall[iconfig("unport")]);
  if (bind(s, (struct sockaddr *)&addr, len) == -1)
     Error(errno, "Cannot bind() socket for unproto");
  len = convert_path(dest, &addr);
  if (len == -1)
    Error(errno, "Ax25io: invalid path %s %s", dest, bcall[iconfig("unport")]);
  if (sendto(s, msg, strlen(msg), 0, (struct sockaddr *)&addr, len) == -1)
     Error(errno, "Cannot sendto() unproto message");
  close(s);
}

//--------------------------------------------------------------------------
// Class ExternCmd
//--------------------------------------------------------------------------
ExternCmd::ExternCmd(int ch, char *cmd, bool remote_cmd, int ret_hnd, char *flags)
{
  strcpy(class_name, "Extern");
  chn = ch;
  remc = remote_cmd;
  rhnd = ret_hnd;
  crlf = (strchr(flags, 'B') == NULL);
  doslf = (strchr(flags, 'D') != NULL);
  console = (strchr(flags, 'C') != NULL);
  doubleio = (strchr(flags, 'S') != NULL);
  waitfor = (strchr(flags, 'W') != NULL);
  strcpy(result, "");
  sync_on = false;
  signal(SIGPIPE, SIG_IGN);
  if (pipe(pip_in) == -1) Error(errno, "ExternCmd: pipe_in: cannot pipe\n");
  if (pipe(pip_out) == -1) Error(errno, "ExternCmd: pipe_out: cannot pipe\n");
  int newpid = fork();
  if (newpid < 0) Error(errno, "ExternCmd: fork: cannot fork\n");
  if (newpid == 0) //Child process
  {
    dup2(pip_in[0], 0);
    if (console) dup2(pip_out[1], 2); //stderr
    else if (doubleio) {dup2(pip_out[1], 1); dup2(pip_out[1], 2);} //both
    else dup2(pip_out[1], 1); //stdout
    close(pip_in[1]);
    close(pip_out[0]);
    //close all not used parent's descriptors
    for (int i = 3; i < 2048; i++)
      if (i != pip_in[0] && i != pip_out[1]) close(i);
    //run application
    char lpchn[10];
    sprintf(lpchn, "%i", ch);
    setenv("LPCHN", lpchn, 1); //set the LPCHN variable to actual channel
    execlp("/bin/sh", "/bin/sh", "-c", cmd, NULL);
    exit(1);
  }
  if (newpid > 0)  //Parent process
  {
    pid = newpid;
    close(pip_in[0]);
    close(pip_out[1]);
    emit(chn, EV_APP_STARTED, pid, remc?FLAG_APP_REMOTE:FLAG_APP_LOCAL, this);
    sprintf(class_name, "Extern%i", pid);
    if (waitfor) wait_app[chn] = true; //start waiting for application
    int ret;
    do
      ret = fcntl(pip_out[0], F_SETFL, O_NONBLOCK);
    while (ret == -1 && errno == EAGAIN);
    if (ret == -1) Error(errno, "ExternCmd(%i): Cannot set pipe flags", pid);
    #ifdef GATE_DEBUG
    fprintf(stderr, "Application %i : %s\n", pid, cmd);
    #endif
  }
}

ExternCmd::~ExternCmd()
{
   kill(pid, SIGTERM);
   close(pip_out[0]);
   close(pip_in[1]);
}

void ExternCmd::handle_event(const Event &ev)
{
   if (ev.type == EV_NONE)
     if (pid > 0)
     {
       int rc, rrc;
       char pbuf[256];
       rrc = read(pip_out[0], pbuf, 256);
       if (rrc > 0)
       {
           if (doslf)
           {
               rc = 0;
               for (int i = 0; i < rrc; i++)
                       if (pbuf[i] != '\r') buf[rc++] = pbuf[i];
           }
           else if (crlf)
           {
               for (int i = 0; i < rrc; i++)
                       if (pbuf[i] == '\n') buf[i] = '\r';
                       else buf[i] = pbuf[i];
               rc = rrc;
           }
           else
           {
               memcpy(buf, pbuf, rrc);
               rc = rrc;
           }
       }
       else rc = rrc;
           
       if (rc > 0)
       {
         if (remc) emit(chn, EV_DATA_OUTPUT, rc, buf);
              else emit(chn, EV_DATA_INPUT, rc, buf);
         wait_app[chn] = false; //data received - application is running
       }
       else if (rc == 0)
       {
         emit(chn, EV_REMOVE_OBJ, oid, this);
         emit(chn, EV_PROCESS_FINISHED, pid, NULL);
         int i;
         waitpid(pid, &i, WNOHANG);
         close(pip_out[0]);
         close(pip_in[1]);
         if (strlen(result) > 0)
           emit(chn, EV_CMD_RESULT, rhnd, result);
       }
     }
 
   if (((remc && ev.type == EV_DATA_INPUT) ||
        (!remc && (ev.type == EV_DATA_OUTPUT || ev.type == EV_LOCAL_MSG)))
       && ev.chn == chn
       && !sync_on)
   {
      send_stream_data(ev);
   }
 
   if (ev.type == EV_APP_RESULT && ev.x == pid)
   {
      strcpy(result, reinterpret_cast<char *>(ev.data));
   }
 
   if (ev.type == EV_QUIT)
   {
      kill(pid, SIGTERM);
   }
}

//This function is called by EventGate after any of the events below is
//succesfully sent to an application and acknowledged. After this the
//data is sent to the application via stdin so the stream is synchronized
//with the events. When sync_on is false this function takes no effect
//and the data is sent just after receiving the data event in handle_event().
void ExternCmd::handle_event_sync(const Event &ev)
{
   if (((remc && ev.type == EV_DATA_INPUT) ||
        (!remc && (ev.type == EV_DATA_OUTPUT || ev.type == EV_LOCAL_MSG)))
       && ev.chn == chn
       && sync_on)
   {
      send_stream_data(ev);
   }
}

void ExternCmd::send_stream_data(const Event &ev)
{
    char *data = new char[ev.x];
    memcpy(data, ev.data, ev.x);
    int pcnt = ev.x;
    
    if (crlf)
    {
        char *p = data;
        for (int i = 0; i < ev.x; i++, p++)
                if (*p == '\r') *p='\n';
    }
    /*else if (doslf)
    {
        pcnt = 0;
        char *p = ev.data;
        for (int i = 0; i < ev.x; i++, p++)
                if (*p != '\r') data[pcnt++] = *p;
    }*/

    int cnt = write(pip_in[1], data, pcnt);
    if (cnt != pcnt)
      if (errno == EAGAIN)
        fprintf(stderr, "ExternCmd: application pid %i not responding\n", pid);
      else
        Error(errno, "Error while sending data to application pid %i", pid);
    delete[] data;
}

//--------------------------------------------------------------------------
// Class EventGate
//--------------------------------------------------------------------------
EventGate::EventGate(int socknum, EventGate *parent, std::vector <tapp> *ppids)
{
  sprintf(class_name, "EventGt?");
  rbuff = NULL;
  rbufsize = 0;
  wbuff = NULL;
  wbufsize = 0;
  app_busy = false;
  app_died = false;
  sig_on = false;
  remote = false;
  sync_on = false;
  sync_with = NULL;
  clrevent(latest_event);
  created = time(NULL);
  chn = 0;
  parent_gate = parent;
  end_round = true;

  if (socknum == 0)
  {
     pid = 0; //we are the base gate
     pids = new std::vector<tapp>;
     
     sock = socket(AF_INET, SOCK_STREAM, 0);
     if (sock == -1) Error(errno, "EventGate: cannot create socket");
     //int readr = 1;
     //setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &readr, sizeof(int));
     
     struct sockaddr_in addr;
     addr.sin_family = AF_INET;
     addr.sin_port = htons(API_PORT);
     addr.sin_addr.s_addr = INADDR_ANY;
     if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
        Error(errno, "EventGate: cannot bind()");
     if (listen(sock, 32) == -1)
        Error(errno, "EventGate: cannot listen()");
  }
  else
  {
     pid = -1; //we're going to get the PID from the application
     sock = socknum;
     pids = ppids;
  }

  if (sock != -1)
  {
     FD_ZERO(&rfds);
     FD_SET(sock, &rfds);
     maxdesc = sock;
   
     int ret;
     do
       ret = fcntl(sock, F_SETFL, O_NONBLOCK);
     while (ret == -1 && errno == EAGAIN);
     if (ret == -1) Error(errno, "EventGate: Cannot set socket flags");
  }
  #ifdef GATE_DEBUG
  fprintf(stderr, "Created gate %i\n", pid);
  #endif
}

EventGate::~EventGate()
{
  if (sock != -1) close(sock);
}

void EventGate::add_wbuffer(const char *data, size_t count)
{
  if (wbuff == NULL || wbufsize == 0)
  {
    wbuff = new char[count];
    memcpy(wbuff, data, count);
    wbufsize = count;
  }
  else
  {
    char *p = wbuff;
    wbuff = new char[wbufsize+count];
    memcpy(wbuff, p, wbufsize);
    memcpy(wbuff+wbufsize, data, count);
    wbufsize += count;
    delete[] p;
  }
}

void EventGate::add_rbuffer(const char *data, size_t count)
{
  if (rbuff == NULL || rbufsize == 0)
  {
    rbuff = new char[count];
    memcpy(rbuff, data, count);
    rbufsize = count;
  }
  else
  {
    char *p = rbuff;
    rbuff = new char[rbufsize+count];
    memcpy(rbuff, p, rbufsize);
    memcpy(rbuff+rbufsize, data, count);
    rbufsize += count;
    delete[] p;
  }
}

bool EventGate::flush_wbuffer(int fd)
{
  if (wbufsize == 0 && fd == -1) return true;
  else
  {
    int w;
    w = write(fd, wbuff, wbufsize);
    if (w == -1)             //couldn't write anything
      return false;
    else if (w == wbufsize)  //all data written
    {
      delete[] wbuff;
      wbuff = NULL;
      wbufsize = 0;
      return true;
    }
    else                     //some data written
    {
      char *p = wbuff;
      int size = wbufsize;
      wbuff = NULL;
      add_wbuffer(p + w, size - w); //new buffer with the rest of data
      delete[] p;
      return false;
    }
  }
  return false;
}

int EventGate::write_app(int fd, const void *buf, size_t count)
{
  int w;

  if (fd == -1) return count;
  if (!write_error)
  {
    w = write(fd, buf, count);
    if (w != (int)count) //couldn't write the whole buffer
    {
      if (w < 0)
      {
         w = 0;
         if (errno != EINTR && errno != EAGAIN) app_died = true;
      }
      char *p = (char *)buf;
      add_wbuffer(p + w, count - w); //store the rest
      write_error = true;
    }
    w = -1;
  }
  else
  {
    add_wbuffer((char *)buf, count);
    w = -1; //write failed recently - do not continue writing
  }
  return w;
}

int EventGate::read_app(int fd, void *buf, size_t count)
{
  int r;
  if (app_died || fd == -1) return 0;

  r = read(fd, buf, count);
  if (r == 0) app_died = true;
  return r;
}

bool EventGate::parse_event_cmd()
{
   if (rbufsize == 0) return false;
   int index = 0;

   //read identification
   Event re;
   char t = rbuff[index]; index++;

   if (t == '\0') //event follows
   {
       int len;
       
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&re.type, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&re.chn, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&re.x, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&re.y, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&len, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (len == -1)
       {
          if (index + (int)sizeof(void *) > rbufsize) return false;
          memcpy(&re.data, &rbuff[index], sizeof(void *)); index += sizeof(void *);
       }
       else if (len > 0)
       {
          if (index + len > rbufsize) return false;
          re.data = &rbuff[index]; index += len;
       }
       if (re.type < 300) //ignore events with pointer data
       {
          if (own_handle_event(re)) emit(re);
       }
   }
   else
   {
       int cmd, data;

       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&cmd, &rbuff[index], sizeof(int)); index += sizeof(int);
       if (index + (int)sizeof(int) > rbufsize) return false;
       memcpy(&data, &rbuff[index], sizeof(int)); index += sizeof(int);
       interpret_command(cmd, data);
   }
   rbufsize -= index;
   //cut parsed data from buffer
   if (rbufsize > 0)
   {
      char *newbuf = new char[rbufsize];
      memcpy(newbuf, &rbuff[index], rbufsize);
      delete[] rbuff;
      rbuff = newbuf;
   }
   else
   {
      delete[] rbuff;
      rbuff = NULL;
   }
   return true;
}

void EventGate::serialize(const Event &ev, char **buffer, int *size)
{
  int len;
  int blen;
  int bpos;
  char *buf;
  
  if (ev.type >=   0 && ev.type < 100) len = 0;
  if (ev.type >= 100 && ev.type < 200) len = strlen((char *)ev.data) + 1;
  if (ev.type >= 200 && ev.type < 300) len = ev.x;
  if (ev.type >= 400) len = 0;

  blen = len + 5*sizeof(int) + 1; //data + fixed values + 1b header
  buf = new char[blen];
  buf[0] = '\0'; //event introduction
  bpos = 1;
  memcpy(buf+bpos, &ev.type, sizeof(int)); bpos += sizeof(int);
  memcpy(buf+bpos, &ev.chn, sizeof(int)); bpos += sizeof(int);
  memcpy(buf+bpos, &ev.x, sizeof(int)); bpos += sizeof(int);
  memcpy(buf+bpos, &ev.y, sizeof(int)); bpos += sizeof(int);
  memcpy(buf+bpos, &len, sizeof(int)); bpos += sizeof(int);
  if (len > 0) memcpy(buf+bpos, ev.data, len);
  
  *buffer = buf;
  *size = blen;
}

bool EventGate::send_event(const Event &ev)
{
  if (sock == -1) return false;
  if (!flush_wbuffer(sock)) return false; //finish previous writes

  write_error = false; //clear the error

  char *buf;
  int size;
  serialize(ev, &buf, &size);
  write_app(sock, buf, size);
  delete[] buf;

  return true; //event is written or stored in buffer
}

bool EventGate::send_command(int cmd, int data)
{
  if (sock == -1) return false;
  if (!flush_wbuffer(sock)) return false; //finish previous writes

  write_error = false; //clear the error

  char *buf = new char[2*sizeof(int) + 1];
  buf[0] = 1; //command introduction
  memcpy(buf+1, &cmd, sizeof(int));
  memcpy(buf+1+sizeof(int), &data, sizeof(int));
  write_app(sock, buf, 2*sizeof(int) + 1);
  delete[] buf;

  return true; //event is written or stored in buffer
}

void EventGate::set_sock(int thesock)
{
   sock = thesock;
   FD_ZERO(&rfds);
   FD_SET(sock, &rfds);
   maxdesc = sock;
}

void EventGate::set_pid(int thepid)
{
   pid = thepid;
   sprintf(class_name, "EventGt%i", pid);
}

void EventGate::data_arrived()
{
   #ifdef GATE_DEBUG
   fprintf(stderr, "Data arrived: pid=%i\n", pid);
   #endif
   if (pid == 0) //base gate - create a new one
   {
      struct sockaddr_in addr;
      socklen_t len = sizeof(struct sockaddr_in);
      int newsock = accept(sock, (struct sockaddr *)&addr, &len);
      if (newsock == -1)
         Error(errno, "EventGate: cannot accept() connection");

      EventGate *newgate = new EventGate(newsock, this, pids);
      emit(0, EV_INSERT_OBJ, 0, newgate);
      children.push_back(newgate);
      FD_SET(newsock, &rfds);
      if (newsock > maxdesc) maxdesc = newsock;
      //newgate->data_arrived();
   }
   else //regular gate
   {
      char buffer[1024];
      int rc = read_app(sock, buffer, 1024);
      if (rc > 0) add_rbuffer(buffer, rc);
      if (app_died) end_work("Connection lost (eof)");
   }
}

//accept the gate creation request
void EventGate::accept_request()
{
   if (sock != -1)
   {
      Event ev;
      if (remote)
      {
         ev.chn = chn; ev.type = EV_APP_STREMOTE; ev.x = pid; ev.data = NULL;
         while (!send_event(ev)) ;
      }
      ev.chn = chn; ev.type = EV_GATE_FINISHED; ev.x = pid; ev.data = NULL;
      while (!send_event(ev)) ;
   }
}

//returns true if the event should be sent to all (public)
bool EventGate::own_handle_event(const Event &ev)
{
   //creating new gate -- public
   if (ev.type == EV_CREATE_GATE)
   {
      #ifdef GATE_DEBUG
      fprintf(stderr, "%i: app introduction from %i\n", pid, ev.x);
      #endif

      bool fnd = false;
      bool cont = true;
      std::vector <tapp>::iterator it;
      for (it = pids->begin(); it < pids->end(); it++)
         if (it->pid == ev.x)
         {
            if (it->gate) //another gate exists
            {
               #ifdef GATE_DEBUG
               fprintf(stderr, "Pre-created gate found\n");
               #endif
               it->gate->set_sock(dup(sock));
               parent_gate->change_child(this, it->gate);
               it->gate->accept_request();
               it->gate = NULL; //the gate is no more waiting
               pid = it->pid;
               chn = it->chn;
               remote = it->remote;
               //sock = -1; //stop listening
               //emit(0, EV_REMOVE_OBJ, oid, this); //cancel this gate
               end_work("Pre-created gate found");
               cont = false;
            }
            else //pre-created gate doesn't exist more
            {
               #ifdef GATE_DEBUG
               fprintf(stderr, "No gate exists\n");
               #endif
               chn = it->chn;
               pid = it->pid;
               remote = it->remote;
            }
            fnd = true;
            break;
         }

      //applications not executed by LinPac
      if (!fnd) { pid = ev.x; chn = 0; remote = false; }
         
      sprintf(class_name, "EventGt%ix", pid);
      if (cont) accept_request();
      return true;
   }

   //Start event synchronization -- public
   if (ev.type == EV_SYNC_REQUEST)
   {
      #ifdef GATE_DEBUG
      fprintf(stderr, "%i: sync request, chns = %i\n", pid, iconfig("maxchn"));
      #endif

      Event ev;
      int chns = iconfig("maxchn");

      ev.chn = 0;
      ev.type = EV_SYNC_DONE;
      ev.x = chns;
      ev.data = NULL;
      queue.fstore(ev);
      
      for (int i = 0; i <= chns; i++)
      {
         scanVarNames(i);
         while (char *name = nextVarName())
         {
            char *value = get_var(i, name);
            #ifdef GATE_DEBUG
            fprintf(stderr, "Variable sync: %s = %s\n", name, value);
            #endif
            int bsize = strlen(name)+strlen(value)+2;
            char *buf = new char[bsize];
            strcpy(buf, name);
            strcpy(buf+strlen(name)+1, value);
            ev.chn = i;
            ev.type = EV_VAR_CHANGED;
            ev.x = bsize;
            ev.data = buf;
            queue.fstore(ev);
            delete[] buf;
         }
         releaseVarNames();
      }

      ev.chn = 0;
      ev.type = EV_VAR_SYNC;
      ev.x = chns;
      ev.data = NULL;
      queue.fstore(ev);

      if (queue.empty()) fprintf(stderr, "empty!\n");
      return true;
   }

   //Variable changed in application -- private
   if (ev.type == EV_VAR_CHANGED)
   {
      char *name = (char *) ev.data;
      char *value = name + strlen(name) + 1;
      set_var(ev.chn, name, value);
      return false;
   }

   //Variable destroyed in application -- private
   if (ev.type == EV_VAR_DESTROYED)
   {
      char *name = (char *)ev.data;
      del_var(ev.chn, name);
      return false;
   }

   return true;
}

void EventGate::handle_event(const Event &ev)
{
  if (ev.type == EV_NONE)
  {
    if (pid == 0) //for base gate only
    {
       struct timeval tv;
       int rc;
       fd_set rdset;
       
       tv.tv_sec = 0;
       tv.tv_usec = 100;
       memcpy(&rdset, &rfds, sizeof(fd_set));
       FD_SET(sock, &rdset); if (maxdesc < sock) maxdesc = sock;
       rc = select(maxdesc+1, &rdset, NULL, NULL, &tv);
       if (rc>0)
       {
          if (FD_ISSET(sock, &rdset)) data_arrived();
          std::vector <EventGate *>::iterator it;
          for (it = children.begin(); it < children.end(); it++)
             if (FD_ISSET((*it)->get_sock(), &rdset))
                (*it)->data_arrived();
       }
    }

    //parse received events
    while (parse_event_cmd());
    
    //try to send events from queue
    if (!queue.empty() && (!app_busy || !sig_on))
    {
       bool sent = false;
       while (!queue.empty() && send_event(queue.top()))
       {
          #ifdef GATE_EVENT_DEBUG
          fprintf(stderr, "%i: TX %s\n", pid, strevent(queue.top()));
          #endif
          //remember data events for synchronization
          copy_event(latest_event, queue.top());
          queue.pop();
          sent = true; end_round = false;
          if (latest_event.type != EV_DATA_INPUT &&
              latest_event.type != EV_DATA_OUTPUT &&
              latest_event.type != EV_LOCAL_MSG) clrevent(latest_event);
          else break;
       }
       if (sent)
       {
          if (sig_on) app_busy = true;
          if (send_command(100, sig_on?1:0))
          {
             end_round = true; //end of event serie
             //send the USR1 signal
             if (pid != 0 && sig_on) kill(pid, SIGUSR1);
             #ifdef GATE_EVENT_DEBUG
             fprintf(stderr, "%i: signal sent\n", pid);
             #endif
          }
       }
    }
    else
    {
       if (!end_round)
          if (send_command(100, sig_on?1:0))
          {
             end_round = true; //end of event serie
             if (pid != 0 && sig_on) kill(pid, SIGUSR1);
          }
       flush_wbuffer(sock); //nothing to send, try to complete previous writes
    }
    
    //check waiting gates
    std::vector <tapp>::iterator it;
    for (it = pids->begin(); it < pids->end(); it++)
       if (it->gate && time(NULL) > it->started + MAX_GATE_WAIT)
       { //waiting too long - application probably won't start
          emit(0, EV_REMOVE_OBJ, 0, it->gate);
          it->gate = NULL; //!!! or erase this entry completely???!!!
       }
    
  }
  else
  if (ev.type == EV_REMOVE_GATE && pid == ev.x)
  {
    end_work("EV_REMOVE_GATE received");
  }
  else
  if (ev.type == EV_PROCESS_FINISHED && pid == ev.x)
  {
    end_work("EV_PROCESS_FINISHED received");
  }
  else
  if (ev.type == EV_PROCESS_FINISHED && pid == 0)
  {
    //remove the application entry from PID list
    std::vector <tapp>::iterator it;
    for (it = pids->begin(); it < pids->end(); it++)
       if (it->pid == ev.x) {pids->erase(it); break;}
  }
  else if (pid > 0 && ev.type < 300) //do not send events with pointer data
  {
    #ifdef GATE_EVENT_DEBUG
    fprintf(stderr, "%s: RX %s\n", class_name, strevent(ev));
    #endif
    queue.store(ev);

    if ((!app_busy || !sig_on) && send_event(queue.top()))
    {
       #ifdef GATE_EVENT_DEBUG
       fprintf(stderr, "%s: TX %s\n", class_name, strevent(queue.top()));
       #endif
       end_round = false;
       if (send_command(100, sig_on?1:0))
       {
          end_round = true; //end of event serie
          //send the USR1 signal
          if (pid != 0 && sig_on) kill(pid, SIGUSR1);
          #ifdef GATE_EVENT_DEBUG
          fprintf(stderr, "%i: signal sent(2)\n", pid);
          #endif
       }
       if (sig_on) app_busy = true;
       //remember data events for synchronization
       copy_event(latest_event, queue.top());
       queue.pop();
       if (latest_event.type != EV_DATA_INPUT &&
           latest_event.type != EV_DATA_OUTPUT &&
           latest_event.type != EV_LOCAL_MSG) clrevent(latest_event);
       
    }
    else  //no response - is the application still working ?!
    {
       if (queue.length() > MAX_EVENT_QUEUE_LENGTH)
       {
          Error(0, "Application PID %i: event queue exceeded %i events - terminating it",
                pid, MAX_EVENT_QUEUE_LENGTH);
          kill(pid, SIGTERM);
          end_work("queue overflow");
       }
       else if (queue.length() > 0 && queue.last_pop() > MAX_RESPONSE_TIME)
       {
          Error(0, "Application PID %i hasn't responded for %i seconds - terminating it",
                pid, MAX_RESPONSE_TIME);
          kill(pid, SIGTERM);
          end_work("queue timeout");
       }
    }
  }

  if (ev.type == EV_APP_STARTED && pid == 0)
  {
     tapp app;
     app.pid = ev.x;
     app.chn = ev.chn;
     app.remote = (ev.y == FLAG_APP_REMOTE);
     //create new gate which will wait for the application
     EventGate *newgate = new EventGate(-1, this, pids);
     newgate->set_pid(app.pid);
     newgate->set_chn(app.chn);
     newgate->set_remote(app.remote);
     newgate->set_sync_with((ExternCmd *) ev.data);
     emit(0, EV_INSERT_OBJ, 0, newgate);

     app.gate = newgate;
     app.started = time(NULL);
     pids->push_back(app);
  }
}

void EventGate::interpret_command(int cmd, int data)
{
  switch (cmd)
  {
    //command 0: application acknowledges event reception
    case 0: app_busy = false;
            if (sync_on && sync_with && latest_event.type != 0)
            {
               sync_with->handle_event_sync(latest_event);
               clrevent(latest_event);
            }
            #ifdef GATE_EVENT_DEBUG
            fprintf(stderr, "%i: ack\n", pid);
            #endif
            break;
    //command 1: switch signal sending on/off
    case 1: sig_on = (data != 0);
            #ifdef GATE_EVENT_DEBUG
            fprintf(stderr, "%i: sig %i\n", pid, data);
            #endif
            break;
    //command 2: switch ExternCmd/EventGate synchronization on/off
    case 2: sync_on = (data != 0);
            if (sync_with) sync_with->set_sync(sync_on);
            #ifdef GATE_EVENT_DEBUG
            fprintf(stderr, "%i: sync %i\n", pid, data);
            #endif
            break;
  }
}

void EventGate::child_finished(EventGate *child)
{
   std::vector <EventGate *>::iterator it;
   maxdesc = 0;
   for (it = children.begin(); it < children.end(); it++)
      if (*it == child)
      {
         FD_CLR((*it)->get_sock(), &rfds);
         children.erase(it);
      }
      else
         if ((*it)->get_sock() > maxdesc) maxdesc = (*it)->get_sock();
}

void EventGate::change_child(EventGate *from, EventGate *to)
{
   std::vector <EventGate *>::iterator it;
   maxdesc = 0;
   for (it = children.begin(); it < children.end(); it++)
      if (*it == from)
      {
         #ifdef GATE_DEBUG
         fprintf(stderr, "Child changed from %i to %i\n",
                 (*it)->get_pid(), to->get_pid());
         #endif
         *it = to;
         break;
      }
   FD_CLR(from->get_sock(), &rfds);
   FD_SET(to->get_sock(), &rfds);
   if (to->get_sock() > maxdesc) maxdesc = to->get_sock();
}

void EventGate::end_work(char *reason)
{
  emit(0, EV_REMOVE_OBJ, oid, this);
  close(sock);
  while (!queue.empty()) queue.pop(); //discard all events
  if (parent_gate) parent_gate->child_finished(this);
  #ifdef GATE_DEBUG
  Error(0, "Application PID %i finished, maximal event queue length %i", pid, queue.max_length());
  if (reason != NULL) Error(0, "Reason: %s\n", reason);
  #endif
}

