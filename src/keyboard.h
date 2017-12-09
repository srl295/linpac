/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   keyboard.h

   Keyboard interface
   
   Last update 21.10.2000
  =========================================================================*/

#ifndef KEYBOARD_H
#define KEYBOARD_H

class Keyscan : public Object
{
  private:
    WINDOW *keywin;
  public:
    Keyscan();
    virtual ~Keyscan();
    virtual void handle_event(const Event &);
};

#endif

