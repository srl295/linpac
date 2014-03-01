/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2001

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   status.h

   Connection status classes

   Last update 8.12.2001
  =========================================================================*/
#ifndef STATUS_H
#define STATUS_H

#include "event.h"

// The only function of StatusServer object is to generate the EV_STATUS
// event each time the EV_STAT_REQ event is received

class StatusServer : public Object
{
  private:
    int procform;
    bool enable_ioctl; //use ioctl for infolevel 1
    bool force_ioctl;  //use ioctl for infolevels 1 and 2

  public:
    StatusServer();
    virtual ~StatusServer();
    virtual void handle_event(const Event &);

  private:
    int check_proc_format();
};

#endif

