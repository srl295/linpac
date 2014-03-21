/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_screen.cc - screen management
*/
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include "mail_screen.h"

bool act;
bool stop;
int maxx, maxy;
void *main_window, *focus_window;
screen_obj *focused = NULL;
screen_obj * message_handler[16];

char ttable[256];
int tabsize;
char tables[32][16];
int ntables = 0;

bool view_returned = false;
bool blist_returned = false;

char *mailpath = NULL;

void init_main_screen()
{
  initscr();

  maxx = stdscr->_maxx;
  maxy = stdscr->_maxy;
  noecho();
  cbreak();
  nodelay(stdscr, TRUE);
  start_color();
  init_pair(C_BACK, COLOR_WHITE, COLOR_BLUE);
  init_pair(C_TEXT, COLOR_WHITE, COLOR_GREEN);
  init_pair(C_SELECT, COLOR_YELLOW, COLOR_CYAN);
  init_pair(C_UNSELECT, COLOR_YELLOW, COLOR_GREEN);
  init_pair(C_MSG, COLOR_YELLOW, COLOR_CYAN);
  init_pair(C_ERROR, COLOR_WHITE, COLOR_RED);
  init_pair(ED_TEXT, COLOR_WHITE, COLOR_BLUE);
  init_pair(ED_INFO, COLOR_CYAN, COLOR_BLUE);
  init_pair(ED_CTRL, COLOR_RED, COLOR_BLUE);
  init_pair(ED_STAT, COLOR_MAGENTA, COLOR_BLUE);
  init_pair(IL_TEXT, COLOR_BLUE, COLOR_CYAN);
  WINDOW *win = newwin(maxy+1, maxx+1, 0, 0);
  if (win == NULL)
  {
    fprintf(stderr, "Cannot create main window\n");
    exit(1);
  }
  main_window = win;
  focus_window = NULL;
  tabsize = 0;
  for (int i = 0; i < 16; i++) message_handler[i] = NULL;
}

void redraw()
{
   WINDOW *win = reinterpret_cast<WINDOW *>(main_window);
   win->_clear = TRUE;
   redrawwin(win);
   wrefresh(win);
   /*if (focus_window != NULL)
   {
     WINDOW *win = reinterpret_cast<WINDOW *>(focus_window);
     win->_clear = TRUE;
     redrawwin(win);
   }*/
}

void help(char const *s)
{
  WINDOW *win = reinterpret_cast<WINDOW *>(main_window);
  int ct = CENTER(strlen(s))-1;
  mvwprintw(win, maxy-1, 1, "%*s%s%*s", ct, "", s, maxx-strlen(s)-ct-1, "");
  wrefresh(win);
}

void load_table(int n)
{
  FILE *f;

  if (n < ntables)
  {
    f = fopen(tables[n], "r");
    if (f == NULL) {beep(); return;}
    tabsize = fread(ttable, 1, 256, f);
    fclose(f);
  }
}

int safe_wrefresh(void *win)
{
  //event_handling_off();
  int r = wrefresh((WINDOW *)win);
  //event_handling_on();
  return r;
}

//------------------------------------------------------------------------

void Main_scr::handle_event(Event *ev)
{
}

void Main_scr::draw(bool all)
{
  WINDOW *win = reinterpret_cast<WINDOW *>(main_window);
  wbkgdset(win, ' ' | COLOR_PAIR(C_BACK));
  werase(win);
  box(win, ACS_VLINE, ACS_HLINE);
  mvwaddstr(win, 2, CENTER(26), "LinPac Packet Radio Mailer");
  mvwaddstr(win, 3, CENTER(26), "(c) 1998 - 2001  by OK2JBG");
  wrefresh(win);
}

WINDOW *msgwin;

void Main_scr::show_message(char *text)
{
  if (!blocked)
  {
    blocked = true;
    WINDOW *win = reinterpret_cast<WINDOW *>(focus_window);
    int len = strlen(text) + 4;
    msgwin = subwin(win, 5, len, 15, CENTER(len));
    wbkgdset(msgwin, ' ' | COLOR_PAIR(C_BACK));
    werase(msgwin);
    box(msgwin, ACS_VLINE, ACS_HLINE);
    mvwaddstr(msgwin, 2, 2, text);
    wrefresh(msgwin);
  }
}

void Main_scr::discard_message()
{
  if (blocked)
  {
    blocked = false;
    delwin(msgwin);
  }
}

