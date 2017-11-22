/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_input.h - input line classes
*/

#ifndef MAIL_INPUT_H
#define MAIL_INPUT_H

#include "mail_screen.h"

//Input line modes
#define INPUT_ALL   0   //input all non-control chars
#define INPUT_ALNUM 1   //input digits and letters
#define INPUT_UPC   2   //input digits and upper-case letters
#define INPUT_NUM   3   //input numbers only
#define INPUT_ONECH 4   //input one char only

//-------------------------------------------------------------------------
// InputLine - input line
//-------------------------------------------------------------------------
class InputLine : public screen_obj
{
  private:
    int x, y;           //screen position
    int fieldln;        //field length
    unsigned textln;    //text length
    char ptext[256];    //prompt text
    char *buf;          //input buffer
    int imode;          //input mode

    WINDOW *old_focus_window;
    screen_obj *old_focused;

  public:
    int crx;            //cusror position

    InputLine(WINDOW *parent, int wx, int wy, int flen, unsigned textlen,
              char const *prompt, char *buffer, int mode = INPUT_ALL);
    void handle_event(Event *ev);
    void draw(bool all = false);
    virtual ~InputLine() {};
};

#endif

