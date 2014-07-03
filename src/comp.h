/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   comp.h

   Huffman compression routines. Based on the original source of:

   tnt: Hostmode Terminal for TNC
   Copyright (C) 1993-1996 by Mark Wahl
   For license details see documentation
   Procedures for huffman compression (comp.c)
   created: Mark Wahl DL4YBG 95/02/05
   updated: Johann Hanne 99/01/02
   
   Last update 4.4.2000
  =========================================================================*/

#ifndef COMP_H
#define COMP_H

struct huffencodtab {
  unsigned short code;
  unsigned short len;
};

struct huffdecodtab {
  unsigned short node1;
  unsigned short node2;
};

int decstathuf(char *src, char *desc);
int encstathuf(char *src, int srclen, char *dest, int *destlen);

#endif

