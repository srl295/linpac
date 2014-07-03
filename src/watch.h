/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (radkovo@centrum.cz) 1998 - 2002

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   watch.h

   Objects for automatic command execution

   Last update 21.6.2002
  =========================================================================*/
#ifndef WATCH_H
#define WATCH_H

#define WATCH_BUFFER_SIZE 1024

#include <vector>
#include "event.h"
#include "constants.h"

struct autorun_entry
{
  char key[256];
  int chn; // 0 = all channels
  char command[128];
};

class Watch : public Object
{
  private:
    std::vector <autorun_entry> watch;             //list of autorun entries
    char buf[MAX_CHN+1][WATCH_BUFFER_SIZE];
    int buflen[MAX_CHN+1];
    bool disabled[MAX_CHN+1]; //watching is disabled (binary transfer)
    bool csent[MAX_CHN+1];    //connect command sent
    char ctext[15];
    char cinit[15];
    char cexit[15];
    char lastcall[15];        //previous connected callsign

    bool key_found(int chn, char *key);
    int com_is(char *s1, char *s2);
    void extract_call(const char *from, char *to, int len);

  public:
    Watch();
    virtual ~Watch() {};
    virtual void handle_event(const Event &);
};

#endif

