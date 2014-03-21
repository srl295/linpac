/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_input.cc - input line classes
*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>

#include "mail_input.h"

#define LINE_LEN 200

WINDOW *win;

//--------------------------------------------------------------------------
// Class InputLine
//--------------------------------------------------------------------------
InputLine::InputLine(void *parent, int wx, int wy, int flen, unsigned textlen,
                    char const *prompt, char *buffer, int mode)
{
  sprintf(class_name, "InputLine");
  x = wx; y = wy; fieldln = flen; textln = textlen;
  strncpy(ptext, prompt, 255);
  buf = buffer;
  imode = mode;

  strcpy(buf, "");

  win = subwin((WINDOW *)parent, 1, fieldln, y, x);
  scrollok(win, false);
  draw(true);

  old_focus_window = focus_window;
  focus_window = win;
  old_focused = focused;
  focused = this;
}

void InputLine::handle_event(Event *ev)
{
  if (ev->type==EV_KEY_PRESS)
  {
      switch (ev->x)
      {
        case KEY_BACKSPACE:
        case '\b':      if (strlen(buf) > 0) buf[strlen(buf) - 1] = '\0';
                        break;
        case KEY_ENTER:
        case '\r':
        case '\n':      focus_window = old_focus_window;
                        focused = old_focused;
                        delwin(win);
                        focused->draw(true);
                        focused->handle_event(NULL);
                        break;
        case '\x1b':    strcpy(buf, "");
                        focus_window = old_focus_window;
                        focused = old_focused;
                        delwin(win);
                        focused->draw(true);
                        focused->handle_event(NULL);
                        break;
        default:
              char ch;
              switch (imode)
              {
                case INPUT_ALL:
                  if (ev->x > ' ' && !ev->y)
                  {
                    ch = ev->x;
                    if (strlen(buf) <= textln) strncat(buf, &ch, 1);
                  }
                  break;
                case INPUT_ALNUM:
                  if (isalnum(ev->x) && !ev->y)
                  {
                    ch = ev->x;
                    if (strlen(buf) <= textln) strncat(buf, &ch, 1);
                  }
                  break;
                case INPUT_UPC:
                  if (isalnum(ev->x) && !ev->y)
                  {
                    ch = toupper(ev->x);
                    if (strlen(buf) <= textln) strncat(buf, &ch, 1);
                  }
                  break;
                case INPUT_NUM:
                  if (isdigit(ev->x) && !ev->y)
                  {
                    ch = ev->x;
                    if (strlen(buf) <= textln) strncat(buf, &ch, 1);
                  }
                  break;
                case INPUT_ONECH:
                  if (isalnum(ev->x) && !ev->y)
                  {
                    ch = ev->x;
                    strncat(buf, &ch, 1);
                    focus_window = old_focus_window;
                    focused = old_focused;
                    delwin(win);
                    focused->draw(true);
                    focused->handle_event(NULL);
                  }
                  break;
              }
      }
      if (focused == this) draw();
  }
}

void InputLine::draw(bool all)
{
  if (all)
  {
    wclear(win);
    wbkgd(win, ' ' | COLOR_PAIR(IL_TEXT));
    wbkgdset(win, ' ');
    mvwprintw(win, 0, 1, "%s ", ptext);
  }
  wbkgd(win, ' ' | COLOR_PAIR(IL_TEXT));
  mvwprintw(win, 0, strlen(ptext)+2, "%s ", buf);
  wmove(win, 0, strlen(ptext) + strlen(buf) + 2);
  wrefresh(win);
}
