/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   evqueue.cc

   Queue of events
   
   Last update 23.2.2000
  =========================================================================*/
#include <stdio.h>

#include <string.h>
#include <stdlib.h>
#include "evqueue.h"

EventQueue::EventQueue()
{
  last = time(NULL);
  maxl = 0;
}

void EventQueue::store(const Event &ev)
{
    Event re;
    re.chn = ev.chn;
    re.type = ev.type;
    re.x = ev.x;
    re.y = ev.y;
    re.ch = ev.ch;

    if (ev.type < 100) re.data = NULL;   //no data
    if (ev.type >= 100 && ev.type < 200) //string allocation
    {
      re.data = malloc(strlen((char *)ev.data)+1);
      strcpy((char *)re.data, (char *)ev.data);
    }
    if (ev.type >= 200 && ev.type < 300) //data buffer allocation
    {
      re.data = malloc(ev.x);
      memcpy(re.data, ev.data, ev.x);
    }
    if (ev.type >= 300 && ev.type < 400) re.data = ev.data; //pointer data

    event_queue.insertLast(re);
    if (maxl < length()) maxl = length();
}

void EventQueue::fstore(const Event &ev)
{
    Event re;
    re.chn = ev.chn;
    re.type = ev.type;
    re.x = ev.x;
    re.y = ev.y;
    re.ch = ev.ch;

    if (ev.type < 100) re.data = NULL;   //no data
    if (ev.type >= 100 && ev.type < 200) //string allocation
    {
      re.data = malloc(strlen((char *)ev.data)+1);
      strcpy((char *)re.data, (char *)ev.data);
    }
    if (ev.type >= 200 && ev.type < 300) //data buffer allocation
    {
      re.data = malloc(ev.x);
      memcpy(re.data, ev.data, ev.x);
    }
    if (ev.type >= 300 && ev.type < 400) re.data = ev.data; //pointer data

    event_queue.insertFirst(re);
    if (maxl < length()) maxl = length();
}

Event &EventQueue::top()
{
    return event_queue.first();
}

void EventQueue::pop()
{
    Event ev = event_queue.first();
    if (ev.type >= 100 && ev.type < 300 && ev.data != NULL)
      free(ev.data);
    event_queue.removeFirst();
    last = time(NULL);
}

time_t EventQueue::last_pop()
{
    return time(NULL) - last;
}

