/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   names.cc

   Station database classes

   Last update 23.10.1999
  =========================================================================*/
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include "names.h"

StnDB::StnDB(char const *dbname)
{
  strcpy(class_name, "StnDB");
  f = fopen(dbname, "r");
  if (f == NULL)
  {
    f = fopen(dbname, "w+");
    fclose(f);
    f = fopen(dbname, "r");
  }
  if (f == NULL) Error(errno, "Cannot open station database");
}

StnDB::~StnDB()
{
  if (f != NULL) fclose(f);
}

void StnDB::handle_event(const Event &ev)
{
  if (ev.type == EV_CONN_TO || ev.type == EV_RECONN_TO)
  {
    char call[20], ccall[20];
    strcopy(call, reinterpret_cast<char *>(ev.data), 20);
    //add the zero SSID to call
    if (strchr(call, '-') == NULL) strcat(call, "-0");
    //get the call w/o ssid
    strcopy(ccall, call_call(call), 20);
    clear_var_names(ev.chn, "STN_");
    if (find_data(call)) read_data(ev.chn);
    else if (find_data(ccall)) read_data(ev.chn);

    if (ev.type == EV_CONN_TO || ev.x == 1)
    {
      static char info[256], *name, *loc;
      name = get_var(ev.chn, "STN_NAME");
      loc = get_var(ev.chn, "STN_LOC");
      if (loc != NULL)
        sprintf(info, "*** Info: %s, loc. %s\n", name, loc);
      else
        sprintf(info, "*** Info: %s\n", name);
      emit(ev.chn, EV_LOCAL_MSG, strlen(info), info);
    }
  }
  if (ev.type == EV_DISC_FM || ev.type == EV_DISC_TIME ||
      ev.type == EV_FAILURE)
          clear_var_names(ev.chn, "STN_");
}

bool StnDB::find_data(char *stn)
{
  char s[20], srch[20];
  sprintf(srch, "[%s]\n", stn);
  rewind(f);
  while (!feof(f))
  {
    strcpy(s, "");
    if (fgets(s, 19, f) != NULL)
    {
      if (strcasecmp(s, srch) == 0) break;
    }
  }
  return !feof(f);
}

void StnDB::read_data(int chn)
{
  char s[256], cmd[10], par[256];
  clear_var_names(chn, "STN_");
  do
  {
    strcpy(s, "");
    if (fgets(s, 256, f) != NULL)
    {
      char *q = s + (strlen(s) - 1);
      while (q > s && (isspace(*q) || *q == '\n')) {*q = '\0'; q--;}
      //if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
      char *p = strchr(s, '=');
      if (p != NULL)
      {
        strcpy(par, p+1);
        *p = '\0';
        strcpy(cmd, s);
      } else break;
      if (strlen(cmd) > 0)
      {
        sprintf(s, "STN_%s", cmd);
        set_var(chn, s, par);
      }
      else break;
    }
  } while (!feof(f));
}

