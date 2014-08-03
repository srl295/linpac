/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (radkovo@centrum.cz) 1998 - 2002

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   commands.cc

   Objects for commands executing

   Last update 27.11.2002
  =========================================================================*/
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "t_stack.h"
#include "tools.h"
#include "version.h"
#include "sources.h"
#include "commands.h"
#include "watch.h"

bool read_param(char *cmdline, char *s, unsigned &pos, unsigned n)
{
  unsigned i = pos;
  unsigned cnt = 0;
  bool end = false;      //end of current parameter found
  bool bslash = false;   //backslash found
  bool inq = false;      //in quotes
  bool inqq = false;     //in double quotes
  bool hex = false;      //reading hex number
  bool rnum = false;     //reading any number
  char num[5];

  strcpy(num, "");
  strcpy(s, "");
  while (isspace(cmdline[i]) && i < strlen(cmdline)) i++;
  while (!end && i < strlen(cmdline))
  {
     if (bslash) //character(s) after backslash
     {
       switch (cmdline[i])
       {
         case '\\': strcat(s, "\\"); bslash = false; break;
         case '"' : strcat(s, "\""); bslash = false; break;
         case '\'': strcat(s, "\'"); bslash = false; break;
         case '?' : strcat(s, "\?"); bslash = false; break;
         case 'a' : strcat(s, "\a"); bslash = false; break;
         case 'b' : strcat(s, "\b"); bslash = false; break;
         case 'f' : strcat(s, "\f"); bslash = false; break;
         case 'n' : strcat(s, "\n"); bslash = false; break;
         case 'r' : strcat(s, "\r"); bslash = false; break;
         case 't' : strcat(s, "\t"); bslash = false; break;
         case 'v' : strcat(s, "\v"); bslash = false; break;
         case '0' : strcat(s, "\0"); bslash = false; break;
         case 'x' : hex = true; break; //hex number follows
         default:
           if ((hex && isxdigit(cmdline[i])) || (!hex && isdigit(cmdline[i])))
           {
             if (strlen(num) < 4) strncat(num, &cmdline[i], 1); //numbers
             rnum = true;
             bslash = false;
           }
           else //not a number
           {
             if (strlen(num) == 0)
               strncat(s, &cmdline[i-1], 2);
           }
       }
     }
     else if (rnum)
     {
       if (((hex && !isxdigit(cmdline[i])) || (!hex && !isdigit(cmdline[i])))
           && strlen(num) != 0) //end of number
       {
         char *endptr;
         char c;
         if (hex) c = char(strtol(num, &endptr, 16));
             else c = char(strtol(num, &endptr, 10));
         strncat(s, &c, 1);
         rnum = false;
         strcpy(num, "");
         i--; //read this char again
       }
       else
         if (strlen(num) < 4) strncat(num, &cmdline[i], 1); //numbers
     }
     else
     {
       if (cmdline[i] == '\\')
         if (inq) strncat(s, &cmdline[i], 1); //no BS sequences in quotes
             else bslash = true;
       else if (cmdline[i] == '\'' && !inqq)
         inq = !inq;
       else if (cmdline[i] == '\"' && !inq)
         inqq = !inqq;
       else if (!inq && !inqq && isspace(cmdline[i]))
         end = true;
       else strncat(s, &cmdline[i], 1);
     }
     i++;
     cnt++;
     if (cnt > n) break;
  }
  if (strlen(num) != 0)
  {
    char *endptr;
    char c;
    if (hex) c = char(strtol(num, &endptr, 16));
        else c = char(strtol(num, &endptr, 10));
    strncat(s, &c, 1);
    bslash = false;
    strcpy(num, "");
  }
  pos = i;
  return (i < strlen(cmdline));
}

void send_result(int chn, const char *fmt, ...)
{
  va_list argptr;
  static char s[1024];

  va_start(argptr, fmt);
  vsnprintf(s, 1023, fmt, argptr);
  emit(chn, EV_CMD_RESULT, -1, s);
  va_end(argptr);
}

Commander::Commander()
{
  strcpy(class_name, "Command");
  cmdline = NULL;
  gps=NULL;
  for (int i=0; i<=MAX_CHN; i++)
  {
    remote_disabled[i] = false;
  }

  FILE *f;
  //--- Load binary command database ---
  f = fopen("./bin/commands", "r");
  if (f)
  {
    Command comm;
    while (!feof(f))
    {
      char p[1024];
      strcpy(p, "");
      if (fgets(p, 1023, f) != NULL)
      {
        striplf(p);
        char *pp = strchr(p, '#');
        if (pp != NULL) *pp='\0';
        strcpy(comm.flags, "");
        int n = sscanf(p, "%s %s %s", comm.name, comm.cmd, comm.flags);
        if (n != -1) bin.push_back(comm);
      }
    }
    fclose(f);
  } else Error(errno, "cannot open \".bin/commands\"");
  //--- Load macro command database ---
  f= fopen("macro/commands", "r");
  if (f)
  {
    Command comm;
    while (!feof(f))
    {
      char p[1024];
      strcpy(p, "");
      if (fgets(p, 1023, f) != NULL)
      {
        striplf(p);
        char *pp = strchr(p, '#');
        if (pp != NULL) *pp='\0';
        strcpy(comm.flags, "");
        int n = sscanf(p, "%s %s %s", comm.name, comm.cmd, comm.flags);
        if (n != -1) mac.push_back(comm);
      }
    }
    fclose(f);
  }
}

bool Commander::load_language(const char *lang)
{
  char dbname[256];
  strcopy(lang_name, lang, 32);
  FILE *f;
  lmac.erase(lmac.begin(), lmac.end());
  snprintf(dbname, 255, "macro/%s/commands", lang);
  f = fopen(dbname, "r");
  if (f)
  {
    Command comm;
    while (!feof(f))
    {
      char p[1024];
      strcpy(p, "");
      if (fgets(p, 1023, f) != NULL)
      {
        striplf(p);
        char *pp = strchr(p, '#');
        if (pp != NULL) *pp='\0';
        strcpy(comm.flags, "");
        int n = sscanf(p, "%s %s %s", comm.name, comm.cmd, comm.flags);
        if (n != -1) lmac.push_back(comm);
      }
    }
    fclose(f);
    return true;
  }
  return false;
}

void Commander::handle_event(const Event &ev)
{
  if (ev.type == EV_REG_COMMAND)
  {
     Command comm;
     strcpy(comm.name, "");
     strcpy(comm.cmd, "");
     strcpy(comm.flags, "");
     char *p = (char *)ev.data;
     while (strlen(comm.cmd) < 15 && isalpha(*p))
        { strncat(comm.cmd, p, 1); p++; }
     
     std::vector <Command>::iterator it;
     bool exists = false;
     for (it = reg.begin(); it < reg.end(); it++)
        if (strcmp(it->cmd, comm.cmd) == 0) {exists = true; break;}

     if (!exists) reg.push_back(comm);
  }

  if (ev.type == EV_UNREG_COMMAND)
  {
     Command comm;
     strcpy(comm.name, "");
     strcpy(comm.cmd, "");
     strcpy(comm.flags, "");
     char *p = (char *)ev.data;
     while (strlen(comm.cmd) < 15 && isalpha(*p))
        { strncat(comm.cmd, p, 1); p++; }
     
     std::vector <Command>::iterator it;
     for (it = reg.begin(); it < reg.end(); it++)
        if (strcmp(it->cmd, comm.cmd) == 0)
        {
           reg.erase(it);
           break;
        }
  }

  if (ev.type == EV_TEXT_COOKED && ev.x != FLAG_MACRO)
    if (((char *)ev.data)[0] == ':') //Local commands
    {
      remote = ((ev.x == FLAG_REMOTE || ev.x == FLAG_FM_MACRO) &&
                (iconfig(ev.chn, "state") != ST_DISC));
      secure = !(ev.x == FLAG_REMOTE);
      send_res = false;
      res_hnd = FLAG_NO_HANDLE;
      pos = 1;
      do_command(ev.chn, (char *)ev.data);
    }

  if (ev.type == EV_TEXT_COOKED && ev.x == FLAG_EDIT)
    if (iconfig(ev.chn, "state") == ST_DISC) //Local loopback for macros
      emit(ev.chn, EV_LINE_RECV, 0, ev.data);

  if (ev.type == EV_LINE_RECV)      //Remote commands
  {
    if (((char *)ev.data)[0] == '/' && ((char *)ev.data)[1] == '/'
         && bconfig("remote") && !remote_disabled[ev.chn])
    {
      remote = true;
      secure = false;
      send_res = false;
      res_hnd = FLAG_NO_HANDLE;
      pos = 2;
      do_command(ev.chn, (char *)ev.data);
    }
  }
  if (ev.type == EV_WANT_RESULT)    //Result wanted
  {
    send_res = true;
    res_hnd = ev.x;
    pos = 0;
    do_command(ev.chn, (char *)ev.data);
    if (send_res) emit(ev.chn, EV_CMD_RESULT, ev.x, result);
  }
  if (ev.type == EV_DO_COMMAND)     //Do command
  {
    remote = false;
    secure = true;
    send_res = false;
    res_hnd = FLAG_NO_HANDLE;
    pos = 0;
    do_command(ev.chn, (char *)ev.data);
  }

  if (ev.type == EV_CMD_RESULT && ev.x == FLAG_NO_HANDLE)  //Treat result with no handle
  {
    static char s[1024];
    if (remote)
    {
      snprintf(s, 1023, "%s: %s\r", PACKAGE, (char *)ev.data);
      emit(ev.chn, EV_TEXT_COOKED, 0, s);
      emit(ev.chn, EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL);
    }
    else
    {
      strcopy(s, (char *)ev.data, 1024);
      emit(ev.chn, EV_EDIT_INFO, strlen(s), s);
    }
  }

  if (ev.type == EV_DISABLE_SCREEN)
     remote_disabled[ev.chn] = true;
  if (ev.type == EV_ENABLE_SCREEN)
     remote_disabled[ev.chn] = false;
}

bool Commander::com_is(char *s1, char const *s2)
{
  unsigned i,j;
  bool res;

  if (strlen(s1)>strlen(s2)) return 0;
  //How many capital letters
  for (i=0, j=0; i<strlen(s2); i++)
    if (s2[i]==toupper(s2[i])) j++;
  if (strlen(s1)<j) return false;
  //Match all capitals
  for (i=0, res=true; i<strlen(s1); i++)
    if (toupper(s1[i])!=toupper(s2[i])) res=false;
  return res;
}

bool Commander::com_ok(int chn, int echn, char *s1, char const *s2)
{
  return (com_is(s1, s2) && is_secure(chn, echn, s2));
}

bool Commander::is_secure(int chn, int echn, char const *cmd)
{
  if (secure) return true;
  bool sec = false;
  std::vector <RCommand>::iterator it;
  for (it = aclist[echn].begin(); it < aclist[echn].end() && !sec; it++)
  {
    char *p = strdup(*it);
    bool cchn = false;
    if (p[strlen(p)-1] == '@') //is it allowed to specify the channel?
    {
        cchn = true;
        p[strlen(p)-1] = '\0';
    }
    if (cchn || chn == echn) //channel for command OK
    {
        if (p[0] == '*') sec = true;
        else if (com_is(p, cmd)) sec = true;
    }
    free(p);
  }
  return sec;
}

bool Commander::nextp(char *s, int n)
{
  return read_param(cmdline, s, pos, n);
}

void Commander::whole(char *s)
{
  char *p = new char[strlen(cmdline)+1];
  strcpy(s, "");
  while (is_next())
  {
    nextp(p, strlen(cmdline));
    strcat(s, p);
    if (is_next()) strcat(s, " ");
  }
  delete[] p;
}

void Commander::whole_quot(char *s, bool fixpath)
{
  strcpy(s, "");
  while (is_next())
  {
    char *qts = new char[strlen(cmdline)+1];
    nextp(qts);
    //code ' -> '\''
    char *p = new char[strlen(qts)*4+4];
    strcpy(p, "");
    char *q = qts;
    while (*q)
    {
      if (*q == '\'') strcat(p, "'\\''");
      else strncat(p, q, 1);
      q++;
    }
    if (fixpath)
    {
      char *q = strrchr(p, '/');
      if (q != NULL) memmove(p, q, strlen(q)+1);
    }
    delete[] qts;
    char *add = new char[strlen(p)+3];
    sprintf(add, "'%s'", p);
    strcat(s, add);
    delete[] add;
    delete[] p;
    if (is_next()) strcat(s, " ");
  }
}

void Commander::do_command(int chn, char *cmds)
{
  //create new command line
  if (cmdline != NULL) free(cmdline);
  cmdline = strdup(cmds);
  //allocate the buffer for parametres (min 256b)
  int maxlen = 4*strlen(cmdline) +  4;
  if (maxlen < 256) maxlen = 256;
  if (gps != NULL) delete[] gps;
  gps = new char[maxlen];
  //parse the command
  char cmd[256];
  nextp(cmd, 255);
  int echn = chn; //channel where the command was entered
  command_channel(cmd, &chn); //effective channel
  if (strlen(cmd) == 0) return;
  strcpy(result, "");
  bool ok = false;

  //---- Messages to other channels ----
  if (strlen(cmd) == 1 && cmd[0] >= '1' && cmd[0] <= MAX_CHN + '0')
  {
    char *msg = new char[maxlen];
    char *output = new char[maxlen+32];
    char *split = new char[maxlen+32];
    whole(msg);
    sprintf(output, "(%i) %s: %s\n", chn, sconfig(chn, "cwit"), msg);
    if (strlen(output) > LINE_LEN)
    {
      char *p = output + LINE_LEN;
      while (!isspace(*p) && p > output) p--;
      if (p == output) strcpy(split, "");
      else
      {
        *p = '\0';
        strcopy(split, p+1, 256);
        strcat(output, "\n");
      }
    }
    else strcpy(split, "");
    emit(cmd[0] - '0', EV_TEXT_COOKED, 0, output);
    if (strlen(split) > 0)
    {
      sprintf(output, "(%i) %s: %s", chn, sconfig(chn, "cwit"), split);
      emit(cmd[0] - '0', EV_TEXT_COOKED, 0, output);
    }
    emit(cmd[0] - '0', EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL);
    delete[] split;
    delete[] output;
    delete[] msg;
    ok = true;
  }
  //---- COMMANDS ----
  if (com_ok(chn, echn, cmd, "ABort")) {
                              static char adr[256];
                              if (is_next())
                              {
                                nextp(adr, 255);
                                emit(chn, EV_ABORT, 0, adr);
                              }
                              else
                                emit(chn, EV_ABORT, 0, NULL);
                              ok = true;
                            }
  if (com_ok(chn, echn, cmd, "PCONNECT")) {do_connect(chn); ok = true;}
  if (com_ok(chn, echn, cmd, "Disconnect")) {emit(chn, EV_DISC_LOC, 0, NULL); ok = true;}
  if (com_ok(chn, echn, cmd, "Echo")) {echo(chn); ok = true;}
  if (com_ok(chn, echn, cmd, "FLUSH")) {emit(chn, EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL); ok = true;}
  if (com_ok(chn, echn, cmd, "SYstem")) {emit(0, EV_QUIT, 0, NULL); ok = true;}
  if (com_ok(chn, echn, cmd, "UNProto")) {whole(gps); emit(0, EV_TEXT_RAW, 0, gps); ok = true;}
  if (com_ok(chn, echn, cmd, "VERsion")) {version(chn); ok = true;}
  //----- Environment commands ----
  if (com_ok(chn, echn, cmd, "SET")) {
                            static char s1[256], s2[1024];
                            nextp(s1, 255); nextp(s2, 1024);
                            set_var(chn, s1, s2);
                            ok = true;
                          }
  if (com_ok(chn, echn, cmd, "UNSET")) {
                              char s1[256];
                              while (is_next())
                              {
                                 nextp(s1, 255);
                                 del_var(chn, s1);
                              }
                              ok = true;
                            }
  if (com_ok(chn, echn, cmd, "GET")) {
                            nextp(gps);
                            char *p = get_var(chn, gps);
                            if (p == NULL)
                              strcpy(result, "*not found*");
                            else
                              strcopy(result, p, 256);
                            ok = true;
                           }
  if (com_ok(chn, echn, cmd, "EXISTs")) {
                            nextp(gps);
                            char *p = get_var(chn, gps);
                            if (p == NULL) strcpy(result, "0");
                                      else strcpy(result, "1");
                            ok = true;
                            }
  if (com_ok(chn, echn, cmd, "ENVINFO")) {
                            /*int used = env_end(chn) - ENV(chn);
                            sprintf(result, "Size: %i, Used: %i, Free: %i",
                                    ENV_SIZE, used, ENV_SIZE - used);*/
                            ok = true;
                            }
  //----- Information commands ----
  if (com_ok(chn, echn, cmd, "PCALL")) {
                              strcopy(result, sconfig(chn, "cphy"), 256);
                              normalize_call(result);
                              ok = true;
                            }
  if (com_ok(chn, echn, cmd, "UTCtime")) {
                              strcopy(result, time_stamp(true), 256); ok = true;
                            }
  if (com_ok(chn, echn, cmd, "ISCONnected")) {
                              strcpy(result, (iconfig(chn, "state") == ST_CONN) ? "1":"0");
                              ok = true;
                            }
  if (com_ok(chn, echn, cmd, "MAXCHannels")) {
                              snprintf(result, 255, "%i", iconfig("maxchn")); ok = true;
                            }
  //----- Setup commands ----
  if (com_ok(chn, echn, cmd, "MACRO")) {ok = true;}
  if (com_ok(chn, echn, cmd, "CBell")) {bool_set_config("cbell"); ok = true;}
  if (com_ok(chn, echn, cmd, "COMPRess"))  {bool_set(&(huffman_comp[chn])); ok = true;}
  if (com_ok(chn, echn, cmd, "FIXPath")) {bool_set_config("fixpath"); ok = true;}
  if (com_ok(chn, echn, cmd, "INFOLEvel")) {int_set_config("info_level", 0, 2); ok = true;}
  if (com_ok(chn, echn, cmd, "KNax")) {bool_set_config("knax"); ok = true;}
  if (com_ok(chn, echn, cmd, "Language")) {
                                 if (is_next())
                                 {
                                   nextp(gps);
                                   if (load_language(gps))
                                     set_var(chn, "STN_LANG", gps);
                                   else
                                     snprintf(result, 255, "Unknown language (%s)", gps);
                                 }
                                 else
                                 {
                                   char *lang = get_var(chn, "STN_LANG");
                                   if (lang == NULL)
                                     strcpy(result, "(default)");
                                   else
                                     strcopy(result, lang, 256);
                                 }
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "LIsten")) {
                                 bool b = bconfig("listen");
                                 bool_set_config("listen");
                                 if (b != bconfig("listen"))
                                   emit(chn,
                                        bconfig("listen")?EV_LISTEN_ON:EV_LISTEN_OFF,
                                        0, NULL);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "MYcall")) {
                               if (is_next())
                               {
                                 nextp(gps);
                                 normalize_call(gps);
                                 emit(chn, EV_CALL_CHANGE, 0, gps);
                               }
                               else strcopy(result, sconfig(chn, "call"), 256);
                               ok = true;
                             }
  if (com_ok(chn, echn, cmd, "Port")) {
                             if (is_next())
                             {
                               nextp(gps);
                               setSConfig("def_port", gps);
                             }
                             else strcopy(result, sconfig("def_port"), 256);
                             ok = true;
                           }
  if (com_ok(chn, echn, cmd, "PRIVate")) {
                                bool b = ch_disabled[chn];
                                bool old = b;
                                bool_set(&b);
                                if (b != old)
                                {
                                  if (b) emit(chn, EV_DISABLE_CHN, 0, NULL);
                                    else emit(chn, EV_ENABLE_CHN, 0, NULL);
                                }
                                ok = true;
                              }
  if (com_ok(chn, echn, cmd, "RXFlow")) {
                                if (is_next())
                                {
                                  nextp(gps);
                                  if (strcasecmp(gps, "on") == 0)
                                    emit(chn, EV_RX_CTL, 1, NULL);
                                  else if (strcasecmp(gps, "off") == 0)
                                    emit(chn, EV_RX_CTL, 0, NULL);
                                  else
                                    strcpy(result, "Unknown switch");
                                }
                                else strcpy(result, "Use RXFlow ON/OFF");
                                ok = true;
                              }
  if (com_ok(chn, echn, cmd, "REMote")) {bool_set_config("remote"); ok = true;}
  if (com_ok(chn, echn, cmd, "TIMEZone")) {
                                 if (is_next())
                                 {
                                    char tz[10];
                                    nextp(tz, 8);
                                    setSConfig("timezone", tz);
                                 }
                                 else
                                   strcopy(result, sconfig("timezone"), 256);
                                 ok = true;
                                }
  if (com_ok(chn, echn, cmd, "UNSrc")) {
                               if (is_next())
                               {
                                 nextp(gps);
                                 normalize_call(gps);
                                 emit(chn, EV_UNPROTO_SRC, 0, gps);
                               }
                               else strcopy(result, sconfig(0, "call"), 256);
                               ok = true;
                             }
  if (com_ok(chn, echn, cmd, "UNDest")) {
                               if (is_next())
                               {
                                 //changes by dranch to support 7 digis in path
                                 //More in the path is a limitation of the linux kernel
                                 //strcpy(result, "ki6zhd digi patch enabled");
                                 nextp(gps);
                                 //---- normalize_call(gps);
                                 emit(chn, EV_UNPROTO_DEST, 0, gps);
                               }
                               else strcopy(result, sconfig(0, "cwit"), 256);
                               ok = true;
                             }
  if (com_ok(chn, echn, cmd, "UNPOrt")) {
                               if (is_next())
                               {
                                 nextp(gps);
                                 emit(chn, EV_UNPROTO_PORT, 0, gps);
                               }
                               else strcopy(result, sconfig("unportname"), 256);
                               ok = true;
                             }
  if (com_ok(chn, echn, cmd, "WAtch"))  {
                               if (is_next())
                               {
                                 nextp(gps);
                                 char *endptr;
                                 int i = strtol(gps, &endptr, 10);
                                 if (*endptr == '\0' && i >= 0 && i <= MAX_CHN)
                                 {
                                   static autorun_entry entry;
                                   entry.chn = i;
                                   if (is_next()) nextp(entry.key, 255);
                                   if (is_next())
                                   {
                                     nextp(entry.command, 127);
                                     emit(chn, EV_ADD_WATCH, sizeof(autorun_entry), &entry);
                                   }
                                   else strcpy(result, "Key & command spec. required");
                                 }
                                 else strcpy(result, "Channel number required");
                               }
                               ok = true;
                             }
  //----- Screen commands ----
  if (com_ok(chn, echn, cmd, "REDRAW")) {
                                 emit(chn, EV_REDRAW_SCREEN, 0, NULL);
                                 ok = true;
                             }
  if (com_ok(chn, echn, cmd, "STATLINE")) {
                                 int_set_config("stat_line", 1, iconfig("chn_line")-2);
                                 emit(chn, EV_STAT_LINE, iconfig("stat_line"), NULL);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "CHNLINE")) {
                                 int_set_config("chn_line", iconfig("stat_line")+2, iconfig("mon_end_line"));
                                 emit(chn, EV_CHN_LINE, iconfig("chn_line"), NULL);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "SWAPEDit")) {
                                 setBConfig("swap_edit", !bconfig("swap_edit"));
                                 emit(chn, EV_SWAP_EDIT, 0, NULL);
                                 emit(chn, EV_STAT_LINE, iconfig("stat_line"), NULL);
                                 emit(chn, EV_CHN_LINE, iconfig("chn_line"), NULL);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "INFOLine")) {
                                 char n[10];
                                 nextp(n, 9);
                                 nextp(gps);
                                 emit(chn, EV_CHANGE_STLINE, atoi(n), gps);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "REMOVEINFO")) {
                                 char n[10];
                                 nextp(n, 9);
                                 emit(chn, EV_REMOVE_STLINE, atoi(n), NULL);
                                 ok = true;
                               }
  if (com_ok(chn, echn, cmd, "TRanslate")) {
                                 int len;
                                 if (is_next())
                                 {
                                   nextp(gps);
                                   char name[256];
                                   char table[256];
                                   if (get_enc_alias(gps, name, table))
                                   {
                                     if (strlen(table) > 0)
                                     {
                                       len = load_conversion_tables(chn, table);
                                       if (len != -1)
                                       {
                                         emit(chn, EV_CONV_IN, len, conv_in[chn]);
                                         emit(chn, EV_CONV_OUT, len, conv_out[chn]);
                                       }
                                       else
                                         strcpy(result, "Warning: conversion table doesn't exist");
                                     }
                                     else //send empty tables
                                     {
                                       emit(chn, EV_CONV_IN, 0, NULL);
                                       emit(chn, EV_CONV_OUT, 0, NULL);
                                     }
                                     emit(chn, EV_CONV_NAME, 0, name); //send current encoding name
                                   }
                                   else
                                     strcpy(result, "Unknown translation table");
                                 }
                                 else strcpy(result, "Table name required");
                                 ok = true;
                                }
  if (com_ok(chn, echn, cmd, "TErm"))      {
                                  if (is_next())
                                  {
                                    nextp(gps);
                                    strcopy(term[chn], gps, 10);
                                    emit(chn, EV_SET_TERM, 0, term[chn]);
                                  }
                                  else strcopy(result, term[chn], 256);
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "DEFColor"))  {
                                  whole(gps);
                                  emit(chn, EV_CHANGE_COLOR, 0, gps);
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "SCRLIMit")) {
                                  char *endptrs, *endptrt;
                                  char s[80], t[80];
                                  if (is_next())
                                  {
                                    nextp(s, 79);
                                    if (is_next())
                                    {
                                      nextp(t, 79);
                                      int losize = strtol(s, &endptrs, 10);
                                      int hisize = strtol(t, &endptrt, 10);
                                      if (*endptrs == '\0' &&
                                          *endptrt == '\0')
                                      {
                                        emit(chn, EV_HIGH_LIMIT, hisize, NULL);
                                        emit(chn, EV_LOW_LIMIT, losize, NULL);
                                      }
                                      else strcpy(result, "Numeric arguments required");
                                    }
                                    else strcpy(result, "low and high limit required");
                                  }
                                  else strcpy(result, "low and high limit required");
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "MOnitor")) {
                                  if (bconfig("no_monitor"))
                                    strcpy(result, "Monitor not present");
                                  else
                                  {
                                    bool_set_config("monitor");
                                    emit(chn, EV_MONITOR_CTL, bconfig("monitor")?1:0, NULL);
                                  }
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "MBIN")) {
                                  bool_set_config("mon_bin");
                                  emit(chn, EV_MONITOR_CTL, bconfig("mon_bin")?3:2, NULL);
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "MONPort")) {
                             if (is_next())
                             {
                               nextp(gps);
                               setSConfig("monport", gps);
                             }
                             else
                             {
                               if (sconfig("monport"))
                                 strcopy(result, sconfig("monport"), 256);
                               else
                                 strcpy(result, "*");
                             }
                             ok = true;
                           }
                                    
  //----- String commands ----
  if (com_ok(chn, echn, cmd, "STRMid")) {
                               char p1[32] ,p2[32];
                               if (nextp(p1, 31) && nextp(p2, 31))
                               {
                                 unsigned n1 = atoi(p1);
                                 unsigned n2 = atoi(p2);
                                 if ((int)n1 >= 0 && (int)n2 >= 0)
                                 {
                                   whole(gps);
                                   if (n1 < strlen(gps))
                                   {
                                     char *p = gps + n1;
                                     strncpy(result, p, n2);
                                     result[n2] = '\0';
                                   }
                                 }
                               }
                               else strcpy(result, "Index and length required");
                               ok = true;
                             }

  if (com_ok(chn, echn, cmd, "STRLeft")) {
                               char p1[32];
                               if (is_next())
                               {
                                 nextp(p1, 31);
                                 unsigned n1 = atoi(p1);
                                 if ((int)n1 >= 0)
                                 {
                                   whole(gps);
                                   strncpy(result, gps, n1);
                                   result[n1] = '\0';
                                 }
                               }
                               else strcpy(result, "Length required");
                               ok = true;
                             }

  if (com_ok(chn, echn, cmd, "STRRight")) {
                               char p1[32];
                               if (is_next())
                               {
                                 nextp(p1, 31);
                                 int n1 = atoi(p1);
                                 if ((int)n1 >= 0)
                                 {
                                   whole(gps);
                                   n1 = strlen(gps) - n1;
                                   if (n1 < 0) n1 = 0;
                                   char *p = gps + n1;
                                   strcopy(result, p, 256);
                                 }
                               }
                               else strcpy(result, "Length required");
                               ok = true;
                             }

  if (com_ok(chn, echn, cmd, "STRLen")) {
                               whole(gps);
                               snprintf(result, 255, "%i", (int)strlen(gps));
                               ok = true;
                             }

  if (com_ok(chn, echn, cmd, "STRPos")) {
                               char p1[256];
                               if (is_next())
                               {
                                 nextp(p1);
                                 whole(gps);
                                 char *p = strstr(gps, p1);
                                 if (p == NULL) strcpy(result, "-1");
                                           else snprintf(result, 255, "%i", (int)(p-gps));
                               }
                               else strcpy(result, "Substring required");
                               ok = true;
                             }

  if (com_ok(chn, echn, cmd, "UPCASE")) {
                               whole(gps);
                               char *p = gps;
                               while (*p) {*p = toupper(*p); p++;}
                               strcopy(result, gps, 256);
                               ok = true;
                             }
  
  //----- System commands ----
  if (com_ok(chn, echn, cmd, "RCMD")) {
                                  if (is_next())
                                  {
                                    aclist[chn].erase(aclist[chn].begin(),
                                                      aclist[chn].end());
                                    while (is_next())
                                    {
                                      nextp(gps);
                                      aclist[chn].push_back(gps);
                                    }
                                  }
                                  else
                                  {
                                    std::vector <RCommand>::iterator it;
                                    for (it = aclist[chn].begin();
                                         it < aclist[chn].end();
                                         it++)
                                    {
                                      strcat(result, it->name);
                                      strcat(result, " ");
                                    }
                                  }
                                  ok = true;
                                }
  if (com_ok(chn, echn, cmd, "RESULT")) {nextp(result); ok = true;}
  if (com_ok(chn, echn, cmd, "MACRO")) ok = true;
  if (com_ok(chn, echn, cmd, "LABEL")) ok = true;
  if (com_ok(chn, echn, cmd, "ERRORLOG")) {
                                 if (is_next())
                                 {
                                   nextp(gps);
                                   if (!redirect_errorlog(gps))
                                     strcpy(result, "Cannot redirect");
                                 }
                                 ok = true;
                                }
  if (com_ok(chn, echn, cmd, "SELCHn")) {
                               char *endptr;
                               nextp(gps);
                               int a = strtol(gps, &endptr, 10);
                               if (*endptr == '\0' && a <= MAX_CHN)
                                 emit(a, EV_SELECT_CHN, 0, NULL);
                               else
                                 strcpy(result, "Invalid channel number");
                               ok = true;
                             }
  if (com_ok(chn, echn, cmd, "SYSREQ")) {
                               nextp(gps);
                               int a = atoi(gps);
                               emit(chn, EV_SYSREQ, a, NULL);
                               ok = true;
                             }

  //Try to find unknown cmd in registered commands
  if (!ok)
  {
    std::vector <Command>::iterator it;
    for (it = reg.begin(); it < reg.end(); it++)
      if (com_ok(chn, echn, cmd, it->cmd))
      {
         char *s = new char[maxlen];
         whole(s);
         char *p = new char[strlen(it->cmd)+strlen(s)+2];
         sprintf(p, "%s %s", it->cmd, s);
         emit(chn, EV_APP_COMMAND, 0, p);
         delete[] p;
         delete[] s;
         ok = true;
         break;
      }
  }

  //Try to find unknown cmd in "./macro"
  if (!ok)
  {
    std::vector <Command>::iterator it;
    char *lang = get_var(chn, "STN_LANG");
    if (lang != NULL) //try to find in specified language
    {
      if (strcmp(lang, lang_name) != 0) load_language(lang);

      for (it = lmac.begin(); it < lmac.end(); it++)
        if (com_ok(chn, echn, cmd, (*it).cmd)) {ok = exec_mac(chn, (*it).name, (*it).flags, lang); break;}
      if (!ok) //not in database, try "<command>.mac"
      {
        ok = exec_mac(chn, cmd, NULL, lang);
        if (ok) send_res = false;
      }
    }

    if (!ok) //try to find in default language
    {
      for (it = mac.begin(); it < mac.end(); it++)
        if (com_ok(chn, echn, cmd, (*it).cmd)) {ok = exec_mac(chn, (*it).name, (*it).flags); break;}
      if (!ok)
      {
        ok = exec_mac(chn, cmd, NULL);
        if (ok) send_res = false;
      }
    }
  }

  //Try to found unknown cmd in "./bin"
  if (!ok)
  {
    std::vector <Command>::iterator it;
    for (it = bin.begin(); it < bin.end(); it++)
      if (com_ok(chn, echn, cmd, (*it).cmd))
        if (!remote || strchr((*it).flags, 'L') == NULL) //isn't it only local cmd ?
        {
          exec_bin(chn, (*it).name, (*it).flags);
          ok = true;
          send_res = false;
          break;
        }
  }

  if (!ok) snprintf(result, 255, "Unknown command (%s). Try :help.", cmd);
  //Treat command results
  if (strlen(result) != 0 && !send_res)
  {
    if (remote)
    {
      char *s = new char[strlen(PACKAGE)+strlen(result)+4];
      sprintf(s, "%s: %s\r", PACKAGE, result);
      emit(chn, EV_TEXT_COOKED, 0, s);
      emit(chn, EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL);
      delete[] s;
    }
    else
      emit(chn, EV_EDIT_INFO, strlen(result), result);
  }
}

void Commander::exec_bin(int chn, char *name, char *flags)
{
  bool rem = remote;
  char *p = new char[strlen(cmdline)*4+4];
  whole_quot(p, (strchr(flags, 'P') != NULL) && bconfig("fixpath"));
  char *s = new char[strlen(p) + strlen(name) + 10];
  sprintf(s, "./bin/%s %s", name, p);
  if (strchr(flags, 'R') != NULL) rem = true;
  ext = new ExternCmd(chn, s, rem, res_hnd, flags);
  emit(chn, EV_INSERT_OBJ, 0, ext);
  delete[] s;
  delete[] p;
}

bool Commander::exec_mac(int chn, char *name, char *flags, char *lang)
{
  char s[256];
  if (lang == NULL) snprintf(s, 255, "./macro/%s.mac", name);
               else snprintf(s, 255, "./macro/%s/%s.mac", lang, name);

  int oldpos = pos;
  char **argv = new char*[10];
  argv[0] = new char[strlen(name)+1];
  strcpy(argv[0], name);
  int n = 1;
  while (is_next() && n < 10)
  {
    char p[256];
    nextp(p);
    argv[n] = new char[strlen(p)+1];
    strcpy(argv[n], p);
    n++;
  }
  mcm = new Macro(chn, s, n, argv);
  if (mcm->exitcode != 1)
  {
    emit(chn, EV_INSERT_OBJ, 0, mcm);
    emit(chn, EV_MACRO_STARTED, 0, mcm);
    return true;
  }
  else
  {
    delete[] argv;
    pos = oldpos;
  }

  return false;
}

void Commander::command_channel(char *cmd, int *chn)
{
  char *p = strchr(cmd, '@');
  if (p != NULL)
  {
    *p = '\0';
    p++;
    *chn = atoi(p);
  }
}

void Commander::bool_set(bool *b)
{
  if (is_next())
  {
    char s[256];
    nextp(s);
    for (char *p = s; *p; p++) *p = toupper(*p);
    if (strcmp(s, "ON") == 0 || strcmp(s, "1") == 0) *b = true;
    else if (strcmp(s, "OFF") == 0 || strcmp(s, "0") == 0) *b = false;
    else strcpy(result, "Unknown switch");
  }
  else
    if (*b) strcpy(result, "ON"); else strcpy(result, "OFF");
}

void Commander::bool_set_config(const char *name)
{
   bool b = bconfig(name);
   bool_set(&b);
   setBConfig(name, b);
}

void Commander::int_set(int *i, int lowest, int highest)
{
  if (is_next())
  {
    char s[256];
    char *endptr;
    int value;
    nextp(s);
    value = strtol(s, &endptr, 10);
    if (*endptr == '\0')
    {
      if (value >= lowest && value <= highest) *i = value;
      else strcpy(result, "Invalid value");
    }
    else strcpy(result, "Numeric value excepted");
  }
  else
    snprintf(result, 255, "%i", *i);
}

void Commander::int_set_config(const char *name, int lowest, int highest)
{
   int i = iconfig(name);
   int_set(&i, lowest, highest);
   setIConfig(name, i);
}

//=========================== TERMINAL COMMANDS ============================
void Commander::do_connect(int chn)
{
  whole(gps);
  char port[256];
  char *p = strchr(gps, ':');
  if (p == NULL)
  {
     p = gps;
     char *def = get_var(chn, "CHN_PORT");
     if (def) strcopy(port, def, 256);
     else strcopy(port, sconfig("def_port"), 256);
  }
  else
  {
     *p = '\0';
     strcopy(port, gps, 256); p++;
  }

  char *adr = new char[strlen(port)+strlen(p)+2];
  sprintf(adr, "%s:%s", port, p);
  emit(chn, EV_CONN_LOC, 0, adr);
  delete[] adr;
}

void Commander::echo(int chn)
{
  whole(gps);
  strcat(gps, "\r");
  if (remote) emit(chn, EV_DATA_OUTPUT, strlen(gps), gps);
         else emit(chn, EV_LOCAL_MSG, strlen(gps), gps);
}

void Commander::version(int chn)
{
  static char s[256];
  #ifndef NEW_AX25
  sprintf(s, "\r%s / %s\rVersion %s (Compiled on %s)\r%s\r\r",
          PACKAGE, sys_info(), VERSION, __DATE__, VERINFO);
  #else
  sprintf(s, "\r%s / %s\rVersion %s (Compiled on %s) (new AX.25)\r%s\r\r",
          PACKAGE, sys_info(), VERSION, __DATE__, VERINFO);
  #endif
  emit(chn, EV_TEXT_COOKED, 0, s);
  emit(chn, EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL);
}

//=========================================================================
// Class Macro
//=========================================================================
Macro::Macro(int chnum, char *fname, int m_argc, char **m_argv)
{
  sprintf(class_name, "Macro");
  chn = chnum;
  exitcode = 0;
  argc = m_argc;
  argv = m_argv;
  pos = 0;
  max = 0;
  macro = false;
  waiting = false;
  childs = 0;
  sleep_stp = 0;

  FILE *f;
  f = fopen(fname, "r");  //load all lines
  if (!f)
  {
    exitcode = 1;
    return;
  }
  while (!feof(f))
  {
    char pline[1024];
    strcpy(pline, "");
    if (fgets(pline, 1023, f) != NULL)
    {
      striplf(pline);

      char s[1024];
      strcopy(s, pline, 1024);
      
      if (macro) convert_line(s);
      if (strcasecmp(s, ":MACRO") == 0 || strncasecmp(s, ":MACRO ", 7) == 0)
        macro = true;
      if (strlen(s) != 0 || (!feof(f) && !macro))
      {
        char *ins = new char[strlen(s)+1];
        strcpy(ins, s);
        prg.push_back(ins);
        max++;
      }
    }
  }
  fclose(f);

  //Handle special paramatres
  if (argc > 1 && argv[1][0] == '@')
  {
    char s[256];
    std::vector <char *>::iterator it;
    it = prg.begin();
    if (macro) it++;
    snprintf(s, 255, ":GOTO %s", &(argv[1][1]));
    char *ins = new char[strlen(s)+1];
    strcpy(ins, s);
    prg.insert(it, ins);
    for (int i=2; i < argc; i++) argv[i-1] = argv[i];
  }

  index_conds();
}

void Macro::convert_line(char *s)
{
  char *p;
  if (strlen(s) != 0)
  {
    //Remove coments
    p = strstr(s, ";;"); if (p != NULL) *p = '\0';
    //Remove initial spaces
    p = s; while (isspace(*p)) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    //Remove terminal spaces
    while (strlen(s) > 0 && s[strlen(s)-1] == ' ') s[strlen(s)-1] = '\0';
    //Add 'LABEL' command to labels
    if (s[0] == ':')
    {
      char *t = new char[strlen(s)+1];
      strcpy(t, s+1); sprintf(s, "LABEL %s", t);
      delete[] t;
    }
  }
  //ECHO command replace with text only
  if (strncasecmp(s, "ECHO ", 5) == 0)
  {
    unsigned pos;
    char par[256];
    char t[256];
    bool first = true;

    strcpy(t, "");
    pos = 0;
    while (pos < strlen(s))
    {
      read_param(s, par, pos, 255);
      if (!first)
      {
        strcat(t, par);
        if (pos < strlen(s)) strcat(t, " ");
      }
      first = false;
    }
    strcpy(s, t);
  }
  else if (strcasecmp(s, "ECHO") == 0)
    strcpy(s, " ");
  //Normal command
  else if (strlen(s) > 0)
  {
    char *tmp = new char[strlen(s)+1];
    strcpy(tmp, s);
    sprintf(s, ":%s", tmp);
    delete[] tmp;
  }
}

void Macro::handle_event(const Event &ev)
{
  if (ev.type == EV_NONE && !waiting && childs == 0)
  {
    if (pos < max)
    {
      strcpy(s, prg[pos]);
      if (s[strlen(s)-1] == '\\')
        s[strlen(s)-1] = '\0';
      else
        strcat(s, "\n");

      args.fromArray(argc, argv);
      emit(chn, EV_TEXT_ARGS, args.length(), args.toBuffer());
      emit(chn, EV_TEXT_RAW, FLAG_MACRO, s);

      if (s[0] == ':') waiting = true;
      else pos++;
    }
    else
    {
      emit(chn, EV_TEXT_FLUSH, FLAG_FLUSH_IMMEDIATE, NULL);
      emit(chn, EV_REMOVE_OBJ, oid, this);
      emit(chn, EV_MACRO_FINISHED, 0, this);
    }
  }

  if (ev.type == EV_TEXT_COOKED && ev.chn == chn && ev.x == FLAG_MACRO && waiting)
  {
    char *data_line = new char[strlen((char *)ev.data) + 1];
    strcpy(data_line, (char *)ev.data);

    if (!macro_command(data_line))
    {
      Event re;
      re = ev;
      re.x = FLAG_FM_MACRO;
      emit(re);
    }
    waiting = false;
    pos++;
    delete[] data_line;
  }

  if (ev.type == EV_MACRO_STARTED && ev.chn == chn && ev.data != this)
  {
    childs++;
  }

  if (ev.type == EV_MACRO_FINISHED && ev.chn == chn && ev.data != this)
  {
    childs--;
  }
}

int Macro::label_line(char *label)
{
  std::vector <char *>::iterator it;
  int line = 0;
  for (it = prg.begin(); it < prg.end(); it++, line++)
    if (strncasecmp(*it, ":LABEL ", 7) == 0)
    {
      char *p = *it + 7;
      while (isspace(*p) && *p != '\0') p++;
      if (strcmp(p, label) == 0) return line;
    }
  return -1;
}

bool Macro::macro_command(char *s)
{
  if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
  char *cmd = new char[strlen(s) +1];
  char *par = s+1;
  while (*par && !isspace(*par)) par++;
  if (*par)
  {
    *par = '\0'; strcpy(cmd, s); *par = ' ';
    while(*par && isspace(*par)) par++;
  }
  else strcpy(cmd, s);

  if (strcasecmp(cmd, ":RETURN") == 0)
  {
    pos = max;
    return true;
  }

  if (strcasecmp(cmd, ":GOTO") == 0)
  {
    char *endptr;
    int line = strtol(par, &endptr, 10);
    if (*endptr == '\0') pos = line - 1;
    else if ((line = label_line(par)) != -1) pos = line - 1;
    else
    {
      send_result(chn, "GOTO (%i) : Unknown destination (%s)", pos, par);
    }
    return true;
  }

  if (strcasecmp(cmd, ":IF") == 0)
  {
    char *endptr;
    int line = strtol(par, &endptr, 10);
    if (*endptr != ' ')
    {
      send_result(chn, "IF (%i) : Invalid internal index", pos);
      return true;
    }
    while (isspace(*endptr) && *endptr) endptr++;
    if (!true_condition(endptr)) pos = line - 1;
    return true;
  }

  if (strcasecmp(cmd, ":WAITFOR") == 0)
  {
    if (!true_condition(par)) pos = pos - 1;
    return true;
  }

  if (strcasecmp(cmd, ":SLEEP") == 0)
  {
    if (sleep_stp == 0)
    {
      char *endptr;
      int value = strtol(par, &endptr, 10);
      if (*endptr && *endptr != ' ')
      {
        send_result(chn, "SLEEP (%i) : Invalid value", pos);
        return true;
      }
      sleep_stp = time(NULL) + value;
      pos -= 1;
    }
    else
    {
      if (time(NULL) < sleep_stp) pos -= 1;
      else sleep_stp = 0;
    }

    return true;
  }

  delete[] cmd;
  return false;
}

void Macro::index_conds()
{
  /* User IF ~ ELSE ~ ENDIF syntax:
       IF <condition>
         ... commands ...
       ELSE
         ... commands ...
       ENDIF (=FI)

       IF (<condition>) command      - short

     Internal representation:
       IF <line_to_skip_to_when_false> <condition>
         ... commands ...
  */
  Stack <int> stk;
  std::vector <char *>::iterator it;
  std::vector <char *> newprg;
  int line = 0;
  int destline = 0;
  char s[256];

  for (it = prg.begin(); it < prg.end(); it++, line++)
  {
    strcopy(s, *it, 256);
    //Read command name and parametres
    if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
    char *cmd = new char[strlen(s) +1];
    char *par = s+1;
    while (*par && !isspace(*par)) par++;
    if (*par)
    {
      *par = '\0'; strcpy(cmd, s); *par = ' ';
      while(*par && isspace(*par)) par++;
    }
    else strcpy(cmd, s);

    if (strcasecmp(cmd, ":IF") == 0)
    {
      if (par[0] == '(') //short variant - can be indexed immediatelly
      {
        char cond[256];  //separate condition
        strcpy(cond, "");
        char *p = par+1;
        int z = 1;
        while (z > 0)
        {
          if (*p == '(') z++;
          if (*p == ')') z--;
          if (z != 0) strncat(cond, p, 1);
          p++;
        }
        p++; while (isspace(*p) && *p) p++;

        char *newline;
        char newln[256];
        sprintf(newln, ":IF %i %s", destline + 2, cond); //store IF command
        newline = new char[strlen(newln)+1]; strcpy(newline, newln);
        newprg.push_back(newline);

        sprintf(newln, "%s", p);  //store action command
        if (macro) convert_line(newln);
        newline = new char[strlen(newln)+1]; strcpy(newline, newln);
        newprg.push_back(newline);

        destline += 2;
      }
      else //long format - it will be indexed later
      {
        char *newln = new char[strlen(par)+1]; strcpy(newln, par);
        newprg.push_back(newln);
        stk.push(destline);
        destline++;
      }
    }
    else
    if (strcasecmp(cmd, ":ELSE") == 0)
    {
      if (stk.isEmpty())
        send_result(chn, "ELSE (%i) : No matching IF", pos);
      else
      {
        int ifline = stk.top(); stk.pop();
        char newline[256];
        sprintf(newline, ":IF %i %s", destline+1, newprg[ifline]);
        char *newln = new char[strlen(newline)+1]; strcpy(newln, newline);
        //char *newln = strdup(newline);
        delete[] newprg[ifline];
        newprg[ifline] = newln;
        stk.push(destline);
        destline++; //leave space for ELSE
        newprg.push_back((char *)NULL);
      }
    }
    else
    if (strcasecmp(cmd, ":ENDIF") == 0)
    {
      if (stk.isEmpty())
        send_result(chn, "ENDIF (%i) : No matching IF/ELSE", pos);
      else
      {
        int ifline = stk.top(); stk.pop();
        char newline[256];
        if (newprg[ifline] == NULL)
           sprintf(newline, ":GOTO %i", destline);
        else
        {
           sprintf(newline, ":IF %i %s", destline, newprg[ifline]);
           delete[] newprg[ifline];
        }
        char *newln = new char[strlen(newline)+1]; strcpy(newln, newline);
        newprg[ifline] = newln;
      }
    }
    else //no condition - copy line
    {
      char *newln = new char[strlen(*it)+1]; strcpy(newln, *it);
      newprg.push_back(newln);
      destline++;
    }

    delete[] cmd;
  }

  if (!stk.isEmpty())
  {
    send_result(chn, "Warning: Some 'IF' statements with no matching ENDIF");
  }

#ifdef LINPAC_DEBUG
  if (DEBUG.macro)
  {
    fprintf(stderr, "***\n");
    line = 0;
    for (it = newprg.begin(); it < newprg.end(); it++, line++)
      fprintf(stderr, "%3i: %s\n", line, *it);
  }
#endif

  clear_prg(prg);
  max = destline;
  prg = newprg;
}

int Macro::compare(const char *s1, const char *s2)
{
  if (is_number(s1) && is_number(s2))
  {
    int i1 = atoi(s1);
    int i2 = atoi(s2);
    if (i1 == i2) return 0;
    if (i1 < i2) return -1;
    if (i1 > i2) return 1;
  }
  return strcmp(s1, s2);
}

bool Macro::true_condition(char *s)
{
  char o1[256], o2[256], op[10];
  unsigned pos = 0;
  bool b;
  read_param(s, o1, pos);
  read_param(s, op, pos);
  read_param(s, o2, pos);
  if (strcmp(op, "=") == 0)  b=(compare(o1, o2) == 0);
  if (strcmp(op, "==") == 0)  b=(compare(o1, o2) == 0);
  if (strcmp(op, ">") == 0)  b=(compare(o1, o2) > 0);
  if (strcmp(op, "<") == 0)  b=(compare(o1, o2) < 0);
  if (strcmp(op, ">=") == 0)  b=(compare(o1, o2) >= 0);
  if (strcmp(op, "=>") == 0)  b=(compare(o1, o2) >= 0);
  if (strcmp(op, "<=") == 0)  b=(compare(o1, o2) <= 0);
  if (strcmp(op, "=<") == 0)  b=(compare(o1, o2) <= 0);
  if (strcmp(op, "!=") == 0)  b=(compare(o1, o2) != 0);
  if (strcmp(op, "<>") == 0)  b=(compare(o1, o2) != 0);
  return b;
}

void Macro::clear_prg(std::vector <char *> &prg)
{
  std::vector <char *>::iterator it;
  for (it = prg.begin(); it < prg.end(); it++)
    delete[] *it;
  prg.erase(prg.begin(), prg.end());
}

Macro::~Macro()
{
  delete[] argv;
  clear_prg(prg);
}

