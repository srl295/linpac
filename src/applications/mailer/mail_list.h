/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_list.h - message list viewer and text viewer
*/
#include <vector>
#include "mail_screen.h"
#include "mail_filt.h"
#include "mail_input.h"

class TheFile : public screen_obj
{
  private:
    Message *msg;           //the message

    WINDOW *mwin;           //window
    int x, y, xsize, ysize; //screen position
    //int slct;
    unsigned pos;           //first visible line
    unsigned xpos;
    bool head;              //header on/OFF
    bool wrap;              //wrap ON/off
    int attach;             //number of attachments (7plus)
    bool err;               //error message is displayed

    bool wait_reply;        //wait for answer about reply mode
    bool wait_sname;        //wait for filename to save
    bool wait_xnum;         //wait for number of attachment to save
    char ibuffer[64];       //input line buffer
    InputLine *iline;       //input line

    WINDOW *old_focus_window;
    screen_obj *old_focused;

  public:
    std::vector <char *> line;

    TheFile(Message *themsg);
    virtual ~TheFile();

    void load_message(bool head); //read the message from the file (display header)
    void extract_attach(int num); //extract n-th attachment
    void destroy_message();       //delete the message
    unsigned max_len();           //the length of longest line
    bool return_addr(char *result); //try to determine the return address

    void init_screen(WINDOW *pwin, int height, int width, int wy, int wx);
    virtual void handle_event(Event *);
    virtual void draw(bool all = false);

    void errormsg(const char *fmt, ...);
    void clear_error();
};


class Messages : public screen_obj
{
   private:
      char *bbs;         //BBS callsign
      char *mycall;      //user's callsign
      WINDOW *mwin;      //window handle
      int x, y, xsize, ysize; //window geometry
      int slct;          //current message index
      int pos;           //scroll position
      bool priv;         //show private on/off
      int state;         //download state
      int folder;        //displayed folder

      MessageIndex *ndx; //message index
      std::vector <Msg> msg;  //filtered message list
      std::vector <Board> boards; //list of boards

      //saved old screen status
      WINDOW *old_focus_window;
      screen_obj *old_focused;
 
      TheFile *viewer;    //text viewer
      BoardList *blister; //board list viewer
  
      void clear_msgs();
      void load_list(bool all);  //re-filter messages
  
   public:
      //NOTE: in function arguments 'index' means index of the message
      //      in global list 'ndx', 'num' means message numbers at BBS
      Messages(char *bbs_name, char *call, bool all = false);
      void init_screen(WINDOW *pwin, int height, int width, int wy, int wx);
      void reload(bool all = false);
      
      int count() {return msg.size();} //number of messages
      long min();                      //lowest message number
      long max();                      //highest message number
      long last_read();                //last downloaded message number
      bool present(int num);           //is this message present ?
      bool readable(int index);        //is this message readable ?
      bool check_filter(int index);    //is this message visible through filter ?
      int selected();                  //index of selected message
      void del_msg(std::vector <Msg>::iterator it); //delete message
      void draw_line(int i);           //draw a single screen line

      virtual void handle_event(Event *);
      virtual void draw(bool all = false);
  
      virtual ~Messages() {};
};


