/*
  LinPac Mailer
  (c) 1998 - 2001 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_filt.cc - select message destinations
*/
#include <algorithm>
#include <string.h>
#include <ctype.h>
#include <ncurses.h>

#include "mail_filt.h"

#define COLUMN1 20
#define COLUMN2 40

std::vector <tbname> bfilter;
std::vector <tbname>::iterator bfit;

tbname &tbname::operator = (const tbname &src)
{
  strcpy(name, src.name);
  return *this;
}

void create_list(MessageIndex *ndx, std::vector <Msg> &msgs,
                 std::vector <Board> &boards, char *mycall)
{
   std::vector <Msg>::iterator it;
   for (it = msgs.begin(); it < msgs.end(); it++)
      if (strchr(ndx->getMessage(it->index)->getFlags(), 'B') != NULL)
      {
         Board newb;
         strncpy(newb.name, ndx->getMessage(it->index)->getDest(), 30);
         newb.name[30] = '\0';
         char *p = strchr(newb.name, '@');
         if (p != NULL) *p = '\0';
         newb.sel = false;
         boards.push_back(newb);
      }

   //add a personal folder
   Board personal;
   strncpy(personal.name, mycall, 30); personal.name[30] = '\0';
   personal.sel = false;
   boards.push_back(personal);

   //sort it
   sort(boards.begin(), boards.end());
   std::vector <Board>::iterator where = unique(boards.begin(), boards.end());
   boards.erase(where, boards.end());
}

//=========================================================================
// Class Board
//=========================================================================
Board &Board::operator = (const Board &src)
{
   strcpy(name, src.name);
   sel = src.sel;
   return *this;
}

bool operator == (const Board &b1, const Board &b2)
{
   return (strcmp(b1.name, b2.name) == 0);
}

bool operator < (const Board &b1, const Board &b2)
{
   return (strcmp(b1.name, b2.name) < 0);
}

//=========================================================================
// Class BGroup
//=========================================================================

bool BGroup::contains(const char *name)
{
   std::vector <tbname>::iterator it;
   for (it = content.begin(); it < content.end(); it++)
      if (strcasecmp(name, it->name) == 0) return true;
   return false;
}

BGroup &BGroup::operator = (const BGroup &src)
{
   for (unsigned i = 0; i < src.content.size(); i++)
       content.push_back(src.content[i]);

   return *this;
}

//=========================================================================
// Class BoardList
//=========================================================================
BoardList::BoardList(MessageIndex *ndx, std::vector <Msg> &msgs, char *mycall)
{
   create_list(ndx, msgs, boards, mycall);
   pos = 0;
   slct = 0;
   gpos = 0;
   gslct = 0;
   col = 1;
   wait_gname = false;
   iline = NULL;

   BGroup *none = new BGroup;
   strcpy(none->name, "<all>");
   none->sel = false;
   groups.push_back(*none);
   load_groups();
}

BoardList::BoardList(std::vector <Board> &bds)
{
   boards = bds;
   pos = 0;
   slct = 0;
   gpos = 0;
   gslct = 0;
   col = 1;
   wait_gname = false;
   iline = NULL;

   BGroup *none = new BGroup;
   strcpy(none->name, "<all>");
   none->sel = false;
   groups.push_back(*none);
   load_groups();
}

void BoardList::clear_filter()
{
   bfilter.erase(bfilter.begin(), bfilter.end());
}

void BoardList::save_groups()
{
   char *fname;
   FILE *f;
   char *p = lp_get_var(0, "MAIL_PATH");

   if (p == NULL)
   {
      fprintf(stderr, "Cannot save groups - MAIL_PATH@0 is not set\n");
      return;
   }

   fname = new char[strlen(p) + 20];
   sprintf(fname, "%s/mail.groups", p);
   f = fopen(fname, "w");
   if (f != NULL)
   {
      std::vector <BGroup>::iterator it;
      for (it = groups.begin()+1; it < groups.end(); it++)
      {
         fprintf(f, "[%s]\n", it->name);
         std::vector <tbname>::iterator bname;
         for (bname = it->content.begin(); bname < it->content.end(); bname++)
            fprintf(f, "%s\n", bname->name);
         fprintf(f, "\n");
       }
       fclose(f);
    }
    else
       fprintf(stderr, "Cannot save groups - cannot write to `%s'\n", fname);
    delete[] fname;
}

void BoardList::load_groups()
{
   char *fname;
   char s[256];
   BGroup newgroup;
   tbname newname;
   bool creating = false;
   FILE *f;
   char *p = lp_get_var(0, "MAIL_PATH");

   if (p == NULL)
   {
      fprintf(stderr, "Cannot load groups - MAIL_PATH@0 is not set\n");
      return;
   }

   fname = new char[strlen(p) + 20];
   sprintf(fname, "%s/mail.groups", p);
   f = fopen(fname, "r");
   if (f != NULL)
   {
      while (!feof(f))
      {
         strcpy(s, "");
         if (fgets(s, 255, f) != NULL)
         {
            if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
            if (strlen(s) == 0) continue;

            if (s[0] == '[')
            {
                //store previous group
                if (creating) groups.push_back(newgroup);
                //create new group
                memmove(s, s+1, strlen(s));
                s[strlen(s)-1] = '\0'; //remove ]
                strcpy(newgroup.name, s);
                newgroup.content.erase(newgroup.content.begin(),
                                    newgroup.content.end());
                newgroup.sel = false;
                creating = true;
            }
            else
            {
                strcpy(newname.name, s);
                newgroup.content.push_back(newname);
            }
         }
      }
      if (creating) groups.push_back(newgroup);
      fclose(f);
   }
   delete[] fname;
}

void BoardList::delete_group(int sel)
{
   std::vector <BGroup>::iterator it = groups.begin();
   for (int i = 0; i < sel; i++) it++;
   groups.erase(it);
}

//-------------------------------------------------------------------------

void BoardList::init_screen(WINDOW *pwin, int height, int width, int wy, int wx)
{
   xsize = width;
   ysize = height;
   x = wx;
   y = wy;
 
   //WINDOW *win = subwin(pwin, ysize, xsize, y, x);
   mwin = pwin;
   keypad(pwin, true);
   meta(pwin, true);
   nodelay(pwin, true);
 
   draw(true);
 
   old_focus_window = focus_window;
   focus_window = pwin;
   old_focused = focused;
   focused = this;
 
   help(HELP_BOARDS2);
}

void BoardList::draw(bool all)
{
   if (all)
   {
      wbkgdset(mwin, ' ' | COLOR_PAIR(C_TEXT) | A_BOLD);
      werase(mwin);
      box(mwin, ACS_VLINE, ACS_HLINE);
      mvwprintw(mwin, 0, 2, "Select bulletins");
      mvwprintw(mwin, 1, 4, "Groups");
      mvwprintw(mwin, 1, COLUMN1+4, "Bulletins");
  
      mvwvline(mwin, 1, COLUMN1, ACS_VLINE, ysize-2);
      mvwvline(mwin, 1, COLUMN2, ACS_VLINE, ysize-2);
   }
   
   //Draw group names
   for (unsigned i=0; i<ysize-3; i++)
      if (i+gpos < groups.size())
      {
         long attr;
         if (gslct == i+gpos && col == 0) attr = COLOR_PAIR(C_SELECT);
                                     else attr = COLOR_PAIR(C_UNSELECT);
         
         wbkgdset(mwin, ' ' | attr);
     
         int sl;
         if (groups[i+gpos].sel) sl = ACS_BULLET;
                            else sl = ' ';
   
         mvwprintw(mwin, i+2, 2, "%c %-*.*s", sl, COLUMN1-5, COLUMN1-5, groups[i+gpos].name);
      }
      else
      {
         wbkgdset(mwin, COLOR_PAIR(C_TEXT));
         mvwprintw(mwin, i+2, 2, "%-*.*s", COLUMN1-3, COLUMN1-3, "");
      }
 
   //Draw bulletin names
   for (unsigned i=0; i<ysize-3; i++)
      if (i+pos < boards.size())
      {
         long attr;
         if (slct == i+pos && col == 1) attr = COLOR_PAIR(C_SELECT);
                                   else attr = COLOR_PAIR(C_UNSELECT);
         
         wbkgdset(mwin, ' ' | attr);
     
         int sl;
         if (boards[i+pos].sel) sl = ACS_BULLET;
                           else sl = ' ';
   
         mvwprintw(mwin, i+2, COLUMN1+2, "%c %-*.*s", sl, COLUMN2-COLUMN1-5, COLUMN2-COLUMN1-5, boards[i+pos].name);
      }
      else
      {
         wbkgdset(mwin, COLOR_PAIR(C_TEXT));
         mvwprintw(mwin, i+2, COLUMN1+2, "%-*.*s", COLUMN2-COLUMN1-3, COLUMN2-COLUMN1-3, "");
      }
 
    wnoutrefresh(mwin);
    for (int i = 0; i < 5; i++) doupdate(); //must be done many times, don't know why
}

void BoardList::handle_event(Event *ev)
{
  if (wait_gname)
  {
    wait_gname = false;
    if (iline != NULL) delete iline;

    if (strlen(ibuffer) > 0)
    {
      BGroup newgrp;
      strcpy(newgrp.name, ibuffer);
      newgrp.sel = false;

      tbname newname;
      std::vector <Board>::iterator it;
      for (it = boards.begin(); it < boards.end(); it++)
        if (it->sel)
        {
          strcpy(newname.name, it->name);
          newgrp.content.push_back(newname);
        }

      groups.push_back(newgrp);
      draw();
    }
  }

  if (wait_gdel)
  {
    wait_gdel = false;
    if (iline != NULL) delete iline;

    if (ibuffer[0] == 'Y' || ibuffer[0] == 'y')
    {
      delete_group(gslct);
      gslct = 0;
      draw();
    }
  }

  if (ev == NULL) return;

  if (ev->type == EV_KEY_PRESS)
  {
    if (ev->x == KEY_DOWN)
      if (col == 0)
      {
        if (gslct < groups.size()-1) gslct++;
        if (gslct-gpos >= ysize-3) gpos++;
      }
      else
      {
        if (slct < boards.size()-1) slct++;
        if (slct-pos >= ysize-3) pos++;
      }

    if (ev->x == KEY_UP)
      if (col == 0)
      {
        if (gslct > 0) gslct--;
        if (gslct < gpos) gpos--;
      }
      else
      {
        if (slct > 0) slct--;
        if (slct < pos) pos--;
      }

    if (ev->x == KEY_NPAGE)
      for (unsigned j=0; j < (ysize-3); j++)
        if (col == 0)
        {
          if (gslct < groups.size()-1) gslct++;
          if (gslct-gpos >= ysize-3) gpos++;
        }
        else
        {
          if (slct < boards.size()-1) slct++;
          if (slct-pos >= ysize-3) pos++;
        }

    if (ev->x == KEY_PPAGE)
      for (unsigned j=0; j < (ysize-3); j++)
        if (col == 0)
        {
          if (gslct > 0) gslct--;
          if (gslct < gpos) gpos--;
        }
        else
        {
          if (slct > 0) slct--;
          if (slct < pos) pos--;
        }

    if (ev->x == '\t')
      if (col == 1)
      {
        col = 0;
        help(HELP_BOARDS1);
      }
      else
      {
        col = 1;
        help(HELP_BOARDS2);
      }

    //select bulletins in group
    if (col == 0)
    {
      std::vector <Board>::iterator it;
      for (it = boards.begin(); it < boards.end(); it++)
        it->sel = groups[gslct].contains(it->name) ||
                  groups[gslct].content.empty();
    }

    if (toupper(ev->x) == 'Q')
    {
      focused = NULL; //exit immediatelly
    }

    if (ev->x == ' ')
    {
      if (col == 1) boards[slct].sel = !boards[slct].sel;
    }

    if (col == 1 && ev->x == '*')
    {
      std::vector <Board>::iterator it;
      for (it = boards.begin(); it < boards.end(); it++)
        it->sel = !it->sel;
    }

    if (ev->x == '\n') //Enter
    {
      save_groups();
      
      //update bfilter
      tbname newname;
      clear_filter();
      std::vector <Board>::iterator it;
      for (it = boards.begin(); it < boards.end(); it++)
        if (it->sel)
        {
          strcpy(newname.name, it->name);
          bfilter.push_back(newname);
        }

      if (bfilter.empty() && col == 1)
      {
        strcpy(newname.name, boards[slct].name);
        bfilter.push_back(newname);
      }

      blist_returned = true;
      focus_window = old_focus_window;
      focused = old_focused;
      focused->handle_event(NULL);
      focused->draw(true);
      help(HELP_LIST);
    }
    else if (ev->x == '\x1b' || toupper(ev->x) == 'L') //ESC
    {
      save_groups();
      
      blist_returned = true;
      focus_window = old_focus_window;
      focused = old_focused;
      focused->handle_event(NULL);
      focused->draw(true);
      help(HELP_LIST);
    }
    else if (toupper(ev->x) == 'A')
    {
      wait_gname = true;
      iline = new InputLine(mwin, 5, ysize-1, xsize-10, 20,
                            "New group name:", ibuffer, INPUT_UPC);
    }
    else if (toupper(ev->x) == 'D' && col == 0)
    {
      wait_gdel = true;
      iline = new InputLine(mwin, 5, ysize-1, xsize-10, 1,
                            "Really delete this group (y/N) ?", ibuffer, INPUT_ONECH);
    }
    else draw();
  }
}

