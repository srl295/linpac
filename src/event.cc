/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   event.cc

   Routines for event handling
   
   Last update 29.9.2001
  =========================================================================*/
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include "t_queue.h"
#include "constants.h"
#include "data.h"
#include "event.h"
#include "tools.h"
#include "version.h"

//Uncomment this to get complete event list in stderr
//#define EVENT_DEBUG

//Uncomment this to get event list with destinations
//#define EVENT_ADDRESS_DEBUG

//Uncomment this to get data allocation info (only when OWN_DATA)
//#define EVENT_ALLOC_DEBUG

//Uncomment this to debug the event queue status
//#define EVENT_QUEUE_DEBUG

#ifdef MEMCHECK
unsigned long hist_step = 0;
long hist_val = 0;
#endif

bool max_priority = false; //max priority mode
Queue <Event> event_queue; //the queue of events
Obj_man *public_mgr;       //public instance of Obj_man

char EMPTY[] = "";         //empty string for replacing NULL in string data

#ifdef MEMCHECK
long get_vm_data()
{
  FILE *f;
  long retval = 0;
  char fname[256], s[256];
  sprintf(fname, "/proc/%i/status", getpid());
  f = fopen(fname, "r");
  if (f == NULL) return 0;
  while (!feof(f))
  {
    char name[256], mm[256];
    int val;
    fgets(s, 255, f);
    int n = sscanf(s, "%s %i %s", name, &val, mm);
    if (n == 3 && strcmp(name, "VmData:") == 0)
    {
      retval = val;
      break;
    }
  }
  fclose(f);
  return retval;
}
#endif

#ifdef EVENT_QUEUE_DEBUG
char *queue_stat()
{
  static char s[1024];
  strcpy(s, "");
  for (int i = 0; i < event_queue.size(); i++)
  {
    char num[10];
    sprintf(num, "%i ", event_queue.at(i).type);
    strcat(s, num);
  }
  return s;
}
#endif

//---------------------------------------------------------------------------
// Class Obj_man
//---------------------------------------------------------------------------
Obj_man::Obj_man()
{
  next_oid = 0;
}

void Obj_man::find_oid()
{
  vector <Object *>::iterator it;
  unsigned old_value = next_oid;
  bool ok;
  do
  {
    //generate next OID
    next_oid++;
    //test if the OID doesn't exist
    ok = true;
    for (it=children.begin(); it < children.end(); it++)
      if ((*it)->oid == next_oid) {ok = false; break;}
  } while (!ok && next_oid != old_value);
  if (next_oid == old_value) Error(0, "Obj_man: couldn't find free OID");
}

void Obj_man::insert(Object *obj)
{
  obj->oid = next_oid; find_oid();
  children.push_back(obj);
  active.push_back(obj);
}

void Obj_man::remove(Object *obj)
{
  vector <Object *>::iterator it;
  //Remove from object list
  for (it=children.begin(); it < children.end(); it++) if (*it==obj) break;
  if (it != children.end())
  {
    delete *it;
    children.erase(it);
  }
  else Error(0, "Problems with object removal");
  //Remove from active objects
  for (it=active.begin(); it < active.end(); it++) if (*it==obj) break;
  if (it != active.end())
  {
    active.erase(it);
  }
}

void Obj_man::remove(unsigned aoid)
{
  vector <Object *>::iterator it;
  //Remove from object list
  for (it=children.begin(); it < children.end(); it++)
    if ((*it)->oid == aoid) break;
  if (it != children.end())
  {
    delete *it;
    children.erase(it);
  }
  else Error(0, "Problems with object removal");
  //Remove from active objects
  for (it=active.begin(); it < active.end(); it++)
    if ((*it)->oid == aoid) break;
  if (it != active.end())
  {
    active.erase(it);
  }
}

//send all events created in last round
void Obj_man::workout()
{
  while (!event_queue.isEmpty())
  {
    #ifdef EVENT_QUEUE_DEBUG
    fprintf(stderr, "getting first: %s\n", queue_stat());
    #endif
    Event re = event_queue.first();
    event_queue.removeFirst();
    send_all(re);
    #ifdef EVENT_QUEUE_DEBUG
    fprintf(stderr, "got %i, status %s\n", re.type, queue_stat());
    #endif
    
    #ifdef OWN_DATA
    #ifdef EVENT_ALLOC_DEBUG
    fprintf(stderr, "free :%i %lx ", re.type, (unsigned long)re.data);
    fflush(stderr);
    #endif

    if (re.type >= 100 && re.type < 300 && re.data != NULL && re.data != EMPTY)
      free(re.data);

    #ifdef EVENT_ALLOC_DEBUG
    fprintf(stderr, "OK\n");
    fflush(stderr);
    #endif
    #endif
  }
}

void Obj_man::send_all(const Event &ev)
{
  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "sendall - start\n");
  #endif
  #ifdef LINPAC_DEBUG
  if (ev.type == EV_KEY_PRESS && ev.x == 'q' && ev.y == FLAG_MOD_ALT) list_objects();
  #endif
  //handle system requests
  if (ev.type == EV_SYSREQ)
  {
    char msg[128];
    switch (ev.x)
    {
      case 0: //check if debugging is compiled
              #ifdef LINPAC_DEBUG
              strcpy(msg, "Debugging supported.\n");
              #else
              strcpy(msg, "Debugging not supported.\n");
              #endif
              emit(ev.chn, EV_TEXT_COOKED, 0, msg);
              break;
#ifdef LINPAC_DEBUG
      case 1: list_objects(); //list active objects
              strcpy(msg, "Done1\n");
              emit(ev.chn, EV_TEXT_COOKED, 0, msg);
              break;
      case 2: DEBUG.macro = !DEBUG.macro; //switch macro debugging
              strcpy(msg, DEBUG.macro ? "Macro debug ON\n":"Macro debug OFF\n");
              emit(ev.chn, EV_TEXT_COOKED, 0, msg);
              break;
#endif
      case 3: break; //disable ioctls for get_axstat (on/off) - checked in InfoLine
      case 4: break; //force ioctls for get_axstat (on/off) - checked in InfoLine
      case 5: break; //toggle proc format - checked in InfoLine
#ifdef LINPAC_DEBUG
      /*case 88: emit(0, EV_DISABLE_OBJ, 0, "Editor2"); break;
      case 89: emit(0, EV_ENABLE_OBJ, 0, "Editor2"); break;*/
      case 99: {char *p = NULL; *p = 0;} break; //simulate SEGV
#endif
    }
    emit(ev.chn, EV_TEXT_FLUSH, 0, 0);
  }

#ifdef EVENT_DEBUG
  fprintf(stderr, "Event: type=%i, chn=%i, x=%i", ev.type, ev.chn, ev.x);
  if (ev.type >= 100 && ev.type < 200) fprintf(stderr, ", data=`%s'\n", (char *)ev.data);
                                  else fprintf(stderr, "\n");
#endif
  int i;
  for (i = 0; i < (int)active.size(); i++)
  {
#ifdef EVENT_ADDRESS_DEBUG
    fprintf(stderr, "Ev %i to %s\n", ev.type, (active[i])->class_name);
    fflush(stderr);
#endif
#ifdef MEMCHECK
    hist_val = get_vm_data();
#endif

    (active[i])->handle_event(ev);

#ifdef MEMCHECK
    if (i < MEMCHECK)
    {
      long amount = get_vm_data() - hist_val;
      if (amount != 0) fprintf(stderr, "Memory leak: step %li oid=%i amount=%li\n",
           hist_step-1, i, amount);
    }
#endif

    if (max_priority) workout(); //handle the events immediately
  }

  //interpret events significant for this module
  if (ev.type == EV_QUIT) quit = true;
  if (ev.type == EV_INSERT_OBJ) insert((Object *)ev.data);
  if (ev.type == EV_REMOVE_OBJ) remove((Object *)ev.data);
  if (ev.type == EV_DISABLE_OBJ) deactivate((char *)ev.data);
  if (ev.type == EV_ENABLE_OBJ) activate((char *)ev.data);
  if (ev.type == EV_USER_ACT) clear_last_act();

  if (!max_priority) workout();
  #ifdef EVENT_DEBUG
  if (max_priority) fprintf(stderr, "Priority NORMAL\n");
  #endif
  max_priority = false;

  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "sendall - stop\n");
  #endif
}

void Obj_man::do_step()
{
  Event noev;
  clrevent(noev);
  send_all(noev);
}

void Obj_man::stop()
{
  vector <Object *>::iterator it;
  for (it=children.begin(); it < children.end(); it++)
     delete *it;
  children.erase(children.begin(), children.end());
  active.erase(active.begin(), active.end());
}

void Obj_man::list_objects()
{
  #ifdef CLIENT_ONLY
  fprintf(stderr, "Obj_man - list of objects (terminal)\n");
  #else
  #ifdef DAEMON_ONLY
  fprintf(stderr, "Obj_man - list of objects (server)\n");
  #else
  fprintf(stderr, "Obj_man - list of objects\n");
  #endif
  #endif
  fprintf(stderr, " oid   class_name\n");
  vector <Object *>::iterator it;
  for (it=children.begin(); it < children.end(); it++)
  {
    fprintf(stderr, "%5i  %s\n", (*it)->oid, (*it)->class_name);
  }
  fprintf(stderr, "---");
}

void Obj_man::deactivate(char *obj_name)
{
   vector <Object *>::iterator it;
   //Remove from active objects
   for (it=active.begin(); it < active.end(); it++)
      if (strcmp((*it)->class_name, obj_name) == 0) break;
   if (it != active.end())
   {
      delete *it;
      active.erase(it);
   }
}

void Obj_man::activate(char *obj_name)
{
   vector <Object *>::iterator objs, act;
   //Find the right position for an object in active objects
   act = active.begin();
   for (objs = children.begin();
        objs < children.end() && strcmp((*objs)->class_name, obj_name) != 0;
        objs++)
      if (*act == *objs) act++;
   //Insert the object if found
   if (strcmp((*objs)->class_name, obj_name) == 0)
   {
      active.insert(act, *objs);
   }
}

//-------------------------------------------------------------------------

void emit(const Event &ev)
{
  Event re;
  re.chn = ev.chn;
  re.type = ev.type;
  re.x = ev.x;
  re.y = ev.y;
  re.ch = ev.ch;
 #ifdef OWN_DATA
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
  #ifdef EVENT_ALLOC_DEBUG
  fprintf(stderr, "emit: %i %lx\n", re.type, re.data);
  #endif
 #else
  re.data = ev.data;
 #endif  
  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "Inserting last %i, state %s\n", re.type, queue_stat());
  #endif
  event_queue.insertLast(re);
  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "Inserted last, state %s\n", queue_stat());
  #endif
}

void emit(int chn, int type, int x, void *data)
{
  emit(chn, type, x, 0, data);
}

void emit(int chn, int type, int x, int y, void *data)
{
  Event ev;
  ev.chn = chn;
  ev.type = type;
  ev.x = x;
  ev.y = y;
 #ifdef OWN_DATA
  if (ev.type < 100) ev.data = NULL;   //no data
  if (ev.type >= 100 && ev.type < 200) //string allocation
  {
    if (data == NULL) ev.data = EMPTY;
    else
    {
        ev.data = malloc(strlen((char *)data)+1);
        strcpy((char *)ev.data, (char *)data);
    }
  }
  if (ev.type >= 200 && ev.type < 300) //data buffer allocation
  {
    ev.data = malloc(ev.x);
    memcpy(ev.data, data, ev.x);
  }
  if (ev.type >= 300 && ev.type < 400) ev.data = data; //pointer data
  #ifdef EVENT_ALLOC_DEBUG
  fprintf(stderr, "emit: %i %lx\n", ev.type, ev.data);
  #endif
 #else
  ev.data = data;
 #endif
  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "Inserting last %i, state %s\n", ev.type, queue_stat());
  #endif
  event_queue.insertLast(ev);
  #ifdef EVENT_QUEUE_DEBUG
  fprintf(stderr, "Inserted last, state %s\n", queue_stat());
  #endif
}

//--------------------------------------------------------------------------
void clrevent(Event &ev)
{
  ev.chn=0;
  ev.type=EV_NONE;
}

/* Clear event structure and free memory */
void discard_event(Event &ev)
{
   if (ev.data != NULL && ev.type >= 100 && ev.type < 300)
   {
      delete[] (char *)ev.data;
      clrevent(ev);
   }
}

/* Copy one event structure to another */
void copy_event(Event &dest, const Event &src)
{
   dest.type = src.type;
   dest.chn = src.chn;
   dest.x = src.x;
   dest.y = src.y;
   if (dest.type < 100) dest.data = NULL;   //no data
   if (dest.type >= 100 && dest.type < 200) //string allocation
   {
      dest.data = new char[strlen((char *)src.data)+1];
      strcpy((char *)dest.data, (char *)src.data);
   }
   if (dest.type >= 200 && dest.type < 300) //data buffer allocation
   {
      dest.data = new char[src.x];
      memcpy(dest.data, src.data, src.x);
   }
   if (dest.type >= 300 && dest.type < 400) dest.data = src.data; //pointer data
}


//--------------------------------------------------------------------------
void Message(int no, const char *fmt, ...)
{
  va_list argptr;
  char info[256];
  char msg[256];

  va_start(argptr, fmt);
  vsprintf(info, fmt, argptr);
  if (no != 0) sprintf(msg, "%s : %s", info, strerror(no));
  else strcopy(msg, info, 256);
  emit(0, EV_CMD_RESULT, FLAG_NO_HANDLE, msg);
  va_end(argptr);
}


//--------------------------------------------------------------------------
void priority_max()
{
   #ifdef EVENT_DEBUG
   fprintf(stderr, "Priority MAX\n");
   #endif
   max_priority = true;
}

//--------------------------------------------------------------------------
void set_public_mgr(Obj_man *mgr)
{
  public_mgr = mgr;
}

