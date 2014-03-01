/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   calls.h

   Functions for callsign conversions

   Last update 20.5.1998
  =========================================================================*/
#include <linux/ax25.h>
#ifndef CALLS_H
#define CALLS_H

void norm_call(char *peer, const ax25_address *addr);
int convert_call(char *name, char *buf);
int convert_path(char *call, struct full_sockaddr_ax25 *sax);

#endif

