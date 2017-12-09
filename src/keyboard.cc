/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   keyboard.cc

   Keyboard interface
   
   Last update 21.10.2000
  =========================================================================*/

#include <string.h>
#include <iostream>
#include <ncurses.h>

#include "version.h"
#include "event.h"
#include "data.h"
#include "keyboard.h"

//--------------------------------------------------------------------------
// Class Keyscan
//--------------------------------------------------------------------------

Keyscan::Keyscan()
{
  strcpy(class_name, "Keyscan");
  keywin = newwin(1, 1, 0, 0);
  keypad(keywin, true);
  meta(keywin, true);
  nodelay(keywin, true);
}

Keyscan::~Keyscan()
{
  delwin(keywin);
}

void Keyscan::handle_event(const Event &ev)
{
  if (ev.type == EV_NONE) //only when void-loop
  {
    int ch = wgetch(keywin);
    //int ch = getch();
    std::cout.flush();
    if (ch != ERR)
    {
      Event re;
      re.type = EV_KEY_PRESS;
      if (ch == '\x1B')
      {
        re.y = FLAG_MOD_ALT;
        ch = wgetch(keywin);
      }
      else re.y = FLAG_MOD_NONE;
      re.x = ch;
      clear_last_act(); //clear last activity counter
      //Interpret shortcuts
      if (re.y != FLAG_MOD_ALT) //Alt not pressed
      {
        //Function keys - select channels
        if (ch == KEY_F(1)) emit(1, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(2)) emit(2, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(3)) emit(3, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(4)) emit(4, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(5)) emit(5, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(6)) emit(6, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(7)) emit(7, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(8)) emit(8, EV_SELECT_CHN, 0, NULL);
        else if (ch == KEY_F(10)) emit(0, EV_SELECT_CHN, 0, NULL);
        else if (!isprint(ch)) emit(re);
        else
        {
            // Accumulate printable characters into a single event while ncurses
            // still has input to give us or until we fill our buffer.
            char buffer[LINE_LEN+1];
            int ix = 0;

            buffer[ix++] = ch;
            while (isprint(ch = wgetch(keywin)) && ix < LINE_LEN)
                buffer[ix++] = ch;
            // If we got a non-printable character, put it back for next time.
            if (ch != ERR) ungetch(ch);
            // Only emit a multi-key event if we have multiple keys.
            if (ix > 1)
            {
                buffer[ix] = '\0';
                re.type = EV_KEY_PRESS_MULTI;
                re.x = 0;
                re.data = buffer;
            }
            emit(re);
        }
      }
      else
      {
        if (ch >= '1' && ch <= '8') emit(-(ch - '0'), EV_SELECT_CHN, 0, NULL);
        else if (ch == '0') emit(-999, EV_SELECT_CHN, 0, NULL);
        else if (ch == 'x') emit(0, EV_QUIT, 0, NULL); // alt-X -> exit
        else emit(re);
      }
    }
  }
}

