/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   sounds.h

   Sound signal class

   Last update 31.10.1999
  =========================================================================*/
#ifndef SOUNDS_H
#define SOUNDS_H

#include "event.h"

//Class Sound - produce sounds when some events occur
class Sound : public Object
{
  private:
    int console;
    bool ready;

  public:
    Sound();
    virtual ~Sound();
    virtual void handle_event(const Event &);

  private:
    void sound(int frequency);   //start generating the sound
    void nosound();              //stop generating the sound
    void wait(int micros);       //wait for specifiend number of micro-seconds

    void connect_sound();
    void disconnect_sound();
    void failure_sound();
    void request_sound();
    void knax_sound();
};

#endif

