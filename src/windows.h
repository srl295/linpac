/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   windows.h

   This module contains objects for terminal output windows

   Last update 29.10.2001
  =========================================================================*/
#ifndef WINDOWS_H
#define WINDOWS_H

#include "infoline.h"
#include <ncurses.h>
#include <sys/time.h>
#include "screen.h"
#include "constants.h"

#define TCH_SIZE 2  //The size of all data in TChar

//TChar - character with attribute
//Attributes: 1=in, 2=out, 3=info, 4=local, 255=nothing
class TChar
{
  public:
    char ch; //character
    int typ; //attribute
    TChar &operator = (const char &src);
};

//typedef TChar[LINE_LEN] TLine;

//TLine - a line of TChar characters
class TLine
{
  private:
    TChar *data;
    TChar void_ch; //this is returned by [], when index exceeds line
  public:
    int length;    //length of the line
    TLine();
    ~TLine();
    TChar &operator [] (int);
    TLine &operator = (const TLine &);
    char *getstring(char **); //converts line to c-string
    void clear_it(int attr); //clears the line
};

//A stack of lines in the window
class LineStack
{
  private:
    int f;                        //workfile
    char filename[256];           //work file name
    long fsize;                   //file size in records
    int step;                     //size of one record
    char *buffer;                 //cache buffer
    long poz;                     //logical file position (records)
    long bufsize;                 //size of cache buffer
    long bufstart;                //start position of buffer in file
    long bufend;                  //effective buffer size
    long lomark, himark;          //min and max file size
    long truncb;                  //number of lines truncated after last write
  public:
    LineStack(char *fname, size_t bsize);
    void init_buffer();           //initialize the buffer from the file
    void get(TLine &);            //get onle line
    void store(TLine &);          //store the line to the back
    void go(long);                //go to line number
    long size() {return fsize;};  //return number of lines
    void flush();                 //flush buffer
    void set_lo_mark(long size);
    void set_hi_mark(long size);
    void check_buffers();         //check buffer sizes
    long trunc() {return truncb;}
    ~LineStack();
};


class Window : public Screen_obj
{
  protected:
    bool act;             //the window is now active (selected)
    bool enabled;         //output enabled
    bool colorset;        //color id is enabled
    int wnum;             //window number (1..MAX_CHN)
    int csx, csy;         //window cursor position
    long poz;             //pozice fff
    short lpoz;           //last line position
    bool saved_last;      //last line was stored (completed)
    TLine last;           //last line (work-line)
    LineStack *fff;       //line manager
    WINDOW *win;          //ncurses window
    long selb, sele;      //selection begin & end
    bool scroll;          //window can scroll on keys
    bool sounds;          //beep on BEL enabled
    bool in_seq;          //reading ANSI-color sequence
    char a_seq[256];      //ANSI-sequence
    int fg, bg;           //Color foreground and background
    int atr;              //active ANSI attribute
    char *conv;           //charset conversion table
    int convcnt;          //size of conversion table
    short default_attr;   //default character's attribute

  public:
    void update(int wx1, int wy1, int wx2, int wy2); //update size
    void draw();                //draw window buffer
    void redraw(bool update = true);  //redraw window content (physical)
    void draw_line(long n);     //redraw specified line
    void outch(char ch, int typ);
    void outstr(char *s, int typ);
    void handle_ansi(char endchar);
    void line_b();              //scroll one line back
    void line_f();              //scroll one line forward
    void page_b();              //page back
    void page_f();              //page forward
    void go_bottom();           //go to last line
    void active(bool b) {act = b;};     //activate window
    void out_ok(bool b) {enabled=b;};  //enable/disable output
    void colors(bool b) {colorset=b;}; //enable color id of char type
    virtual ~Window() {};
};

class QSOWindow : public Window
{
  private:
    bool ctrlp; //ctrl-P was pressed
  public:
    QSOWindow(int wx1, int wy1, int wx2, int wy2, int num, char *fname);
    void handle_event(const Event &);
    virtual ~QSOWindow();
};

class MonWindow : public Window
{
  private:
    int list;             //input descr. from listen
    int listpid;          //PID of listen (0 = running spy, -1 = not ready)
    char mon_line[256];
    int ay;
    bool alt_pos;
    bool ctrlp;           //ctrl-P was pressed
    bool quiet;           //quiet mode - don't display anything
    int format;           //input format: 0 = detect
                          //              1 = 'Port ' + '->' (ax25-utils-2.1.42)
                          //              2 = ': fm' + ' to ' + \r (ax25-apps)
                          //              3 = 'AX25:' + '->' (ax25spyd)
    bool header;          //last line was a header
    bool bin;             //binary data detected
    bool dontshow;        //don't show actual frame
  public:
    MonWindow(int wx1, int wy1, int wx2, int wy2, int alt_y, char *fname);
    void handle_event(const Event &);
    void next_mon_line();
    int detect_format(char *line); //detect monitor format from 1st line
    bool try_spyd();            //try to run monitor from ax25spyd
    bool try_listen();          //try to run monitor from listen
    bool binary(char *data);    //check if it's binary data
    bool filter(char *line);    //filter frame starting with this line?
    virtual ~MonWindow();
};

class ChannelInfo : public Screen_obj
{
  private:
    bool act;
    int achn; //active channel
    WINDOW *win;
    int ay;  //alternate Y-pos
    bool alt_pos;
    int visible; //number of visible channels

    bool blink[MAX_CHN+1];
    bool priv[MAX_CHN+1];

  public:
    ChannelInfo(int wx1, int wx2, int wy, int alt_y);
    void handle_event(const Event &);
    void draw();
    void redraw(bool update = true);
};

class InfoLine : public Screen_obj
{
  private:
    bool act;
    int achn; //active channel
    WINDOW *win;
    int ay;
    bool alt_pos;
    int procform; //format of /proc/net/ax25: 0 = 2.0.x, 1 = 2.2.x
    bool enable_ioctl; //use ioctl for infolevel 1
    bool force_ioctl;  //use ioctl for infolevels 1 and 2
    time_t last_time;

  public:
    InfoLine(int wx1, int wx2, int wy, int alt_y);
    void handle_event(const Event &);
    void draw();
    void redraw(bool update = true);

    int check_proc_format();
};

class StatLines : public Screen_obj
{
  private:
    WINDOW *win;
    vector <info_line> lines;
    int nlines;
    int mychn;
    bool act;

  public:
    StatLines(int chn, int wx1, int wx2, int wy);
    virtual ~StatLines() {};
    void update(int wx1, int wy1, int wx2, int wy2);
    void handle_event(const Event &);
    void draw();
    void redraw(bool update = true);
};

#endif

