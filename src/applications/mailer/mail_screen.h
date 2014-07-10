/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_screen.h - screen management
*/
#ifndef _MAIL_SCREEN_H
#define _MAIL_SCREEN_H

#include "lpapp.h"

//color pairs
#define C_BACK 1
#define C_TEXT 2
#define C_SELECT 3
#define C_UNSELECT 4
#define C_MSG 5
#define C_ERROR 6

#define ED_TEXT 10 //editor
#define ED_INFO 11
#define ED_CTRL 12
#define ED_STAT 13

#define IL_TEXT 14 //input line

#define HELP_LIST "(D)ownload selected, (F)ilter, toggle (I)ndex, (T)ranslate, (H)elp, (Q)uit"
#define HELP_VIEW "(W)rap (T)ranslate (H)eader (R)eply (C)omment (S)ave E(x)port (F)wd (Q)uit"
#define HELP_EDIT "^X:send  ^A:abort  ^T:time  ^D:date  ^R:read file  ^F:attach  ^E:export"
#define HELP_BOARDS1 "(A)dd new group, (D)elete group, TAB - switch, Enter - accept"
#define HELP_BOARDS2 "(A)dd group, Space - select, * - invert, TAB - switch, Enter - accept"
#define HELP_HELP "ESC - return, (Q)uit"

#define CENTER(xxx) ((maxx-(xxx)+1)/2)

extern bool act;            //application is visible
extern bool stop;           //abort received
extern int maxx, maxy;      //screen sizes
extern void *main_window;   //main screen window
extern void *focus_window;  //focused window

extern char ttable[256];    //actual translation table
extern int tabsize;         //size of actual table
extern char tables[32][16]; //usable table names
extern int ntables;         //number of usable names

extern bool view_returned;  //returned from viewer to message list
extern bool blist_returned; //returned from board list to message list

extern char *mailpath;      //local mail path

void init_main_screen();
void redraw();
void help(const char *s);

void load_table(int n);
int safe_wrefresh(void *win);

//Generic screen object class
class screen_obj
{
  public:
    char class_name[10];
    virtual void handle_event(Event *ev) = 0;
    virtual void draw(bool all = false) = 0;
};

//Main screen
class Main_scr : public screen_obj
{
  public:
    bool blocked;

    virtual void handle_event(Event *ev);
    void draw(bool all = false);
    void show_message(char *text);
    void discard_message();
    virtual ~Main_scr() {};
};

//Focused object
extern screen_obj *focused;

//Message handling objects (or NULL)
extern screen_obj * message_handler[16];

#endif

