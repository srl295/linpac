/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   evqueue.h

   Queue of events
   
   Last update 23.2.2000
  =========================================================================*/

#ifndef EVQUEUE_H
#define EVQUEUE_H

#include <time.h>
#include "t_queue.h"
#include "event.h"

class EventQueue
{
  private:
    Queue <Event> event_queue;    //queue of events
    time_t last;                  //time of last pop
    int maxl;                     //maximal length

  public:
    EventQueue();
    
    void store(const Event &ev); //add new event to the queue
    void fstore(const Event &ev); //add new event to the front of queue
    Event &top();               //return first event in the queue
    void pop();                 //remove first event in the queue
    int length() { return event_queue.size(); } //return number of events
    bool empty() { return event_queue.isEmpty(); } //return true if empty
    int max_length() { return maxl; }
    time_t last_pop();          //return the time since last pop
};

#endif

