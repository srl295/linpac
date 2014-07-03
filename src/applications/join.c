/*
   join - join two LinPac channels
   (c) 1998-2001 by OK2JBG
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "lpapp.h"

#define MAGIC 555

int stop = 0;
int jchn;

void my_handler(Event *ev)
{
   if (ev->type == EV_ABORT &&
       (ev->chn == lp_channel() || ev->chn == jchn))
   {
      char *s = (char *)ev->data;
      if (strcasecmp(s, "join") == 0) stop = 1;
   }

   if (ev->type == EV_DATA_INPUT && ev->chn == lp_channel()
       && ev->y != MAGIC)
      lp_emit_event(jchn, EV_DATA_OUTPUT, ev->x, ev->data);

   if (ev->type == EV_DATA_INPUT && ev->chn == jchn
       && ev->y != MAGIC)
      lp_emit_event(lp_channel(), EV_DATA_OUTPUT, ev->x, ev->data);

   if (ev->type == EV_DATA_OUTPUT && ev->chn == lp_channel()
       && ev->y != MAGIC)
      lp_emit_event(jchn, EV_DATA_INPUT, ev->x, ev->data);

   if (ev->type == EV_DATA_OUTPUT && ev->chn == jchn
       && ev->y != MAGIC)
      lp_emit_event(lp_channel(), EV_DATA_INPUT, ev->x, ev->data);
}

void chn_statline(int chn, const char *fmt, ...)
{
   va_list argptr;
   static char s[256];
 
   va_start(argptr, fmt);
   vsprintf(s, fmt, argptr);
   lp_wait_init(chn, EV_CHANGE_STLINE);
   lp_emit_event(chn, EV_CHANGE_STLINE, getpid(), s);
   lp_wait_realize();
   va_end(argptr);
}

void chn_remove_statline(int chn)
{
   lp_wait_init(chn, EV_REMOVE_STLINE);
   lp_emit_event(chn, EV_REMOVE_STLINE, getpid(), NULL);
   lp_wait_realize();
}

int main(int argc, char **argv)
{
  char teststr[256];

  lp_app_ident("join");
  if (lp_start_appl())
  {
     char *endptr;
     
     if (argc != 2)
     {
        lp_appl_result("Usage: join <channel number>");
        lp_end_appl();
        return 1;
     }

     jchn = strtol(argv[1], &endptr, 10);
     if (jchn <= 0 || jchn > lp_iconfig("maxchn") || *endptr != '\0')
     {
        lp_appl_result("Invalid channel number");
        lp_end_appl();
        return 1;
     }

     chn_statline(lp_channel(), "Joined to channel %i", jchn);
     chn_statline(jchn, "Joined to channel %i", lp_channel());
     lp_event_y(MAGIC);
     lp_set_event_handler(my_handler);

     while (!stop) pause();

     lp_remove_statline();
     lp_end_appl();
  } else fprintf(stdout, "Cannot start application\n");
  return 0;
}

