/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 1999

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   commands.h

   Objects for commands executing

   Last update 24.6.2002
  =========================================================================*/
#include <vector>
#include "event.h"
#include "sources.h"
#include "data.h"

//Read next parameter from a cmdline starting at specified position
//Returns false when end of string has been reached
bool read_param(char *cmdline, char *s, unsigned &pos, unsigned n = 256);

//A command database record
struct Command
{
  char name[64];     //application name
  char cmd[16];      //linpac command
  char flags[10];    //flags
};

class Macro : public Object
{
  private:
    int argc;
    char **argv;
    int chn;
    int pos;                //line position
    int max;                //number of lines
    std::vector <char *> prg;    //lines
    char s[256];
    Arguments args;         //for sending arguments of macro
    bool macro;             //translating from 'macro' notation
    bool waiting;           //waiting for EV_TEXT_COOKED
    int childs;             //nm of child macros
    time_t sleep_stp;       //end time of sleeping (0 = no sleeping)

  public:
    int exitcode;           //exit code: 0=OK, 1=Not found
    Macro(int chnum, char *fname, int argc, char **argv);
    void convert_line(char *s);    //convert line from macro notation
    virtual void handle_event(const Event &);
    int label_line(char *label);   //find the line signed with this label
    bool macro_command(char *cmd); //execute special macro command, true when known command
    void index_conds();            //index conditions
    int compare(const char *s1, const char *s2); //compare strings or numbers
    bool true_condition(char *s);  //return true, when condition is true
    void clear_prg(std::vector <char *> &); //clear prg structure
    virtual ~Macro();
};

class Commander : public Object
{
  private:
    char *cmdline;
    unsigned pos;
    char result[256];
    char *gps; //for emitting strings (general purpose string)
    char lang_name[32]; //actual language name
    bool remote;   //command from remote source
    bool secure;   //command from secure source - do not check permissions
    bool send_res; //result must be sent back
    bool remote_disabled[MAX_CHN+1]; //remote commands are disabled due to binary transfer
    std::vector <Command> reg; //registered commands handled by ext. application
    std::vector <Command> bin; //commands found in ./bin
    std::vector <Command> mac; //commands found in ./macro (default language)
    std::vector <Command> lmac; //macro commands for selected language
    ExternCmd *ext;
    Macro *mcm;
    int res_hnd; //result handle (when send_res == true)

  public:
    Commander();
    virtual ~Commander() {};
    virtual void handle_event(const Event &);
    bool load_language(const char *lang); //load a macro database for spec. language
    void whole(char *s); //return whole command line
    void whole_quot(char *s, bool fixpath = false); //return whole command line with quoted args
    bool nextp(char *s, int n = 256); //return next cmdline parameter
    bool is_next() {return (pos < strlen(cmdline));} //does next param.exist ?
    bool com_is(char *s1, char *s2); //compare commands
    bool com_ok(int chn, int echn, char *s1, char *s2); //compare commands and check security
    bool is_secure(int chn, int echn, char *cmd); //check command security
    void do_command(int chn, char *cmd); //execute command
    void exec_bin(int chn, char *name, char *flags); //execute binary extcmd
    bool exec_mac(int chn, char *name, char *flags, char *lang = NULL); //execute macro extcmd
    void command_channel(char *cmd, int *chn); //decode cmd. channel spec (@)

    void bool_set(bool *b); //set boolean flag ON/OFF
    void bool_set_config(const char *name);
    void int_set(int *i, int lowest = 0, int highest = 999); //set integer value
    void int_set_config(const char *name, int lowest = 0, int highest = 999);
    //terminal commands
    void do_connect(int chn);
    void echo(int chn);
    void version(int chn);
};

