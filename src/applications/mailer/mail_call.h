/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_list.h - message list viewer and text viewer
*/
char *first_call(const char *src, char *dest);
int call_ssid(const char *src);
char *call_call(const char *src);
char *strip_ssid(char *src);
char *normalize_call(char *call);

char *addr_call(const char *addr);
char *addr_bbs(const char *addr);

