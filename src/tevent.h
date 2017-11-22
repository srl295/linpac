/*
   LinPac: the Event structure and type constants
*/
#ifndef TEVENT_H
#define TEVENT_H

// Event types 0..99 are reserved for events with no data
// Event types 100..199 are reserved for events with c-string data
// Event types 200..299 are reserved for events with data and length in x
// Event types 300..399 are reserved for events with data pointer only

//Possible types of events
#define EV_NONE 0               //no event
//User interaction
#define EV_KEY_PRESS 1          //key pressed (x=keycode), y=FLAG_MOD_ALT if 'Alt' pressed
#define EV_KEY_PRESS_MULTI 103  //accumulated key presses (printable only)
#define EV_BUTTON_PRESS 2       //mouse button pressed (x,y = position)
#define EV_BUTTON_RELEASE 3     //mouse button released (x,y = position)
#define EV_TEXT_ARGS 204        //cooking arguments for TEXT_RAW, must precede TEXT_RAW
#define EV_TEXT_RAW 104         //a text line from editor (data = c-string, x = flag)
#define EV_TEXT_COOKED 105      //a text with replaced macros
#define EV_TEXT_FLUSH 5         //send all text lines
#define EV_EDIT_INFO 106        //info to be inserted in editor
#define EV_CALL_CHANGE 107      //callsign changed
#define EV_UNPROTO_SRC 108      //set unproto frames source
#define EV_UNPROTO_DEST 109     //set unproto frames destination
#define EV_WANT_RESULT 110      //do this command and send me result (x = handle)
#define EV_CMD_RESULT 111       //command result (x = handle, -1 = no handle)
#define EV_DO_COMMAND 112       //do this command
#define EV_SELECT_CHN 13        //select channel (chn)
#define EV_UNPROTO_PORT 114     //set unproto frames port
#define EV_USER_ACT 15          //user has done something - he's not sleeping
//Macros
#define EV_MACRO_STARTED 318    //macro has been started on channel - block other macros
#define EV_MACRO_FINISHED 319   //macro has been finished on channel
//Character input/output
#define EV_DATA_INPUT 220       //data, x=length
#define EV_DATA_OUTPUT 221 
#define EV_LOCAL_MSG 222        //data = message, x=length
#define EV_LINE_RECV 123        //whole line received
#define EV_RX_CTL 24            //x=0 disable RX on all channels, x=1 enable RX
//Plug-in communication
#define EV_APP_STARTED 330      //application has been started (x=pid, data=pointer to ExternCmd)
#define EV_CREATE_GATE 31       //create new gate for x=pid
#define EV_REMOVE_GATE 32       //remove gate (x=pid)
#define EV_GATE_FINISHED 33     //let's change to new gateway (x=pid, chn=your channel)
#define EV_APP_RESULT 134       //application result (x=pid, data=result)
#define EV_PROCESS_FINISHED 35  //sub-process has finished
#define EV_APP_MESSAGE 136      //a message from application to other app. (data = text)
#define EV_GATE_TIMEOUT 37      //no application connected to this gate - removing it (x=pid)
#define EV_APP_STREMOTE 38      //application was started remotely
#define EV_APP_COMMAND 139      //command to be handled by an application
//Connection management
#define EV_CONN_LOC 140          //internal connect request (data="port:path")
#define EV_DISC_LOC 41           //internal disconnect request
#define EV_CONN_TO 142           //connected to (data), x=0 if we have initiated connection
#define EV_DISC_FM 143           //disconnected from
#define EV_DISC_TIME 144         //disconnected for timeout
#define EV_FAILURE 145           //link failure with
#define EV_CONN_REQ 146          //unsuccesful connect request
#define EV_STATUS_CHANGE 47      //status of connection changed
#define EV_RECONN_TO 148         //reconnected to (logical connection) x=1: connected forward
#define EV_ENABLE_CHN 42         //enable connects to this channel
#define EV_DISABLE_CHN 43        //disable connects to this channel
#define EV_LISTEN_ON 44          //enable all connectts
#define EV_LISTEN_OFF 45         //disable all connects
//Object manager commands
#define EV_INSERT_OBJ 350        //insert new object (data = ptr)
#define EV_REMOVE_OBJ 351
//Communication between objects
#define EV_DISABLE_OBJ 153       //prevent the objec from receiving events (data=obj_name)
#define EV_ENABLE_OBJ 154        //re-activate object (data=obj_name)
#define EV_DISABLE_SCREEN 55     //disable output to screen (due to binary transfer)
#define EV_ENABLE_SCREEN 56      //enable output to screen
#define EV_ADD_WATCH 257         //add autorun entry
#define EV_REG_COMMAND 158       //register command executed by application
#define EV_UNREG_COMMAND 159     //cancel the registration
//Screen management
#define EV_STAT_LINE 60          //change status line position (x)
#define EV_CHN_LINE 61           //change channel line position (x)
#define EV_QSO_END 62            //change qso-window endline (x)
#define EV_CHANGE_STLINE 163     //change contents of status line with id=x, or create new
#define EV_REMOVE_STLINE 64      //remove status line with id=x
#define EV_SWAP_EDIT 65          //swap editor with qso-window
#define EV_CONV_IN 265           //set input conversion table (chn=0 - all channels)
#define EV_CONV_OUT 266          //set output conversion table (chn=0 - all channels)
#define EV_CONV_NAME 166         //set the name of conversion table (chn=0 - all channels)
#define EV_SET_TERM 167          //set terminal type
#define EV_REDRAW_SCREEN 68      //redraw the whole screen
#define EV_CHANGE_COLOR 169      //change the color (data = "<name> <fg_color> <bg_color>"
#define EV_LOW_LIMIT 70          //set low buffer limit
#define EV_HIGH_LIMIT 71         //set high buffer limit
#define EV_MONITOR_CTL 72        //control monitor: x: 0=off, 1=on, 2=binary off, 3=binary on
//Status info
#define EV_STAT_REQ 80           //request for connection status info
#define EV_STATUS 281            //connection status info (data ~ Ax25Status)
//Shared memory synchronization
#define EV_VAR_CHANGED 285       //variable value changed (data = name\0value\0)
#define EV_VAR_DESTROYED 186     //variable destroyed (data = name)
#define EV_SYNC_REQUEST 87       //variable sync. request
#define EV_VAR_SYNC 88           //start of variables resync (x=number of channels)
#define EV_SYNC_DONE 89          //variable resync done
//Miscelancelous
#define EV_VOID 96               //for testing linpac response
#define EV_SYSREQ 97             //system request (x=req_number)
#define EV_ABORT 198             //abort current process
#define EV_QUIT 99               //quit LinPac

//Event flags in Event.x
#define FLAG_NO_HANDLE -1        //EV_CMD_RESULT with no handle

#define FLAG_REMOTE 1            //EV_TEXT_RAW && EV_TEXT_COOKED from remote source
#define FLAG_MACRO 2             //EV_TEXT_RAW && EV_TEXT_COOKED from Macro (not executed)
#define FLAG_EDIT 4              //EV_TEXT_RAW && EV_TEXT_COOKED from Editor
#define FLAG_FM_MACRO 8          //EV_TEXT_RAW && EV_TEXT_COOKED from Macro (executed)

#define FLAG_FLUSH_IMMEDIATE 1   //EV_TEXT_FLUSH - flush just now

#define FLAG_APP_LOCAL 0         //EV_APP_STARTED (y field) local application
#define FLAG_APP_REMOTE 1        //EV_APP_STARTED (y field) remote application

#define FLAG_MOD_NONE 0          //no modifier pressed
#define FLAG_MOD_ALT 1           //Alt pressed

//-------------------------------------------------------------------------
// Event - structure for event specification
//-------------------------------------------------------------------------
struct Event
{
  int type; //event type (defined above)
  int chn;  //source channel
  int x,y;  //int data fileds
  char ch;  //char data field
  void *data; //additional data - depends on event type
};

#endif

