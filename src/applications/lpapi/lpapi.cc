/*
  LinPac API for shell scripts
  (c) 1999 by Radek Burget OK2JBG

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lpapp.h"

int chn;
char s[1024];

void help()
{
  printf("lpapi: LinPac API for shell scripts\n");
  printf("Use: lpapi <channel> [command] [parametres]\n");
  printf("Known commands:\n");
  printf(" do <command> [params]  - execute LinPac command\n");
  printf(" get <variable>         - return the value of variable\n");
  printf(" call                   - return channel callsign\n");
  printf(" cwit                   - return callsign of connected stn\n");
  printf(" port                   - return port of connected stn\n");
  printf(" state                  - return current status (number)\n");
  printf(" pname <number>         - return the name of port\n");
  printf(" <no command>           - exit with 0 if LinPac is running\n");
}

void finish(int code)
{
   lp_end_appl();
   exit(code);
}

int main(int argc, char **argv)
{
   if (argc == 1)
   {
      help();
      return 0;
   }
   else
   {
      if (lp_start_appl())
      {
         chn = atoi(argv[1]);
         if (chn > lp_iconfig("maxchn"))
         {
            fprintf(stderr, "lpapi: Channel number %i doesn't exist\n", chn);
            finish(1);
         }
    
         if (argc == 2) finish(0);
    
         if (strcmp(argv[2], "do") == 0)
         {
            int i;
            strcpy(s, "");
            for (i = 3; i < argc; i++)
            {
                strcat(s, "'");
                strcat(s, argv[i]);
                strcat(s, "'");
                if (i < argc-1) strcat(s, " ");
            }
            lp_emit_event(chn, EV_DO_COMMAND, 0, s);
         }
         else if (strcmp(argv[2], "get") == 0)
         {
            char *p;
            if (argc > 3) p = lp_get_var(chn, argv[3]);
                     else p = NULL;
            if (p != NULL) printf("%s", p);
         }
         else if (strcmp(argv[2], "call") == 0)
         {
            printf("%s", lp_chn_call(chn));
         }
         else if (strcmp(argv[2], "cwit") == 0)
         {
            printf("%s", lp_chn_cwit(chn));
         }
         else if (strcmp(argv[2], "port") == 0)
         {
            printf("%i", lp_chn_port(chn));
         }
         else if (strcmp(argv[2], "state") == 0)
         {
            printf("%i", lp_chn_status(chn));
         }
         else if (strcmp(argv[2], "pname") == 0)
         {
            if (argc > 3)
               printf("%s", get_port_name(atoi(argv[3])));
         }
         else
         {
            fprintf(stderr, "lpapi: unknown command\n");
            finish(1);
         }
         
         lp_end_appl();
      }
      else return 1;
   }
   return 0;
}

