/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   infoline.cc

   Infoline utilities

   Last update 19.9.1998
  =========================================================================*/
#include "infoline.h"

void info_line_erase(std::vector <info_line> &v, std::vector <info_line>::iterator it)
{
  v.erase(it);
}
