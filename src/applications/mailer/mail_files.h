/*
  LinPac Mailer
  (c) 1998 - 2000 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_files.cc - routines for file paths
*/

#ifndef MAIL_FILES_H
#define MAIL_FILES_H

void get_file_paths();

char *load_file_path(const char *name);
char *save_file_path(const char *name);

#endif

