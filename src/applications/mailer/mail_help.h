/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_help.h - help window
*/
#include "mail_screen.h"

class HelpWindow : public screen_obj
{
  private:
    void *mwin;
    int x, y, xsize, ysize;
    int page;
    int max_pages;

    void *old_focus_window;
    screen_obj *old_focused;

  public:
    HelpWindow(void *pwin, int height, int width, int wy, int wx);
    void show();
    virtual void handle_event(Event *);
    virtual void draw(bool all = false);
    virtual ~HelpWindow() {};
};

