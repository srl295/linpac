/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   screen.cc

   Objects for user interaction

   Last update 23.1.2000
  =========================================================================*/
#include <ctype.h>
#include <string.h>
#include "constants.h"
#include "screen.h"

int foreground_attr[COL_NUM];
int background_attr[COL_NUM];

//-------------------------------------------------------------------------
// Class Screen
//-------------------------------------------------------------------------
void Screen::insert(Screen_obj *obj)
{
  *inserter(children, children.end()) = obj;
}

/*void Screen::remove(Screen_obj *obj)
{
  Screen_obj **it;
  for (it=children.begin(); it<children.end(); it++)
    if (*it==obj) break;
  if (*it==obj) children.erase(it, it++);
}*/

void Screen::redraw()
{
  std::vector <Screen_obj *>::iterator it;
  for (it=children.begin(); it<children.end(); it++)
    (*it)->draw();
}

Screen::Screen()
{
  strcpy(class_name, "SCREEN");
  mainwin=initscr();
  start_color();
  cbreak();
  noecho();
  keypad(stdscr, true);
  meta(stdscr, true);
  nodelay(stdscr, true);

  for (int i = 0; i < COL_NUM; i++)
    foreground_attr[i] = background_attr[i] = A_NORMAL;

  //Setup default colors
  foreground_attr[QSO_RX] = A_BOLD;
  foreground_attr[QSO_TX] = A_BOLD;
  foreground_attr[QSO_INFO] = A_BOLD;
  foreground_attr[QSO_CTRL] = A_BOLD;
  foreground_attr[CI_NORM] = A_BOLD;
  foreground_attr[CI_SLCT] = A_BOLD;
  foreground_attr[CI_NCHN] = A_BOLD;
  foreground_attr[CI_PRVT] = A_BOLD;
  foreground_attr[IL_TEXT] = A_BOLD;

  init_pair(QSO_RX, COLOR_GREEN, COLOR_BLACK);
  init_pair(QSO_TX, COLOR_RED, COLOR_BLACK);
  init_pair(QSO_INFO, COLOR_CYAN, COLOR_BLACK);
  init_pair(QSO_CTRL, COLOR_RED, COLOR_WHITE);

  init_pair(ED_TEXT, COLOR_BLACK, COLOR_GREEN);
  init_pair(ED_INFO, COLOR_BLUE, COLOR_GREEN);
  init_pair(ED_CTRL, COLOR_RED, COLOR_GREEN);

  init_pair(CI_NORM, COLOR_RED, COLOR_WHITE);
  init_pair(CI_SLCT, COLOR_YELLOW, COLOR_MAGENTA);
  init_pair(CI_NCHN, COLOR_YELLOW, COLOR_CYAN);
  init_pair(CI_PRVT, COLOR_RED, COLOR_CYAN);

  init_pair(IL_TEXT, COLOR_WHITE, COLOR_BLUE);
}

void Screen::handle_event(const Event &ev)
{
  if (ev.type == EV_REDRAW_SCREEN) redraw();
  if (ev.type == EV_CHANGE_COLOR)
  {
    char arg[3][20];
    short code, fg, bg;
    int i;    
    // read arguments
    char *p = reinterpret_cast<char *>(ev.data);
    for (i = 0; i<3; i++)
    {
      strcpy(arg[i], "");
      while (*p && isspace(*p)) p++;
      while (*p && !isspace(*p))
      {
        strncat(arg[i], p, 1);
        p++;
      }
      p++;
    }
    code = color_code(arg[0]);
    if (code == -1)
    {
      char msg[128];
      sprintf(msg, "Unknown palette code");
      emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, msg);
      return;
    }
    fg = color_num(arg[1]);
    bg = color_num(arg[2]);
    if (fg == -1 || bg == -1)
    {
      char msg[128];
      sprintf(msg, "Unknown color name");
      emit(ev.chn, EV_CMD_RESULT, FLAG_NO_HANDLE, msg);
      return;
    }
    init_pair(code, fg, bg);
    foreground_attr[code] = color_attr(arg[1]);
    background_attr[code] = color_attr(arg[2]);
  }    
}

void Screen::stop()
{
  clear();
  refresh();
  endwin();
}

Screen::~Screen()
{
  stop();
}

//--------------------------------------------------------------------------

short color_num(char *name)
{
  if (strcasecmp(name, "BLACK") == 0) return COLOR_BLACK;
  if (strcasecmp(name, "RED") == 0) return COLOR_RED;
  if (strcasecmp(name, "GREEN") == 0) return COLOR_GREEN;
  if (strcasecmp(name, "BROWN") == 0) return COLOR_YELLOW;
  if (strcasecmp(name, "BLUE") == 0) return COLOR_BLUE;
  if (strcasecmp(name, "MAGENTA") == 0) return COLOR_MAGENTA;
  if (strcasecmp(name, "CYAN") == 0) return COLOR_CYAN;
  if (strcasecmp(name, "LIGHTGRAY") == 0) return COLOR_WHITE;

  if (strcasecmp(name, "DARKGRAY") == 0) return COLOR_BLACK;
  if (strcasecmp(name, "LIGHTRED") == 0) return COLOR_RED;
  if (strcasecmp(name, "LIGHTGREEN") == 0) return COLOR_GREEN;
  if (strcasecmp(name, "YELLOW") == 0) return COLOR_YELLOW;
  if (strcasecmp(name, "LIGHTBLUE") == 0) return COLOR_BLUE;
  if (strcasecmp(name, "LIGHTMAGENTA") == 0) return COLOR_MAGENTA;
  if (strcasecmp(name, "LIGHTCYAN") == 0) return COLOR_CYAN;
  if (strcasecmp(name, "WHITE") == 0) return COLOR_WHITE;
  return -1;
}

int color_attr(char *name)
{
  if (strcasecmp(name, "BLACK") == 0 ||
      strcasecmp(name, "RED") == 0   ||
      strcasecmp(name, "GREEN") == 0 ||
      strcasecmp(name, "BROWN") == 0 ||
      strcasecmp(name, "BLUE") == 0  ||
      strcasecmp(name, "MAGENTA") == 0 ||
      strcasecmp(name, "CYAN") == 0  ||
      strcasecmp(name, "LIGHTGRAY") == 0) return A_NORMAL;

  if (strcasecmp(name, "DARKGRAY") == 0 ||
      strcasecmp(name, "LIGHTRED") == 0   ||
      strcasecmp(name, "LIGHTGREEN") == 0 ||
      strcasecmp(name, "YELLOW") == 0 ||
      strcasecmp(name, "LIGHTBLUE") == 0  ||
      strcasecmp(name, "LIGHTMAGENTA") == 0 ||
      strcasecmp(name, "LIGHTCYAN") == 0  ||
      strcasecmp(name, "WHITE") == 0) return A_BOLD;

  return -1;
}

short color_code(char *code)
{
  if (strcasecmp(code, "QSO_RX") == 0) return QSO_RX;
  if (strcasecmp(code, "QSO_TX") == 0) return QSO_TX;
  if (strcasecmp(code, "QSO_INFO") == 0) return QSO_INFO;
  if (strcasecmp(code, "QSO_CTRL") == 0) return QSO_CTRL;
  if (strcasecmp(code, "ED_TEXT") == 0) return ED_TEXT;
  if (strcasecmp(code, "ED_INFO") == 0) return ED_INFO;
  if (strcasecmp(code, "ED_CTRL") == 0) return ED_CTRL;
  if (strcasecmp(code, "CI_NORM") == 0) return CI_NORM;
  if (strcasecmp(code, "CI_SLCT") == 0) return CI_SLCT;
  if (strcasecmp(code, "CI_NCHN") == 0) return CI_NCHN;
  if (strcasecmp(code, "CI_PRVT") == 0) return CI_PRVT;
  if (strcasecmp(code, "IL_TEXT") == 0) return IL_TEXT;
  if (strcasecmp(code, "COL_NUM") == 0) return COL_NUM;
  return -1;
}

int fgattr(int code)
{
  return foreground_attr[code];
}

int bgattr(int code)
{
  return background_attr[code];
}

