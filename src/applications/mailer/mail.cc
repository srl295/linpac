/*
  LinPac Mailer
  (c) 1998 - 2001 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail.cc - main file
*/

#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <axmail.h>
#include <ncurses.h>
#include "lpapp.h"
#include "mail_call.h"
#include "mail_data.h"
#include "mail_list.h"
#include "mail_route.h"
#include "mail_files.h"

//directory of translation tables
#define TABLE_DIR "."
//path to message lists
#define LIST_PATH "/var/ax25/ulistd"
//path to bulletins
#define BULLETIN_PATH "/var/ax25/mail"

int channel; //the channel where mailer is running
Main_scr main_scr; //main screen

//read LinPac environment to get BBS name and address
void get_bbs_names()
{
   char *p = lp_get_var(0, "HOME_BBS");
   char *q = lp_get_var(0, "HOME_ADDR");
   if (p != NULL && q != NULL)
   {
      home_bbs = strdup(p);
      home_addr = strdup(q);
      home_call = new char[strlen(home_addr)];
      first_call(home_addr, home_call);
   }
   main_scr.blocked = false;
}

//read the names of available conversion tables
void get_table_names()
{
   DIR *dir;
   struct dirent *dd;
 
   dir = opendir(TABLE_DIR);
   if (dir == NULL) {ntables = 0; return;}
   do
   {
      dd = readdir(dir);
      if (dd != NULL)
        if (strstr(dd->d_name, ".ctt") != NULL)
        {
           strncpy(tables[ntables], dd->d_name, 32);
           ntables++;
        }
   } while (dd != NULL && ntables < 16);
   closedir(dir);
}

//-------------------------------------------------------------------------
// User-defined Event handler
void my_handler(Event *ev)
{
   if (ev->type == EV_ABORT && ev->chn == lp_channel()) stop = true;
   else if (ev->type == EV_SELECT_CHN)
   {
      if (ev->chn == channel)
      {
         if (!act)
         {
            act = true;
            if (focused != NULL) focused->draw(true);
            redraw();
         }
      }
      else act = false;
   }
   else if (act && (ev->type == EV_KEY_PRESS || ev->type == EV_KEY_PRESS_MULTI))
   {
      main_scr.handle_event(ev);
      if (focused != NULL)
      {
         if (!main_scr.blocked) focused->handle_event(ev);
      }
   }
   else if (ev->type == EV_APP_MESSAGE && ev->chn == lp_channel())
   {
      //focused->handle_event(ev);
      for (int i = 0; i < 16; i++)
         if (message_handler[i] != NULL) message_handler[i]->handle_event(ev);
   }
}

//==========================================================================

int main(int argc, char **argv)
{
   if (lp_start_appl())
   {
      lp_event_handling_off();
  
      get_file_paths();
      get_bbs_names();
      if (!home_addr)
      {
         fprintf(stderr, "No home BBS specified\n");
         return 1;
      }
  
      mailpath = lp_get_var(0, "MAIL_PATH");
      if (mailpath == NULL)
      {
         fprintf(stderr, "No MAIL_PATH@0 specified\n");
         return 1;
      }
  
      get_table_names();
  
      //check if this channel is free
      if (lp_chn_status(lp_channel()) != ST_DISC)
      {
         fprintf(stderr, "This channel is not free. Mail needs free channel for connecting BBS\n");
         fprintf(stderr, "Please run mail on some not connected channel\n");
         return 1;
      }
  
      //disable connects to this channel
      lp_emit_event(lp_channel(), EV_DISABLE_CHN, 0, NULL);
  
      //switch to appl screen and draw
      channel = -lp_channel();
      if (channel == 0) channel = -999;
      lp_emit_event(channel, EV_SELECT_CHN, 0, NULL);
      init_main_screen();
  
      focused = NULL;
  
      //init axmail library
      axmail_init(LIST_PATH, BULLETIN_PATH, mailpath, mailpath);

      //load mail routes
      load_routes();

      main_scr.draw();
  
      //display the message list
      Messages msgs(home_call, lp_chn_call(lp_channel()));
      msgs.init_screen(main_window, maxy-3, maxx-1, 1, CENTER(maxx-1));
      message_handler[0] = focused; //message list listens to msg events
  
      lp_event_handling_off();
  
      //MAIN LOOP - repeat while some window is focused
      Event ev;
      while (focused != NULL)
      {
         ev.data = NULL;
         if (lp_get_event(&ev)) my_handler(&ev);
      }
  
      //enable connects to this channel
      lp_emit_event(lp_channel(), EV_ENABLE_CHN, 0, NULL);
  
      if (act)  //switch back to normal channel
      {
         lp_emit_event(lp_channel(), EV_SELECT_CHN, 0, NULL);
         lp_wait_event(lp_channel(), EV_SELECT_CHN);
      }
      lp_end_appl();
   } else fprintf(stderr, "Cannot start application\n");
   return 0;
}

