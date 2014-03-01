/*
  LinPac - application interface library
  (c) by Radek Burget (OK2JBG) 1998 - 2001
*/
#ifndef LIN_PAC_APPL
#define LIN_PAC_APPL

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <sys/types.h>
#include "tevent.h"

/*========================= Common constants =============================*/

/* Library interface version. Use lp_version() for obtaining current LinPac
   version */
#define LPAPP_VERSION "0.16"

/* connection status constants */
#define ST_DISC 0 /* disconnected */
#define ST_DISP 1 /* disconnecting */
#define ST_TIME 2 /* disconnecting for timeout */
#define ST_CONN 3 /* connected */
#define ST_CONP 4 /* connectin in progress */

/*============================= Data types ================================*/

/* event type */
typedef struct Event Event;

/* event handler function type */
typedef void (*handler_type)(Event *);

/* ax25 status structure */
struct ax25_status
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
  int dama;
  int sendq, recvq;
};

/*=================== Safe versions of syscalls ==========================*/
/*!! These functions are interrupt-safe, use them rather than originals !!*/
size_t safe_read(int fd, void *buf, size_t count);
size_t safe_write(int fd, const void *buf, size_t count);
char *safe_fgets(char *s, int size, FILE *stream);
int safe_fgetc(FILE *stream);
int safe_close(int fd);

/*================= Application information functions =====================*/

/* lp_version() - get LinPac version (e.g. "0.16") */
char *lp_version();

/* lp_channel() - LinPac channel where the application is running */
int lp_channel();

/* lp_app_remote() - true if application was started by remote user */
int lp_app_remote();

/*=============== Basic LinPac communication functions ====================*/

/* lp_app_ident() - set application identification string. This function
                    only takes effect when used before contacting LinPac. */
void lp_app_ident(const char *ident);

/* lp_start_appl() - start communication with LinPac. Returns true when
                     LinPac was contacted sucessfully */
int lp_start_appl();

/* lp_end_appl() - close the communication with LinPac. */
void lp_end_appl();

/*=========================== Event handling ============================*/

/* lp_event_handling_on() - Switch automatic event reading on (default).
                            When event_handler function is set, it's called
                            automatically for each event */
void lp_event_handling_on();

/* lp_event_handling_off() - Switch automatic event reading off. Application
                             should call lp_get_event() periodicaly. If the
                             application stops to read the events, the event
                             queue overflows and LinPac will kill such
                             application. */
void lp_event_handling_off();

/* lp_set_event_handler() - Set user event handler - a function like
                            void my_handler(Event *ev)
                            This function is called for each event, when
                            automatic event handling is on. */
void lp_set_event_handler(handler_type handler);

/* lp_get_event() - get next event from event queue. This function is only
                    useable when automatic event handling is off. Returns
                    0 if no event is available. */
int lp_get_event(Event *ev);

/* lp_emit_event() - send an event to LinPac. The event is constructed
                     from data fields:
                     chn  - event channel
                     type - event type
                     x    - int data
                     data - other data */
int lp_emit_event(int chn, int type, int x, void *data);

/* lp_wait_event() - wait for specified event to arrive on specified
                     channel. This function blocks until the event
                     arrives. */
void lp_wait_event(int chn, int type);

/* lp_wait_init() - initialize waiting for specified event. This function
                    returns immediately, waiting is realized by
                    lp_wait_realize(). */
void lp_wait_init(int chn, int type);

/* lp_wait_realize() - wait for the event previously defined by
                       lp_wait_init(). Blocks until the event occurs.
                       Returns immediatelly if the event arrived
                       after last call of lp_wait_init(). */
void lp_wait_realize();

/* lp_awaited_event() - after return from lp_wait_event() or
                       lp_wait_realize() this function returns
                       the event that stopped waiting */
Event *lp_awaited_event();

/* lp_descard_event - discard the event, free memory */
void lp_discard_event(Event *ev);

/* lp_copy_event - copy the event structure (deep copy). */
Event *lp_copy_event(Event *dest, const Event *src);

/* lp_clear_event_queue - discard all events in the queue */
void lp_clear_event_queue();

/* lp_event_y - set a default value for the `y' field in outgoing events */
void lp_event_y(int y);

/*========================= Synchronization ==============================*/

/* lp_syncio_on() - synchronize i/o streams with the events */
void lp_syncio_on();

/* lp_syncio_off() - stop i/o synchronization (default) */
void lp_syncio_off();

/* lp_block() - block event acknowledgement - following event won't be
                acknowledged to LinPac, event queue is blocked. */
void lp_block();

/* lp_block_after_wait() - start blocking after awaited event arrives
                           (lp_wait_event should follow) */
void lp_block_after_wait();

/* lp_unblock() - stop blocking, acknowledge the last event */
void lp_unblock();

/*====================== Environment functions ===========================*/

/* lp_set_var() - create/modify variable */
void lp_set_var(int chn, const char *name, const char *value);

/* lp_del_var() - delete variable */
void lp_del_var(int chn, const char *name);

/* lp_clear_var_names() - delete all variables with names starting with <names> */
void lp_clear_var_names(int chn, const char *names);

/* lp_get_var() - returns the value of specified variable */
char *lp_get_var(int chn, const char *name);

/*======================= Shared configuration ===========================*/
/* Following functions allow the access to shared configuration variables
   of LinPac. The functions are equivalent but convert the result to
   various data types. Configuration variables are stored in channel 0
   environment under the name preceeded by _.
   Example: lp_bconfig("remote") checks if remote commands are allowed.
   At present following variables are defined:

  int remote;                        //Remote commands enabled?

  int cbell;                         //connection bell?
  int knax;                          //incomming frame bell?

  char *def_port;                    //Default port
  char *unportname;                  //Unproto port name
  int unport;                        //Unproto port

  int info_level;                    //Statusline: 0=none 1=short 2=full
  char *no_name;                     //Default name of stn
  char *timezone;                    //Local timezone
  int qso_start_line, qso_end_line,  //Screen divisions
      mon_start_line, mon_end_line,
      edit_start_line, edit_end_line,
      stat_line, chn_line;
  int max_x;                         //screen length
  int swap_edit;                     //swap editor with qso-window?
  int fixpath;                       //use fixed paths only?
  int daemon;                        //linpac works as daemon?
  int monitor;                       //monitor on/off
  int no_monitor;                    //monitor not installed?
  int listen;                        //listening to connection requests?
  int disable_spyd;                  //disable ax25spyd usage?
  int mon_bin;                       //monitor shows binary data?
  char *monparms;                    //parametres to 'listen' program
  int maxchn;                        //number of channels
  int envsize;                       //environment size
  time_t last_act;                   //last activity (seconds);
*/

/* lp_sconfig() - read the configuration variable */
char *lp_sconfig(const char *name);

/* lp_iconfig() - read the configuration variable and convert to int */
int lp_iconfig(const char *name);

/*========================= Status functions =============================*/

/* lp_chn_status() - get status of specified channel
                     (see "connection status constants" above) */
int lp_chn_status(int chn);

/* lp_chn_call() - get my callsign on specified channel */
char *lp_chn_call(int chn);

/* lp_chn_cwit() - get callsign of connected station */
char *lp_chn_cwit(int chn);

/* lp_chn_cphy() - get callsign of physicaly connected station */
char *lp_chn_cphy(int chn);

/* lp_chn_port() - get the number of port where the channel is connected */
int lp_chn_port(int chn);

/* lp_last_activity() - get the time of last activity - compatible with
                        the time() call. */
time_t lp_last_activity();

/* lp_set_last_activity() - set the time of last activity of operator */
void lp_set_last_activity(time_t timeval);

/*========================== User functions ==============================*/

/* lp_appl_result() - set the return value string of the application.
                      This result is used by LinPac when the application
                      finishes */
void lp_appl_result(const char *fmt, ...);

/* lp_statline() - create / change a status line for this application
                   on LinPac's screen */
void lp_statline(const char *fmt, ...);

/* lp_remove_statline() - remove the application's status line */
void lp_remove_statline();

/* lp_disable_screen() - disable writing incomming and outgoing data
                         to LinPac's screen */
void lp_disable_screen();

/* lp_enable_screen() - enable writing data to the screen */
void lp_enable_screen();

/* lp_wait_connect() - wait for a connection to specified callsign
                       on specified channel */
void lp_wait_connect(int chn, const char *call);

/* lp_wait_connect_prepare() - store current status before initiating
                               the connection */
void lp_wait_connect_prepare(int chn);

/*============================== Tools ====================================*/

/* return actual time string (or UTC time if utc != 0)*/
char *time_stamp(int utc);

/* return actual date string (or UTC date if utc != 0)*/
char *date_stamp(int utc);

/* replace % macros in this string */
void replace_macros(int chn, char *s);

/* find the name of n-th port (first is 0) */
char *get_port_name(int n);

/*=========================================================================*/

#ifdef __cplusplus
}
#endif

#endif

