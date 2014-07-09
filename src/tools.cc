/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   tools.cc

   Various tool functions
   
   Last update 14.3.2000
  =========================================================================*/
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "tools.h"

char *conv_in[MAX_CHN+1];
char *conv_out[MAX_CHN+1];

FILE *errlog;

void Error(int no, const char *fmt, ...)
{
  va_list argptr;

  va_start(argptr, fmt);
  fprintf(stderr, "%s %s ", date_stamp(), time_stamp());
  vfprintf(stderr, fmt, argptr);
  if (no != 0) fprintf(stderr, " : %s\n", strerror(no));
          else fprintf(stderr, "\n");
  va_end(argptr);
}

char *strcopy(char *dest, const char *src, size_t n)
{
  strncpy(dest, src, n-1);
  dest[n-1] = '\0';
  return dest;
}

void striplf(char *s)
{
  if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
}

char *time_stamp(bool utc)
{
  struct tm *cas;
  time_t secs = time(NULL);
  if (utc) cas = gmtime(&secs);
      else cas = localtime(&secs);
  static char tt[32];
  //sprintf(tt, "%2i.%02i.%2i %2i:%02i:%02i", cas->tm_mday, cas->tm_mon+1,
  //        cas->tm_year, cas->tm_hour, cas->tm_min, cas->tm_sec);
  sprintf(tt, "%2i:%02i", cas->tm_hour, cas->tm_min);
  //strftime(tt, 30, "%X", cas);
  return tt;
}

char *date_stamp(bool utc)
{
  struct tm *cas;
  time_t secs = time(NULL);
  if (utc) cas = gmtime(&secs);
      else cas = localtime(&secs);
  static char tt[32];
  //sprintf(tt, "%i.%i.%i", cas->tm_mday, cas->tm_mon+1, cas->tm_year+1900);
  strftime(tt, 30, "%x", cas);
  return tt;
}

char *time_zone_name(int i)
{
  return tzname[i];
}

char *sys_info()
{
  static char s[128];
  struct utsname info;
  uname(&info);
  sprintf(s, "%s %s / %s", info.sysname, info.release, info.machine);
  return s;
}

#ifndef DAEMON_ONLY_MODE
//Prints into the ncurses window using specified attribute
void wprinta(void *win, int attr, const char *fmt, ...)
{
  va_list argptr;
  char s[256];
  unsigned i;

  va_start(argptr, fmt);
  vsprintf(s, fmt, argptr);
  va_end(argptr);
  for (i=0; i<strlen(s); i++) waddch((WINDOW *)win, s[i] | attr);
}
#endif

char *first_call(const char *src, char *dest)
{
  char *s = const_cast<char *>(src);
  char *d = dest;
  while (*s != '\0' && *s != ' ') *d++ = *s++;
  return dest;
}

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
  strcopy(c, call_call(call), 8);
  int ssid = call_ssid(call);
  if (ssid == 0) strcpy(call, c); else sprintf(call, "%s-%i", c, ssid);
  for (char *p = call; *p; p++) *p = toupper(*p);
  return call;
}

bool call_match(const char *call1, const char *call2)
{
  char s1[15];
  char s2[15];
  strcopy(s1, call_call(call1), 15);
  strcopy(s2, call_call(call2), 15);
  return (*s1 && *s2 && strcasecmp(s1, s2) == 0 && call_ssid(call1) == call_ssid(call2));
}

char *state_str(int state)
{
  static char s[20];
  switch (state)
  {
    case 0: strcpy(s, "Disconnected"); break;
    case 1: strcpy(s, "Link setup"); break;
    case 2: strcpy(s, "Disconnecting"); break;
    case 3: strcpy(s, "Info transfer"); break;
    case 4: strcpy(s, "Wait ack"); break;
    case 5: strcpy(s, "State 5"); break;
    case 6: strcpy(s, "State 6"); break;
    case 7: strcpy(s, "State 7"); break;
    case 8: strcpy(s, "State 8"); break;
    default: strcpy(s, "Unknown");
  }
  return s;
}

bool is_number(const char *s)
{
  char *p = const_cast<char *>(s);
  bool b = true;
  while (*p)
  {
    if (!isdigit(*p)) {b = false; break;}
    p++;
  }
  return b;
}

int load_conversion_tables(int chn, char *name)
{
  FILE *f;
  char fname[256];
  char buffer[512]; /* max 256 chars for input and output */

  sprintf(fname, "%s.ctt", name);
  f = fopen(fname, "r");
  if (f == NULL) return -1;

  int size = fread(buffer, 1, 512, f);
  conv_in[chn] = (char *)realloc(conv_in[chn], size/2);
  memcpy(conv_in[chn], buffer, size/2);
  conv_out[chn] = (char *)realloc(conv_out[chn], size/2);
  memcpy(conv_out[chn], buffer + size/2, size/2);
  fclose(f);
  return size/2;
}

bool compare_aliases(const char *alias1, const char *alias2)
{
  int i;
  for (i = 0; alias1[i] && alias2[i]; i++)
    if (!
        (toupper(alias1[i]) == toupper(alias2[i]) ||
         (alias1[i] == '-' && alias2[i] == '_') ||
         (alias1[i] == '_' && alias2[i] == '-'))) return false;
  if (alias1[i] != alias2[i]) return false; //different lengths
  return true;
}

bool get_enc_alias(const char *alias, char *name, char *table)
{
  FILE *f;
  f = fopen(ENCODINGS, "r");
  if (f)
  {
    char al[256];
    char nm[256];
    char tb[256];
    while (!feof(f))
    {
      char s[1024];
      strcpy(s, "");
      if (fgets(s, 1023, f) != NULL)
      {
        striplf(s);
        char *p = strchr(s, '#');
        if (p != NULL) *p = '\0';
        int n = sscanf(s, "%s %s %s", al, nm, tb);
        if ((n == 2 || n == 3) && compare_aliases(alias, al))
        {
          strcpy(name, nm);
          if (n == 3) strcpy(table, tb); else strcpy(table, "");
          return true;
        }
      }
    }
    fclose(f);
  }
  return false;
}


bool redirect_errorlog(char const *path)
{
  fflush(stderr);
  if (errlog != NULL) fclose(errlog);
  errlog = fopen(path, "a");
  if (errlog != NULL)
  {
    if (dup2(fileno(errlog), 2) == -1) return false;
  }
  else return false;
  Error(0, "Logging started");
  return true;
}

