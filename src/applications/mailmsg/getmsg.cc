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
#include "lpapp.h"
#include "bbs.h"
#include "bbs_fbb.h"
#include "bbs_pbbs.h"

#define ABORT_ADDR "getmsg"

BBS* bbs = NULL;

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
                        bbs->abort();
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
  if (lp_start_appl())
  {
    //event_handling_off();
    lp_set_event_handler(my_handler);
    lp_syncio_on();

    char* homebbs = lp_get_var(0, "HOME_BBS");
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
    char home_call[64];
    strcpy(home_call, homeaddr);
    normalize_call(home_call);

    char* mailhome = lp_get_var(0, "MAIL_PATH");
    if (mailhome == NULL)
    {
      printf("No MAIL_PATH@0 specified\r");
      return 1;
    }

    //Is some kill command set?
    char *killpers = lp_get_var(0, "KILLPERS");
    char kill_cmd[64]; //BBS command for killing messages
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
      char* bbs_type = lp_get_var(0, "HOME_TYPE");
      if (strcmp(bbs_type, "PBBS") == 0)
        bbs = new PBBS(homebbs, home_call, mailhome);
      else
        // Default is FBB for backwards compatibility
        bbs = new FBB(homebbs, home_call, mailhome);

      bbs->set_kill_cmd(kill_cmd);

      //send start info (collaboration with mailer)
      lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, (void*)"getmsg: start");

      lp_statline("getmsg: connecting BBS");
      if (lp_chn_status(lp_channel()) == ST_DISC) bbs->connect_bbs(homebbs);
      lp_statline("getmsg: initializing transfer");
      bbs->send_tag();
      bbs->set_limit(stdin);
      
      for (int i=1; i<argc; i++)
      {
        char subj[256];
        char *buf;
        int bsize;
        bool priv;
        int num;
        char *p = argv[i];

        if (toupper(*p) == 'P') { priv = true; p++;} else priv = false;
        num = atoi(p);
        
        bbs->wait_prompt(stdin);
        bbs->send_request(num);
        lp_disable_screen();
        if (bbs->get_one_message(stdin, &buf, &bsize, subj) == 0)
        {
          bbs->save_msg(num, priv, buf, bsize);
          notify_others(num);
          lp_enable_screen();
          if (priv)
          {
            bbs->wait_prompt(stdin);
            bbs->delete_message(num);
          }
        }
        else
        {
          lp_enable_screen();
          printf("Error: Message broken\r");
          break;
        }
      }
      bbs->wait_prompt(stdin);
      bbs->disc_bbs();
      lp_emit_event(lp_channel(), EV_APP_MESSAGE, 0, (void*)"getmsg: done");
    }
    lp_remove_statline();
    lp_end_appl();
  }
  else printf("Linpac is not running\n");
}
