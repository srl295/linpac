/*
  LinPac Mailer
  (c) 1998 - 2001 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_edit.cc - message composer classes
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include <axmail.h>
#include <ncurses.h>

#include "lpapp.h"
#include "mail_edit.h"
#include "mail_call.h"
#include "mail_data.h"
#include "mail_route.h"
#include "mail_files.h"

#define LINE_LEN 200
#define ADDR_LEN 64
#define ADDR_FMT "%-64.64s"
#define SUBJ_LEN 78
#define SUBJ_FMT "%-78.78s"
#define ENC_LEN 32
#define ENC_FMT "%-32.32s"
#define HEADER_FMT(hdr,fmt) ("%-7.7s : " fmt) , hdr

#define TAB_SIZE 8

#define SYSLINES 3
#define SCREEN_LINES (x2-x1-2)

//uncoment this to make BS to skip to previous line when pressed at the
//begining of line
//#define BS_SKIP_LINES

//uncoment this to make BS to join the line with the previous one when
//pressed at the begining of line
#define BS_JOIN_LINES

//uncomment this to add a path to yourself into each message
//#define PATH_TO_OWN

char old_help[80];

//convert string to uppercase
char *strupr(char *s)
{
   char *p = s;
   while (*p) { *p = toupper(*p); p++; }
   return s;
}

//return true if the string contains at least one digit
bool contains_digit(char *s)
{
   char *p = s;
   while (*p)
   {
     if (isdigit(*p)) return true;
     p++;
   }
   return false;
}

//--------------------------------------------------------------------------
// Class Edline
//--------------------------------------------------------------------------
Edline::Edline()
{
   attr = ED_TEXT;
   editable = true;
   s = new char[LINE_LEN+1];
   clrline();
}

Edline::~Edline()
{
   delete[] s;
}

Edline &Edline::operator = (const Edline &source)
{
   strcpy(s, source.s);
   attr = source.attr;
   editable = source.editable;
   return *this;
}

int Edline::lastpos()
{
   int i;
   for(i = strlen(s)-1; i>-1 && isspace(s[i]); i--) ;
   return i;
}

void Edline::clrline()
{
   for (int i=0; i<LINE_LEN; i++) s[i]=' ';
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
         mvwaddch(win, y, i, (s[i]+64) | COLOR_PAIR(ED_CTRL));
       else if (!editable)
         mvwaddch(win, y, i, s[i] | COLOR_PAIR(ED_STAT));
       else
         mvwaddch(win, y, i, s[i] | COLOR_PAIR(attr));
     }
     else
         mvwaddch(win, y, i, ' ' | COLOR_PAIR(attr));
}

//--------------------------------------------------------------------------
// Class Editor
//--------------------------------------------------------------------------
Editor::Editor(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, int maxlines)
{
   sprintf(class_name, "Editor");
   x1 = wx1; y1 = wy1; x2=wx2; y2=wy2;
   limit = maxlines;
   max = 1;
   line = new Edline[limit];
   crx = 0;
   cry = 0;
   poz = 0;
   err = false;
   win = subwin(parent, y2-y1+1, x2-x1+1, y1, x1);
   scrollok(win, false);
   redraw(true);
}

void Editor::update(int wx1, int wy1, int wx2, int wy2)
{
   x1 = wx1; y1 = wy1; x2=wx2; y2=wy2;
   int old_cry = cry;
   if (cry > (y2-y1)) {cry = y2 - y1; poz += old_cry - cry;}
   delwin(win);
   win = newwin(y2-y1+1, x2-x1+1, y1, x1);
   redraw(true);
}

void Editor::handle_event(Event *ev)
{
   char *s;
 
   if (ev == NULL) return;
 
   if (ev->type==EV_KEY_PRESS)
   {
       if (!ev->y)
          switch (ev->x)
          {
             case KEY_LEFT:  cr_left(); break;
             case KEY_RIGHT: cr_right(); break;
             case KEY_UP:    cr_up(); break;
             case KEY_DOWN:  cr_down(); break;
             case KEY_PPAGE: cr_pgup(); break;
             case KEY_NPAGE: cr_pgdn(); break;
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
             case '\n':      newline(); break;
             case 20  :      s = time_stamp(0); /* ^T */
                             for (unsigned i=0; i<strlen(s); i++) newch(s[i]);
                             break;
             case 4   :      s = date_stamp(0); /* ^D */
                             for (unsigned i=0; i<strlen(s); i++) newch(s[i]);
                             break;
             case 18  :
             case 22  :
             case 5   :
             case 24  :      break; //keys used by QSO-Window
             default:
                   if (ev->x <= 255) newch(ev->x);
          }
       wmove(win, cry, crx);
       wrefresh(win);
   }

   if (ev->type==EV_KEY_PRESS_MULTI)
   {
        if (!ev->y)
        {
            // Treat this the same as if we received multiple EV_KEY_PRESS events
            // where each event is guaranteed to be only a printable character.
            char *buffer = (char *)ev->data;
            for (unsigned ix = 0; ix < strlen(buffer); ix++)
                newch(buffer[ix]);
        }
       wmove(win, cry, crx);
       wrefresh(win);
   }
}

void Editor::draw(bool all)
{
   //if (all || err) redraw(false);
   if (err) redraw(true);
   else if (all) redraw(false);
   wrefresh(win);
   err = false;
}

void Editor::redraw(bool update)
{
   if (update)
   {
      wclear(win);
      wbkgd(win, ' ' | COLOR_PAIR(ED_TEXT));
      wbkgdset(win, ' ');
   }
   draw_lines();
   //if (update) wrefresh(win);
}

void Editor::draw_lines()
{
   int i;
   for (i=0; i<=(y2-y1); i++)
      if (poz+i < max) line[i+poz].draw(win, i);
   wmove(win, cry, crx);
}

void Editor::draw_line(int n)
{
   if ((n >= poz) && (n <= poz+(y2-y1)))
      line[n].draw(win, n-poz);
   wmove(win, cry, crx);
}

void Editor::cr_right()
{
   crx++;
   if (crx >= SCREEN_LINES) {crx = 0; cr_down();}
   wmove(win, cry, crx);
}

void Editor::cr_left()
{
   crx--;
   if (crx < 0) {cr_up(); cr_end();}
   wmove(win, cry, crx);
}

void Editor::cr_up(bool update)
{
   if (poz > 0 || cry > 0)
   {
      if (cry==0) {poz--; if (update) draw(true);}
             else cry--;
   }
   if (update) wmove(win, cry, crx);
}

void Editor::cr_down(bool update)
{
   if (poz+cry < max-1)
   {
      if (cry==y2-y1) {poz++; if (update) draw(true);}
                 else cry++;
   }
   if (update) wmove(win, cry, crx);
}

void Editor::cr_pgup()
{
   for (int i = 0; i < (y2-y1); i++)
      if (poz > 0) poz--;
      else if (cry > 0) cry--;
   draw(true);
   wmove(win, cry, crx);
}

void Editor::cr_pgdn()
{
   for (int i = 0; i < (y2-y1); i++)
     if (poz + (y2-y1) < max-1) poz++;
     else if (poz+cry < max-1) cry++;
   draw(true);
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
   draw(true);
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
   if (line[poz+cry].editable)
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
      #ifdef BS_JOIN_LINES
      else
      {
         char rem[LINE_LEN+1];
         line[poz+cry].get_text(rem);
         delline();
         cr_up();
         cr_end();
         int zcrx = crx; int zcry = cry;
         char *p = rem;
         while (*p) { newch(*p); p++; }
         crx = zcrx; cry = zcry;
         wmove(win, cry, crx);
      }
      #endif
   } else cr_left();
}

void Editor::del_ch()
{
   int i;
 
   if (line[poz+cry].editable)
   {
      for (i = crx; i < LINE_LEN-1; i++) line[poz+cry].s[i] = line[poz+cry].s[i+1];
      line[poz+cry].s[i]=' ';
      draw_line(poz+cry);
      wmove(win, cry, crx);
   }
}

void Editor::newline(bool update)
{
   if (line[poz+cry].editable || crx > line[poz+cry].lastpos())
   {
      if (poz+cry < max-1 || max < limit)
      {
         char next_line[LINE_LEN+1];
         
         char *p = &(line[poz+cry].s[crx]);
         strcpy(next_line, p);
         for (int i = crx; i < LINE_LEN; i++) line[poz+cry].s[i] = ' ';
         draw_line(poz+cry);
     
         if (max < limit) max++;
         for (int i = max-1; i > cry+poz; i--)
           line[i+1] = line[i];
         cr_down(update);
     
         line[poz+cry].accept(next_line);
         if (update) { draw_lines(); wrefresh(win); }
      }
   }
   cr_home();
   wmove(win, cry, crx);
}

void Editor::ins_info(char *ch)
{
   line[cry+poz].accept(ch);
   line[cry+poz].attr = ED_INFO;
   line[cry+poz].editable = true;
   crx = strlen(ch);
   newline(false);
}

void Editor::ins_nonedit(char *ch)
{
   line[cry+poz].accept(ch);
   line[cry+poz].attr = ED_TEXT;
   line[cry+poz].editable = false;
   crx = strlen(ch);
   newline(false);
}

void Editor::newch(char ch)
{
   if (line[poz+cry].editable)
   {
      int i;
    
      line[poz+cry].attr = ED_TEXT;
      for (i=LINE_LEN-1; i>crx; i--) line[poz+cry].s[i] = line[poz+cry].s[i-1];
      line[poz+cry].s[crx] = ch;
      draw_line(poz+cry);
      crx++;
      if (line[poz+cry].lastpos() >= SCREEN_LINES) //end of line exceeded
      {
        int zx, zy=-1;
        if (crx < SCREEN_LINES) {zx = crx; zy = cry;}
        int p = line[poz+cry].lastpos() - 1;
        while (p>0 && !isspace(line[poz+cry].s[p])) p--;
        crx = p+1;
        newline();
        cr_end();
        if (zy != -1) {crx = zx; cry = zy;}
      }
      wmove(win, cry, crx);
    }
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

void Editor::errormsg(char const *msg)
{
   wbkgdset(win, ' ' | COLOR_PAIR(C_ERROR));
   mvwprintw(win, y2-y1, 0, " %s ", msg);
   wbkgdset(win, ' ' | COLOR_PAIR(ED_TEXT));
   err = true;
   wmove(win, cry, crx);
   wrefresh(win);
}

void Editor::clear_error()
{
   if (err) draw(true);
}

Editor::~Editor()
{
}

//========================================================================

Composer::Composer(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, char *toaddr, char *subject)
{
   const int maxlines = 500;      //number of editor lines
 
   x1 = wx1; y1 = wy1; x2 = wx2; y2 = wy2;
   bool reply = (toaddr != NULL);
   type = 'P';
   from = NULL;
   to = new char[ADDR_LEN]; to[ADDR_LEN-1] = '\0';
   if (toaddr != NULL)
   {
      strncpy(to, toaddr, ADDR_LEN-1);
      check_dest();
   }
   else strcpy(to, "");

   subj = new char[SUBJ_LEN];
   if (subject != NULL) strncpy(subj, subject, SUBJ_LEN-1);
                   else strcpy(subj, "");
 
   //win = subwin(parent, y2-y1+1, x2-x1+1, y1, x1);
   win = parent;
   if (win == NULL) beep();
   keypad(win, true);
   meta(win, true);
   nodelay(win, true);
 
   wbkgdset(win, ' ' | COLOR_PAIR(ED_TEXT) | A_BOLD);
   werase(win);
   box(win, ACS_VLINE, ACS_HLINE);
   mvwprintw(win, 0, 2, "Composer");
 
   ed = new Editor(win, x1+1, y1+SYSLINES+3, x2-1, y2-1, maxlines);
   load_texts();
 
   old_focus_window = focus_window;
   focus_window = win;
   old_focused = focused;
   focused = this;
 
   crx = 0;
   if (!reply) cry = 0;
          else {cry = SYSLINES + head_lines + 1; ed->cry = head_lines;}
   ttabnum = -1;
   ttabsize = 0;
 
   help(HELP_EDIT);
 
   draw(true);
   if (cry == 0)
   {
      wmove(win, 1, 12);
      wrefresh(win);
   }
   else
   {
      wrefresh(win);
      ed->cr_home(); //to draw the cursor to the right place
      ed->draw();
   }
 
   wait_rname = false;
   wait_sname = false;
   wait_fname = false;
}

Composer::~Composer()
{
   if (from) delete[] from;
   delete[] to;
   delete[] subj;
}

void Composer::handle_event(Event *ev)
{
  if (ev == NULL)
  {
     if (wait_rname)
     {
        wait_rname = false;
        int ch;
        char *name = load_file_path(ibuffer);
        FILE *f = fopen(name, "r");
        if (f != NULL)
        {
           while ((ch = fgetc(f)) != EOF)
              if (ch == '\n') ed->newline(false);
              else ed->newch((char) ch);
           fclose(f);
           ed->draw(true);
        }
        else ed->errormsg("File not found");
        delete[] name;
     }
     if (wait_sname)
     {
        wait_sname = false;
        char *name = save_file_path(ibuffer);
        FILE *f = fopen(name, "w");
        if (f != NULL)
        {
           for (int i = 0; i < ed->max; i++)
           {
              char s[LINE_LEN+1];
              (ed->line[i]).get_text(s);
              fputs(s, f);
              fputc('\n', f);
           }
           fclose(f);
           ed->draw(true);
           delete[] name;
        }
        else ed->errormsg("Cannot create file");
     }
     if (wait_fname)
     {
        wait_fname = false;
        char *name = load_file_path(ibuffer);
        FILE *f = fopen(name, "r");
        if (f != NULL)
        {
           char msg[256];
           sprintf(msg, "Attached file: ");
           strncat(msg, name, 255);
           ed->ins_nonedit(msg);
           sprintf(msg, "This file will be converted with 7plus and sent in separate message(s)");
           ed->ins_nonedit(msg);
           sprintf(msg, "To remove the attachment just delete following line (Ctrl-Y).");
           ed->ins_nonedit(msg);
           sprintf(msg, ".attach ");
           strncat(msg, name, 255);
           ed->ins_nonedit(msg);
           
           fclose(f);
           ed->draw(true);
        }
        else ed->errormsg("Cannot open file");
        delete[] name;
     }
     return;
  }

  if (ev->type == EV_KEY_PRESS)
  {
    ed->clear_error();
    if (!ev->y) switch (ev->x)
    {
      case '\n':
      case KEY_DOWN: if (cry > SYSLINES)
                     {
                       ed->handle_event(ev);
                       cry = ed->cry+SYSLINES+1;
                     }
                     else
                     {
                       if (cry == 0) check_dest();
                       cry++;
                       if (cry > SYSLINES) ed->draw();
                     }
                     break;
      case KEY_UP  : if (cry > SYSLINES)
                     {
                       if (ed->poz != 0 || ed->cry > 0)
                         ed->handle_event(ev);
                       else
                         cry = SYSLINES;
                     } else if (cry > 0) cry--;
                     break;
      case '\x18'  : if (send_message()) /* ^X */
                     {
                       focus_window = old_focus_window;
                       focused = old_focused;
                       if (focused != NULL) focused->draw(true);
                       help(old_help);
                       return;
                     }
                     else
                     {
                       beep();
                       break;
                     }
      case '\x01'  : focus_window = old_focus_window; /* ^A */
                     focused = old_focused;
                     if (focused != NULL) focused->draw(true);
                     help(old_help);
                     return;
      case '\x12'  : if (cry > SYSLINES) /* ^R */
                     {
                        wait_rname = true;
                        iline = new InputLine(win, 2, y2-y1, x2-x1-1, x2-x1-19,
                                        "Insert file:", ibuffer, INPUT_ALL);
                     }
                     break;
      case '\x05'  : if (cry > SYSLINES) /* ^E */
                     {
                        wait_sname = true;
                        iline = new InputLine(win, 2, y2-y1, x2-x1-1, x2-x1-16,
                                        "Export to file:", ibuffer, INPUT_ALL);
                     }
                     break;
      case '\x06'  : if (cry > SYSLINES) /* ^F */
                     {
                        wait_fname = true;
                        iline = new InputLine(win, 2, y2-y1, x2-x1-1, x2-x1-16,
                                        "Attach file:", ibuffer, INPUT_ALL);
                     }
                     break;
      default : if (cry > SYSLINES)
                  ed ->handle_event(ev);
                else
                  if (ev->x == KEY_BACKSPACE || ev->x == '\b')
                  {
                    if (cry == 0) to[strlen(to)-1] = '\0';
                    if (cry == 2) subj[strlen(subj)-1] = '\0';
                  }
                  else if (ev->x == KEY_DC || ev->x == '\a')
                  {
                    if (cry == 0) to[strlen(to)-1] = '\0';
                    if (cry == 2) subj[strlen(subj)-1] = '\0';
                  }
                  else if (ev->x == ' ' && cry == 3)
                  {
                    if (ttabnum < ntables-1) ttabnum++;
                                        else ttabnum = -1;
                  }
                  else if (!iscntrl(ev->x))
                  {
                    char c = (char) ev->x;
                    if (cry == 0)
                      if (strlen(to)<ADDR_LEN-1) strncat(to, &c, 1);
                    if (cry == 1)
                      if (toupper(ev->x) == 'P' ||
                          toupper(ev->x) == 'B') type = toupper(ev->x);
                    if (cry == 2)
                      if (strlen(subj)<SUBJ_LEN-1) strncat(subj, &c, 1);
                  }
                break;
                    
    }
  }

  if (ev->type == EV_KEY_PRESS_MULTI)
  {
    // Only triggered by pasting, so here we need only support the To and Subject
    // fields, plus the editor.
    ed->clear_error();
    if (cry > SYSLINES) // editor
        ed->handle_event(ev);
    else if (cry == 0 || cry == 2) // To or Subject
    {
      // Treat this the same as if we received multiple EV_KEY_PRESS events
      // where each event is guaranteed to be only a printable character.
      char *buffer = (char *)ev->data;
      char *target = (cry == 0 ? to : subj);
      unsigned target_len = (cry == 0 ? ADDR_LEN : SUBJ_LEN);
      for (unsigned ix = 0; ix < strlen(buffer); ix++)
      {
        if (strlen(target)<target_len-1) strncat(target, &buffer[ix], 1);
      }
    }
  }

  if (ev->type == EV_KEY_PRESS || ev->type == EV_KEY_PRESS_MULTI)
  {
    if (cry <= SYSLINES) draw_header();
    if (cry == 0) {crx = strlen(to); wmove(win, 1, 12+crx);}
    if (cry == 1) {crx = 0; wmove(win, 2, 12);}
    if (cry == 2) {crx = strlen(subj); wmove(win, 3, 12+crx);}
    if (cry == 3) {crx = 0; wmove(win, 4, 12);}
    if (cry <= SYSLINES) wrefresh(win);
  }
}

void Composer::draw_header()
{
  mvwprintw(win, 1, 2, HEADER_FMT("To",ADDR_FMT), to);
  mvwprintw(win, 2, 2, HEADER_FMT("Type","%c"), type);
  mvwprintw(win, 3, 2, HEADER_FMT("Subject",SUBJ_FMT), subj);
  if (ttabnum == -1)
    mvwprintw(win, 4, 2, HEADER_FMT("Encode","<no encoding>"));
  else
    mvwprintw(win, 4, 2, HEADER_FMT("Encode",ENC_FMT), tables[ttabnum]);
}

void Composer::draw(bool all)
{
  if (all)
  {
    wbkgdset(win, ' ' | COLOR_PAIR(ED_INFO) | A_BOLD);
    draw_header();
    mvwhline(win, SYSLINES+2, 1, ACS_HLINE, x2-x1-2);

    ed->draw(all);
  }
  wrefresh(win);
}

void Composer::edredraw()
{
   ed->draw(true);
}

void Composer::load_texts()
{
   char fname[256];
   FILE *f;
   char *p = lp_get_var(0, "MAIL_PATH");
   int i = 0;
 
   if (p == NULL) return;
 
   head_lines = 0;
   sprintf(fname, "%s/mail.head", lp_get_var(0, "MAIL_PATH"));
   f = fopen(fname, "r");
   if (f != NULL)
   {
     while (!feof(f))
     {
       char s[LINE_LEN+1];
       strcpy(s, "");
       if (fgets(s, LINE_LEN, f) != NULL)
       {
         if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
         replace_macros(lp_channel(), s);
         ed->line[i].accept(s);
         i++;
         head_lines++;
       }
     }
     fclose(f);
   }
 
   i += 2;
   sprintf(fname, "%s/mail.tail", lp_get_var(0, "MAIL_PATH"));
   f = fopen(fname, "r");
   if (f != NULL)
   {
     while (!feof(f))
     {
       char s[LINE_LEN+1];
       strcpy(s, "");
       if (fgets(s, LINE_LEN, f) != NULL)
       {
         if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
         replace_macros(lp_channel(), s);
         ed->line[i].accept(s);
         i++;
       }
     }
     fclose(f);
   }
 
   ed->max = i;
}


bool Composer::send_message()
{
   struct tm *tim;
   time_t secs = time(NULL);
   tim = gmtime(&secs);

   if (strlen(to) == 0)
   {
      ed->errormsg("No destination address specified");
      return false;
   }
   if (strlen(subj) == 0)
   {
      ed->errormsg("No subject specified");
      return false;
   }

   //open the index
   OutgoingIndex ndx(home_call);
   //get message number
   msgnum = ndx.newIndex();
 
   if (ttabnum > -1) load_table(ttabnum);
   //translate subject
   for (unsigned i = 0; i < strlen(subj); i++)
       if (subj[i] < ttabsize) subj[i] = ttable[(unsigned)subj[i]];
   
   //get source callsign
   char *mycall = strdup(call_call(lp_chn_call(lp_channel())));
   //!!!alokace from???
   if (from) delete[] from;
   from = new char[strlen(mycall)+strlen(home_addr)+2];
   sprintf(from, "%s@%s", mycall, home_addr);
   //complete destination callsign if no destination specified
   if (strchr(to, '@') == NULL)
   {
      strcat(to, "@");
      strcat(to, home_addr);
   }
   strupr(to);
   strupr(from);
 
   //create message file
   char *fname = strdup("/tmp/lp_mail.XXXXXX");
   int tmpdesc = mkstemp(fname); close(tmpdesc);
   FILE *f = fopen(fname, "w");
   if (f == NULL)
   {
      ed->errormsg("Cannot create temp file");
      beep();
      return false;
   }
   fprintf(f, "%s\n", subj);
 
   //write header
   char date[256];
   sprintf(date, "%02i%02i%02i/%02i%02i", (tim->tm_year+1900)%100,
            tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min);
 #ifdef PATH_TO_OWN
   fprintf(f, "R:%sZ @:%s.\n", date, mycall);
   fprintf(f, "\n");
   fprintf(f, "From: %s\n", from);
   fprintf(f, "To: %s\n", to);
   fprintf(f, "\n");
 #endif
 
   //write message body
   for (int i=0; i < ed->max; i++)
   {
      char s[LINE_LEN+1];
      ed->line[i].get_text(s);
  
      if (ed->line[i].editable)
      {
         for (unsigned i = 0; i < strlen(s); i++)
            if (s[i] < ttabsize) s[i] = ttable[(unsigned)s[i]];
         fprintf(f, "%s\n", s);
      }
   }
 
   size = ftell(f);
   fclose(f);
   char *body = NULL;
   f = fopen(fname, "r");
   if (f)
   {
      body = new char[size+1];
      int n = fread(body, 1, size, f);
      body[n] = '\0';
      fclose(f);
      close(tmpdesc);
   }
   else
   {
      ed->errormsg("Cannot create temp file");
      beep();
      return false;
   }
   unlink(fname);
   free(fname);
 
   //create MID
   sprintf(mid, "%06i%s", msgnum, mycall);
   while (strlen(mid) < 12) strcat(mid, "0");
 
   //add to index
   Message newmsg(msgnum,
                  type=='P'?"PO":"BO",
                  mid, from, to, date, subj);
   newmsg.setBody(body);
   ndx.addMessage(newmsg);
   ndx.writeIndex();
   delete[] body;
 
   //scan for inserted commands
   for (int i=0; i < ed->max; i++)
   {
      char s[LINE_LEN+1];
      ed->line[i].get_text(s);
  
      if (!ed->line[i].editable && s[0] == '.')
      {
         char cmd[LINE_LEN+1];
         char par[LINE_LEN+1];
         sscanf(s+1, "%s %s", cmd, par);
  
         if (strcmp(cmd, "attach") == 0)
         {
            sprintf(cmd, "sendfile %c %s %s", tolower(type), par, to);
            fprintf(stderr, "Command: %s\n", cmd);
            lp_emit_event(lp_channel(), EV_DO_COMMAND, 0, cmd);
         }
      }
   }
   delete[] mycall;
   return true;
}

void Composer::load_table(int n)
{
   FILE *f;
 
   if (n < ntables)
   {
      f = fopen(tables[n], "r");
      if (f == NULL)
      {
         fprintf(stderr, "Cannot load translation table\n");
         beep();
         return;
      }
      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      fseek(f, size/2, SEEK_SET);
      ttabsize = fread(ttable, 1, 256, f);
      fclose(f);
   }
}

void Composer::check_dest()
{
   strupr(to);
   if (!contains_digit(to)) type = 'B';
   if (strchr(to, '@') == NULL)
   {
      char route[256];
      if (find_route(to, route))
      {
         strcat(to, "@");
         strcat(to, route);
      }
      strupr(to);
   }
}

void Composer::insert(char *s, bool quote)
{
   char ln[LINE_LEN+1];
   char *p = s;
 
   unsigned limit;
   if (quote)
      limit = x2-x1-5 < LINE_LEN-2 ? x2-x1-5 : LINE_LEN-2;
   else
      limit = x2-x1-5 < LINE_LEN ? x2-x1-5 : LINE_LEN;
 
   while (strlen(p) > limit)
   {
      char *q = p + limit;
      while (q > p && !isspace(*q)) q--;
      *q = '\0';
      if (quote) sprintf(ln, "> %s", p);
      else strcpy(ln, p);
      ed->ins_info(ln);
      p = q+1;
   }
 
   if (strlen(p) > 0)
   {
      if (quote) sprintf(ln, "> %s", p);
      else strcpy(ln, p);
      ed->ins_info(ln);
   }
}

