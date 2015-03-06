/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   event.h

   Routines for event handling
   
   Last update 29.9.2001
  =========================================================================*/
#ifndef EVENT_H
#define EVENT_H

#define OWN_DATA

#include <vector>
#include "tevent.h"

//-------------------------------------------------------------------------
// Object - basic object class
//-------------------------------------------------------------------------
class Object
{
  public:
    unsigned oid;
    char class_name[16];
    virtual void handle_event(const Event &) = 0;
    virtual ~Object(){}
};

//-------------------------------------------------------------------------
// Obj_man - All objects & events manager
//-------------------------------------------------------------------------
class Obj_man
{
  private:
    unsigned next_oid;           //next free object id
    std::vector <Object *> children;  //list of inserted objects
    std::vector <Object *> active;    //active objects (receive events)

  public:
    bool quit;

    Obj_man();
    void find_oid();              //find next free oid
    void insert(Object *);        //insert next object
    void remove(Object *);        //remove object
    void remove(unsigned int);    //remove object with this oid
    void workout();               //send all events from the queue
    void send_all(const Event &); //send event to all objects
    void do_step();              //do one loop (0=end_prg)
    void stop();                 //finish working
    void list_objects();         //list all objects to stderr
    void deactivate(char *obj_name);  //prevent object from receiving events
    void activate(char *obj_name);    //start sending events to the object (default)
};

//------------------------------------------------------------------------
// Functions for event generating
//------------------------------------------------------------------------
void clrevent(Event &); //clears event
void discard_event(Event &ev); //disposes the event data
void copy_event(Event &dest, const Event &src); //copy the events

void emit(const Event &); //add event to event queue
void emit(int chn, int type, int x, void *data);
void emit(int chn, int type, int x, int y, void *data);
void Message(int no, const char *fmt, ...); //emit EV_CMD_RESULT with msg.
void priority_max(); //set max priority mode for following set of events
                     //In max priority mode events are read from the queue
                     //after each call of handle_event() of each object.
                     //In normal mode the events are read after calling
                     //all the handle_event() methods.
                     
//------------------------------------------------------------------------
// Public Obj_man instance
//------------------------------------------------------------------------
extern Obj_man *public_mgr;
void set_public_mgr(Obj_man *); //set this instance as public instance

#endif

