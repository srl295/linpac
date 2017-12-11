/*
   LinPac Application interface testing program
   (c) David Ranch KI6ZHD (linpac@trinnet.net) 2002 - 2016
   (c) 1998-2001 by OK2JBG
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lpapp.h"

int stop = 0;
int lpbck = 0;

void my_handler(Event *ev)
{
  if (ev->type == EV_ABORT && ev->chn == lp_channel()) stop = 1;
  if (ev->type == EV_SYSREQ && ev->chn == lp_channel() && ev->x == 999)
  {
     lp_emit_event(lp_channel(), EV_STAT_REQ, 0, NULL);
     if (lp_chn_status(lp_channel()) == ST_DISC) printf("No connection\n");
  }
  if (ev->type == EV_STATUS && ev->chn == lp_channel())
  {
     struct ax25_status *state = (struct ax25_status *)ev->data;
     printf("Status: device = %s, t1 = %i / %i, unack = %i\n",
            state->devname, state->t1, state->t1max, state->vs - state->va);
  }
  if (ev->type == EV_CONV_IN && ev->chn == lp_channel())
  {
     char *p = (char *)ev->data;
     if (p == NULL) printf("null!!!\n");
     if (ev->x > 'A')
        printf("Input conversion table: length %i, table[A] = %c\n", ev->x, p['A']);
     else
        printf("Input conversion table: length %i\n", ev->x);
  }
  if (ev->type == EV_CONV_OUT && ev->chn == lp_channel())
  {
     char *p = (char *)ev->data;
     if (ev->x > 'A')
        printf("Output conversion table: length %i, table[A] = %c\n", ev->x, p['A']);
     else
        printf("Output conversion table: length %i\n", ev->x);
  }
  if (ev->type == EV_APP_COMMAND && ev->chn == lp_channel())
  {
     char *p = (char *)ev->data;
     printf("App command: `%s'\n", p);
     if (strncasecmp(p, "loopback", 8) == 0) lpbck = 1;
  }
}

void loopback()
{
   char inpline[256];
   int n = 0;

   fprintf(stdout, "This is LOOPBACK (end with /ex)\n");
   setbuf(stdout, NULL);
   lp_syncio_on();
   do
   {
     safe_fgets(inpline, 256, stdin);
     fprintf(stdout, "(%i) Read: %s", n++, inpline);
   } while (strcmp(inpline, "/ex\n") != 0);
   fprintf(stdout, "Finished\n");
   lp_syncio_off();
}

int main(int argc, char **argv)
{
  char teststr[256];

  lp_app_ident("testapp");
  if (lp_start_appl())
  {
     lp_statline("Test application running");
     lp_set_event_handler(my_handler);
     printf("\nTest application started at channel %i.\n", lp_channel());
     printf("Started by %s user\n", lp_app_remote()?"remote":"local");
     printf("argc = %i\n", argc);
     printf("my_call = `%s'\n", lp_chn_call(lp_channel()));
     printf("remote_call = `%s'\n", lp_chn_cwit(lp_channel()));
     printf("remote_name = `%s'\n", lp_get_var(lp_channel(), "STN_NAME"));
     printf("nickname = `%s'\n", lp_get_var(lp_channel(), "STN_NICKNAME"));

     strcpy(teststr, "Today is %D time %T, I am %C, BBS is %HOME_BBS@0");
     printf("\nMacro replacement test:\n");
     printf("Source: `%s'\n", teststr);
     replace_macros(lp_channel(), teststr);
     printf("Result: `%s'\n", teststr);

     printf("fixpath is %s\n", lp_iconfig("fixpath") ? "ON":"OFF");

     printf("\nStop me with \"ABort\" command.\n");
     printf("Sysreq 999 will print the status\n");
     printf("Use :loopback command for loopback function\n");
     strcpy(teststr, "loopback");
     lp_emit_event(lp_channel(), EV_REG_COMMAND, 0, teststr);
     do
     {
        pause();
        if (lpbck) { loopback(); lpbck = 0; }
     }
     while (!stop);

     lp_emit_event(lp_channel(), EV_UNREG_COMMAND, 0, teststr);
     printf("Application stopped.\n\n");
     lp_remove_statline();
     lp_end_appl();
  } else fprintf(stdout, "Cannot start application\n");
  return 0;
}

