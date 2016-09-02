/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) David Ranch KI6ZHD (linpac@trinnet.net) 2002 - 2016
   (c) Radek Burget OK2JBG (radkovo@centrum.cz) 1998 - 2002

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   linpac.cc

   MAIN PROGRAM

   Last update 3.7.2002
  =========================================================================*/
#include <unistd.h>
#include <signal.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "sources.h"
#include "commands.h"
#include "event.h"
#include "names.h"
#include "screen.h"
#include "windows.h"
#include "editor.h"
#include "data.h"
#include "constants.h"
#include "version.h"
#include "tools.h"
#include "watch.h"
#include "status.h"
#include "sounds.h"
#include "keyboard.h"

#ifndef DAEMON_ONLY_MODE
Screen *scr;
QSOWindow *win[MAX_CHN];
#endif

Obj_man mgr;
EventGate *egate;

char keyfile[64]; //shared memory keyfile
int core = 0; //segv ocurred

int daemon_start(int ignsigcld)
{
	/* Code to initialiaze a daemon process. Taken from _UNIX Network	*/
	/* Programming_ pp.72-85, by W. Richard Stephens, Prentice		*/
	/* Hall PTR, 1990							*/

	register int childpid, fd;

	/* If started by init, don't bother */
	if (getppid() != 1)
   {
   	/* Ignore the terminal stop signals */
   	signal(SIGTTOU, SIG_IGN);
   	signal(SIGTTIN, SIG_IGN);
   	signal(SIGTSTP, SIG_IGN);
   
   	/* Fork and let parent exit, insures we're not a process group leader */
   	if ((childpid = fork()) < 0) {
   		return 0;
   	} else if (childpid > 0) {
   		exit(0);
   	}
   
   	/* Disassociate from controlling terminal and process group.		*/
   	/* Ensure the process can't reacquire a new controlling terminal.	*/
   	if (setpgrp() == -1)
   		return 0;
   
   	if ((fd = open("/dev/tty", O_RDWR)) >= 0) {
   		/* loose controlling tty */
   		ioctl(fd, TIOCNOTTY, NULL);
   		close(fd);
   	}
   }

	/* Move the current directory to root, to make sure we aren't on a	*/
	/* mounted filesystem.							*/
	//chdir("/"); //we can't afford this - we need our work directory

	/* Clear any inherited file mode creation mask.	*/
	umask(0);

	if (ignsigcld)
		signal(SIGCHLD, SIG_IGN);

	/* That's it, we're a "daemon" process now */
	return 1;
}

void add_objects()
{
  Object *obj;
  #ifndef DAEMON_ONLY_MODE
  QSOWindow *win;
  Editor *ed;
  #endif

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon"))
  {
    scr = new Screen;
    mgr.insert(scr);

    obj = new Keyscan;
    mgr.insert(obj);
  }
  #endif
  obj = new Commander;
  mgr.insert(obj);
  Ax25io *axobj = new Ax25io(MAX_CHN);
  mgr.insert(axobj);

  egate = new EventGate(0, NULL, NULL);
  mgr.insert(egate);

  Cooker *cook = new Cooker;
  mgr.insert(cook);

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) //do not include screen objects in daemon mode
  {
    ChannelInfo *chni = new ChannelInfo(0, iconfig("max_x"), iconfig("chn_line"),
           iconfig("qso_start_line")+(iconfig("edit_end_line") - iconfig("edit_start_line"))+2);
    mgr.insert(chni);
    scr->insert(chni);

    InfoLine *linf = new InfoLine(0, iconfig("max_x"), iconfig("stat_line"),
                                  iconfig("qso_start_line"));
    mgr.insert(linf);
    scr->insert(linf);

    if (!bconfig("no_monitor"))
    {
      MonWindow *mon = new MonWindow(0, iconfig("mon_start_line"), iconfig("max_x"), iconfig("mon_end_line"),
             iconfig("qso_start_line")+(iconfig("edit_end_line") - iconfig("edit_start_line"))+3,
             "monitor.screen");
      mgr.insert(mon);
      scr->insert(mon);
    }

    int i;
    for (i=1; i<=MAX_CHN; i++)
    {
      char fname[20];
      sprintf(fname, "window%i.screen", i);
      win = new QSOWindow(0, iconfig("qso_start_line"), iconfig("max_x"), iconfig("qso_end_line"), i, fname);
      if (i==1) win->active(true);
      mgr.insert(win);
      scr->insert(win);

      ed = new Editor(0, iconfig("edit_start_line"), iconfig("max_x"), iconfig("edit_end_line"), 20, i);
      if (i==1) ed->active(true);
      mgr.insert(ed);
      scr->insert(ed);
  
      StatLines *lins = new StatLines(i, 0, iconfig("max_x"), iconfig("stat_line"));
      mgr.insert(lins);
      scr->insert(lins);
    }
  
    ed = new Editor(0, iconfig("qso_start_line")+1, iconfig("max_x"),
                    iconfig("qso_start_line")+(iconfig("edit_end_line") - iconfig("edit_start_line"))+1,
                    20, 0);
    mgr.insert(ed);
    scr->insert(ed);
  }
  #endif //DAEMON_ONLY_MODE
  
  StnDB *dbase = new StnDB("station.data");
  mgr.insert(dbase);

  Watch *watch = new Watch;
  mgr.insert(watch);

  StatusServer *status = new StatusServer;
  mgr.insert(status);

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon"))
  {
    Sound *snd = new Sound;
    mgr.insert(snd);
  }
  #endif
}

void send_init_events()
{
  char data[10];
  strcpy(data, "NOCALL"); emit(0, EV_UNPROTO_SRC, 0, data);
  strcpy(data, "ALL"); emit(0, EV_UNPROTO_DEST, 0, data);
  strcpy(data, "INIT"); emit(0, EV_DO_COMMAND, 0, data);
}

void init()
{
  set_public_mgr(&mgr);

  setIConfig("maxchn", MAX_CHN);
  setBConfig("remote", false);
  setBConfig("cbell", false);
  setBConfig("knax", false);
  setSConfig("def_port", "");
  setSConfig("unportname", "");
  setIConfig("unport", 0);
  setIConfig("info_level", 1);
  setBConfig("swap_edit", false);
  setBConfig("monitor", !bconfig("no_monitor"));
  setBConfig("listen", true);
  setSConfig("no_name", "%C");
  setSConfig("timezone", time_zone_name());
  setIConfig("last_act", time(NULL));

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon"))
  {
    setIConfig("qso_start_line", 0); setIConfig("qso_end_line", 12);
    setIConfig("stat_line", iconfig("qso_end_line") + 1);
    setIConfig("edit_start_line", iconfig("stat_line") + 1);
    setIConfig("edit_end_line", 17);
    setIConfig("chn_line", iconfig("edit_end_line") + 1);
    setIConfig("mon_start_line", iconfig("chn_line") +1);
    setIConfig("mon_end_line", stdscr->_maxy);
    setIConfig("max_x", stdscr->_maxx);
    setBConfig("swap_edit", false);
    setBConfig("monitor", true);
    setBConfig("mon_bin", true);
  }
  #endif

  setBConfig("fixpath", true);
  
  for (int i=0; i<=MAX_CHN; i++)
  {
    conv_in[i] = conv_out[i] = NULL;
    strcpy(term[i], "");
    ch_disabled[i] = false;
    huffman_comp[i] = false;
    //aclist[i].push_back("*"); //! this is not too secure !
  }

  #ifdef LINPAC_DEBUG
  DEBUG.macro = false;
  #endif
}

void check_lock()
{
  FILE *lock;
  char keyfile[256];

  sprintf(keyfile, "/var/lock/LinPac.%i", getuid());
  lock = fopen(keyfile, "r");
  if (lock != NULL)
  {
    int lpid, lmax, key;
    struct stat st;
    
    if (fscanf(lock, "%i %i %x", &lpid, &lmax, &key) == 3)
    {
      sprintf(keyfile, "/proc/%i", lpid);
      int r = stat(keyfile, &st);

      if (r != -1)
      {
        printf("LinPac seems to be already running with PID %i.\n", lpid);
        printf("LinPac can't run twice for one user - sorry.\n");
        exit(1);
      }
    }
    else
    {
      printf("The LinPac lock file, %s, has incompete information.\n", keyfile);
      printf("LinPac may be running. Exiting.\n");
      exit(1);
    }
    fclose(lock);
  }
}

void create_lock()
{
  int lock;
  sprintf(keyfile, "/var/lock/LinPac.%i", getuid());
  lock = open(keyfile, O_RDWR | O_CREAT, 0600);
  if (lock == -1)
  {
    fprintf(stderr, "Cannot create the lockfile %s : %s\n",keyfile, strerror(errno));
    exit(1);
  }
  close(lock);
}

void remove_lock()
{
  unlink(keyfile);
}

void parse_cmdline(int argc, char **argv)
{
  #ifndef DAEMON_ONLY_MODE
  setBConfig("daemon", false);
  setBConfig("no_monitor", false);
  setBConfig("disable_spyd", false);
  setSConfig("monparms", "ar8");
  #else
  setBConfig("daemon", true);
  #endif
  for (int i = 1; i < argc; i++)
  {
    if (argv[i][0] == '-')
      for (char *p = argv[i]+1; *p; p++)
        switch (*p)
        {
          #ifndef DAEMON_ONLY_MODE
          case 'd': setBConfig("daemon", true); break;
          case 'm': setBConfig("no_monitor", true); break;
          case 's': setBConfig("disable_spyd", true); break;
          case 'p': if (*(++p))
                    {
                       char par[10];
                       int n = 0;
                       char *d = par;
                       while (n < 9 && *p) { *d = *p; d++; p++; n++; }
                       *d = '\0'; p--;
                       setSConfig("monparms", par);
                    }
                    else
                    {
                       i++;
                       if (i < argc)
                           setSConfig("monparms", argv[i]);
                       else
                           fprintf(stderr, "Option argument missing\n");
                    }
                    break;
          default: fprintf(stderr, "Unknown option %s\n", p);
          #endif
        }
  }
}

//------------------------------------------------------------------------
// SIGTERM handler
void sigterm_handler(int sig)
{
  emit(0, EV_QUIT, 0, NULL);
  mgr.do_step();

  Error(0, "LinPac going down (SIGTERM)");

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) scr->stop();
  #endif
  mgr.stop();
  egate->end_work();
  redirect_errorlog("/dev/null"); //close previous logs
  remove_lock();
  exit(0);
}

//------------------------------------------------------------------------
// SIGSEGV handler
void sigsegv_handler(int sig)
{
  if (core) abort(); //already reported SEGV - unrecoverable error
  core = 1;

  emit(0, EV_QUIT, 0, NULL);
  mgr.do_step();

  Error(0, "LinPac going down - ABNORMAL TERMINATION (SIGSEGV)");

  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) scr->stop();
  #endif
  mgr.stop();
  egate->end_work();
  redirect_errorlog("/dev/null"); //close previous logs
  remove_lock();
  printf("Sorry, LinPac has crashed (segmentation fault).\n");
  abort();
}

//------------------------------------------------------------------------
int main(int argc, char **argv)
{
  signal(SIGINT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTERM, sigterm_handler);
  //signal(SIGSEGV, sigsegv_handler);
  setlocale(LC_ALL, "");
  #ifndef DAEMON_ONLY_MODE
  printf("LinPac version %s (%s) by KI6ZHD\n\n", VERSION, __DATE__);
  #endif

  //init environment variables
  init_env();

  //create lockfile
  check_lock();
  create_lock();

  //read configuration from command line
  parse_cmdline(argc, argv);

  //create screen driver
  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) scr = new Screen;
  #endif

  //start initialization
  init();
  #ifndef DAEMON_ONLY_MODE
  set_var(0, VAR_CLIENT, "builtin");
  #endif
  set_var(0, VAR_VERSION, VERSION);
  add_objects();
  send_init_events();
  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) scr->redraw();
  #endif

  //become a daemon when needed
  if (bconfig("daemon"))
    if (!daemon_start(1))
      printf("LinPac: cannot become a daemon - aborting\n");

  Message(1, "Alt-X to exit.");

  //start main loop
  mgr.quit = false;
  while (mgr.quit == false)
  {
     mgr.do_step();
  }

  Error(0, "LinPac going down (quit)");

  //free resources
  #ifndef DAEMON_ONLY_MODE
  if (!bconfig("daemon")) scr->stop();
  #endif
  mgr.stop();
  egate->end_work();
  redirect_errorlog("/dev/null"); //close previous logs
  remove_lock();
  return 0;
}

