/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   names.h

   Station database classes

   Last update 8.10.1999
  =========================================================================*/
#include <stdio.h>
#include "event.h"
#include "tools.h"
#include "data.h"

class StnDB : public Object
{
  private:
    FILE *f;                      //station database file

  public:
    StnDB(char const *dbname);
    virtual ~StnDB();
    virtual void handle_event(const Event &);
    bool find_data(char *stn);    //find the data record for stn
    void read_data(int chn);      //read the data and set the variables
};

