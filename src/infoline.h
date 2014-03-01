/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   infoline.h

   Objects for commands executing

   Last update 19.9.1998
  =========================================================================*/
#include <vector.h>

class info_line
{
  public:
    int id;  //owner's id
    char content[256];
    info_line() {};
    info_line(const info_line &);
    info_line &operator = (const info_line &);
};

//This fixes the stupid ncurses 'erase' macro that overrides all
//vector::erase() functions
void info_line_erase(vector <info_line> &v, vector <info_line>::iterator it);
