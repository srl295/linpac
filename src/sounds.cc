/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   sounds.cc

   Sound signal class

   Last update 31.10.1999
  =========================================================================*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "data.h"
#include "sounds.h"

#include <ncurses.h>

#define CONSOLE "/dev/console"

Sound::Sound()
{
  strcpy(class_name, "Sound");
  console = open(CONSOLE, O_RDWR);
  ready = (console != -1);
}

Sound::~Sound()
{
  if (ready) close(console);
}

void Sound::handle_event(const Event &ev)
{
  if (bconfig("cbell"))
  {
    if (ev.type == EV_CONN_TO) connect_sound();
    if (ev.type == EV_DISC_FM || ev.type == EV_DISC_TIME) disconnect_sound();
    if (ev.type == EV_FAILURE) failure_sound();
    if (ev.type == EV_CONN_REQ) request_sound();
  }
  if (bconfig("knax") && ev.type == EV_DATA_INPUT) knax_sound();

  if (ev.type == EV_SYSREQ && ev.x == 10) connect_sound();
  if (ev.type == EV_SYSREQ && ev.x == 11) disconnect_sound();
  if (ev.type == EV_SYSREQ && ev.x == 12) failure_sound();
  if (ev.type == EV_SYSREQ && ev.x == 13) request_sound();
  if (ev.type == EV_SYSREQ && ev.x == 14) knax_sound();
}

//--------------------------------------------------------------------------

void Sound::sound(int frequency)
{
  if (ready)
  {
    unsigned long arg = 1193180/frequency;
    ioctl(console, KIOCSOUND, arg);
  }
}

void Sound::nosound()
{
  if (ready)
  {
    unsigned long arg = 0;
    ioctl(console, KIOCSOUND, arg);
  }
}

void Sound::wait(int micros)
{
  struct timeval tim, wtim;
  struct timezone tzon;
  
  gettimeofday(&tim, &tzon);
  wtim.tv_sec = tim.tv_sec;
  wtim.tv_usec = tim.tv_usec + micros;
  while (wtim.tv_usec > 1000000L)
  {
    wtim.tv_usec=wtim.tv_usec-1000000L;
    wtim.tv_sec++;
  }
  do 
    gettimeofday(&tim, &tzon); 
  while ((tim.tv_sec < wtim.tv_sec) ||
        ((tim.tv_sec == wtim.tv_sec) && (tim.tv_usec < wtim.tv_usec)));
}

//------------------------------------------------------------------------

void Sound::connect_sound()
{
  if (ready)
  {
    sound(600); wait(40000);
    sound(700); wait(40000);
    sound(800); wait(50000);
    nosound();
  }
  else beep();
}

void Sound::disconnect_sound()
{
  if (ready)
  {
    sound(800); wait(40000);
    sound(600); wait(40000);
    sound(400); wait(50000);
    nosound();
  }
  else beep();
}

void Sound::failure_sound()
{
  if (ready)
  {
    sound(800); wait(40000);
    nosound(); wait(20000);
    sound(400); wait(40000);
    nosound();
  }
  else beep();
}

void Sound::request_sound()
{
  if (ready)
  {
    sound(800); wait(40000);
    nosound(); wait(20000);
    sound(800); wait(40000);
    nosound();
  }
  else beep();
}

void Sound::knax_sound()
{
  if (ready)
  {
    sound(600); wait(1000);
    nosound();
  }
  else beep();
}

