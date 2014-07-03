/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 1999

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   editor.h

   Text/command editor objects

   Last update 20.11.1999
  =========================================================================*/
#ifndef EDIT_H
#define EDIT_H

#include "screen.h"

//-------------------------------------------------------------------------
// Edline - one editor line
//-------------------------------------------------------------------------
class Edline
{
  public:
    char *s;
    int attr;

    Edline();
    ~Edline();
    Edline &operator =(const Edline &);
    int lastpos();
    void clrline();
    void get_text(char *); //returns the text line w/o terminal spaces
    void accept(char *);   //accept new string
    void draw(WINDOW *win, int y);
};

//-------------------------------------------------------------------------
// Editor - text editor
//-------------------------------------------------------------------------
class Editor : public Screen_obj
{
  private:
    bool act;          //editor active
    Edline *line;      //editor lines
    int max;           //number of active lines
    int limit;         //max number of lines
    int poz;           //first visible line
    int crx, cry;      //cusror position in window
    int chnum;         //channel number
    WINDOW *win;       //ncurses window
    char sent[256];    //sent text
    char *conv;        //character conversion table
    int convcnt;       //conversion table size
    bool ctrlp;        //Ctrl-P was pressed

  public:
    Editor(int wx1, int wy1, int wx2, int wy2, int maxlines, int ch);
    void update(int wx1, int wy1, int wx2, int wy2);
    void handle_event(const Event &);
    void draw();
    void redraw(bool update = true);
    void draw_line(int n);
    void cr_left();
    void cr_right();
    void cr_up();
    void cr_down();
    void cr_home();
    void cr_end();
    void cr_tab();
    void backspace();
    void del_ch();
    void delline();
    void newline();
    void ins_info(char *ch);
    void newch(char ch);
    void active(bool b) {act = b;};
    void convert_charset(char *s);
    virtual ~Editor();
};

#endif

