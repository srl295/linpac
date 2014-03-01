/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2001

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   windows.h

   This module contains objects for terminal output windows

   Last update 29.10.2001
  =========================================================================*/
#include "windows.h"
#include <errno.h>
#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <sys/stat.h>

#include "tools.h"
#include "data.h"
#include "screen.h"
#include "version.h"

#ifdef WITH_AX25SPYD
#include <sys/socket.h>
#include <netinet/in.h>
#endif

//-------------------------------------------------------------------------
// Commonly used routines
//-------------------------------------------------------------------------

int act_col = COL_NUM;

//Allocate a new color pair or use an old pair when present
int alloc_pair(short f, short b)
{
  short ff, bb;
  int i, j=0;
  for (i=1; i < act_col; i++) //search in already allocated colors
  {
    pair_content(i, &ff, &bb);
    if (ff == f && bb == b) {j = i; break;}
  }
  if (j == 0)
  {                                       //allocate new color pair
    j = act_col;
    if (act_col < COLOR_PAIRS) act_col++;
    init_pair(j, f ,b);
  }
  return j;
}

//Draw a TLine
void putline(WINDOW *win, int y, TLine &lin, bool inv)
{
  int i;
  short cp;

  for (i=0; i<lin.length; i++)
  {
    int f = lin[i].typ % 16;
    int b = lin[i].typ / 16;
    bool bright = (f > 8);
    if (f > 8) f -= 8;
    cp = alloc_pair(f, b);
    if (bright)
      if (inv) mvwaddch(win, y, i, lin[i].ch | COLOR_PAIR(cp) | A_BOLD | WA_REVERSE);
          else mvwaddch(win, y, i, lin[i].ch | COLOR_PAIR(cp) | A_BOLD);
    else
      if (inv) mvwaddch(win, y, i, lin[i].ch | COLOR_PAIR(cp) | WA_REVERSE);
          else mvwaddch(win, y, i, lin[i].ch | COLOR_PAIR(cp));
  }
}

//-------------------------------------------------------------------------
// Class TChar
//-------------------------------------------------------------------------
/*TChar &TChar::operator = (const TChar &src)
{
  ch=src.ch;
  typ=src.typ;
  return *this;
}*/

//-------------------------------------------------------------------------
// Class TLine
//-------------------------------------------------------------------------
TLine::TLine()
{
  void_ch.ch = ' ';
  void_ch.typ = 1;
  length = LINE_LEN;
  data = new TChar[length];
}

TLine::~TLine()
{
  delete[] data;
}

TChar & TLine::operator [] (int n)
{
  if ((n >= 0) && (n < length)) return data[n];
                             else return void_ch;
}

TLine & TLine::operator = (const TLine &src)
{
  int i;
  for (i=0; i<length; i++) data[i]=src.data[i];
  return *this;
}

char *TLine::getstring(char **s)
{
  int i;
  *s = new char[length+1];
  for (i=0; i<length; i++) *s[i]=data[i].ch;
  *s[length]='\0';
  return *s;
}

void TLine::clear_it(int attr)
{
  short f, b;
  pair_content(attr, &f, &b);
  if (fgattr(attr) == A_BOLD) f += 8;
  for (int i=0; i<length; i++) {data[i].ch=' '; data[i].typ=b*16 + f;}
}

//-------------------------------------------------------------------------
// Class LineStack
//-------------------------------------------------------------------------
LineStack::LineStack(char *fname, size_t bsize)
{
  umask(0);
  strcopy(filename, fname, 256);
  f = open(fname, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
  if (f==-1) Error(errno, "LineStack: Error opening/creating workfile");

  step = LINE_LEN * TCH_SIZE;
  long flen = lseek(f, 0, SEEK_END);
  fsize = flen/step;

  poz = 0;
  set_hi_mark(512*1024);
  set_lo_mark(384*1024);
  truncb = false;

  bufsize = bsize - bsize%step; //whole lines
  buffer = new char[bufsize];   //create buffer
  init_buffer();
}

void LineStack::init_buffer()
{
  long flen = lseek(f, 0, SEEK_END);
  fsize = flen/step;
  if (flen >= bufsize)
    bufstart = lseek(f, -bufsize, SEEK_END); //index of start of buffer in file
  else
  {
    lseek(f, 0, SEEK_SET);            //begining of the file
    bufstart = 0;
  }
  bufend = read(f, buffer, bufsize);  //real buffer size
}

void LineStack::get(TLine &line)
{
  int i;
  char ch;

  long lpos = poz * step;
  if (lpos >= bufstart)
  {
    if (poz < fsize)
    {
      for (i=0; i<line.length; i++)
      {
        line[i].ch = buffer[lpos-bufstart+2*i];
        line[i].typ = int(buffer[lpos-bufstart+2*i+1]);
      }
      poz++;
    }
  }
  else
  {
    lseek(f, lpos, SEEK_SET);
    for (i=0; i<line.length; i++)
    {
      read(f, &ch, 1);
      line[i].ch = ch;
      read(f, &ch, 1);
      line[i].typ = int(ch);
    }
    poz++;
  }
}

void LineStack::store(TLine &line)
{
  long phsize;
  int i;

  if (bufend == bufsize) //buffer full
  {
    phsize = lseek(f, 0, SEEK_END);
    if (phsize == bufstart) write(f, buffer, step);
    bufstart += step;
    memmove(buffer, buffer+step, bufsize-step);
    for (i=0; i<line.length; i++)
    {
      buffer[bufsize-step+2*i] = line[i].ch;
      buffer[bufsize-step+2*i+1] = (char)line[i].typ;
    }
    fsize++;
  }
  else
  {
    for (i=0; i<line.length; i++)
    {
      buffer[bufend+2*i] = line[i].ch;
      buffer[bufend+2*i+1] = (char)line[i].typ;
    }
    bufend += step;
    fsize++;
  }
  check_buffers();
}

void LineStack::go(long pos)
{
  poz = pos;
}

void LineStack::flush()
{
  long phsize = lseek(f, 0, SEEK_END);
  if (phsize < bufstart+bufend)  //is something to flush ?
    write(f, buffer+(phsize-bufstart), bufend-(phsize-bufstart));
  init_buffer(); //re-read buffer
}

void LineStack::set_hi_mark(long size)
{
  himark = size + step - (size % step); //align to whole lines
  if (himark <= bufsize) himark = bufsize + step;
}

void LineStack::set_lo_mark(long size)
{
  lomark = size + step - (size % step); //align to whole lines
  if (lomark <= bufsize) lomark = bufsize + step;
}

void LineStack::check_buffers()
{
  if (himark <= lomark) himark = lomark + step;
  if (bufstart + bufend > himark)  //Hi mark exceeded
  {
    long old_fsize = fsize;
    flush(); //all to the file
    char tmpname[1024];
    lseek(f, -lomark, SEEK_END); //truncate to low mark size
    sprintf(tmpname, "%s.tmp", filename);
    int newfile = open(tmpname, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (newfile == -1)
    {
      Error(errno, "Cannot open file %s", tmpname);
      return;
    }
    char *copy = new char[lomark];
    if (copy == NULL)
    {
      Error(errno, "Cannot allocate buffers");
      return;
    }
    int rd = read(f, copy, lomark);
    write(newfile, copy, rd);
    close(newfile);
    close(f);
    delete[] copy;
    
    //delete old file and move the new one to it's place
    unlink(filename);
    rename(tmpname, filename);
    f = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    fsize = lseek(f, 0, SEEK_END) / step;
    init_buffer(); //reload buffer from the file
    truncb = old_fsize - fsize;
  }
  else truncb = 0;
}


LineStack::~LineStack()
{
  flush();
  close(f);
}

//-------------------------------------------------------------------------
// Class Window
//-------------------------------------------------------------------------
void Window::update(int wx1, int wy1, int wx2, int wy2)
{
  if (wx2 > wx1 && wy2 > wy1)
  {
     x1 = wx1; y1 = wy1; x2 = wx2; y2 = wy2;
     if (poz < (y2-y1+1)) csy = poz; else csy = (y2-y1);
     delwin(win);
     win = newwin(y2-y1+1, x2-x1+1, y1, x1);
     redraw(false);
  }
}

void Window::draw()
{
  if (act) wrefresh(win); //draw only if active
}

void Window::redraw(bool update)
{
  int i;
  TLine lin;

  //wattrset(win, A_BOLD);
  wclear(win);
  int strt = poz-(y2-y1);
  strt = strt<0 ? 0:strt;
  fff->go(strt);

  for (i=0; i<=(y2-y1); i++)
    if (strt+i < fff->size())
    {
      fff->get(lin);
      putline(win, i, lin, (strt+i>=selb && strt+i<sele));
    }
    else if (strt+i == fff->size())
                 putline(win, i, last, (strt+i>=selb && strt+i<sele));

  if (poz != fff->size())
    mvwaddch(win, y2-y1, iconfig("max_x"), ACS_DARROW | A_REVERSE | A_BLINK);

  if (update) draw();
}

void Window::draw_line(long n)
{
  TLine lin;

  if (n >= poz-(y2-y1) && n<=poz)
  {
    if (n==fff->size()) lin=last;
    else
    {
      fff->go(n);
      fff->get(lin);
    }
    putline(win, (y2-y1)-(poz-n), lin, (n>=selb && n<sele));
  }
}

void Window::outch(char ch, int typ)
{
  if (typ < 1 || typ > 4 || !enabled) return;
  int cp = typ;
  //try to convert character
  if (ch < convcnt) ch = conv[ch];
  //interpret character
  switch (ch)
  {
    case '\x7': if (sounds) beep(); break;
    case '\xD':
    case '\xA': break;
    default:  if (in_seq)
              {
                if (isalpha(ch)) {handle_ansi(ch); in_seq = false;}
                            else strncat(a_seq, &ch, 1);
              }
              else
              {
                //test begining of ESC sequence
                if (ch == 27 && colorset) { strcpy(a_seq, ""); in_seq = true; break;}
                //handle TABs
                if (ch == '\t' && lpoz < LINE_LEN)
                {
                  do lpoz++; while (lpoz < LINE_LEN && lpoz%TAB_SIZE != 0);
                  break;
                }
                //convert control characters
                if (ch < ' ') { cp=4; ch+=('A'-1); }
                short f, b; //foreground and background
                int atr;    //resultant color_pair number
                if (colorset && (cp != 4)) //color mode - alloc colors
                {
                  f = fg; b = bg;
                  if (fg > 7) atr = COLOR_PAIR(alloc_pair(f - 8, b)) | A_BOLD;
                         else atr = COLOR_PAIR(alloc_pair(f, b));
                }
                else //normal mode - use appropriate colorpair
                {
                  pair_content(cp, &f, &b);
                  if (fgattr(cp) == A_BOLD) f += 8;
                  atr = COLOR_PAIR(cp) | fgattr(cp);
                }
                last[lpoz].ch = ch;
                last[lpoz].typ = b*16 + f;
                lpoz++;
                saved_last=false;
                if (act && poz==fff->size())
                {
                  mvwaddch(win, csy, csx, ch | atr);
                }
              }
              break;
  }

  if (ch=='\xA' || ch=='\xD' || lpoz >= LINE_LEN) //end of line
  {
    fff->go(fff->size());
    fff->store(last);
    poz -= fff->trunc(); if (poz < 0) poz = 0; //update position when buffer truncated
    saved_last = true;
    last.clear_it(default_attr);
    lpoz=0;
    if (csy < (y2-y1)) {csy++; poz = fff->size();}
    else if (poz+1==fff->size()) line_f();
  }
  csx = lpoz;
}

void Window::outstr(char *s, int typ)
{
  char *p;
  for (p = s; *p; p++) outch(*p, typ);
}

void Window::handle_ansi(char endchar)
{
  int cofs = 0;
  char par[5];
  char *p = a_seq;
  p++; //skip '['
  if (endchar == 'm') //color change
     while(*p)
     {
       strcpy(par, "");
       while(*p && *p != ';' && strlen(par) < 3) {strncat(par, p, 1); p++;}
       switch(atoi(par))
       {
         case 0: cofs = 0; break;
         case 1: cofs = 8; break;
         case 8: fg = bg; break;
         //Foreground
         case 30: fg = COLOR_BLACK + cofs; break;
         case 31: fg = COLOR_RED + cofs; break;
         case 32: fg = COLOR_GREEN + cofs; break;
         case 33: fg = COLOR_YELLOW + cofs; break;
         case 34: fg = COLOR_BLUE + cofs; break;
         case 35: fg = COLOR_MAGENTA + cofs; break;
         case 36: fg = COLOR_CYAN + cofs; break;
         case 37: fg = COLOR_WHITE + cofs; break;
         //Background
         case 40: bg = COLOR_BLACK; break;
         case 41: bg = COLOR_RED; break;
         case 42: bg = COLOR_GREEN; break;
         case 43: bg = COLOR_YELLOW; break;
         case 44: bg = COLOR_BLUE; break;
         case 45: bg = COLOR_MAGENTA; break;
         case 46: bg = COLOR_CYAN; break;
         case 47: bg = COLOR_WHITE; break;
       }
       if (*p) p++;
     }
  /*else if (endchar == 'H') outch('\r', 3);
  else fprintf(stderr, "%c", endchar);*/
}

void Window::line_b()
{
  if (poz > (y2-y1))
  {
    scrollok(win, true);
    wscrl(win, -1);
    scrollok(win, false);
    poz--;
    int a = poz-(y2-y1);
    draw_line(a<0 ? 0 : a);
    draw();
  }
}

void Window::line_f()
{
  if (poz < fff->size())
  {
    scrollok(win, true);
    wscrl(win, 1);
    scrollok(win, false);
    poz++;
    draw_line(poz);
    draw();
  }
}

void Window::page_b()
{
  int skip = (y2-y1)-1;
  if (poz-skip < y2-y1) skip=poz-(y2-y1);
  poz-=skip;
  if (skip > 0) redraw();
}

void Window::page_f()
{
  int skip = (y2-y1)-1;
  if (poz+skip > fff->size()) skip=fff->size()-poz;
  poz+=skip;
  if (skip > 0) redraw();
}

void Window::go_bottom()
{
  poz = fff->size();
  redraw();
}

//------------------------------------------------------------------------
// Class QSOWindow
//------------------------------------------------------------------------
QSOWindow::QSOWindow(int wx1, int wy1, int wx2, int wy2, int num, char *fname)
{
  sprintf(class_name, "QSOWin%i", num);
  act = false;
  saved_last = true;
  enabled = true;
  wnum = num;
  scroll = true;
  sounds = true;
  convcnt = 0;
  conv = NULL;
  ctrlp = false;
  fg = COLOR_WHITE; bg = COLOR_BLACK;
  x1 = wx1; y1 = wy1; x2 = wx2; y2 = wy2;
  fff = new LineStack(fname, WIN_BUFFER_SIZE);
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  poz = fff->size();
  fff->go(poz);
  default_attr = QSO_RX;
  last.clear_it(default_attr);
  csx = 0;
  lpoz = 0;
  selb = 2; sele = 1;
  if (poz < (y2-y1+1)) csy = poz; else csy = (y2-y1);
  scrollok(win, false);
  redraw(false);
}

void QSOWindow::handle_event(const Event &ev)
{
  if (ev.type == EV_SELECT_CHN)
  {
    act = (ev.chn == wnum);
    if (act) redraw();
  }

  if (ev.type == EV_QUIT) fff->flush();

  if (act)
  {
    if (ev.type == EV_KEY_PRESS)
      if (!ctrlp)
        switch (ev.x)
        {
          case 22:  //Ctrl-V
          case KEY_NPAGE: if (scroll) page_f(); break;
          case 18:  //Ctrl-R
          case KEY_PPAGE: if (scroll) page_b(); break;
          case 24:  //Ctrl-X
          case KEY_SF:    if (scroll) line_f(); break;
          case 5:   //Ctrl-E
          case KEY_SR:    if (scroll) line_b(); break;
          case 2:   //Ctrl-B
          case KEY_SEND:  if (scroll) go_bottom(); break;
          case 16:  ctrlp = true; break; //Ctrl-P
        }
      else ctrlp = false;

    if (ev.type == EV_CONN_REQ)
    {
      char s[256];
      normalize_call((char *)ev.data);
      sprintf(s, "\n*** %s Connect request from %s\n", time_stamp(), (char *)ev.data);
      outstr(s, 3); draw();
    }
  }

  if (ev.chn == wnum)
  {
    int i;
    if (ev.type == EV_DATA_INPUT)
    {
      for (i = 0; i < ev.x; i++) outch(((char *)ev.data)[i], 1);
      draw();
    }
    if (ev.type == EV_DATA_OUTPUT)
    {
      for (i = 0; i < ev.x; i++) outch(((char *)ev.data)[i], 2);
      draw();
    }
    if (ev.type == EV_LOCAL_MSG)
    {
      for (i = 0; i < ev.x; i++) outch(((char *)ev.data)[i], 3);
      draw();
    }
    if (ev.type == EV_CONN_TO)
    {
      char s[256];
      normalize_call((char *)ev.data);
      sprintf(s, "\n*** %s Connected to %s\n", time_stamp(), (char *)ev.data);
      outstr(s, 3);
      draw();
    }
    if (ev.type == EV_DISC_FM)
    {
      char s[256];
      normalize_call((char *)ev.data);
      sprintf(s, "\n*** %s Disconnected from %s\n", time_stamp(), (char *)ev.data);
      outstr(s, 3);
      draw();
    }
    if (ev.type == EV_DISC_TIME)
    {
      char s[256];
      normalize_call((char *)ev.data);
      sprintf(s, "\n*** %s Timeout with %s\n", time_stamp(), (char *)ev.data);
      outstr(s, 3);
      draw();
    }
    if (ev.type == EV_FAILURE)
    {
      char s[256];
      normalize_call((char *)ev.data);
      sprintf(s, "\n*** %s Link failure with %s\n", time_stamp(), (char *)ev.data);
      outstr(s, 3);
      draw();
    }

    if (ev.type == EV_SET_TERM)
      colorset = (strcasecmp((char *)ev.data, "ansi") == 0);
    if (ev.type == EV_DISABLE_SCREEN)
      enabled = false;
    if (ev.type == EV_ENABLE_SCREEN)
      enabled = true;
  }

  if (ev.type == EV_STAT_LINE)
  {
    if (bconfig("swap_edit"))
      update(x1, ev.x+1, x2, y2);
    else
      update(x1, y1, x2, ev.x-1);
  }
  if (ev.type == EV_CHN_LINE)
  {
    if (bconfig("swap_edit"))
      update(x1, y1, x2, ev.x-1);
  }
  if (ev.type == EV_QSO_END && ev.chn == wnum)
  {
    if (bconfig("swap_edit"))
    {
      update(x1, ev.x, x2, y2);
      redraw();
    }
    else
    {
      update(x1, y1, x2, ev.x);
      redraw();
    }
  }
  if (ev.type == EV_SWAP_EDIT)
  {
    if (bconfig("swap_edit"))
       update(0, iconfig("edit_start_line"), iconfig("max_x"),
              iconfig("edit_end_line"));
    else
       update(0, iconfig("qso_start_line"), iconfig("max_x"),
              iconfig("qso_end_line"));
  }

  if (ev.type == EV_CONV_IN && (ev.chn == wnum || ev.chn == 0))
  {
    if (conv != NULL) delete[] conv;
    conv = new char[ev.x];
    memcpy(conv, ev.data, ev.x);
    convcnt = ev.x;
  }

  if (ev.type == EV_HIGH_LIMIT && ev.chn == wnum) fff->set_hi_mark(ev.x * 1024);
  if (ev.type == EV_LOW_LIMIT && ev.chn == wnum) fff->set_lo_mark(ev.x * 1024);
}

QSOWindow::~QSOWindow()
{
  if (!saved_last)
  {
    fff->go(fff->size());
    fff->store(last);
  }
  delwin(win);
}

//------------------------------------------------------------------------
// Class MonWindow
//------------------------------------------------------------------------
MonWindow::MonWindow(int wx1, int wy1, int wx2, int wy2, int alt_y, char *fname)
{
  strcpy(class_name, "MonWin");
  act = true;
  quiet = false;
  saved_last = true;
  enabled = true;
  scroll = false;
  sounds = false;
  colorset = false;
  alt_pos = false;
  ctrlp = false;
  fg = COLOR_WHITE; bg = COLOR_BLACK;
  x1 = wx1; y1 = wy1; x2 = wx2; y2 = wy2; ay = alt_y;
  fff = new LineStack(fname, WIN_BUFFER_SIZE);
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  poz = fff->size();
  fff->go(poz);
  default_attr = QSO_RX;
  last.clear_it(default_attr);
  csx = 0;
  lpoz = 0;
  selb = 2; sele = 1;
  if (poz < (y2-y1+1)) csy = poz; else csy = (y2-y1);
  scrollok(win, false);
  strcpy(mon_line, "");
  format = 0;
  header = false;
  bin = false;
  dontshow = false;
  //outstr("Linpac version 0.01 ready.\r", 3);
  redraw(false);

  if (bconfig("disable_spyd") || !try_spyd()) try_listen(); //try one monitoring method
}

void MonWindow::handle_event(const Event &ev)
{
  if (ev.type == EV_QUIT)
  {
    if (listpid > 0) kill(listpid, SIGTERM);
    fff->flush();
  }

  if (ev.type == EV_MONITOR_CTL)
  {
    if (ev.x == 0) { quiet = true; act = false; }
    if (ev.x == 1) { quiet = false; act = true; }
  }

  if (ev.type == EV_SELECT_CHN && ev.chn >= 0)
  {
    if (!quiet || ev.chn == 0) act = true; //do not enable monitor if it's in quiet mode
    if (quiet && ev.chn != 0) act = false;
  }

  if (act)
  {
    if (ev.type == EV_SELECT_CHN)
      if (ev.chn >= 0)
      {
        if ((ev.chn == 0 && !alt_pos) || (ev.chn != 0 && alt_pos))
        {
          alt_pos = !alt_pos;
          int p = y1; y1 = ay; ay = p;
          update(x1, y1, x2, y2);
          if (!alt_pos) go_bottom();
          redraw();
        }
        else redraw();
      }
      else act = false;

    if (ev.type == EV_KEY_PRESS)
      if (!ctrlp)
        switch (ev.x)
        {
          case 22:  //Ctrl-V
          case KEY_NPAGE: if (scroll || alt_pos) page_f(); break;
          case 18:  //Ctrl-R
          case KEY_PPAGE: if (scroll || alt_pos) page_b(); break;
          case 24:  //Ctrl-X
          case KEY_SF:    if (scroll || alt_pos) line_f(); break;
          case 5:   //Ctrl-E
          case KEY_SR:    if (scroll || alt_pos) line_b(); break;
          case 2:   //Ctrl-B
          case KEY_SEND:  if (scroll) go_bottom(); break;
          case 16: ctrlp = true; break; //Ctrl-P
        }
      else ctrlp = false;
  }

  if (ev.type == EV_NONE)
  {
    if (listpid >= 0)
    {
      char buf[512];
      int rc = read(list, buf, 512);
      if (rc > 0)
      {
        int i;
        for (i=0; i < rc; i++)
        {
          if (buf[i] == '\r' || buf[i] == '\n')
          {
            next_mon_line();
            strcpy(mon_line, "");
          }
          else strncat(mon_line, &buf[i], 1);
        }
        draw();
      }
    }
  }

  if (ev.type == EV_CHN_LINE) update(x1, ev.x+1, x2, y2);
  if (ev.type == EV_LOW_LIMIT && ev.chn == 0) fff->set_hi_mark(ev.x * 1024);
  if (ev.type == EV_HIGH_LIMIT && ev.chn == 0) fff->set_lo_mark(ev.x * 1024);

  if (ev.type == EV_CONV_IN && ev.chn == 0)
  {
    if (conv != NULL) delete[] conv;
    conv = new char[ev.x];
    memcpy(conv, ev.data, ev.x);
    convcnt = ev.x;
  }

  //monitor requests
  if (ev.type == EV_SYSREQ)
  {
    if (ev.x == 20) Message(0, "Monitor fmt: %i", format);
    else if (ev.x == 21) format = 1;
    else if (ev.x == 22) format = 2;
    else if (ev.x == 23) format = 3;
  }
}

void MonWindow::next_mon_line()
{
  if (format == 0) format = detect_format(mon_line);

  //try to find dest/src separator
  char *p;
  if (format == 1 || format == 3)
    p = strstr(mon_line, "->");
  else
    p = strstr(mon_line, " to ");
    
  char src[16];

  if (p != NULL &&
      ((format == 1 && strstr(mon_line, "Port ") == mon_line) ||
       (format == 2 && strstr(mon_line, ": fm ") != NULL) ||
       (format == 3 && strstr(mon_line, ": AX25: ") != NULL)))
  {
     if (!filter(mon_line))
     {
        //Find source call
        while (isalnum(*(p - 1)) && (p - 1) != mon_line) p--;
        strcpy(src, "");
        while (*p && isalnum(*p)) {strncat(src, p, 1); p++;}
        //Is it our call ?
        bool is = false;
        for (int i=1; i <= MAX_CHN; i++)
        {
          char sc[15];
          strcopy(sc, sconfig(i, "call"), 15);
          strip_ssid(sc);
          if (strcmp(sc, src) == 0) {is = true; break;}
        }
        if (is)
        {
          if (format == 2 || format ==3) outch('\r', 2);
          outstr(mon_line, 2);
        }
        else
        {
          if (format == 2 || format ==3) outch('\r', 1);
          outstr(mon_line, 1);
        }
        dontshow = false;
     }
     else dontshow = true;
     header = true;
  }
  else
  {
     if (!dontshow)
     {
        if (header)
          if (!bconfig("mon_bin")) bin = binary(mon_line);
          else bin = false;
    
        if (!bin)
        {
          outch('\r', 3);
          outstr(mon_line, 3);
        }
        else if (header)
        {
          outch('\r', 3);
          outstr("<Binary data>", 3);
        }
     }
   
     header = false;
  }
}

int MonWindow::detect_format(char *line)
{
  if (strstr(line, "Port ") == line && strstr(line, "->") != NULL) return 1;
  if (strstr(line, ": fm ") != NULL && strstr(line, " to ") != NULL) return 2;
  return 3;
}

bool MonWindow::try_spyd()
{
#ifdef WITH_AX25SPYD
  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(AX25SPYD_PORT);
  server.sin_addr.s_addr = 1 * 256*256*256 + 127; //127.0.0.1

  int s = socket(PF_INET, SOCK_STREAM, 0);
  if (s == -1)
  {
    Error(errno, "MonWindow: cannot create inet socket");
    return false;
  }
  int r = connect(s, (struct sockaddr *)&server, sizeof(server));
  if (r == -1)
  {
    if (errno != ECONNREFUSED)
      Error(errno, "MonWindow: cannot connect ax25spyd");
    close(s);
    return false;
  }
  r = fcntl(s, F_SETFL, O_NONBLOCK);
  if (r == -1) Error(errno, "MonWindow: cannot set pipe flags");
  list = s;
  listpid = 0;
  return true;
#else
  return false;
#endif
}

bool MonWindow::try_listen()
{
  int pip[2];
  listpid = -1;
  if (pipe(pip) == -1)
  {
    outstr("Monitor not ready - cannot pipe()\r", 3);
    draw();
    return false;
  }
  int newpid = fork();
  if (newpid == -1)
  {
    outstr("Monitor not ready - cannot fork()\r", 3);
    draw();
    return false;
  }
  if (newpid > 0)
  {
    close(pip[1]);
    int rc = fcntl(pip[0], F_SETFL, O_NONBLOCK);
    if (rc == -1) Error(errno, "MonWindow: cannot set pipe flags");
    listpid = newpid;
    list = pip[0];
    return true;
  }
  if (newpid == 0)
  {
    close(0); close(1);
    dup2(pip[1], 1); dup2(pip[1], 2);
    for (int i = 3; i < 16; i++) close(i);
    char s[15]; strcpy(s, "-"); strcat(s, sconfig("monparms"));
    execlp(LISTEN, LISTEN, s, NULL);
    fprintf(stderr, "Error: Cannot exec LISTEN\n");
    exit(1);
  }
  return true;
}

/*bool MonWindow::binary(char *data)
{
   unsigned char *p = (unsigned char *)data;
   int i = 0;
   int a = 0, b = 0;

   while (*p && i < 20)
   {
      if (isalnum(*p)) a++; else b++;
      p++; i++;
   }

   return b > a;
}*/

bool MonWindow::binary(char *data)
{
  int len;
  int i;
  int a;
  int b;

  a = 0;
  b = 0;
  len = strlen(data);

  for (i = 0; i < len; i++) {
    if ((data[i] > 128))
      b++;
    if ((data[i] == 'e') || (data[i] == 'E') || (data[i] == 'n') || (data[i] == ' '))
      a++;
  }
	
  return b > a;
}	

bool MonWindow::filter(char *line)
{
   char pname[32];
   if (format == 1)
      sscanf(line+5, "%31[a-zA-Z0-9]s", pname); //skip "Port "
   else
      sscanf(line, "%31[a-zA-Z0-9]s", pname);

   char *conf = sconfig("monport");
   if (conf == NULL || strcmp(conf, "*") == 0 || strcmp(conf, pname) == 0)
      return false;

   return true;
}

MonWindow::~MonWindow()
{
  if (!saved_last)
  {
    fff->go(fff->size());
    fff->store(last);
  }
  delwin(win);
  close(list);
  if (listpid > 0) kill(listpid, SIGTERM);
}

//-------------------------------------------------------------------------
// Class ChannelInfo
//-------------------------------------------------------------------------
ChannelInfo::ChannelInfo(int wx1, int wx2, int wy, int alt_y)
{
  act = true;
  strcpy(class_name, "ChnInfo");
  achn = 1;
  alt_pos = false;
  x1 = wx1; y1 = wy; x2 = wx2; y2 = wy; ay = alt_y;
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  int i;
  for (i=0; i<=MAX_CHN; i++)
  {
    blink[i] = false;
    priv[i] = false;
  }
  visible = (x2 - x1) / 9; //9 chars for channel
  if (visible > MAX_CHN) visible = MAX_CHN;
  redraw(false);
}

void ChannelInfo::redraw(bool update)
{
  //wattrset(win, A_BOLD);
  wbkgd(win, ' ' | COLOR_PAIR(CI_NORM) | fgattr(CI_NORM));
  wbkgdset(win, ' ');
  wmove(win, 0, 0);
  int i;
  for (i=1; i <= visible; i++)
  {
    wprinta(win, COLOR_PAIR(CI_NORM) | fgattr(CI_NORM), "%2i:", i);
    int atr;
    if (achn == i) atr = COLOR_PAIR(CI_SLCT) | fgattr(CI_SLCT);
              else atr = COLOR_PAIR(CI_NCHN) | fgattr(CI_NCHN);
    if (blink[i]) atr |= A_BLINK;
    if (priv[i]) atr = COLOR_PAIR(CI_PRVT) | fgattr(CI_PRVT);
    char cw[12]; strcopy(cw, sconfig(i, "cwit"), 12); strip_ssid(cw);
    if (iconfig(i, "state") == ST_DISC)
    {
      if (ch_disabled[i])
        wprinta(win, atr, "++++++");
      else
        wprinta(win, atr, "------");
    }
    else wprinta(win, atr, "%s", cw);
  }
  wbkgdset(win, ' ' | COLOR_PAIR(CI_NORM) | fgattr(CI_NORM));
  waddch(win, '\n');
  if (visible < MAX_CHN)
     if (achn <= visible)
       mvwaddch(win, 0, x2-x1-1, '>' | COLOR_PAIR(CI_NORM) | fgattr(CI_NORM));
     else
       mvwaddch(win, 0, x2-x1-1, '>' | COLOR_PAIR(CI_NORM) | fgattr(CI_NORM) | A_BLINK);
  if (update && act) wrefresh(win);
}

void ChannelInfo::draw()
{
  if (act) wrefresh(win);
}

void ChannelInfo::handle_event(const Event &ev)
{
  switch(ev.type)
  {
    case EV_RECONN_TO: redraw(); break;
    case EV_STATUS_CHANGE: redraw(); break;
    case EV_SELECT_CHN:
      if (ev.chn == 0)
        if (!alt_pos)
        {
          mvwin(win, ay, x1);
          alt_pos = true;
          redraw();
        }
      if (ev.chn > 0)
        if (alt_pos)
        {
          mvwin(win, y1, x1);
          alt_pos = false;
          redraw();
        }
      if (ev.chn >= 0) {act = true; achn = ev.chn; blink[ev.chn] = false; redraw();}
      else act = false;
      break;
    case EV_DATA_INPUT:
      if (ev.chn != achn) {blink[ev.chn] = true; redraw();}
      break;
    case EV_CHN_LINE:
      y1 = y2 = ev.x;
      delwin(win);
      win = newwin(y2-y1+1, x2-x1+1, y1, x1);
      redraw(false);
      break;
    case EV_ENABLE_CHN:
    case EV_DISABLE_CHN: redraw(); break;
  }
}

//-------------------------------------------------------------------------
// Class InfoLine
//-------------------------------------------------------------------------
InfoLine::InfoLine(int wx1, int wx2, int wy, int alt_y)
{
  act = true;
  strcpy(class_name, "Infoline");
  achn = 1;
  alt_pos = false;
  x1 = wx1; y1 = wy; x2 = wx2; y2 = wy; ay = alt_y;
  win = newwin(y2-y1+1, x2-x1+1, y1, x1);
  procform = check_proc_format();
  enable_ioctl = false;
  #ifndef NEW_AX25
  force_ioctl = false;
  #else
  force_ioctl = true;
  #endif
  redraw(false);
}

void InfoLine::redraw(bool update)
{
  //Channel callsign
  //wattrset(win, A_BOLD);
  wbkgd(win, ' ' | COLOR_PAIR(IL_TEXT) | fgattr(IL_TEXT));
  wmove(win, 0, 0);
  if (achn != 0)
  {
    wprintw(win, "My:%10s %c", sconfig(achn, "call"), ACS_VLINE);

    int strategy = procform;
    if ((iconfig("info_level") == 1 && enable_ioctl) || force_ioctl) strategy = 2; //use ioctl
    
    if (iconfig("info_level") > 0)
    {
      //Channel status
      if (iconfig(achn, "state") != ST_DISC)
        if (get_axstat(sconfig(achn, "call"), sconfig(achn, "cphy"), strategy))
          if (iconfig("info_level") == 1)
            wprintw(win, "Ack:%2d/%2d%cUnack:%2d%cTries:%2d/%2d%cRtt:%2d%c%16.16s",
                   chstat.t1, chstat.t1max,  ACS_VLINE,
                   chstat.vs - chstat.va,    ACS_VLINE,
                   chstat.n2, chstat.n2max,  ACS_VLINE,
                   chstat.rtt,               ACS_VLINE,
                   state_str(chstat.state));
          else
            wprintw(win, " %s %d %d %d %d %d/%d %d/%d %3d/%3d %d/%d %d/%d %d %d %d %s %d %d   ",
                   chstat.devname,
                   chstat.state,
                   chstat.vs, chstat.vr, chstat.va,
                   chstat.t1, chstat.t1max,
                   chstat.t2, chstat.t2max,
                   chstat.t3, chstat.t3max,
                   chstat.idle, chstat.idlemax,
                   chstat.n2, chstat.n2max,
                   chstat.rtt,
                   chstat.window,
                   chstat.paclen,
                   "",
                   chstat.sendq, chstat.recvq);
        else
          wprintw(win, "%54s", "Disconnected");
      else
        wprintw(win, "%54s", "Disconnected");
    } else wprintw(win, "%58s", " ");
  } else wprintw(win, "%70s", " ");
  //Actual time
  wmove(win, 0, 69);
  struct tm *cas;
  time_t secs = time(NULL);
  cas = localtime(&secs);
  wprintw(win, "%c %2i:%02i:%02i", ACS_VLINE, cas->tm_hour, cas->tm_min, cas->tm_sec);

  if (update && act) wrefresh(win);
}

void InfoLine::draw()
{
  if (act) wrefresh(win);
}

void InfoLine::handle_event(const Event &ev)
{
  switch(ev.type)
  {
    case EV_SELECT_CHN:
      if (ev.chn >= 0) {act = true; achn = ev.chn;}
      else act = false;
      if (ev.chn == 0)
        if (!alt_pos)
        {
          mvwin(win, ay, x1);
          alt_pos = true;
        }
      if (ev.chn > 0)
        if (alt_pos)
        {
          mvwin(win, y1, x1);
          alt_pos = false;
        }
      if (act) redraw();
      break;
    case EV_STAT_LINE:
      y1 = y2 = ev.x;
      delwin(win);
      win = newwin(y2-y1+1, x2-x1+1, y1, x1);
      redraw(false);
      break;
    case EV_SYSREQ:
      if (ev.x == 3) enable_ioctl = !enable_ioctl;
      if (ev.x == 4) force_ioctl = !force_ioctl;
      if (ev.x == 5) procform = !procform;
      break;
  }
  time_t act_time = time(NULL);
  if (act_time != last_time) redraw();
  last_time = act_time;
}

int InfoLine::check_proc_format()
{
  //return: 0 = 2.2.x format
  //        1 = 2.0.x format
  int res = 0;
  char s[256];
  FILE *f;

  f = fopen(PROCFILE, "r");
  if (f != NULL)
  {
    fgets(s, 255, f);
    fgets(s, 255, f);
    if (strchr(s, '/') == NULL) res = 1; //older format contains '/'
    fclose(f);
  }
  return res;
}

//-------------------------------------------------------------------------
// Class StatLines
//-------------------------------------------------------------------------
info_line::info_line(const info_line &src)
{
  id = src.id;
  strcopy(content, src.content, 256);
}

info_line &info_line::operator = (const info_line &src)
{
  id = src.id;
  strcpy(content, src.content);
  return *this;
}

StatLines::StatLines(int chn, int wx1, int wx2, int wy)
{
  sprintf(class_name, "StatLn%i", chn);
  act = (chn == 1);
  mychn = chn;
  x1 = wx1; y1 = wy; x2 = wx2; y2 = wy;
  win = NULL;
  nlines = 0;
  redraw(false);
}

void StatLines::update(int wx1, int wy1, int wx2, int wy2)
{
  x1 = wx1; y1 = wy1; x2 = wx2; y2 = wy2;
  if (win != NULL) delwin(win);
  if (y2 > y1) win = newwin(y2-y1+1, x2-x1+1, y1, x1);
          else win = NULL;
}

void StatLines::handle_event(const Event &ev)
{
  std::vector <info_line>::iterator it;

  switch(ev.type)
  {
    case EV_SELECT_CHN:
      act = (ev.chn == mychn);
      redraw();
      break;
    case EV_CHANGE_STLINE:
      if (ev.chn == mychn)
      {
        for (it = lines.begin(); it < lines.end(); it++)
          if (it->id == ev.x) break;
        if (it == lines.end())
        {
          info_line newline;
          newline.id = ev.x;
          strcopy(newline.content, (char *)ev.data, 256);
          if (newline.content[strlen(newline.content)-1] == '\n')
               newline.content[strlen(newline.content)-1] = '\0';
          lines.push_back(newline);
          nlines++;
          if (bconfig("swap_edit"))
          {
            update(x1, y1, x2, y2+1);
            emit(mychn, EV_QSO_END, y2 + 1, NULL);
          }
          else
          {
            update(x1, y1 - 1, x2, y2);
            emit(mychn, EV_QSO_END, y1 - 1, NULL);
          }
        }
        else strcopy(it->content, (char *)ev.data, 256);
        redraw();
      }
      break;
    case EV_PROCESS_FINISHED: //remove statline when process is finished
    case EV_REMOVE_STLINE:
      if (ev.chn == mychn)
      {
        for (it = lines.begin(); it < lines.end(); it++)
          if (it->id == ev.x) break;
        if (it != lines.end())
        {
          info_line_erase(lines, it);
          nlines--;
          if (bconfig("swap_edit"))
          {
            update(x1, y1, x2, y2 - 1);
            emit(mychn, EV_QSO_END, y2 + 1, NULL);
          }
          else
          {
            update(x1, y1 + 1, x2, y2);
            emit(mychn, EV_QSO_END, y1 - 1, NULL);
          }
          redraw();
        }
      }
      break;
    case EV_STAT_LINE:
      y1 += (ev.x - y2);
      y2 = ev.x;
      update(x1, y1, x2, y2);
      redraw(false);
      break;
  }
  /*count++;
  if (count > 3) {redraw(); count = 0;}*/
}

void StatLines::draw()
{
  if (act && win != NULL) wrefresh(win);
}

void StatLines::redraw(bool update)
{
  if (act && win != NULL)
  {
    wattrset(win, A_BOLD);
    wbkgd(win, ' ' | COLOR_PAIR(IL_TEXT) | fgattr(IL_TEXT));
    if (bconfig("swap_edit"))
      wmove(win, 1, 0);
    else
      wmove(win, 0, 0);
    for (int i = 0; i < nlines; i++)
    {
      waddnstr(win, lines[i].content, x2 - x1 - 1);
      waddch(win, '\n');
    }
  }
  if (update) draw();
}

