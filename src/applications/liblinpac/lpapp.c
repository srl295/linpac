/*
  LinPac - application interface library
  (c) by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 2001

  Last update: 19.5.2002
*/
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <locale.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "hash.h"
#include "tevent.h"
#include "lpapp.h"

/* uncomment this to debug event synchronization */
/*#define SYNC_DEBUG*/

/* Path to AXPORTS */
#define AXPORTS "/etc/ax25/axports"

/* Number of waiting cycles for creating the event gate */
#define WAIT_CYCLES 64

/* LinPac TCP port */
#define API_PORT 0x4c50

/* Default number of channels */
#define MAX_CHN 8

/*-- basic info --*/
static int app_chn;                   /* channel number in LinPac */
static int app_pid;                   /* application PID */
static int app_uid;                   /* application UID */
static int app_remote = 0;            /* application started by remote user */
static char *app_ident;               /* appl. indentification (not used) */
static int maxchn = MAX_CHN;          /* max. number of channels */

/*-- communication --*/
static int sock;                      /* API socket descriptor */
static struct hash **env;             /* environment variables */

/*-- event handling --*/
static handler_type event_handler = NULL;  /* user event handler */
static int sig_on;                    /* automatic handling is on (signals) */
static int sync_on;                   /* i/o stream synchronization is on */
static int blocked;                   /* event acknowledgement is blocked */
static int blockaw;                   /* start blocking after wait_event */
static int unack;                     /* there is some unacknowledged event */
static int wait_event_type,
           wait_event_chn;            /* description of event we're waiting for */
static volatile int event_came;       /* expected event came */
static Event awaited_event;           /* the event we've been waiting for */
static int events_stored = 0;         /* some events stored? */
static int transfer = 0;              /* event transfer is in progress? */
static sigset_t usr1mask;             /* USR1 signal mask */
static int end_round = 0;             /* end of event round */
static int default_y = 0;             /* default event y field */

static int wconnect_prepared = 0;     /* connection wait prepared? */

/* read and work out all events from pipe */
void lp_internal_sig_resync();

/* send a protocol command */
void lp_send_command(int cmd, int data);

/*=============================== DEBUGGING ============================*/
void sync_debug_msg(const char *fmt, ...)
{
  #ifdef SYNC_DEBUG
  va_list argptr;

  va_start(argptr, fmt);
  vfprintf(stderr, fmt, argptr);
  va_end(argptr);
  #endif
}

/*======================================================================*/
/* Interrupt safe versions of syscalls */
size_t safe_write(int fd, const void *buf, size_t count)
{
   int r;
   do
   {
      r = write(fd, buf, count);
   } while (r == -1 && (errno == EINTR || errno == EAGAIN));
   if (r == -1) perror("safe_write");
   return r;
}

size_t safe_read(int fd, void *buf, size_t count)
{
   int r;
   do
   {
      r = read(fd, buf, count);
   } while (r == -1 && errno == EINTR);
   return r;
}

char *safe_fgets(char *s, int size, FILE *stream)
{
   int cnt = 0;
   int ch;
   while (cnt < size)
   {
      do {errno = 0; ch = fgetc(stream);} while (ch == EOF && errno == EINTR);
      if (ch == EOF) {s[cnt++] = '\0'; break;}
      s[cnt++] = ch;
      if (ch == '\n') {s[cnt++] = '\0'; break;}
   }
   return s;
}

int safe_fgetc(FILE *stream)
{
   int c;
   do c = fgetc(stream); while (c == EOF);
   return c;
}

int safe_close(int fd)
{
   int r;
   do r = close(fd); while (r == -1 && errno == EAGAIN);
   /*fprintf(stderr, "*");
   if (r == -1) perror("Close");*/
   return r;
}

/*========================================================================*/

void lp_internal_exit_function()
{
   safe_close(sock);
}

void lp_internal_create_env(int channels)
{
   int i;
   maxchn = channels;
   env = (struct hash **) malloc((maxchn+1) * sizeof(struct hash *));
   for (i = 0; i <= maxchn; i++)
      env[i] = create_hash();
}

void lp_internal_destroy_env()
{
   int i;
   for (i = 0; i <= maxchn; i++)
      destroy_hash(env[i]);
   free(env);
}

void lp_internal_flush_stdin()
{
   fd_set rfds;
   struct timeval tv;
   int retval;

   FD_ZERO(&rfds);
   FD_SET(0, &rfds);
   do
   {
       tv.tv_sec = 0;
       tv.tv_usec = 1;
       retval = select(1, &rfds, NULL, NULL, &tv);
       if (retval)
       {
           char buf[256];
           int r = read(0, buf, 256);
           if (r < 256) break;
       }
   } while(retval);
}

void lp_internal_read_cmd()
{
   int cmd, data;
   safe_read(sock, &cmd, sizeof(int));
   safe_read(sock, &data, sizeof(int));
   sync_debug_msg("%i: APP command %i data=%i\n", app_pid, cmd, data);
   /* interpret commands */
   switch (cmd)
   {
      case 100: end_round = data;
   }
}

/*================= Application information functions =====================*/

int lp_channel()
{
   return app_chn;
}

char *lp_version()
{
   return lp_get_var(0, "VERSION");
}

int lp_app_remote()
{
   return app_remote;
}

/*========================================================================*/

/* Store application identification */
void lp_app_ident(const char *ident)
{
   app_ident = strdup(ident);
}

/* Initialize communication */
int lp_start_appl()
{
   Event ev;
   struct sockaddr_in addr;
   struct hostent *host;
   int cnt = WAIT_CYCLES;   /* Number of waiting cycles for creating
                               the event gate */
   setlocale(LC_ALL, "");   /* use locale */
   setbuf(stdout, NULL);    /* linpac does the buffering for us */
   app_pid = getpid();
   app_uid = getuid();
 
   /* initialize the USR1 mask */
   sigemptyset(&usr1mask);
   sigaddset(&usr1mask, SIGUSR1);

   /* Connect LinPac via TCP */
   sock = socket(AF_INET, SOCK_STREAM, 0);
   if (sock == -1)
   {
      perror("Cannot create socket");
      return 0;
   }
   host = gethostbyname("localhost");
   if (host == NULL)
   {
      fprintf(stderr, "Unknown host: localhost\n");
      return 0;
   }
   addr.sin_family = AF_INET;
   addr.sin_port = 0;
   memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);
   if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
   {
      perror("Cannot bind()");
      return 0;
   }
 
   addr.sin_port = htons(API_PORT);
   if (connect(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
   {
      perror("Cannot connect()");
      return 0;
   }
 
   lp_event_handling_off();    /* for establishing connection */
 
   /* Ask LinPac to create our own gate */
   lp_emit_event(0, EV_CREATE_GATE, getpid(), NULL);
   ev.type = EV_NONE; ev.data = NULL;
 
   /* Wait for the gate */
   do
   {
      /* LinPac will signal the remote application before creating the gate */
      if (lp_get_event(&ev) && ev.type == EV_APP_STREMOTE && ev.x == app_pid)
      {
          cnt = WAIT_CYCLES;
          app_remote = 1;
      }
      cnt--; /* wait cycles */
   } while ((ev.type != EV_GATE_FINISHED || ev.x != app_pid) && cnt > 0);
   if (cnt <= 0) { close(sock); return 0; }/* timeout */
   app_chn = ev.chn; /* connection established */

   /* close sockets on exit */
   atexit(lp_internal_exit_function);
   
   /* Default is automatic handling on */
   lp_event_handling_on();

   /* create the environment of default size */
   lp_internal_create_env(MAX_CHN);

   /* environment synchronization */
   lp_wait_init(0, EV_SYNC_DONE);
   lp_emit_event(0, EV_SYNC_REQUEST, 0, NULL);
   lp_wait_realize();

   /* succesfully connected */
   return 1;
}

/* Handle internal events */
int lp_handle_internal(Event *ev)
{
   if (ev->type == EV_VAR_SYNC)
   {
      sync_debug_msg("Var sync: %i channels\n", ev->x);
      lp_internal_destroy_env();
      lp_internal_create_env(ev->x);
   }
   else if (ev->type == EV_VAR_CHANGED)
   {
      char *name = (char *)ev->data;
      char *value = name + strlen(name) + 1;
      sync_debug_msg("Var sync@%i %s = %s\n", ev->chn, name, value);
      if (ev->chn >= 0 && ev->chn <= maxchn)
         hash_set(env[ev->chn], name, value);
   }
   else if (ev->type == EV_VAR_DESTROYED)
   {
      char *name = (char *)ev->data;
      sync_debug_msg("Var destroyed@%i %s", ev->chn, name);
      if (ev->chn >= 0 && ev->chn <= maxchn)
         delete_element(env[ev->chn], name);
   }
   else return 0;

   return 1;
}

/* Read the event from the queue */
int lp_get_event(Event *ev)
{
   fd_set rfds;
   struct timeval tv;
   int rc, len;
   char t; /* event type: event/command */
 
   sigprocmask(SIG_BLOCK, &usr1mask, NULL);
   transfer = 1;
   sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
 
   do
   {
      FD_ZERO(&rfds);
      FD_SET(sock, &rfds);
      tv.tv_sec = 0;
      tv.tv_usec = 10;
      rc = select(sock+1, &rfds, NULL, NULL, &tv);
      if (rc>0 && FD_ISSET(sock, &rfds))
      {
         /* read identification */
         rc = safe_read(sock, &t, 1);
         if (rc == -1)
         {
            sigprocmask(SIG_BLOCK, &usr1mask, NULL);
            transfer = 0;
            sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
            return 0;
         }
   
         /* if this is not an event, it's a command -> parse it and continue */
         if (t != '\0')
         {
            lp_internal_read_cmd();
            continue;
         }
         
         /* read the event */
         safe_read(sock, &(ev->type), sizeof(int));
         safe_read(sock, &(ev->chn), sizeof(int));
         safe_read(sock, &(ev->x), sizeof(int));
         safe_read(sock, &(ev->y), sizeof(int));
         safe_read(sock, &len, sizeof(int));
         if (len == -1) safe_read(sock, &(ev->data), sizeof(void *));
         else if (len > 0)
         {
            ev->data = (char *) realloc(ev->data, len);
            safe_read(sock, ev->data, len);
         }
         sync_debug_msg("%i: APP got %i chn=%i x=%i\n", app_pid, ev->type, ev->chn, ev->x);
         lp_handle_internal(ev);
         sigprocmask(SIG_BLOCK, &usr1mask, NULL);
         transfer = 0;
         sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
         return 1;
      }
      else break; /* select() timed out - no data available */
   } while (1);

   transfer = 0;
   return 0;
}

void lp_serialize_event(int chn, int type, int x, void *data,
                        char **buffer, int *size)
{
   int len;
   int blen;
   int bpos;
   char *buf;
   int y = default_y;
   
   if (type >=   0 && type < 100) len = 0;
   if (type >= 100 && type < 200) len = strlen((char *)data) + 1;
   if (type >= 200 && type < 300) len = x;
   if (type >= 400) len = 0;
 
   blen = len + 5*sizeof(int) + 1;
   buf = (char *)malloc(blen * sizeof(char));
   buf[0] = '\0'; /* event introduction */
   bpos = 1;
   memcpy(buf+bpos, &type, sizeof(int)); bpos += sizeof(int);
   memcpy(buf+bpos, &chn, sizeof(int)); bpos += sizeof(int);
   memcpy(buf+bpos, &x, sizeof(int)); bpos += sizeof(int);
   memcpy(buf+bpos, &y, sizeof(int)); bpos += sizeof(int);
   memcpy(buf+bpos, &len, sizeof(int)); bpos += sizeof(int);
   if (len > 0) memcpy(buf+bpos, data, len);
   
   *buffer = buf;
   *size = blen;
}


/* Emit the event */
int lp_emit_event(int chn, int type, int x, void *data)
{
   char *buf;
   int size;
   
   sigprocmask(SIG_BLOCK, &usr1mask, NULL);
   transfer = 1;
   sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
 
   sync_debug_msg("%i: APP start %i chn=%i x=%i\n", app_pid, type, chn, x);
 
   lp_serialize_event(chn, type, x, data, &buf, &size);
   safe_write(sock, buf, size);
   free(buf);
   
   sync_debug_msg("%i: APP sent %i chn=%i x=%i\n", app_pid, type, chn, x);
 
   sigprocmask(SIG_BLOCK, &usr1mask, NULL);
   transfer = 0;
   sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
   
   lp_internal_sig_resync(); //workout events that arrived while sending
   return 0;
}

void lp_send_command(int cmd, int data)
{
   char t;

   /* do not block transfer for ACK command, this would block the events
      a while after sending the command */
   sigprocmask(SIG_BLOCK, &usr1mask, NULL);
   if (cmd != 0) transfer = 1;
   sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
 
   /* introduce the command */
   t = '\x1';
   safe_write(sock, &t, 1);
   
   /* send the command */
   safe_write(sock, &cmd, sizeof(int));
   safe_write(sock, &data, sizeof(int));
 
   sigprocmask(SIG_BLOCK, &usr1mask, NULL);
   if (cmd != 0) transfer = 0;
   sigprocmask(SIG_UNBLOCK, &usr1mask, NULL);
}

/* Wait until an event comes */
void lp_wait_event(int chn, int type)
{
   if (sig_on)
   {
      sync_debug_msg("WAITING START (type=%i, chn=%i)\n", type, chn);
      wait_event_type = type; wait_event_chn = chn;
      event_came = 0; //don't accept previous events
      while (!event_came) pause();
      sync_debug_msg("WAITING DONE\n");
   }
   else
   {
      Event ev;
      ev.data = NULL;
      do lp_get_event(&ev); while (ev.type != type || ev.chn != chn);
      lp_copy_event(&awaited_event, &ev);
   }
}

/* Start watching events */
void lp_wait_init(int chn, int type)
{
   sync_debug_msg("WAITING INIT\n (type=%i, chn=%i)\n", type, chn);
   wait_event_type = type; wait_event_chn = chn;
   event_came = 0; //don't accept previous events
}

/* Realize the waiting */
void lp_wait_realize()
{
   if (sig_on)
   {
      sync_debug_msg("WAITING REALIZE\n");
      while (!event_came) pause();
      sync_debug_msg("WAITING REALIZE DONE\n");
   }
   else
   {
      Event ev;
      ev.data = NULL;
      do
         lp_get_event(&ev);
      while (ev.type != wait_event_type || ev.chn != wait_event_chn);
      lp_copy_event(&awaited_event, &ev);
   }
}

/* return the awaited event */
Event *lp_awaited_event()
{
   return &awaited_event;
}

/* Clear event structure and free memory */
void lp_discard_event(Event *ev)
{
   if (ev->data != NULL)
   {
      free(ev->data);
      ev->data = NULL;
   }
}

/* Copy one event structure to another */
Event *lp_copy_event(Event *dest, const Event *src)
{
   dest->type = src->type;
   dest->chn = src->chn;
   dest->x = src->x;
   dest->y = src->y;
   if (dest->type < 100) dest->data = NULL;   //no data
   if (dest->type >= 100 && dest->type < 200) //string allocation
   {
      dest->data = malloc(strlen((char *)src->data)+1);
      strcpy((char *)dest->data, (char *)src->data);
   }
   if (dest->type >= 200 && dest->type < 300) //data buffer allocation
   {
      dest->data = malloc(src->x);
      memcpy(dest->data, src->data, src->x);
   }
   if (dest->type >= 300 && dest->type < 400) dest->data = src->data; //pointer data
   return dest;
}

/* Read all events from the pipe */
void lp_clear_event_queue()
{
   if (!sig_on) //only work when no signals are used
   {
      Event ev;
      while (ev.data = NULL, lp_get_event(&ev)) ;
   }
}

/* End application */
void lp_end_appl()
{
   /* wait until LinPac accepts all previous events */
   lp_wait_init(app_chn, EV_VOID);
   lp_emit_event(app_chn, EV_VOID, 0, NULL);
   lp_wait_realize();
   /* end application */
   safe_close(sock);
}

void lp_event_y(int y)
{
   default_y = y;
}

//======================== Environment functions ========================*/

char *lp_get_var(int chn, const char *name)
{
   if (chn <= maxchn && chn >= 0)
      return hash_get(env[chn], name);
   return NULL;
}

void lp_del_var(int chn, const char *name)
{
   if (chn <= maxchn && chn >= 0)
   {
      delete_element(env[chn], name);
      lp_emit_event(chn, EV_VAR_DESTROYED, 0, (void *)name);
   }
}

void lp_clear_var_names(int chn, const char *names)
{
   if (chn <= maxchn && chn >= 0)
   {
      struct h_name_list *list = get_name_list(env[chn]);
      struct h_name_list *act;
      for (act = list; act; act = act->next)
         if (strstr(act->name, names) == act->name)
            lp_del_var(chn, act->name);
      destroy_name_list(list);
   }
}

void lp_set_var(int chn, const char *name, const char *value)
{
   if (chn <= maxchn && chn >= 0)
   {
      int esize;
      char *edata;
      
      hash_set(env[chn], name, value);
      /* emit synchronization event */
      esize = strlen(name)+strlen(value)+2;
      edata = (char *) malloc(esize * sizeof(char));
      strcpy(edata, name);
      strcpy(edata+strlen(name)+1, value);
      lp_emit_event(chn, EV_VAR_CHANGED, esize, edata);
      free(edata);
   }
}

/*======================= Shared configuration ===========================*/

char *lp_internal_sconfig(int chn, const char *name)
{
   char *nm, *r;

   nm = (char *) malloc((strlen(name)+2)*sizeof(char));
   sprintf(nm, "_%s", name);
   r = lp_get_var(chn, nm);
   free(nm);
   return r;
}

int lp_internal_iconfig(int chn, const char *name)
{
   char *s = lp_internal_sconfig(chn, name);
   if (s) return atoi(s);
   return 0;
}

char *lp_sconfig(const char *name)
{
   return lp_internal_sconfig(0, name);
}

int lp_iconfig(const char *name)
{
   return lp_internal_iconfig(0, name);
}

/*========================= Status functions =============================*/

int lp_chn_status(int chn)
{
   return lp_internal_iconfig(chn, "state");
}

char *lp_chn_call(int chn)
{
   return lp_internal_sconfig(chn, "call");
}

char *lp_chn_cwit(int chn)
{
   return lp_internal_sconfig(chn, "cwit");
}

char *lp_chn_cphy(int chn)
{
   return lp_internal_sconfig(chn, "cphy");
}

int lp_chn_port(int chn)
{
   return lp_internal_iconfig(chn, "port");
}

time_t lp_last_activity()
{
   return (time_t)lp_iconfig("last_act");
}

void lp_set_last_activity(time_t timeval)
{
   char val[32];
   sprintf(val, "%i", (int)timeval);
   lp_set_var(0, "_last_act", val);
}

/*========================== Signal handling ===========================*/

/* resynchronize signals with events (workout postponed events) */
void lp_internal_sig_resync()
{
   Event ev;
   sync_debug_msg("SYNC BEGIN\n");
   if (events_stored)
   {
      ev.data = NULL;
      end_round = 0;
      /*while (ev.data = NULL, lp_get_event(&ev))*/
      while (!end_round)
      {
         ev.data = NULL;
         if (!lp_get_event(&ev)) continue;
         if (wait_event_type == ev.type && wait_event_chn == ev.chn)
         {
            event_came = 1;
            lp_copy_event(&awaited_event, &ev);
            if (blockaw) { blockaw = 0; blocked = 1; } //block after wait
         }
         if (event_handler != NULL) (*event_handler)(&ev);
         lp_discard_event(&ev);
         sync_debug_msg("ROUND DONE\n");
      }
      sync_debug_msg("ALL READ\n");
      events_stored = 0;
      if (!blocked) lp_send_command(0, 0); /* acknowledge the event */
      else unack = 1;
   }
   sync_debug_msg("SYNC END (transfer=%i)\n", transfer);
}

/* Action on SIG_USR1 signal - read event and send it to the handler
   if defined */
void lp_internal_usr1_handler(int sig)
{
   events_stored = 1;
   sync_debug_msg("event received (%s)\n", transfer?"true":"false");
   if (!transfer) lp_internal_sig_resync();
}

void lp_set_event_handler(handler_type handler)
{
   event_handler = handler;
}

void lp_event_handling_on()
{
   struct sigaction sigact;

   sig_on = 1;
   sigact.sa_handler = lp_internal_usr1_handler;
   sigemptyset(&sigact.sa_mask);
   sigact.sa_flags = 0;
   if (sigaction(SIGUSR1, &sigact, NULL) == -1)
   {
       perror("lpapp: Cannot set event handler");
   }
 
   lp_send_command(1, 1);
}

void lp_event_handling_off()
{
   struct sigaction sigact;

   lp_send_command(1, 0);
   sig_on = 0;
   sigact.sa_handler = SIG_IGN;
   sigemptyset(&sigact.sa_mask);
   sigact.sa_flags = 0;
   if (sigaction(SIGUSR1, &sigact, NULL) == -1)
   {
       perror("lpapp: Cannot set event handler");
   }
}

void lp_syncio_on()
{
   sync_on = 1;
   lp_send_command(2, 1);
}

void lp_syncio_off()
{
   sync_on = 0;
   lp_send_command(2, 0);
}

void lp_block()
{
   blocked = 1;
}

void lp_block_after_wait()
{
   blockaw = 1;
}

void lp_unblock()
{
   blocked = 0;
   if (unack) lp_send_command(0, 0); /* acknowledge last event */
   unack = 0;
}

/*=========================== User functions ============================*/

void lp_appl_result(const char *fmt, ...)
{
   va_list argptr;
   static char s[256];
 
   va_start(argptr, fmt);
   vsprintf(s, fmt, argptr);
   lp_wait_init(0, EV_APP_RESULT);
   lp_emit_event(0, EV_APP_RESULT, app_pid, s);
   lp_wait_realize();
   va_end(argptr);
}

void lp_statline(const char *fmt, ...)
{
   va_list argptr;
   static char s[256];
 
   va_start(argptr, fmt);
   vsprintf(s, fmt, argptr);
   lp_wait_init(app_chn, EV_CHANGE_STLINE);
   lp_emit_event(app_chn, EV_CHANGE_STLINE, app_pid, s);
   lp_wait_realize();
   va_end(argptr);
}

void lp_remove_statline()
{
   lp_wait_init(app_chn, EV_REMOVE_STLINE);
   lp_emit_event(app_chn, EV_REMOVE_STLINE, app_pid, NULL);
   lp_wait_realize();
}

void lp_enable_screen()
{
   lp_wait_init(app_chn, EV_ENABLE_SCREEN);
   lp_emit_event(app_chn, EV_ENABLE_SCREEN, 0, NULL);
   lp_wait_realize();
}

void lp_disable_screen()
{
   lp_wait_init(app_chn, EV_DISABLE_SCREEN);
   lp_emit_event(app_chn, EV_DISABLE_SCREEN, 0, NULL);
   lp_wait_realize();
}

void lp_wait_connect_prepare(int chn)
{
   wconnect_prepared = 1;
   lp_wait_init(chn, (lp_chn_status(chn) == ST_DISC)?EV_CONN_TO:EV_RECONN_TO);
}

void lp_wait_connect(int chn, const char *call)
{
   char wcall[12];
   int match;

   sscanf(call, "%10[A-Za-z0-9-]", wcall);
   if (strchr(wcall, '-') == NULL) strcat(wcall, "-0");

   //wait for the right event
   match = 0;
   while (!match)
   {
      char ccall[12];
      lp_block_after_wait();
      if (wconnect_prepared)
         lp_wait_realize();
      else
         lp_wait_event(chn, (lp_chn_status(chn) == ST_DISC)?EV_CONN_TO:EV_RECONN_TO);
      wconnect_prepared = 0;

      //throw out previous input
      lp_internal_flush_stdin();
      lp_unblock();

      sscanf((char *)awaited_event.data, "%10[A-Za-z0-9-]", ccall);
      if (strchr(ccall, '-') == NULL) strcat(ccall, "-0");
      if (strcasecmp(wcall, ccall) == 0) match = 1;
   }
}

/*=============================== Tools ===================================*/

char *time_stamp(int utc)
{
   struct tm *cas;
   static char tt[32];
   time_t secs = time(NULL);
   if (utc) cas = gmtime(&secs);
       else cas = localtime(&secs);
   sprintf(tt, "%2i:%02i", cas->tm_hour, cas->tm_min);
   return tt;
}

char *date_stamp(int utc)
{
   struct tm *cas;
   static char tt[32];
   time_t secs = time(NULL);
   if (utc) cas = gmtime(&secs);
       else cas = localtime(&secs);
   strftime(tt, 30, "%x", cas);
   return tt;
}

void replace_macros(int chn, char *s)
{
   char t[256];
   char f[256];
   char *name, *p, *d, *ver;
   int n, i;
   time_t tm;
 
   strcpy(f, s);
   p = f;
   d = s;
   while (*p)
   {
     while(*p && *p != '%') {*d++ = *p++;}
     if (*p == '%')
     {
       int mchn = chn;
 
       char *src = ++p;
       char *dst = t;
       while (*src && (isalnum(*src) || *src == '_' || *src == '@')) *dst++ = *src++;
       *dst = '\0';
 
       src = strchr(p, '@');
       if (strchr(t, '@') != NULL)
       {
         char num[256];
         char *pchn = src+1;
         strcpy(num, "");
         while (*pchn && (isalnum(*pchn) || *pchn == '_'
                          || *pchn == '@' || *pchn == '%'))
                              {strncat(num, pchn, 1); pchn++;}
         replace_macros(mchn, num);
         if (strlen(num) > 0) mchn = atoi(num);
         memmove(src, pchn, strlen(pchn)+1);
       }
 
       dst = lp_get_var(mchn, p);
       if (dst != NULL)
       {
         strcpy(t, dst);
         p = src;
       }
       else
       {
         switch (toupper(*p))
         {
           case 'V': ver = lp_version();
                     if (ver) strcpy(t, ver);
                     else strcpy(t, "N/A");
                     break;
           case 'C': if (lp_chn_status(mchn) != ST_DISC)
                        strcpy(t, lp_chn_cwit(mchn));
                     else strcpy(t, lp_chn_call(mchn));
                     break;
           case 'N': name = lp_get_var(mchn, "STN_NAME");
                     if (name != NULL) strcpy(t, name);
                     else
                     {
                        strcpy(t, lp_sconfig("no_name"));
                        replace_macros(mchn, t);
                     }
                     break;
           case 'Y': strcpy(t, lp_chn_call(mchn)); break;
           case 'K': sprintf(t, "%i", mchn); break;
           case 'T': strcpy(t, time_stamp(0)); break;
           case 'D': strcpy(t, date_stamp(0)); break;
           case 'B': strcpy(t, "\a"); break;
           case 'Z': strcpy(t, lp_sconfig("timezone")); break;
           case 'U': strcpy(t, lp_sconfig("no_name")); break;
           case 'P': sprintf(t, "%i", lp_chn_port(mchn)); break;
           case 'M': n=0;
                     for (i=1; i <= maxchn; i++)
                       if (lp_chn_status(i) != ST_DISC) n++;
                     sprintf(t, "%i", n);
                     break;
           case 'A': tm = time(NULL);
                     sprintf(t, "%i", (int)(tm - lp_last_activity()));
                     break;
           case '_': strcpy(t, "\r"); break;
           case '#': {char num[256];
                     strcpy(num, "");
                     p++;
                     while (isdigit(*p))
                     {
                       strncat(num, p, 1);
                       p++;
                     }
                     sprintf(t, "%c", atoi(num));}
                     break;
           default: sprintf(t, "%%%c", *p);
         }
         p++;
       }
       strcpy(d, t);
       d += strlen(t);
     }
   }
   *d = '\0';
}

char *get_port_name(int n)
{
   static char name[256];
   char buf[256];
   FILE *f;
   int i = 0;
 
   strcpy(name, "");
 
   f = fopen(AXPORTS, "r");
   if (f != NULL)
      while (!feof(f))
      {
         strcpy(buf, "");
         if (fgets(buf, 255, f) != NULL)
         {
            while (isspace(*buf)) memmove(buf, buf+1, strlen(buf));
            if (buf[0] == '#' || strlen(buf) == 0) continue;
            if (i == n)
            {
                sscanf(buf, "%s", name);
                break;
            }
            i++;
         }
      }
   return name;
}

