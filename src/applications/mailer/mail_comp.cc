/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_comp.cc - message composer interface
*/
#include <ncurses.h>
#include "mail_comp.h"
#include "mail_edit.h"

Composer *comp;

void start_composer(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, char *toaddr, char *subject)
{
  comp = new Composer(parent, wx1, wy1, wx2, wy2, toaddr, subject);
}

void comp_insert(char *s, bool quote)
{
  comp->insert(s, quote);
}

void comp_edredraw()
{
  comp->edredraw();
}

