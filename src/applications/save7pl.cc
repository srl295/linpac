/*
   7plus autosave for LinPac
   (c) 1998 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Note: When running from LinPac use the B (binary) flag. save7pl works
         in binary mode and uses CR as EOLN.
*/

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include "lpapp.h"
#include "crc.h"

#define LINE_LEN 80
#define LINE_7PLUS 71      /* bytes per line in 7pl. file (incl. CR) */
#define BUFSIZE 256
#define DECODE_CMD "7plus"
#define ABORT_ADDR "7plus"

#define TYPE_BIN 0
#define TYPE_TEXT 1

int abort_all = 0;         /* 1 = abort event was received */
int lp_active = 0;         /* is LinPac active ? */
int send_msgs = 1;         /* send messages to remote user ? */
int type;                  /* TYPE_BIN (7+) or TYPE_TEXT (text, info) */

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

  if (send_msgs) fputs(msg, stdout);
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
    if (ch == EOF) {s[cnt++] = '\0'; break;}
    s[cnt++] = ch;
    if (ch == '\r') {s[cnt++] = '\0'; break;}
  }
  return s;
}

//Try to decode 7plus file
int try_decode(char *path, char *name, int parts)
{
  char cmd[LINE_LEN];
  int r;

  sprintf(cmd, "cd %s; %s %s > /dev/null; cd $OLDPWD", path, DECODE_CMD, name);
  lp_event_handling_off();
  r = system(cmd);
  lp_event_handling_on();
  /*if (r == 0) printf("save7pl: File succesfully decoded.\r");
              else printf(strerror(errno));*/
  return r;
}

//Recieve the file
void receive()
{
  FILE *f;
  char line[LINE_LEN];
  int part;
  int parts;
  long binbytes;
  int lines, blocklines;
  int recvd;
  char fname[LINE_LEN], prefix[LINE_LEN], path[LINE_LEN], ppath[LINE_LEN];
  char buf[20], tmp[80];
  int stop = 0;
  char *path_var;

  //path
  path_var = lp_get_var(0, "SAVE_PATH");
  if (path_var == NULL) strcpy(path, ".");
                   else strcpy(path, path_var);
  strcat(path, "/");

  //wait for header
  type = -1;
  while (type < 0 && !abort_all)
  {
    lp_statline("Waiting for 7plus header");
    do
    {
      fgets_cr(line, LINE_LEN-1, stdin);
    } while(strncmp(line, " go_", 4) != 0);
   if (strncmp(line, " go_7+.", 7) == 0) type = TYPE_BIN;
   if (strncmp(line, " go_text.", 9) == 0) type = TYPE_TEXT;
   if (strncmp(line, " go_info.", 9) == 0) type = TYPE_TEXT;
  }

  if (abort_all) return;

  //decode header
  if (type == TYPE_BIN)
  {
    char *p;
    sscanf(line+8, "%d %s %d %s %ld %s %s",
           &part, buf, &parts, fname, &binbytes, buf, tmp);
    sscanf(tmp, "%x", &blocklines);
    lines = blocklines;
    if (part == parts)
      lines = (int) (((binbytes +61) /62) % blocklines);
    if (!lines)
      lines = blocklines;
    lines += 2;
    for (p = fname; *p; p++) *p = tolower(*p);
  }
  else
    sscanf(line+10, "%s", fname);

  //determine filename (if 7plus) and open file
  if (type == TYPE_BIN)
  {
    char suffix[5];
    char *p = strrchr(fname, '.');
    if (p != NULL) *p = '\0';
    if (parts == 1) strcpy(suffix, ".7pl");
               else sprintf(suffix, ".p%02x", part);
    strcpy(prefix, fname);
    strcat(fname, suffix);
  }
  strcpy(ppath, path); strcat(ppath, fname);
  f = fopen(ppath, "a+");
  if (f == NULL)
  {
    printf("Cannot create file %s\r", ppath);
    return;
  }
  fprintf(f, "%s\n", line);

  //receive
  if (type == TYPE_BIN)
  {
    lp_disable_screen();
  }

  recvd = 0;
  stop = 0;
  while (!stop && !abort_all)
  {
    if (type == TYPE_BIN)
      lp_statline("7plus: recieving %s : %i of %i", fname, recvd, lines*LINE_7PLUS);
    else
      lp_statline("7plus: recieving %s : %i of unknown", fname, recvd);
    fgets_cr(line, LINE_LEN-1, stdin);
    if (line[0] == '\n') fprintf(f, "%s\n", line+1);
                    else fprintf(f, "%s\n", line);
    recvd += strlen(line)+1;
    if (strncmp(line, " stop_", 6) == 0) stop = 1;
  }

  lp_remove_statline();
  if (type == TYPE_BIN)
  {
    lp_enable_screen();
    message("\rsave7pl: %s saved\r", ppath);
  }
  fclose(f);
  if (type == TYPE_BIN) (void)try_decode(path, prefix, parts);
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

     /* receive */
     lp_set_event_handler(my_handler);
     receive();
     lp_end_appl();
   }
   else printf("LinPac is not running\n");
   return 0;
}

