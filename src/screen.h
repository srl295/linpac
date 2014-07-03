/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 1999

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   screen.h

   Objects for user interaction

   Last update 6.1.2000
  =========================================================================*/

#ifndef SCREEN_H
#define SCREEN_H

#include <vector.h>
#include <ncurses.h>
#include "event.h"
#include "constants.h"

//-------------------------------------------------------------------------
// screen_obj - virtual screen object
//-------------------------------------------------------------------------
class Screen_obj : public Object
{
  public:
    int x1, y1, x2, y2;   //object position on screen

    virtual void draw() = 0;
};


//-------------------------------------------------------------------------
// Screen - screen object manager
//-------------------------------------------------------------------------
class Screen : public Object
{
  private:
    WINDOW *mainwin;
    vector <Screen_obj *> children;  //list of inserted objects

  public:
    Screen();
    virtual ~Screen();
    void insert(Screen_obj *); //insert next object
    //void remove(Screen_obj *); //remove object
    void redraw();             //redraw all objects
    void stop();               //finish work

    virtual void handle_event(const Event &);
};

//-------------------------------------------------------------------------
// Public functions
//-------------------------------------------------------------------------

short color_num(char *name);
int   color_attr(char *name);
short color_code(char *code);

int fgattr(int code);
int bgattr(int code);

#endif

