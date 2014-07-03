/*
   Autobin RX/TX for LinPac
   (c) 1998 - 1999 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage RX: autobin
         TX: autotx <filename>

   Note: When running from LinPac use the B (binary) flag. Autobin works
         in binary mode and uses CR as EOLN.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <errno.h>
#include "lpapp.h"
#include "crc.h"

#define LINE_LEN 80
#define BUFSIZE 256

#define ABORT_ADDR "autobin"

int abort_all = 0;         /* 1 = abort event was received */
int lp_active = 0;         /* is LinPac active ? */
int send_msgs = 1;         /* send messages to remote station ? */
int status;

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

/* send message to user */
void message(const char *fmt, ...)
{
  va_list argptr;
  char msg[1024];
  
  va_start(argptr, fmt);
  vsprintf(msg, fmt, argptr);
  va_end(argptr);

  if (send_msgs) printf(msg);
  else lp_emit_event(lp_channel(), EV_LOCAL_MSG, strlen(msg), msg);
}

/* fgets_cr - works like fgets, but uses CR as EOLN */
char *fgets_cr(char *s, int size, FILE *stream)
{
  int cnt = 0;
  int ch;
  while (cnt < size)
  {
    do ch = fgetc(stream); while (ch == EOF && errno == EINTR);
    if (ch == EOF || abort_all) {s[cnt++] = '\0'; break;}
    s[cnt++] = ch;
    if (ch == '\r') {s[cnt++] = '\0'; break;}
  }
  return s;
}

/* file_crc - calculate a CRC of specified file */
int file_crc(char *name)
{
  unsigned char buff[BUFSIZE];
  FILE *f;
  int c;
  int crc = 0;

  init_crc();
  f = fopen(name, "r");
  if (f == NULL) return -1;
  while (!feof(f))
  {
    c = fread(buff, 1, BUFSIZE, f);
    crc = calc_crc(buff, c, crc);
  }
  fclose(f);

  return crc;
}

void abort_transfer()
{
  printf("\n#ABORT#\n");
  lp_enable_screen();
  message("Transfer aborted.\n");
  lp_remove_statline();
}

/*-------------------------------------------------------------------------
   receive - wait for a begining of autobin transfer and receive
             a single file
  -------------------------------------------------------------------------*/
void receive()
{
   unsigned char buff[BUFSIZE];
   char line[LINE_LEN+1];
   char param[LINE_LEN+1];
   char path[LINE_LEN], ppath[LINE_LEN];
   char field_id;
   char *p, *q;
   FILE *f;
   int c;
   int old_form = 1;

   long size;
   unsigned crc, ex_crc;
   int dateinfo;
   struct tm date;
   char name[LINE_LEN+1];
   char fname[LINE_LEN+1];
   long to_load;
   struct utimbuf ut;

   //path
   p = lp_get_var(0, "SAVE_PATH");
   if (p == NULL) strcpy(path, ".");
             else strcpy(path, p);
   strcat(path, "/");

   ut.actime = ut.modtime = 0;

   lp_set_event_handler(my_handler);
   init_crc();
   strcpy(name, "autobin.download");

   lp_statline("Autobin: Waiting for header");
   /* wait for autobin header */
   do
   {
     fgets_cr(line, LINE_LEN, stdin);
   } while (strstr(line, "#BIN#") != line && !abort_all);

   if (abort_all) {abort_transfer(); return;}

   line[strlen(line)-1] = '\0';

   /* read size */
   strcpy(param, "");
   p = &(line[5]);
   while (*p && *p != '#') { strncat(param, p, 1); p++; }
   size = atol(param);
   p++;

   /* other parametres */
   while (*p)
   {
     field_id = *p;
     if (*p == '|' || *p == '$') p++;
     strcpy(param, "");
     while (*p && *p != '#') { strncat(param, p, 1); p++; }
     if (field_id == '|') ex_crc = atoi(param);
     else if (field_id == '$')
     {
       dateinfo = (int) strtol(param, NULL, 16);
       date.tm_sec = (dateinfo & 0x1F) * 2; dateinfo >>= 5;
       date.tm_min = dateinfo & 0x3F; dateinfo >>= 6;
       date.tm_hour = dateinfo & 0x1F; dateinfo >>= 5;
       date.tm_mday = dateinfo & 0x1F; dateinfo >>= 5;
       date.tm_mon = (dateinfo & 0x0F) - 1; dateinfo >>= 4;
       date.tm_year = (dateinfo & 0x7F) + 70;
       date.tm_isdst = 0;
       date.tm_yday = 0;
       date.tm_wday = 0;
       old_form = 0;
       ut.actime = mktime(&date);
       ut.modtime = ut.actime;
     }
     else if (field_id > ' ') strcpy(name, param);

     p++;
   }

   /* strip the path from name */
   p = strrchr(name, '/');
   q = strrchr(name, '\\');
   if (p == NULL && q == NULL) strcpy(fname, name);
   if (p > q) strcpy(fname, p+1);
   if (q > p) strcpy(fname, q+1);
   to_load = size;
   crc = 0;
   strcpy(line, "");

   /* create file */
   sprintf(ppath, "%s%s", path, fname);
   f = fopen(ppath, "w");
   if (f == NULL)
   {
     printf("#NO#\r");
     lp_appl_result("Cannot open specified file");
     lp_remove_statline();
     return;
   }

   printf("#OK#\r");
   lp_emit_event(lp_channel(), EV_DISABLE_SCREEN, 0, NULL);
   while (to_load > 0 && !abort_all)
   {
     int i;
     lp_statline("Autobin RX: %s %i of %i", fname, size - to_load, size);
     c = fread(buff, 1, to_load<BUFSIZE ? to_load:BUFSIZE, stdin);
     /* check for #ABORT# */
     for(i=0; i<c; i++)
       if (buff[i] == '\r')
       {
         if (strcmp(line, "#ABORT#") == 0)
         {
           lp_enable_screen();
           lp_remove_statline();
           //printf("Autobin transfer aborted.\r");
           lp_appl_result("Autobin transfer aborted.");
           return;
         }
         strcpy(line, "");
       }
       else
       {
         if (strlen(line) >= LINE_LEN-1) memmove(line, line+1, strlen(line)+1);
         strncat(line, (char *)&(buff[i]), 1);
       }
     fwrite(buff, c, 1, f);
     crc = calc_crc(buff, c, crc);
     to_load -= c;
   }

   if (abort_all) {abort_transfer(); return;}

   lp_enable_screen();
   lp_remove_statline();
   message("LinPac: Autobin RX finished, CRC = %i.\r", crc);
   if (!old_form)
     if (crc == ex_crc) message("        CRC check OK.\r");
                   else message("        CRC check FAILED.\r");

   fclose(f);
   if (ut.actime != 0) utime(fname, &ut);
}

/*-------------------------------------------------------------------------
   transmit - transmit a single file using autobin protocol
  -------------------------------------------------------------------------*/
void transmit(char *name)
{
  FILE *f;
  int cnt;
  long size, sent;
  char buf[BUFSIZE];
  struct stat fstat;
  struct tm *date;
  unsigned long dateinfo;
  char answer[10];
  char *p;
  char path[1024];

  lp_set_event_handler(my_handler);

  //path
  p = lp_get_var(0, "USER_PATH");
  if (p == NULL) strcpy(path, ".");
            else strcpy(path, p);
  strcat(path, "/");
  if (strchr(name, '/') == NULL) strcat(path, name);
                            else strcpy(path, name);

  //open file
  f = fopen(path, "r");
  if (f == NULL)
  {
    lp_appl_result("File not found");
    return;
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);

  //read file status
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

  printf("#BIN#%li#|%i#$%8lX#%s\r", size, file_crc(path), dateinfo, path);
  lp_statline("Autobin TX: request sent");
  strcpy(answer, "");
  do
  {
    fgets_cr(answer, 9, stdin);
    answer[strlen(answer)-1] = '\0';
  } while (strcasecmp(answer, "#OK#") != 0 &&
           strcasecmp(answer, "#NO#") != 0 &&
           strcasecmp(answer, "#ABORT#") != 0 &&
           !abort_all);

  if (abort_all) {abort_transfer(); return;}

  if (strcasecmp(answer, "#OK#") == 0)
  {
    lp_disable_screen();
    sent = 0;
    while (!feof(f) && !abort_all)
    {
      cnt = fread(buf, 1, BUFSIZE, f);
      fwrite(buf, 1, cnt, stdout);
      sent += cnt;
      lp_statline("Autobin TX: %s %i of %i", path, sent, size);
    }
    if (abort_all) {abort_transfer(); return;}
    lp_enable_screen();
    message("LinPac: Autobin TX finished, CRC = %i\r", file_crc(path));
  }
  else lp_appl_result("Transfer aborted\r");
  lp_remove_statline();

  fclose(f);
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
     
     if (strcmp(p, "autotx") == 0)  /* we'll send something */
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

