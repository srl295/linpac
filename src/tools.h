/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   tools.h

   Various tool functions
   
   Last update 11.4.2000
  =========================================================================*/
#ifndef TOOLS_H
#define TOOLS_H
#include "constants.h"

// I/O conversion tables
extern char *conv_in[MAX_CHN+1];
extern char *conv_out[MAX_CHN+1];

void Error(int no, const char *fmt, ...);

//Copy string to destination with n-bytes space
//The terminating \0 is always added
char *strcopy(char *dest, const char *src, size_t n);

//Remove the trailing \n from the string, if present
void striplf(char *s);

//Returns actual date and time
char *time_stamp(bool utc=false);
char *date_stamp(bool utc=false);
char *time_zone_name(int i=0);

//Return system description
char *sys_info();

#ifndef DAEMON_ONLY_MODE
//Prints into the ncurses window using specified attribute
void wprinta(void *win, int attr, const char *fmt, ...);
#endif

//To dest stores first callsign of the path in src
char *first_call(const char *src, char *dest);

//Retuns SSID of callsign
int call_ssid(const char *src);

//Retuns callsign w/o SSID
char *call_call(const char *src);

//Strips SSID if SSID == 0
char *strip_ssid(char *src);

//Convert callsign to standard format, capital letters, omit SSID 0
char *normalize_call(char *call);

//Return true, when callsign and SSID are the same
bool call_match(const char *call1, const char *call2);

//Return Ax.25 status description
char *state_str(int state);

//Return true if string represents a decadic number
bool is_number(const char *s);

//Load input and output conversion table and return the length of each table
int load_conversion_tables(int chn, char *name);

//Get the name of encoding table from the encodigns file
bool get_enc_alias(const char *alias, char *name, char *table);

//Redirect stderr stream to an error log
bool redirect_errorlog(char *path);

#endif

