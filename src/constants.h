/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 1999

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   constants.h

   Commonly used constants
   
   Last update 13.6.1999
  =========================================================================*/
#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "version.h"

//number of channels
#define MAX_CHN 8

//color constants
#define QSO_RX 1
#define QSO_TX 2
#define QSO_INFO 3
#define QSO_CTRL 4

#define ED_TEXT 5
#define ED_INFO 6
#define ED_CTRL 7

#define CI_NORM 8
#define CI_SLCT 9
#define CI_NCHN 10
#define CI_PRVT 11

#define IL_TEXT 12

//last pre-allocated color
#define COL_NUM 13

//Line length
#define LINE_LEN 80

//TAB size
#define TAB_SIZE 8

//axports file
#define AXPORTS "/etc/ax25/axports"

//ax25 status file in /proc
#define PROCFILE "/proc/net/ax25"

//file with encodings
#define ENCODINGS "./encodings"

//size of channel environment
#define ENV_SIZE 1024

//lp variable which contains the client name (channel 0)
#define VAR_CLIENT "CLIENT"
//lp variable which contains the version (channel 0)
#define VAR_VERSION "VERSION"

//max size of event data
#define MAX_EVENT_DATA 1024

//Maximal number of events waiting in the queue. When the queue grows
//to this length, the application is killed.
#define MAX_EVENT_QUEUE_LENGTH 1024

//Max. time between responses of the application. If the application
//leaves some event in the queue for this time, the application is killed.
//(time in seconds)
#define MAX_RESPONSE_TIME 300

//Max. time the event gate waits for application activation.
//After this time the gate is removed
#define MAX_GATE_WAIT 15

//Number of cycles after connect in which data won't be received
//(Wait cycles for starting macros)
//1cycle == AX25_SCAN_CYCLES
#define CONN_WAIT_CYCLES 1

//Telnet port where to connect ax25spyd
#define AX25SPYD_PORT 14091

//Connection status
const int ST_DISC=0; //disconnected
const int ST_DISP=1; //disconnecting
const int ST_TIME=2; //disconnecting for timeout
const int ST_CONN=3; //connected
const int ST_CONP=4; //connecting in progress

//select() wait time
const int SELECT_TIME=100;

//number of void cycles before rescaning status of connection
const int AX25_SCAN_CYCLES=10;

//size of window buffers
const long WIN_BUFFER_SIZE=16384;

//environment variable name
const char ENV_NAME[] = "LINPACDIR";

//environment variable that contains the home directory
const char ENV_HOME[] = "HOME";

//subdirectory name
const char HOMEDIR[] = "LinPac";

//API TCP port number
const int API_PORT = 0x4c50;

#endif

