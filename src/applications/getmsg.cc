/*
   getmsg - download messages from F6FBB BBS
   (c) 1998 - 1999 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage: getmsg <msgnum> [<msgnum> ...]

   Use the 'P' letter before the message number (w/o spaces) if you
   want to kill this private message after reading. (e.g. getmsg P8455)

   Note: When running from LinPac use the B (binary) flag. Getmsg works
         in binary mode and uses CR as EOLN.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "lzhuf.h"
#include "lpapp.h"

#define NUL 0
#define SOH 1
#define STX 2
#define EOT 4

#define PERSONAL //storing messages to home directory supported

#define ABORT_ADDR "getmsg"

#define MAILDIR "/var/ax25/mail"

char *homebbs;
char *mailhome;
char home_call[64];
int abort_all = 0;
char kill_cmd[64]; //FBB command for killing messages

//-----------------------------------------------------------------------

int call_ssid(const char *src)
{
  char *p = const_cast<char *>(src);
  while (*p != '\0' && *p != '-') p++;
  if (*p) p++;
  if (*p) return atoi(p);
     else return 0;
}

char *call_call(const char *src)
{
  static char s[15];
  char *p = const_cast<char *>(src);
  strcpy(s, "");
  while (*p && isalnum(*p) && strlen(s) < 6) {strncat(s, p, 1); p++;}
  return s;
}

char *strip_ssid(char *src)
{
  if (call_ssid(src) == 0)
  {
    char *p = src;
    while (*p != '\0' && *p != '-') p++;
    *p = '\0';
  }
  return src;
}

char *normalize_call(char *call)
{
  char c[8];
  strcpy(c, call_call(call));
  int ssid = call_ssid(call);
  if (ssid == 0) strcpy(call, c); else sprintf(call, "%s-%i", c, ssid);
  for (char *p = call; *p; p++) *p = toupper(*p);
  return call;
}

//---------------------------------------------------------------------

/* read and store one message */
int get_one_message(FILE *fin, char **buf, int *bsize, char *title)
{
  int eot;
  int hd_length;
  int ch;
  char c;
  char ofset[7];
  signed char sum, chksum;
  int flen;
  int i;
  int bpoz;
  int size;

  size = 1;
  *buf = (char *) malloc(sizeof(char) * size);
  bpoz = 0;
  
  ch = safe_fgetc(fin);
  if (ch != SOH)
  {
     fprintf(stderr, "getmsg: bad starting character (0x%x)\n", ch);
     return 2; /* format violated */
  }
  hd_length = safe_fgetc(fin);

  lp_statline("Reading message");

  /* read title */
  strcpy(title, "");
  do
  {
    ch = safe_fgetc(fin);
    c = (char) ch;
    if (ch != NUL) strncat(title, &c, 1);
  } while (ch != NUL && !abort_all);

  if (abort_all)
  {
    fprintf(stderr, "getmsg: aborted\n");
    return 3;
  }

  lp_statline("Reading message: `%s'", title);

  /* read ofset */
  strcpy(ofset, "");
  do
  {
    ch = safe_fgetc(fin);
    c = (char) ch;
    if (ch != NUL) strncat(ofset, &c, 1);
  } while (ch != NUL && !abort_all);

  if (abort_all)
  {
    fprintf(stderr, "getmsg: aborted\n");
    return 3;
  }

  /* read data frames */
  sum = 0;
  eot = 0;
  while (!eot)
  {
    ch = safe_fgetc(fin);
    if (ch == STX) /* data frame */
    {
      flen = safe_fgetc(fin);
      if (flen == 0) flen = 256;
      if (bpoz+flen >= size) /* buffer exceeded -> expand */
      {
        size += flen;
        *buf = (char *) realloc(*buf, sizeof(char) * size);
      }

      if (abort_all)
      {
        fprintf(stderr, "getmsg: aborted\n");
        return 3;
      }
      /*fread(buf+bpoz, 1, flen, fin);
      for (i=0; i<flen; i++)
      {
        sum += (int) (*buf)[bpoz];
        bpoz++;
      }*/
      for (i=0; i<flen; i++)
      {
        ch = safe_fgetc(fin);
        (*buf)[bpoz] = (char) ch; bpoz++;
        sum += ch;
        if (abort_all)
        {
          fprintf(stderr, "getmsg: aborted\n");
          return 3;
        }
      }
    }
    else if (ch == EOT)/* end of transfer */
    {
      chksum = safe_fgetc(fin);
      if (chksum + sum != 0 && !(chksum == -128 && sum == -128))
      {
        fprintf(stderr, "getmsg: Checksum error\n");
        return 1;
      }
      eot = 1;
    }
    else
    {
      fprintf(stderr, "getmsg: Protocol violation (0x%x)\n", ch);
      return 2;
    }
  }
  *bsize = bpoz;
  return 0;
}

/* save decompressed message (and convert to Unix text) */
void save_msg(int num, bool pers, void *data, unsigned long length)
{
  char fname[256];

  #ifdef PERSONAL
  if (pers)
    sprintf(fname, "%s/%s/%i", mailhome, home_call, num);
  else
    sprintf(fname, "%s/%s/%i", MAILDIR, home_call, num);
  #else
  sprintf(fname, "%s/%s/%i", MAILDIR, home_call, num);
  #endif

  FILE *f = fopen(fname, "w");
  if (f != NULL)
  {
    unsigned long i;
    char *p = reinterpret_cast<char *>(data);
    for (i=0; i<length; i++, p++)
      if (*p != '\r') putc(*p, f);
    fclose(f);
  }
  else fprintf(stderr, "getmsg: cannot write to file %s\n", fname);
}

/* wait for prompt from BBS */
void wait_prompt()
{
  int old_char=0, new_char=0;
  do
  {
    old_char = new_char;
    new_char = safe_fgetc(stdin);
  } while ((old_char != '>' || new_char != '\r') && !abort_all);
}

/* connect BBS */
void connect_bbs(char *addr)
{
   char cmd[255];
   sprintf(cmd, "c %s", addr);
   lp_wait_connect_prepare(lp_channel());
   lp_emit_event(lp_channel(), EV_DO_COMMAND, 0, cmd);
 
   char *call = strchr(addr, ':');
   if (call == NULL) call = addr; else call++;
   lp_wait_connect(lp_channel(), call);
   wait_prompt();
}

/* send LinPac tag */
void send_tag()
{
   printf("[LinPac-%s-$]\r", LPAPP_VERSION);
}

/* disconnect BBS */
void disc_bbs()
{
  lp_emit_event(lp_channel(), EV_DISC_LOC, 0, NULL);
  lp_wait_event(lp_channel(), EV_DISC_LOC);
}

void stop_it()
{
  disc_bbs();
}

void send_request(int num)
{
  printf("r %i\r", num);
}

void delete_message(int num)
{
  char pref[64];
  char suf[64];
  strcpy(pref, kill_cmd);
  strcpy(suf, "");

  char *p = strchr(pref, '#');
  if (p != NULL)
  {
     *p = '\0';
     p++;
     strcpy(suf, p);
     printf("%s%i%s\r", pref, num, suf);
  }
  else printf("%s\n", pref);
}

//Notify other applications about a message received
void notify_others(int num)
{
  char s[64];
  sprintf(s, "getmsg: got %i", num);
  lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, s);
}

//-------------------------------------------------------------------------

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
                       char s[32];
                       sprintf(s, "getmsg: done");
                       lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, s);
                       lp_emit_event(lp_channel(), EV_ENABLE_SCREEN, 0, NULL);
                       sleep(5);
                       exit(4);
                     }
                     break;
  }
}


int main(int argc, char **argv)
{
  LZHufPacker pck;

  if (lp_start_appl())
  {
    //event_handling_off();
    lp_set_event_handler(my_handler);
    lp_syncio_on();

    homebbs = lp_get_var(0, "HOME_BBS");
    if (homebbs == NULL)
    {
      printf("No HOME_BBS@0 specified\r");
      return 1;
    }

    char *homeaddr = lp_get_var(0, "HOME_ADDR");
    if (homeaddr == NULL)
    {
      printf("No HOME_ADDR@0 specified\r");
      return 1;
    }
    strcpy(home_call, homeaddr);
    normalize_call(home_call);

    mailhome = lp_get_var(0, "MAIL_PATH");
    if (mailhome == NULL)
    {
      printf("No MAIL_PATH@0 specified\r");
      return 1;
    }

    //Is some kill command set?
    char *killpers = lp_get_var(0, "KILLPERS");
    if (killpers != NULL)
       strncpy(kill_cmd, killpers, 63);
    else
       strcpy(kill_cmd, "K #"); //use default

    if (argc < 2)
    {
      lp_appl_result("Message numbers missing");
    }
    else
    {
      char subj[256];
      
      //send start info (collaboration with mailer)
      char msg[32];
      sprintf(msg, "getmsg: start");
      lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, msg);

      lp_statline("getmsg: connecting BBS");
      if (lp_chn_status(lp_channel()) == ST_DISC) connect_bbs(homebbs);
      lp_statline("getmsg: initializing transfer");
      send_tag();
      
      for (int i=1; i<argc; i++)
      {
        char *buf;
        int bsize;
        bool priv;
        int num;
        char *p = argv[i];

        if (toupper(*p) == 'P') { priv = true; p++;} else priv = false;
        num = atoi(p);
        
        wait_prompt();
        send_request(num);
        lp_disable_screen();
        if (get_one_message(stdin, &buf, &bsize, subj) == 0)
        {
          unsigned long clen = ((buf[3]*256 + buf[2])*256 + buf[1])*256 + buf[0];
          pck.Decode(clen, buf+4, bsize-4);
          save_msg(num, priv, pck.GetData(), pck.GetLength());
          notify_others(num);
          lp_enable_screen();
          if (priv)
          {
            wait_prompt();
            delete_message(num);
          }
        }
        else
        {
          lp_enable_screen();
          printf("Error: Message broken\r");
          break;
        }
      }
      wait_prompt();
      disc_bbs();
      sprintf(msg, "getmsg: done");
      lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, msg);
    }
    lp_end_appl();
  }
  else printf("Linpac is not running\n");
}

