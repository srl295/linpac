/*
   Raw data RX/TX for LinPac
   (c) 1998 by Radek Burget OK2JBG

   History:
   2002/04/13: Revised, O_NONBLOCK removed

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage: RX: rawio <filename>
          TX: rawtx <filename>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lpapp.h"

#define BUFSIZE 256
#define ABORT_READ "read"
#define ABORT_WRITE "write"

volatile int abort_read = 0;
volatile int abort_write = 0;

/* user event handler */
void my_handler(Event *ev)
{
  switch (ev->type)
  {
    case EV_ABORT: if (ev->chn == lp_channel() &&
                      (ev->data == NULL || strcasecmp((char *)ev->data, ABORT_READ) == 0))
                        abort_read = 1;
                   if (ev->chn == lp_channel() &&
                      (ev->data == NULL || strcasecmp((char *)ev->data, ABORT_WRITE) == 0))
                        abort_write = 1;
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

/* recieve a file */
void recieve(char *fname)
{
  FILE *f;
  char buf[BUFSIZE];
  int rd, r;
  char *p;
  char path[80];
  char name[256];

  /* when the filename is 'off' abort previous read */
  if (strcasecmp(fname, "off") == 0) //off - stop writing
  {
     char s[32];
     strcpy(s, ABORT_WRITE);
     lp_emit_event(lp_channel(), EV_ABORT, 0, s);
     lp_wait_event(lp_channel(), EV_ABORT);
     exit(0); 
  }

  /* get the destination path */
  p = lp_get_var(0, "SAVE_PATH");
  if (p == NULL) strncpy(path, ".", 79);
            else strncpy(path, p, 79);
  path[79] = '\0';
  
  lp_set_event_handler(my_handler);

  p = strrchr(fname, '/'); if (p == NULL) p = fname; else p++;
  snprintf(name, 255, "%s/%s", path, p);
  f = fopen(name, "w");
  if (f == NULL)
  {
     lp_appl_result("Cannot open file `%s'", name);
     exit(1);
  }

  rd = 0;
  lp_statline("File RX: %s", name);

  //if (app_remote) printf("LinPac: writing to file %s\n", name);
      
  while (!abort_write)
  {
     lp_statline("File RX: %s  %i bytes", name, rd);
     r = read(0, buf, BUFSIZE);
     if (r > 0)
     {
        fwrite(buf, 1, r, f);
        rd += r;
     }
  }

  //if (app_remote) printf("LinPac: file closed: %s\n", name);

  fclose(f);
  lp_remove_statline();
}


/* transmit a file */
void transmit(char *fname)
{
  FILE *f;
  char buf[BUFSIZE];
  char path[1024];
  char *p;
  size_t rd, max, r;

  lp_set_event_handler(my_handler);

  //get the default path
  p = lp_get_var(0, "USER_PATH");
  if (p == NULL) strcpy(path, ".");
            else strcpy(path, p);
  strcat(path, "/");
  if (strchr(fname, '/') == NULL) strcat(path, fname);
                             else strcpy(path, fname);

  //open the source file
  f = fopen(path, "r");
  if (f == NULL)
  {
    lp_appl_result("File not found");
    exit(1);
  }
  fseek(f, 0, SEEK_END);
  max = ftell(f);
  rewind(f);
  rd = 0;
  
  while (!feof(f) && !abort_read)
  {
    lp_statline("File TX: %s  %i of %i", path, rd, max);
    r = fread(buf, 1, BUFSIZE, f);
    fwrite(buf, 1, r, stdout);
    rd += r;
  }

  if (abort_read)
    printf("\n*** TX aborted.\n");

  fclose(f);
  lp_remove_statline();
}


int main(int argc, char **argv)
{
   char *p;

   if (lp_start_appl())
   {
     p = strrchr(argv[0], '/');
     if (p == NULL) p = argv[0]; else p++;
     if (strcmp(p, "rawtx") == 0)  /* we'll send something */
     //if (1)
     {
       if (argc == 2) transmit(argv[1]);
       else lp_appl_result("Filename for TX required");
     }
     else
     {
       if (argc == 2) recieve(argv[1]);
       else lp_appl_result("Filename for RX required");
     }
     lp_end_appl();
   }
   else printf("LinPac is not running\n");
   return 0;
}

