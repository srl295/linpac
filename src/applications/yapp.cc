/*
   YappC RX/TX for LinPac
   (c) 1998 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage RX: yapp
         TX: yapptx <filename>

   Note: When running from LinPac use the B (binary) flag. Yapp works
         in binary mode and uses CR as EOLN.

   Changelog:
   12.12.1999 - protocole revision, cancel-states corrected
   12.01.2000 - state SE cannot be cancelled by timeout because the physical
                transfer can be longer than excepted
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <utime.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "lpapp.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define	TIMEOUT		120		/* 2 Minutes */
#define BUFSIZE         300             /* buffer for one frame */
#define RETRY           3               /* retries after timeout */
#define DATALEN         250             /* max. length of data */

#define	NUL		0x00
#define	SOH		0x01
#define	STX		0x02
#define	ETX		0x03
#define	EOT		0x04
#define	ENQ		0x05
#define	ACK		0x06
#define	DLE		0x10
#define	NAK		0x15
#define	CAN		0x18

#define	STATE_S		0
#define	STATE_SH	1
#define	STATE_SD	2
#define	STATE_SE	3
#define	STATE_ST	4
#define	STATE_R		5
#define	STATE_RH	6
#define	STATE_RD	7
#define STATE_CW        8

#define ABORT_ADDR "yapp"

int yappc = 0;     /* use yappc ? */
int abort_all = 0; /* abort received ? */
int send_msgs = 1; /* send messages to remote station ? */

/* user event handler */
void my_handler(Event *ev)
{
  switch (ev->type)
  {
    case EV_ABORT: if (ev->chn == lp_channel() &&
                      (ev->data == NULL || strcasecmp((char *)ev->data, ABORT_ADDR) == 0))
                        abort_all = 1;
                   break;
    case EV_DISC_FM:
    case EV_DISC_TIME:
    case EV_FAILURE: if (ev->chn == lp_channel())
                     {
                       lp_emit_event(lp_channel(), EV_ENABLE_SCREEN, 0, NULL);
                       exit(4);
                     }
                     break;
  }
}

/* print the message to user */
void message(const char *fmt, ...)
{
  va_list argptr;
  char msg[1024];
  
  va_start(argptr, fmt);
  vsprintf(msg, fmt, argptr);
  va_end(argptr);

  if (send_msgs) fputs(msg, stdout);
  else lp_emit_event(lp_channel(), EV_LOCAL_MSG, strlen(msg), msg);
}

/* YAPP frames */
void send_RR()
{
  printf("%c%c", ACK, 0x01);
}

void send_RF()
{
  printf("%c%c", ACK, 0x02);
}

void send_RT() /* Extended TPK reply */
{
  printf("%c%c", ACK, ACK);
}

void send_AF()
{
  printf("%c%c", ACK, 0x03);
}

void send_AT()
{
  printf("%c%c", ACK, 0x04);
}

void send_SI()
{
  printf("%c%c", ENQ, 0x01);
}

void send_HD(char *name, long size, unsigned long date)
{
  char s[20];
  sprintf(s, "%li", size);
  printf("%c%c%s", SOH, (int)(strlen(s)+strlen(name)+11), name);
  putchar('\0');
  printf("%s", s);
  putchar('\0');
  printf("%8lX", date);
  putchar('\0');
}

void send_DT(char *buf, int len)
{
  int l = (len == 0 ? 256 : len);
  printf("%c%c", STX, l);
  fwrite(buf, 1, l, stdout);
  if (yappc)
  {
    int i;
    unsigned char sum = 0;
    for (i=0; i < l; i++) sum += buf[i];
    printf("%c", sum);
  }
}

void send_EF()
{
  printf("%c%c", ETX, 0x01);
}

void send_ET()
{
  printf("%c%c", EOT, 0x01);
}

void send_NR(char const *reason)
{
  printf("%c%c%s", NAK, (int)strlen(reason), reason);
}

void send_RE(long size)
{
  char s[20];
  sprintf(s, "%li", size);
  printf("%c%cR", NAK, (int)(strlen(s)+3));
  putchar('\0');
  printf("%s", s);
  putchar('\0');
}

void send_CN(char const *reason)
{
  printf("%c%c%s", CAN, (int)strlen(reason), reason);
}

void send_CA()
{
  printf("%c%c", ACK, 0x05);
}

/*-------------------------------------------------------------------------*/

void clear_screen()
{
   lp_enable_screen();
   lp_remove_statline();
}

void transmit(char *name)
{
  FILE *f;
  fd_set rfds;
  struct timeval tv;
  int state;
  char buf[BUFSIZE];
  char data[DATALEN];
  int buflen;
  int len;
  int rc;
  int timeout, eofrm, eot, times;
  long fsize;
  long sent = 0;
  int i;
  struct stat fstat;
  char *fname;
  struct tm *date;
  unsigned long dateinfo;
  char path[1024];
  char *p;
  bool waiting; //waiting for reasonable reply to SI

  lp_set_event_handler(my_handler);

  //path
  p = lp_get_var(0, "USER_PATH");
  if (p == NULL) strcpy(path, ".");
            else strcpy(path, p);
  strcat(path, "/");
  if (strchr(name, '/') == NULL) strcat(path, name);
                            else strcpy(path, name);

  f = fopen(path, "r");
  if (f == NULL)
  {
    lp_appl_result("File not found");
    return;
  }

  stat(path, &fstat);
  date = localtime(&(fstat.st_mtime));

  //calculate date&time info
  dateinfo = ((date->tm_year - 80) << 9) +
             ((date->tm_mon+1) << 5) +
             date->tm_mday;
  dateinfo <<= 16;
  dateinfo += (date->tm_hour << 11) +
              (date->tm_min << 5) +
              date->tm_sec;

  fname = strrchr(name, '/');
  if (fname == NULL) fname = name; else fname++;

  fseek(f, 0, SEEK_END);
  fsize = ftell(f);
  rewind(f);

  state = STATE_S;
  waiting = false;
  eot = 0;
  times = 0;

  lp_disable_screen();

  while (!eot)
  {
    switch (state) /* send frames */
    {
      case STATE_S: lp_statline("YAPP: starting transfer");
                    if (!waiting) {send_SI(); waiting = true;}
                    break;
      case STATE_SH: lp_statline("YAPP: header sent");
                     send_HD(fname, fsize, dateinfo); break;
      case STATE_SD: len = fread(data, 1, DATALEN, f);
                     if (len > 0)
                     {
                       send_DT(data, len);
                       sent += len;
                       lp_statline("YAPP: TX %s : %i of %i", fname, sent, fsize);
                     }
                     break;
      case STATE_SE: lp_statline("YAPP: transfer finished"); send_EF(); break;
      case STATE_ST: lp_remove_statline(); send_ET(); break;
    }

    buflen = 0;
    timeout = eofrm = 0;
    if (state != STATE_SD) do //read whole frame
    {
      int tt = TIMEOUT;
      do
      {
        FD_ZERO(&rfds);
        FD_SET(fileno(stdin), &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        do
          rc = select(fileno(stdin)+1, &rfds, NULL, NULL, &tv);
        while (rc == -1 && errno == EINTR);
        tt--;
        if (rc == -1) fprintf(stderr, "YAPP - select() : %s\n", strerror(errno));
      } while (!rc && tt > 0 && !abort_all);
      if (rc > 0 && FD_ISSET(fileno(stdin), &rfds))
      {
        safe_read(fileno(stdin), &(buf[buflen]), 1);
        buflen++;
      } else timeout = 1;

      if (buflen > 0) //test end of frame
        switch (buf[0])
        {
          case ACK:
          case ETX:
          case EOT: eofrm = (buflen >= 2); break;
          case SOH:
          case STX:
          case NAK:
          case CAN:
          case DLE: eofrm = (buflen >= 2 && buflen >= buf[1] + 2); break;
          case ENQ: eofrm = (buflen >= 2 && buf[1] == 0x01) ||
                            (buflen >= 3 && buflen >= buf[2] + 3);
                    break;
          default: eofrm = 1;
        }
    } while (!timeout && !eofrm && buflen < BUFSIZE && !abort_all);

    if (!timeout && buf[0] == CAN) //test CANCEL frame
    {
       send_CA();
       clear_screen();
       printf("Transfer aborted by peer: ");
       for (i=0; i < buf[1]; i++) printf("%c", buf[i+2]);
       printf("\r");
       return;
    }

    if (abort_all)
    {
      send_CN("Transfer aborted by operator");
      clear_screen();
      return;
    }

    switch (state)  //handle reaction
    {
      case STATE_S: if (timeout)
                    {
                      waiting = false;
                      times++;
                      if (times > RETRY)
                      {
                        send_CN("Transmit timeout");
                        state = STATE_CW;
                      }
                    }
                    else
                    {
             /* RR */ if (buf[0] == ACK && buf[1] == 0x01) state = STATE_SH;
             /* RF */ else if (buf[0] == ACK && buf[1] == 0x02) state = STATE_SD;
             /* RT */ else if (buf[0] == ACK && buf[1] == ACK) {state = STATE_SD; yappc = 1;}
             /* RI */ else if (buf[0] == ENQ && buf[1] == 0x02) state = STATE_S;
             /* NR */ else if (buf[0] == NAK)
                           {
                             clear_screen();
                             printf("File rejected: ");
                             for (i=0; i < buf[1]; i++) printf("%c", buf[i+2]);
                             printf("\r");
                             return;
                           }
                      else state = STATE_S; //wait for reasonable answer or timeout
                    }
                    break;
      case STATE_SH: if (timeout) {send_CN("Transmit timeout"); state = STATE_CW;}
            /* RF */ else if (buf[0] == ACK && buf[1] == 0x02) state = STATE_SD;
            /* RT */ else if (buf[0] == ACK && buf[1] == ACK) {state = STATE_SD; yappc = 1;}
            /* NR */ else if (buf[0] == NAK && (buf[2] != 'R' || buf[3] != '\0'))
                          {
                            clear_screen();
                            printf("File rejected: ");
                            for (i=0; i < buf[1]; i++) printf("%c", buf[i+2]);
                            printf("\r");
                            return;
                          }
            /* RE */ else if (buf[0] == NAK && buf[2] == 'R' && buf[3] == '\0')
                          {
                            char *endptr;
                            i = strtol(&(buf[4]), &endptr, 10);
                            endptr++;
                            if (endptr < &(buf[buflen]) && *endptr == 'C') yappc = 1;
                            while (fseek(f, i, SEEK_SET) == -1)
                              if (errno != EINTR)
                                fprintf(stderr, "yapp: cannot seek\n");
                            state = STATE_SD;
                          }
                     else {send_CN("Protocol violation"); state = STATE_CW;}
                     break;
      case STATE_SD: if (feof(f)) state = STATE_SE; break;
      case STATE_SE: if (timeout) {/*send_CN("Transmit timeout"); state = STATE_CW;*/}
            /* AF */ else if (buf[0] == ACK && buf[1] == 0x03) state = STATE_ST;
                     else {send_CN("Protocol violation"); state = STATE_CW;}
                     break;
      case STATE_ST: clear_screen();
                     lp_appl_result("Yapp transfer finished");
                     eot = 1;
                     break;
      case STATE_CW: if (timeout)
                     {
                       clear_screen();
                       lp_appl_result("YAPP: transfer timeout");
                       return;
                     }
            /* CA */ else if (buf[0] == ACK && buf[1] == 0x05)
                     {
                       clear_screen();
                       return;
                     }
                     else send_CA();
                     break;
      default: clear_screen(); return; /* just not to hang in case of miracle */
    }
  }
  fclose(f);
}


void receive()
{
  FILE *f;
  fd_set rfds;
  struct timeval tv;
  int state;
  char buf[BUFSIZE];
  char path[256], ppath[256], *p;
  int buflen;
  int rc;
  int timeout, eofrm, eot, times;
  char name[256];
  long fsize;
  long rcvd = 0;
  unsigned long dateinfo;
  struct tm date;
  int i;
  char *fname;

  //path
  p = lp_get_var(0, "SAVE_PATH");
  if (p == NULL) strcpy(path, ".");
            else strcpy(path, p);
  strcat(path, "/");

  lp_set_event_handler(my_handler);
  state = STATE_R;
  eot = 0;
  times = 0;

  lp_disable_screen();

  lp_statline("YAPP: waiting for transfer start");

  while (!eot)
  {
    buflen = 0;
    timeout = eofrm = 0;
    do //read whole frame
    {
      int tt = TIMEOUT;
      do
      {
        FD_ZERO(&rfds);
        FD_SET(fileno(stdin), &rfds);
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        do
          rc = select(fileno(stdin)+1, &rfds, NULL, NULL, &tv);
        while (rc == -1 && errno == EINTR);
        if (rc == -1) fprintf(stderr, "YAPP - select() : %s\n", strerror(errno));
        tt--;
        if (abort_all)
        {
          send_CN("Aborted by operator");
          /*state = STATE_CW;
          tt = 0;*/
          clear_screen();
          return;
        }
      } while (rc == 0 && tt > 0);

      if (rc)
      {
        safe_read(fileno(stdin), &(buf[buflen]), 1);
        buflen++;
      } else timeout = 1;

      if (buflen > 0) //test end of frame
        switch (buf[0])
        {
          case ACK:
          case ETX:
          case EOT: eofrm = (buflen >= 2); break;
          case SOH:
          case STX:
          case NAK:
          case CAN:
          case DLE: eofrm = (buflen >= 2 && buflen >= buf[1] + 2); break;
          case ENQ: eofrm = (buflen >= 2 && buf[1] == 0x01) ||
                            (buflen >= 3 && buflen >= buf[2] + 3);
                    break;
          default: eofrm = 1;
        }
    } while (!timeout && !eofrm && buflen < BUFSIZE);

    if (!timeout && buf[0] == CAN) //test CANCEL frame
    {
       send_CA();
       printf("Transfer aborted by peer: ");
       for (i=0; i < buf[1]; i++) printf("%c", buf[i+2]);
       printf("\r");
       clear_screen();
       return;
    }

    switch (state)  //handle reaction
    {
      case STATE_R: if (timeout)
                    {
                      send_CN("Receive timeout");
                      state = STATE_CW;
                    }
                    else
                    {
             /* SI */ if (buf[0] == ENQ && buf[1] == 0x01)
                      {
                         state = STATE_RH; send_RR();
                         lp_statline("YAPP: waiting for file header");
                      }
             /* NR */ else if (buf[0] == NAK)
                           {
                             printf("Sender not ready: ");
                             for (i=0; i < buf[1]; i++) printf("%c", buf[i+2]);
                             printf("\r");
                             clear_screen();
                             return;
                           }
                      else /*{send_CN("Protocol violation"); state = STATE_CW;}*/
                           state = STATE_R; //wait for INIT or timeout
                    }
                    break;
      case STATE_RH: if (timeout) {send_CN("Receive timeout"); state = STATE_CW;}
            /* HD */ else if (buf[0] == SOH && buf[1] > 0)
                          {
                            char *p, *endptr;
                            dateinfo = 0;
                            p = &(buf[2]);
                            strcpy(name, p);
                            p += strlen(name)+1;
                            fsize = strtol(p, &endptr, 10);
                            endptr++;
                            if (endptr < &(buf[buflen]))
                            {
                              yappc = 1;
                              sscanf(endptr, "%8lX", &dateinfo);
                            }
                            p = strrchr(name, '/');
                            if (p == NULL) p = name; else p++;
                            fname = p;
                            sprintf(ppath, "%s%s", path, p);
                            f = fopen(ppath, "a");
                            if (f == NULL)
                            {
                              send_NR("Cannot write to file");
                              clear_screen();
                              return;
                            }
                            if (ftell(f) == 0) send_RF();
                                          else send_RE(ftell(f));
                            state = STATE_RD;
                            rcvd = ftell(f);
                            lp_statline("YAPP: RX %s : %i of %i", fname, rcvd, fsize);
                          }
            /* SI */ else if (buf[0] == ENQ && buf[1] == 0x01) state = STATE_RH;
            /* ET */ else if (buf[0] == EOT && buf[1] == 0x01) {send_AT(); clear_screen(); return;}
                     else {send_CN("Protocol violation"); state = STATE_CW;}
                     break;
      case STATE_RD: if (timeout) {send_CN("Timeout"); state = STATE_CW;}
            /* DT */ else if (buf[0] == STX)
                          {
                            int cnt = (buf[1] == 0 ? 256 : buf[1]);
                            rcvd += cnt;
                            fwrite(&(buf[2]), 1, cnt, f);
                            lp_statline("YAPP: RX %s : %i of %i", fname, rcvd, fsize);
                          }
            /* EF */ else if (buf[0] == ETX && buf[1] == 0x01)
                          {
                            send_AF();
                            fclose(f);
                            if (dateinfo != 0)
                            {
                              struct utimbuf ut;
                              date.tm_sec = (dateinfo & 0x1F) * 2; dateinfo >>= 5;
                              date.tm_min = dateinfo & 0x3F; dateinfo >>= 6;
                              date.tm_hour = dateinfo & 0x1F; dateinfo >>= 5;
                              date.tm_mday = dateinfo & 0x1F; dateinfo >>= 5;
                              date.tm_mon = (dateinfo & 0x0F) - 1; dateinfo >>= 4;
                              date.tm_year = (dateinfo & 0x7F) + 70;
                              date.tm_isdst = 0;
                              date.tm_yday = 0;
                              date.tm_wday = 0;
                              ut.actime = mktime(&date);
                              ut.modtime = ut.actime;
                              utime(fname, &ut);
                            }
                            state = STATE_RH;
                            lp_statline("YAPP: waiting for next file header");
                          }
                     else {send_CN("Protocol violation"); state = STATE_CW;}
                     break;
      case STATE_CW: if (timeout)
                     {
                       clear_screen();
                       lp_appl_result("YAPP: transfer timeout");
                       return;
                     }
                     else if (buf[0] == ACK && buf[1] == 0x05)
                     {
                       clear_screen();
                       return;
                     }
                     else send_CA();
                     break;
      default: clear_screen(); return; /* just not to hang in case of miracle */
    }
  }
}


int main(int argc, char **argv)
{
   char *p;

   setbuf(stdout, NULL);
   if (lp_start_appl())
   {
     /* determine the remote station type */
     p = lp_get_var(lp_channel(), "STN_TYPE");
     send_msgs = (p == NULL || strcasecmp(p, "TERM") == 0);
     
     /* remove path from the filename */
     p = strrchr(argv[0], '/');
     if (p == NULL) p = argv[0]; else p++;

     if (strcmp(p, "yapptx") == 0)  /* we'll send something */
     {
       if (argc == 2) transmit(argv[1]);
       else lp_appl_result("Filename for TX required");
     }
     else
       if (argc == 1) receive();
       else lp_appl_result("Command has no parametres");
     lp_end_appl();
   }
   else printf("LinPac is not running\n");
   return 0;
}

