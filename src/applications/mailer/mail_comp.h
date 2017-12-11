/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_comp.h - message composer interface
*/
extern char old_help[80]; //help text to be set back after editor finishes

void start_composer(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, char *toaddr, char *subject);
void comp_insert(char *s, bool quote = true);
void comp_edredraw();

