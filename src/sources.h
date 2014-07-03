/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   sources.h

   Sources of events
   
   Last update 28.10.2000
  =========================================================================*/

#ifndef SOURCES_H
#define SOURCES_H

#include "constants.h"
#include "event.h"
#include "data.h"
#include "evqueue.h"

#include <unistd.h>
#include <sys/types.h>

const int MAX_PORT=32;

class Ax25io : public Object
{
  private:
    //Port parametres
    char *pname[MAX_PORT];       //port names
    char *bcall[MAX_PORT];       //base calls
    unsigned speed[MAX_PORT];    //port baudrates
    int paclen[MAX_PORT];        //port packet-lengths
    int win[MAX_PORT];           //port windows
    char *descript[MAX_PORT];    //port decriptions
    //Connection status
    int **list;                  //sockets for listen (one for each channel and port)
    char orig_call[MAX_CHN+1][10]; //original callsign before SSID change
    //int sock[MAX_CHN+1];         //sockets for connection - now global in data.h
    int num_ports;               //number of ports
    int maxchn;                  //number of channels
    char s[256];
    char line[MAX_CHN+1][256];   //for line recieving
    char sline[MAX_CHN+1][256];  //for line sending
    //Incomming data buffers
    char buffer[MAX_CHN+1][256];
    int in_cnt[MAX_CHN+1];
    bool flush[MAX_CHN+1];       //data will be flushed after next EV_TEXT_COOKED
    //Outgoing data buffers
    char obuffer[MAX_CHN+1][256];
    int out_cnt[MAX_CHN+1];
    //Wait cycle counters
    int wait_cnt[MAX_CHN+1];
    bool waiting_cnt[MAX_CHN+1];

    bool CheckRequests();         //Check connection requests for all channels
    bool CheckIncomming(int chn); //Check for incomming data
    bool CheckConnected(int chn); //Check if connection has been established
    void OutputBuffer(int chn); //Work up the output bufer (transform to events)

    int socket_state(int s);     //read status of AX.25 socket

    void FlushBuffer(int chn);   //flush output buffer
    void ChangeStatus(int chn, int status); //change connection status
    bool InputEnabled();        //is input enabled ?
    bool WaitingApp();          //are we waiting for some application ?

  public:
    Ax25io(int num_chns);        //prepare for specified num. of channels
    virtual ~Ax25io();
    virtual void handle_event(const Event &);

    int get_port_data();         //get port info from /etc/ax25/axports
    void set_channel_call(int chn, char *callsign); //set channel callsign
    void send_unproto(char *src, char *dest, char *msg);
};


class ExternCmd : public Object
{
  private:
    int chn;            //Command channel
    bool remc;          //Is it remote command ?
    bool crlf;          //convert CR to LF ?
    bool doslf;         //ignore LF ?
    bool console;       //output to console ?
    bool doubleio;      //redirect both output streams ?
    bool waitfor;       //wait until this application starts
    int pip_in[2];      //pipe to the subprocess
    int pip_out[2];     //pipe from the subprocess
    int pid;            //subprocess PID
    char buf[512];      //send buffer
    char result[256];   //prg. result
    int rhnd;           //result handle
    bool sync_on;       //synchronized mode

  public:
    ExternCmd(int ch, char *cmd, bool remote_cmd, int ret_hnd, char *flags);
    virtual ~ExternCmd();
    virtual void handle_event(const Event &);

    void set_sync(bool on) { sync_on = on; }
    void handle_event_sync(const Event &);

    void send_stream_data(const Event &);
};

class EventGate;

struct tapp //one application entry
{
  int pid;    //application pid
  int chn;    //channel where app is running
  bool remote;//remote app
  EventGate *gate; //gate prepared for this app
  time_t started; //when the application started?
};

class EventGate : public Object
{
  private:
    int sock;
    char data[MAX_EVENT_DATA];
    int pid;          //what PID is this gate for ? (0 = base gate)
    std::vector <tapp> *pids;
    std::vector <EventGate *> children; //sockets of child gates
    fd_set rfds;      //child descriptors
    int maxdesc;      //maximal descriptor value
    EventGate *parent_gate; //parent gate
    char *rbuff;      //read buffer
    int rbufsize;     //read buffer size
    char *wbuff;      //write buffer for storing unsuccesfully written data
    int wbufsize;     //write buffer size
    EventQueue queue; //queue of events
    bool write_error; //an error during write to the pipe (set by write_app)
    time_t created;   //time when the gate was created
    bool app_busy;    //application is working with the event (waiting for ack)
    bool app_died;    //connection lost
    bool end_round;   //event serie has been succesfully finished with 100 command

    //app data
    bool sig_on;      //application wants the signals
    bool sync_on;     //gate is synchronized with ExternCmd I/O
    bool remote;      //application was remotely started
    int chn;          //channel where application was started
    ExternCmd *sync_with; //sync with this ExternCmd when sync_on is true
    Event latest_event; //date event that waits for synchronization

    //private methods
    void add_wbuffer(const char *data, size_t count);
    void add_rbuffer(const char *data, size_t count);
    bool flush_wbuffer(int fd);
    bool send_event(const Event &);
    bool send_command(int cmd, int data);
    int write_app(int fd, const void *buf, size_t count);
    int read_app(int fd, void *buf, size_t count);
    void interpret_command(int cmd, int data);
    void accept_request();
    bool own_handle_event(const Event &);
    void serialize(const Event &ev, char **buffer, int *size);
    bool parse_event_cmd();

  public:
    EventGate(int socknum, EventGate *parent = NULL, std::vector <tapp> *ppids = NULL);
    virtual ~EventGate();
    virtual void handle_event(const Event &);

    void set_sock(int thesock);
    int get_sock() { return sock; }
    void set_pid(int thepid);
    int get_pid() { return pid; }
    void set_chn(int thechn) { chn = thechn; }
    void set_remote(bool isremote) { remote = isremote; }
    void set_sync_with(ExternCmd *cmdio) { sync_with = cmdio; }

    void data_arrived();
    void child_finished(EventGate *child);
    void change_child(EventGate *from, EventGate *to);
    void end_work(char *reason = NULL);
};

#endif

