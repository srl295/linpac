/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_filter.h - select message destinations
*/
#include <vector>
#include <axmail.h>
#include "mail_screen.h"
#include "mail_input.h"

//One selected board name
class tbname
{
   public:
      char name[32];

      tbname &operator = (const tbname &src);
};

//Vector of selected boards
extern std::vector <tbname> bfilter;
extern std::vector <tbname>::iterator bfit;

//Message info
struct Msg
{
   int index;       //index in msglist
   bool select;
};

//A board name
class Board
{
  public:
    char name[32]; //name of board
    bool sel;      //is this board selected ?

    Board &operator = (const Board &src);
    friend bool operator == (const Board &, const Board &);
    friend bool operator < (const Board &, const Board &);
};

//A group of boards
class BGroup : public Board
{
  public:
    std::vector <tbname> content;

    bool contains(const char *name);
    BGroup &operator = (const BGroup &);
};

//List of boards
class BoardList : public screen_obj
{
  private:
    WINDOW *mwin;                //window
    unsigned x, y, xsize, ysize; //window position
    unsigned slct, gslct;        //selected bulletin, group
    unsigned pos, gpos;          //scroll position in bulletins, groups
    int col;                     //selected column

    bool wait_gname;             //waiting for group name from input line
    bool wait_gdel;              //waiting for delete confitmation
    char ibuffer[30];            //input line buffer
    InputLine *iline;            //input line

    WINDOW *old_focus_window;
    screen_obj *old_focused;

  public:
    std::vector <Board> boards;
    std::vector <BGroup> groups;
    
    BoardList(MessageIndex *ndx, std::vector <Msg> &, char *mycall);
    BoardList(std::vector <Board> &);
    void init_screen(WINDOW *pwin, int height, int width, int wy, int wx);

    void save_groups();
    void load_groups();
    void delete_group(int);

    virtual void handle_event(Event *);
    virtual void draw(bool all = false);

    void clear_filter();

    virtual ~BoardList() {};
};

//Create the sorted list of boards from message list
void create_list(MessageIndex *ndx, std::vector <Msg> &, std::vector <Board> &,
                 char *mycall);

