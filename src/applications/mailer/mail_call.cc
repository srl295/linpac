/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_call.cc - callsign conversion functions
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "mail_call.h"

/* return the first callsign in the path */
char *first_call(const char *src, char *dest)
{
  sscanf(src,"%10[A-Za-z0-9-]",dest);
  return(dest);
}

/* returns the callsign w/o SSID */
char *call_call(const char *src)
{
  static char s[7];
  sscanf(src, "%6[A-Za-z0-9]", s);
  /*char *p = (char *)(src);
  strcpy(s, "");
  while (*p && isalnum(*p) && strlen(s) < 6) {strncat(s, p, 1); p++;}*/
  return s;
}

int call_ssid(const char *src)
{
  char *p = const_cast<char *>(src);
  while (*p != '\0' && *p != '-') p++;
  if (*p) p++;
  if (*p) return atoi(p);
     else return 0;
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

char *addr_call(const char *addr)
{
   static char ret[7];
   char *p = (char *)addr;
   char *q = ret;
   int i = 0;
   while (*p && *p != '@' && i < 6) { *q = *p; p++; q++; i++; }
   *q = '\0';
   return ret;
}

char *addr_bbs(const char *addr)
{
   static char ret[64];
   const char *p = strchr(addr, '@');
   if (p)
   {
      strncpy(ret, p+1, 63); ret[63] = '\0';
   } else strcpy(ret, "");
   return ret;
}

