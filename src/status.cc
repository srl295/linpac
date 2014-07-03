/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2001

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   status.cc

   Connection status classes

   Last update 8.12.2001
  =========================================================================*/
#include <stdio.h>
#include <string.h>
#include "data.h"
#include "status.h"

StatusServer::StatusServer()
{
   strcpy(class_name, "Status");
   procform = check_proc_format();
   enable_ioctl = false;
   #ifndef NEW_AX25
   force_ioctl = false;
   #else
   force_ioctl = true;
   #endif
}

StatusServer::~StatusServer()
{
}

void StatusServer::handle_event(const Event &ev)
{
   if (ev.type == EV_STAT_REQ)
   {
      int strategy = procform;
      if ((iconfig("info_level") == 1 && enable_ioctl) || force_ioctl) strategy = 2; //use ioctl
      if (get_axstat(sconfig(ev.chn, "call"), sconfig(ev.chn, "cphy"), strategy))
      {
         emit(ev.chn, EV_STATUS, sizeof(chstat), &chstat);
      }
   }
   if (ev.type == EV_SYSREQ)
   {
      if (ev.x == 3) enable_ioctl = !enable_ioctl;
      if (ev.x == 4) force_ioctl = !force_ioctl;
      if (ev.x == 5) procform = !procform;
   }
}

int StatusServer::check_proc_format()
{
  //return: 0 = 2.2.x format
  //        1 = 2.0.x format
  int res = 0;
  char s[256];
  FILE *f;

  f = fopen(PROCFILE, "r");
  if (f != NULL)
  {
    fgets(s, 255, f);
    fgets(s, 255, f);
    if (strchr(s, '/') == NULL) res = 1; //older format contains '/'
    fclose(f);
  }
  return res;
}

