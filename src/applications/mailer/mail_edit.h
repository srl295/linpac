/*
  LinPac Mailer
  (c) 1998 - 2000 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_edit.h - message composer classes
*/

#include "mail_screen.h"
#include "mail_input.h"
#include <ncurses.h>

extern char old_help[80]; //help text to be set back after editor finishes

//-------------------------------------------------------------------------
// Edline - one editor line
//-------------------------------------------------------------------------
class Edline
{
  public:
    char *s;
    int attr;
    bool editable;

    Edline();
    ~Edline();
    Edline &operator =(const Edline &);
    int lastpos();
    void clrline();
    void get_text(char *); //returns the text line w/o terminal spaces
    void accept(char *);   //fills the line with the string
    void draw(WINDOW *win, int y);
};

//-------------------------------------------------------------------------
// Editor - text editor
//-------------------------------------------------------------------------
class Editor : public screen_obj
{
  private:
    int limit;         //max number of lines
    WINDOW *win;       //ncurses window
    char sent[256];    //sent text
    char *conv;        //character conversion table
    int convcnt;       //conversion table size
    int x1, y1, x2, y2; //screen position
    bool err;          //error message is displayed

  public:
    Edline *line;      //editor lines
    int crx, cry;      //cusror position
    int poz;           //first visible line
    int max;           //number of active lines

    Editor(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, int maxlines);
    void update(int wx1, int wy1, int wx2, int wy2);
    void handle_event(Event *ev);
    void draw(bool all = false);
    void draw_lines();
    void redraw(bool update = true);
    void draw_line(int n);
    void cr_left();
    void cr_right();
    void cr_up(bool update = true);
    void cr_down(bool update = true);
    void cr_pgup();
    void cr_pgdn();
    void cr_home();
    void cr_end();
    void cr_tab();
    void backspace();
    void del_ch();
    void delline();
    void newline(bool update = true);
    void ins_info(char *ch);
    void ins_nonedit(char *ch);
    void newch(char ch);
    void active(bool b) {act = b;};
    void convert_charset(char *s);
    void load_texts();
    void errormsg(char *msg);
    void clear_error();
    virtual ~Editor();
};

//-------------------------------------------------------------------------
// Composer - message composer
//-------------------------------------------------------------------------
class Composer : public screen_obj
{
  private:
    Editor *ed;

    char *from;                    //source address
    char *to;                      //destination address
    char *subj;                    //message subject
    char mid[16];                  //message ID
    int type;                      //message type (P or B)
    int size;                      //message size
    int msgnum;                    //message number
    int ttabnum;                   //number of translation table
    char ttable[256];              //translation table
    int ttabsize;                  //table size
    int head_lines;                //number of lines of msg. header

    int crx, cry;                  //cursor position
    int x1, y1, x2, y2;            //window position

    bool wait_rname;               //waiting for filename to read
    bool wait_sname;               //waiting for filename to save
    bool wait_fname;               //waiting for filename to attach
    char ibuffer[64];              //input line buffer
    InputLine *iline;              //input line

    WINDOW *win;                   //composer window
    WINDOW *old_focus_window;
    screen_obj *old_focused;

  public:
    Composer(WINDOW *parent, int wx1, int wy1, int wx2, int wy2, char *toaddr, char *subject);
    void handle_event(Event *ev);
    void draw(bool all = false);
    void edredraw();
    void draw_header();
    void load_texts();              //load header and tail from files
    bool send_message();
    void load_table(int n);         //load translation table
    void check_dest();              //check & complete the destination
    void insert(char *s, bool quote); //insert the line from original message

    virtual ~Composer();
};

