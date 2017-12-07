/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   editor.cc

   Text/command editor objects

   Last update 29.1.2001
  =========================================================================*/
#include <iostream>
#include <string.h>

#include "constants.h"
#include "data.h"
#include "tools.h"
#include "editor.h"

//uncoment this to make BS to skip to previous line when pressed at the
//begining of line
//#define BS_SKIP_LINES

//--------------------------------------------------------------------------
// Class Edline
//--------------------------------------------------------------------------
Edline::Edline()
{
  attr = ED_TEXT;
  s = new char[LINE_LEN+1];
  clrline();
}

Edline::~Edline()
{
  delete[] s;
}

Edline &Edline::operator = (const Edline &source)
{
  strcopy(s, source.s, LINE_LEN+1);
  attr = source.attr;
  return *this;
}

int Edline::lastpos()
{
  int i;
  for(i = LINE_LEN-1; i >= 0 && s[i]==' '; i--) ;
  return i;
}

void Edline::clrline()
{
  for (int i = 0; i < LINE_LEN; i++) s[i]=' ';
  s[LINE_LEN] = '\0';
}

void Edline::get_text(char *str)
{
  sprintf(str, "%.*s", lastpos()+1, s);
}

void Edline::accept(char *str)
{
  strcpy(s, str);
  for (int i = strlen(str); i < LINE_LEN; i++) s[i] = ' ';
  s[LINE_LEN] = '\0';
}

void Edline::draw(WINDOW *win, int y)
{
  unsigned i;
  for (i=0; i < LINE_LEN; i++)
    if (i < strlen(s))
    {
      if (s[i] < '\x1F')
        mvwaddch(win, y, i, (s[i]+64) | COLOR_PAIR(ED_CTRL) | fgattr(ED_CTRL));
      else
        mvwaddch(win, y, i, s[i] | COLOR_PAIR(attr) | fgattr(attr));
    }
    else
        mvwaddch(win, y, i, ' ' | COLOR_PAIR(attr));
}

//--------------------------------------------------------------------------
// Class Editor
//--------------------------------------------------------------------------
Editor::Editor(int wx1, int wy1, int wx2, int wy2, int maxlines, int ch)
{
  sprintf(class_name, "Editor%i", ch);
  act = false;
  x1 = wx1; y1 = wy1; x2=wx2; y2=wy2;
  limit = maxlines;
  max = 1;
  line = new Edline[limit];
  crx = 0;
  cry = 0;
  poz = 0;
  chnum = ch;
  conv = NULL;
  convcnt = 0;
  ctrlp = false;
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  scrollok(win, false);
  redraw(false);
}

void Editor::update(int wx1, int wy1, int wx2, int wy2)
{
  x1 = wx1; y1 = wy1; x2=wx2; y2=wy2;
  int old_cry = cry;
  if (cry > (y2-y1)) {cry = y2 - y1; poz += old_cry - cry;}
  delwin(win);
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  //if (act) redraw(); else redraw(false);
  redraw(false);
}

void Editor::handle_event(const Event &ev)
{
  if (ev.type==EV_SELECT_CHN) {act = (ev.chn == chnum); if (act) touchwin(win);}
  if (ev.type==EV_NONE && act) {wmove(win, cry, crx); wrefresh(win);}
  if (ev.type==EV_KEY_PRESS)
    if (act)
    {
      if (!ctrlp)
      {
        if (!ev.y) switch (ev.x)
        {
          case KEY_LEFT:  cr_left(); break;
          case KEY_RIGHT: cr_right(); break;
          case KEY_UP:    cr_up(); break;
          case KEY_DOWN:  cr_down(); break;
          case KEY_HOME:  cr_home(); break;
          case KEY_END:   cr_end(); break;
          case KEY_DC:
          case '\a':      del_ch(); break;
          case KEY_DL:
          case '\031':    delline(); break;
          case KEY_BACKSPACE:
          case '\b':      backspace(); break;
          case '\t':      cr_tab(); break;
          case KEY_ENTER:
          case '\r':
          case '\n':      line[poz+cry].get_text(sent);
                          convert_charset(sent);
                          if (sent[strlen(sent)-1] == '\\')
                            sent[strlen(sent)-1] = '\0';
                          else
                            strcat(sent, "\n");
                          emit(chnum, EV_TEXT_RAW, FLAG_EDIT, sent);
                          emit(chnum, EV_TEXT_FLUSH, 0, NULL);
                          if (iconfig(chnum, "state") == 0)
                            emit(chnum, EV_LINE_RECV, 0, sent);
                          newline();
                          break;
          case 16  :      ctrlp = true; break; // ctrl-P
          case 18  :
          case 22  :
          case 5   :
          case 2   :
          case 24  :      break; //keys used by QSO-Window
          default:
                if (ev.x <= 255) newch(ev.x);
        }
        if (act) wrefresh(win);
      } //if (ctrlp)
      else
      {
        ctrlp = false;
        if (ev.x <= 255) newch(ev.x);
      }
    }

  if (ev.type==EV_KEY_PRESS_MULTI)
    if (act)
    {
        // Treat this the same as if we received multiple EV_KEY_PRESS events
        // where each event is guaranteed to be only a printable character.
        char *buffer = (char *)ev.data;
        for (unsigned ix = 0; ix < strlen(buffer); ix++)
            newch(buffer[ix]);
        ctrlp = false;
    }

  if (act && ev.type == EV_EDIT_INFO) ins_info((char *)ev.data);

  if (ev.type == EV_STAT_LINE && chnum != 0)
  {
    if (bconfig("swap_edit"))
      update(x1, y1, x2, ev.x-1);
    else
      update(x1, ev.x+1, x2, y2);
  }

  if (ev.type == EV_CHN_LINE && chnum != 0)
  {
    if (!bconfig("swap_edit"))
      update(x1, y1, x2, ev.x-1);
  }

  if (ev.type == EV_SWAP_EDIT && chnum != 0)
  {
    if (bconfig("swap_edit"))
       update(0, iconfig("qso_start_line"), iconfig("max_x"),
                 iconfig("qso_end_line"));
    else
       update(0, iconfig("edit_start_line"), iconfig("max_x"),
                 iconfig("edit_end_line"));
  }
  
  if (ev.type == EV_CONV_OUT && (ev.chn == chnum || ev.chn == 0))
  {
    if (conv != NULL) delete[] conv;
    conv = new char[ev.x];
    memcpy(conv, ev.data, ev.x);
    convcnt = ev.x;
  }        
}

void Editor::draw()
{
  if (act) wrefresh(win);
}

void Editor::redraw(bool update)
{
  int i;

  wclear(win);
  wbkgd(win, ' ' | COLOR_PAIR(ED_TEXT) | fgattr(ED_TEXT));
  wbkgdset(win, ' ');
  for (i = 0; i <= (y2-y1); i++)
    if (poz+i < max) line[i+poz].draw(win, i);
  wmove(win, cry, crx);
  if (update) wrefresh(win);
}

void Editor::draw_line(int n)
{
  if ((n >= poz) && (n <= poz+(y2-y1)))
  {
    line[n].draw(win, n-poz);
  }
  wmove(win, cry, crx);
}

void Editor::cr_right()
{
  crx++;
  if (crx >= LINE_LEN) {crx = 0; cr_down();}
  wmove(win, cry, crx);
}

void Editor::cr_left()
{
  crx--;
  if (crx < 0) {cr_up(); cr_end();}
  wmove(win, cry, crx);
}

void Editor::cr_up()
{
  if (poz > 0 || cry > 0)
  {
    if (cry==0) {poz--; redraw();}
           else cry--;
  }
  wmove(win, cry, crx);
}

void Editor::cr_down()
{
  if (poz+cry < max-1)
  {
    if (cry==y2-y1) {poz++; redraw();}
               else cry++;
  }
  wmove(win, cry, crx);
}

void Editor::delline()
{
  if (max-1 > cry+poz)
  {
    int i;
    for (i = cry+poz+1; i < max; i++) line[i-1] = line[i];
    line[max-1].clrline();
    max--;
  }
  redraw();
}

void Editor::cr_home()
{
  crx = 0;
  wmove(win, cry, crx);
}

void Editor::cr_end()
{
  crx = line[cry+poz].lastpos() + 1;
  if (crx > x2 - x1 + 1) crx = x2 - x1 + 1;
  wmove(win, cry, crx);
}

void Editor::cr_tab()
{
  if (crx < LINE_LEN)
    do crx++; while (crx < LINE_LEN && crx%TAB_SIZE != 0);
  wmove(win, cry, crx);
}

void Editor::backspace()
{
  if (crx > 0)
  {
    int i;

    crx--;
    for (i = crx; i < LINE_LEN-1; i++) line[poz+cry].s[i] = line[poz+cry].s[i+1];
    line[poz+cry].s[i]=' ';
    draw_line(poz+cry);
  }
  #ifdef BS_SKIP_LINES
  else cr_left();
  wmove(win, cry, crx);
  #endif
}

void Editor::del_ch()
{
  int i;

  for (i = crx; i < LINE_LEN-1; i++) line[poz+cry].s[i] = line[poz+cry].s[i+1];
  line[poz+cry].s[i]=' ';
  draw_line(poz+cry);
  wmove(win, cry, crx);
}

void Editor::newline()
{
  cr_home();
  if (poz+cry < max-1) cr_down();
  else
  {
    if (max < limit)
    {
      max++;
      cr_down();
    }
    else
    {
      int i;
      for (i=1; i<max; i++) line[i-1] = line[i];
      line[max-1].clrline();
      redraw();
    }
  }
  wmove(win, cry, crx);
}

void Editor::ins_info(char *ch)
{
  line[cry+poz].accept(ch);
  line[cry+poz].attr = ED_INFO;
  newline();
  redraw();
}

void Editor::newch(char ch)
{
  int i;
  char next_line[LINE_LEN+1];

  line[poz+cry].attr = ED_TEXT;
  for (i=LINE_LEN-1; i>crx; i--) line[poz+cry].s[i] = line[poz+cry].s[i-1];
  line[poz+cry].s[crx] = ch;
  draw_line(poz+cry);
  crx++;
  if (crx >= LINE_LEN) //end of line exceeded
  {
    char *p = strrchr(line[poz+cry].s, ' ');
    if (p != NULL)
    {
      strcopy(next_line, p+1, LINE_LEN);
      while (*p) {*p = ' '; p++;} //clear the rest
    }
    else strcpy(next_line, "");
    line[poz+cry].get_text(sent);
    convert_charset(sent);
    strcat(sent, "\n");
    emit(chnum, EV_TEXT_RAW, FLAG_EDIT, sent);
    emit(chnum, EV_TEXT_FLUSH, 0, NULL);
    draw_line(poz+cry);
    newline();
    line[poz+cry].accept(next_line);
    crx = strlen(next_line);
    draw_line(poz+cry);
  }
  wmove(win, cry, crx);
}

void Editor::convert_charset(char *s)
{
  char *p = s;
  while (*p)
  {
    if (*p < convcnt) *p = conv[(unsigned)*p];
    p++;
  }
}

Editor::~Editor()
{
}

