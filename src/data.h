/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2000

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   data.h

   Common data

   Last update 29.10.2001
  =========================================================================*/
#ifndef DATA_H
#define DATA_H

#include <stdio.h> // for NULL
#include <vector>
#include "constants.h"
#include "version.h"
#include "event.h"
#include "hash.h"

/*--------------------------------------------------------------------------
   Configuration data variables
   (in channel 0, name preceeded by _)

  bool remote;                       //Remote active

  bool cbell;                        //connection bell
  bool knax;                         //incomming frame bell

  char def_port[32];                 //Default port
  char unportname[32];               //Unproto port name
  int unport;                        //Unproto port

  int info_level;                    //Statusline: 0=none 1=short 2=full
  char no_name[32];                  //Default name of stn
  char timezone[8];                  //Local timezone
  int qso_start_line, qso_end_line,  //Screen divisions
      mon_start_line, mon_end_line,
      edit_start_line, edit_end_line,
      stat_line, chn_line;
  int max_x;                         //screen length
  bool swap_edit;                    //swap editor with qso-window
  bool fixpath;                      //use fixed paths only
  bool daemon;                       //linpac works as daemon
  bool monitor;                      //monitor on/off
  bool no_monitor;                   //monitor not installed
  bool listen;                       //listening to connection requests
  bool disable_spyd;                 //disable ax25spyd usage
  bool mon_bin;                      //monitor shows binary data
  char monparms[10];                 //parametres to 'listen' program
  char monport[32];                  //which port to monitor (* means all)
  int maxchn;                        //number of channels
  int envsize;                       //environment size
  time_t last_act;                   //last activity (seconds);

  --------------------------------------------------------------------------
   Other shared data
   (each channel, starting with _)

  char call[10];       //callsign for each channel
  char cwit[10];       //connected with callsign
  char cphy[10];       //physical connection to
  int port;            //connected on which port
  int state;           //connection status
*/

//------------------------------------------------------------------------
// Hash tables for environment variables
//------------------------------------------------------------------------

class Hash
{
   private:
      struct hash *hash;
      int chn;

      virtual void setValue(const char *name, const char *value);
      virtual char *getValue(const char *name);

   public:
      Hash();
      virtual ~Hash();

      void setChannel(int channel) { chn = channel; }
      
      void setSValue(const char *name, const char *value);
      void setIValue(const char *name, int value);
      void setBValue(const char *name, bool value);

      char *getSValue(const char *name);
      int getIValue(const char *name);
      bool getBValue(const char *name);

      void remove(const char *name);
      void removeVars(const char *names);

      struct h_name_list *getNames();
};

//variable manipulating functions
void init_env();                  //initialize
void set_var(int chn, const char *pname, const char *contents); //set value
void del_var(int chn, const char *name); //delete variable
void clear_var_names(int chn, const char *names); //delete all vars starting with <names>
char *get_var(int chn, const char *name);        //get value
void close_env();

//configuration variables
char *sconfig(int chn, const char *name);
char *sconfig(const char *name);
int iconfig(int chn, const char *name);
int iconfig(const char *name);
bool bconfig(int chn, const char *name);
bool bconfig(const char *name);

void setSConfig(int chn, const char *name, const char *value);
void setSConfig(const char *name, const char *value);
void setIConfig(int chn, const char *name, int value);
void setIConfig(const char *name, int value);
void setBConfig(int chn, const char *name, bool value);
void setBConfig(const char *name, bool value);

//variable name scanning
void scanVarNames(int chn);
char *nextVarName();
void releaseVarNames();

//------------------------------------------------------------------------
// Access lists
//------------------------------------------------------------------------

//A remote-command name (used in access lists)
class RCommand
{
  public:
    char name[16];
    RCommand();
    RCommand(char *);
    RCommand &operator = (const RCommand &);
    RCommand &operator = (char *);
    operator char*();
};

extern std::vector <RCommand> aclist[MAX_CHN+1];

//------------------------------------------------------------------------
// Miscelancelous data
//------------------------------------------------------------------------
extern char term[MAX_CHN+1][10];    //terminal types
extern bool ch_disabled[MAX_CHN+1]; //connects to channel enabled/disabled
extern bool huffman_comp[MAX_CHN+1];//huffman compression on/off
extern int sock[MAX_CHN+1];         //connection sockets
void clear_last_act();

//------------------------------------------------------------------------
// Debugging data
//------------------------------------------------------------------------
#ifdef LINPAC_DEBUG
extern struct Debug
{
  bool macro;  //macro debugging (commands.cc)
} DEBUG;
#endif

//------------------------------------------------------------------------
// Connection info
//------------------------------------------------------------------------
extern struct Ax25Status
{
  char devname[8];
  int state;
  int vs, vr, va;
  int t1, t2, t3, t1max, t2max, t3max;
  int idle, idlemax;
  int n2, n2max;
  int rtt;
  int window;
  int paclen;
  bool dama;
  int sendq, recvq;
} chstat;

//Read informations about specified connection
//form: 0 = 2.0.x format, 1 = 2.2.x format, 2 = use ioctl
bool get_axstat(const char *src, const char *dest, int form);

//------------------------------------------------------------------------
// Output cooking
//------------------------------------------------------------------------
//arguments structure (arguments of macro)
class Arguments
{
  private:
     char *data;
     int len;
     int cnt;

  public:
     Arguments();
     ~Arguments();

     char *toBuffer() { return data; }
     int length() { return len; }
     int argc() { return cnt; }
     char *argv(int index);

     void fromArray(int count, char **args);
     void fromBuffer(int length, char *buffer);
     void clearit();
};

//One output cooking task
struct cook_task
{
  char str[256];
  char expr[256];
  int handle;
  int x; //RAW x flags
  Arguments *args;
};

//Object for output cooking
class Cooker : public Object
{
  private:
    std::vector <cook_task *> tasks;
    int newhandle;
    char expr[256];
    char lastline[MAX_CHN+1][256]; //last line received (for %< macro)
    Arguments args; //current arguments
  public:
    Cooker();
    void handle_event(const Event &);
    bool replace_macros(int chn, char *s, Arguments *args = NULL, char *res = NULL);  //replace "%" macros
    virtual ~Cooker();
};

#endif

