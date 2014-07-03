/*
  LinPac Mailer
  (c) 1998 - 2000 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_files.cc - routines for file paths
*/

#include <string.h>
#include "lpapp.h"

char *save_path;
char *user_path;

void get_file_paths()
{
   char *p;
   p = lp_get_var(0, "SAVE_PATH");
   if (p == 0) save_path = strdup("");
          else save_path = strdup(p);

   p = lp_get_var(0, "USER_PATH");
   if (p == 0) user_path = strdup("");
          else user_path = strdup(p);
}

char *load_file_path(const char *name)
{
   if (name[0] == '/') return strdup(name);
   else
   {
      char *buf = new char[strlen(name)+strlen(user_path)+2];
      strcpy(buf, user_path);
      strcat(buf, "/");
      strcat(buf, name);
      return buf;
   }
}

char *save_file_path(const char *name)
{
   if (name[0] == '/') return strdup(name);
   else
   {
      char *buf = new char[strlen(name)+strlen(save_path)+2];
      strcpy(buf, save_path);
      strcat(buf, "/");
      strcat(buf, name);
      return buf;
   }
}

