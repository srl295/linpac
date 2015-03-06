/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998 - 1999

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   data.cc

   Common data

   Last update 29.1.2001
  =========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <ctime>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/ax25.h>
#include <stdlib.h>
#include "version.h"
#include "tools.h"
#include "evaluate.h"
#include "hash.h"
#include "data.h"

//-------------------------------- debug info ----------------------------
#ifdef LINPAC_DEBUG
struct Debug DEBUG;
#endif

//============================ Variable hash ==============================

Hash::Hash()
{
   hash = create_hash();
}

Hash::~Hash()
{
   destroy_hash(hash);
}

void Hash::setValue(const char *name, const char *value)
{
   hash_set(hash, name, value);
   int esize = strlen(name)+strlen(value)+2;
   char *edata = new char[esize];
   strcpy(edata, name);
   strcpy(edata+strlen(name)+1, value);
   emit(chn, EV_VAR_CHANGED, esize, edata);
   delete[] edata;
}

char *Hash::getValue(const char *name)
{
   return hash_get(hash, name);
}

void Hash::setSValue(const char *name, const char *value)
{
   setValue(name, value);
}

void Hash::setIValue(const char *name, int value)
{
   char sval[32];
   snprintf(sval, 31, "%i", value);
   setValue(name, sval);
}

void Hash::setBValue(const char *name, bool value)
{
   char sval[3];
   sprintf(sval, "%s", value?"1":"0");
   setValue(name, sval);
}

char *Hash::getSValue(const char *name)
{
   return getValue(name);
}

int Hash::getIValue(const char *name)
{
   char *p = getValue(name);
   if (p) return atoi(p);
   return 0;
}

bool Hash::getBValue(const char *name)
{
   char *p = getValue(name);
   if (p) return (strcmp(p, "1") == 0);
   return false;
}

void Hash::remove(const char *name)
{
   delete_element(hash, name);
   emit(chn, EV_VAR_DESTROYED, 0, (void *)name);
}

void Hash::removeVars(const char *names)
{
   struct h_name_list *list = get_name_list(hash);
   struct h_name_list *act;
   for (act = list; act; act = act->next)
      if (strstr(act->name, names) == act->name)
         remove(act->name);
   destroy_name_list(list);
}

struct h_name_list *Hash::getNames()
{
   return get_name_list(hash);
}

//============================= Environment ===============================
static Hash *env;
static struct h_name_list *enames = NULL, *namestart = NULL;

void init_env()
{
   env = new Hash[MAX_CHN+1];
   for (int i = 0; i <= MAX_CHN; i++) env[i].setChannel(i);
}

void var_channel(const char *name, int *chn, char *newname)
{
   strcpy(newname, name);
   char *p = strchr(newname, '@');
   if (p != NULL)
   {
      *p = '\0';
      p++;
      *chn = atoi(p);
   }
}

char *get_var(int chn, const char *name)
{
   char *pname = strdup(name);
   var_channel(name, &chn, pname);
   char *r = env[chn].getSValue(pname);
   delete[] pname;
   return r;
}

void del_var(int chn, const char *name)
{
   char *pname = strdup(name);
   var_channel(name, &chn, pname);
   env[chn].remove(pname);
   delete[] pname;
}

void clear_var_names(int chn, const char *names)
{
   env[chn].removeVars(names);
}

void set_var(int chn, const char *name, const char *content)
{
   char *pname = strdup(name);
   var_channel(name, &chn, pname);
   env[chn].setSValue(pname, content);
   delete[] pname;
}

void close_env()
{
   delete[] env;
}

char *sconfig(int chn, const char *name)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   char *r = env[chn].getSValue(nm);
   delete[] nm;
   return r;
}

char *sconfig(const char *name)
{
   return sconfig(0, name);
}

int iconfig(int chn, const char *name)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   int r = env[chn].getIValue(nm);
   delete[] nm;
   return r;
}

int iconfig(const char *name)
{
   return iconfig(0, name);
}

bool bconfig(int chn, const char *name)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   bool r = env[chn].getBValue(nm);
   delete[] nm;
   return r;
}

bool bconfig(const char *name)
{
   return bconfig(0, name);
}

void setSConfig(int chn, const char *name, const char *value)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   env[chn].setSValue(nm, value);
   delete[] nm;
}

void setSConfig(const char *name, const char *value)
{
   setSConfig(0, name, value);
}

void setIConfig(int chn, const char *name, int value)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   env[chn].setIValue(nm, value);
   delete[] nm;
}

void setIConfig(const char *name, int value)
{
   setIConfig(0, name, value);
}

void setBConfig(int chn, const char *name, bool value)
{
   char *nm = new char[strlen(name)+2];
   sprintf(nm, "_%s", name);
   env[chn].setBValue(nm, value);
   delete[] nm;
}

void setBConfig(const char *name, bool value)
{
   setBConfig(0, name, value);
}

void scanVarNames(int chn)
{
   if (enames) releaseVarNames();
   enames = env[chn].getNames();
   namestart = enames;
}

char *nextVarName()
{
   char *p = NULL;
   if (enames)
   {
      p = enames->name;
      enames = enames->next;
   }
   return p;
}

void releaseVarNames()
{
   destroy_name_list(namestart);
}

//============================= Acces lists ===============================

RCommand::RCommand()
{
  strcpy(name, "");
}

RCommand::RCommand(char *src)
{
  strcopy(name, src, 16);
}

RCommand &RCommand::operator = (const RCommand &src)
{
  strcpy(name, src.name);
  return *this;
}

RCommand &RCommand::operator = (char *src)
{
  strcopy(name, src, 16);
  return *this;
}

RCommand::operator char*()
{
  return name;
}

std::vector <RCommand> aclist[MAX_CHN+1];


//Misc
char term[MAX_CHN+1][10];
bool ch_disabled[MAX_CHN+1];
bool huffman_comp[MAX_CHN+1];
int sock[MAX_CHN+1];

void clear_last_act()
{
   setIConfig("last_act", time(NULL));
}

//--------------------------------------------------------------------------
// Connection info
//--------------------------------------------------------------------------
struct Ax25Status chstat;

bool get_axstat(const char *src, const char *dest, int form)
{
  int len=0;
  char buffer[256];
  char dama[7];
  char dest_addr[256]; //this can contain digipeater callsigns
  char source_addr[16];
  char dummy[10];
  FILE *f;

  if (form == 0 || form == 1) //read from /proc
  {
    f = fopen(PROCFILE, "r");
    if (!f) return false;
     
    while (!feof(f))
    {
      strcpy(source_addr, "");
      strcpy(buffer, "");
      if (fgets(buffer, 256, f) != NULL)
      {
        chstat.sendq = chstat.recvq = 0;
        if (form == 0) //2.0.x kernels
        {
          len=sscanf(buffer, "%s %s %s %d %d %d %d %d/%d %d/%d %d/%d %d/%d %d/%d %d %d %d %s %d %d",
            dest_addr, source_addr,
            chstat.devname,
            &chstat.state,
            &chstat.vs, &chstat.vr, &chstat.va,
            &chstat.t1, &chstat.t1max,
            &chstat.t2, &chstat.t2max,
            &chstat.t3, &chstat.t3max,
            &chstat.idle, &chstat.idlemax,
            &chstat.n2, &chstat.n2max,
            &chstat.rtt,
            &chstat.window,
            &chstat.paclen,
            dama,
            &chstat.sendq, &chstat.recvq);
            chstat.dama = !strcmp(dama, "slave");
        }
        if (form == 1) //2.2.x kernels
        {
          len=sscanf(buffer, "%s %s %s %s %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d",
            dummy,
            chstat.devname,
            source_addr, dest_addr,
            &chstat.state,
            &chstat.vs, &chstat.vr, &chstat.va,
            &chstat.t1, &chstat.t1max,
            &chstat.t2, &chstat.t2max,
            &chstat.t3, &chstat.t3max,
            &chstat.idle, &chstat.idlemax,
            &chstat.n2, &chstat.n2max,
            &chstat.rtt,
            &chstat.window,
            &chstat.paclen,
            &chstat.sendq, &chstat.recvq);
          //remove digis from dest_addr
          char *p = strchr(dest_addr, ','); if (p) *p = '\0';
        }
        if (call_match(dest_addr, dest) &&
            call_match(source_addr, src)) break;
      }
    }
    fclose(f);
    return (call_match(dest_addr, dest) && call_match(source_addr, src));
  }
  else if (form == 2) //read via ioctl
  {
      ax25_info_struct ax25_info;
      int chn;
      //find the channel
      for (chn = 1; chn <= iconfig("maxchn"); chn++)
        if (iconfig(chn, "state") != ST_DISC &&
            call_match(dest, sconfig(chn, "cphy")) &&
            call_match(src, sconfig(chn, "call"))) break;
      //return info
      if (chn > iconfig("maxchn")) return false; //not found
      if (ioctl(sock[chn], SIOCAX25GETINFO, &ax25_info) == 0)
      {
        #ifndef NEW_AX25
        chstat.t1max = ax25_info.t1;
        chstat.t2max = ax25_info.t2;
        chstat.t3max = ax25_info.t3;
        chstat.idlemax = ax25_info.idle;
        chstat.n2max = ax25_info.n2;
        chstat.t1 = ax25_info.t1timer;
        chstat.t2 = ax25_info.t2timer;
        chstat.t3 = ax25_info.t3timer;
        chstat.idle = ax25_info.idletimer;
        chstat.n2 = ax25_info.n2count;
        chstat.state = ax25_info.state;
        chstat.sendq = ax25_info.snd_q;
        chstat.recvq = ax25_info.rcv_q;
        chstat.vs = chstat.vr = chstat.va = chstat.rtt = chstat.paclen = 0;
        chstat.dama = false;
        strcpy(chstat.devname, "???");
        return true;
        #else //---------------------- NEW AX.25 --------------------------
        chstat.t1max = ax25_info.t1 / 10;
        chstat.t2max = ax25_info.t2 / 10;
        chstat.t3max = ax25_info.t3 / 10;
        chstat.idlemax = ax25_info.idle / 10;
        chstat.n2max = ax25_info.n2;
        chstat.t1 = ax25_info.t1timer / 10;
        chstat.t2 = ax25_info.t2timer / 10;
        chstat.t3 = ax25_info.t3timer / 10;
        chstat.idle = ax25_info.idletimer / 10;
        chstat.n2 = ax25_info.n2count;
        chstat.state = ax25_info.state;
        chstat.sendq = ax25_info.snd_q;
        chstat.recvq = ax25_info.rcv_q;
        chstat.vs = ax25_info.vs;
        chstat.vr = ax25_info.vr;
        chstat.va = ax25_info.va;
        chstat.rtt = ax25_info.vs_max;
        chstat.paclen = ax25_info.paclen;
        chstat.dama = false;
        strcpy(chstat.devname, "???");
        return true;
        #endif
      }
      else
      {
        Error(errno, "InfoLine: SIOCAX25GETINFO");
        return false;
      }
  }
  return false;
}


//--------------------------------------------------------------------------
// Class arguments
//--------------------------------------------------------------------------
Arguments::Arguments()
{
   cnt = 0;
   len = 0;
   data = NULL;
}

Arguments::~Arguments()
{
   if (data != NULL) delete[] data;
}
   
char *Arguments::argv(int index)
{
   if (index >= cnt) return NULL;
   int i = 0;
   char *p = data;
   while (i < index)
   {
      if (*p == '\0') i++;
      p++;
   }
   return p;
}

void Arguments::fromArray(int count, char **args)
{
   if (data != NULL) delete[] data;
   cnt = count;
   len = 0;
   for (int i = 0; i < cnt; i++)
      len = len + strlen(args[i]) + 1;
   data = new char[len];
   char *p = data;
   for (int i = 0; i < cnt; i++)
   {
      strcpy(p, args[i]);
      p += strlen(args[i]);
      *p = '\0';
      p++;
   }
}

void Arguments::fromBuffer(int length, char *buffer)
{
   if (data != NULL) delete[] data;
   len = length;
   data = new char[len];
   memcpy(data, buffer, len);
   cnt = 0;
   for (int i = 0; i < len; i++)
      if (data[i] == '\0') cnt++;
}

void Arguments::clearit()
{
   if (data != NULL) delete[] data;
   data = NULL;
   cnt = 0;
   len = 0;
}

//--------------------------------------------------------------------------
// Class Cooker
//--------------------------------------------------------------------------
Cooker::Cooker()
{
  strcpy(class_name, "Cooker");
  newhandle = 0;
}

bool Cooker::replace_macros(int chn, char *s, Arguments *args, char *res)
{
  char t[256];
  char f[256];
  char temp_expr[256];
  char *name;
  int n, i, z;
  bool finished;
  unsigned long tm;

  strcopy(f, s, 256);
  bool done = true;

  char *p = f;
  char *d = s;
  while (*p)
  {                          //done ~ stop replacing when something can't be finished
    while(*p && (*p != '%' || !done)) {*d++ = *p++;}
    if (*p == '%' && done)
    {
      char *src = ++p;
      char *dst = t;
      while (*src && (isalnum(*src) || *src == '_'
                      || *src == '@' || *src == '%')) *dst++ = *src++;
      *dst = '\0';
      finished = replace_macros(chn, t, args, res);
      if (!finished)
      {
        sprintf(t, "%%%s", t);
        done = false;
        dst = NULL;
      }
      else
      {
        dst = get_var(chn, t);
        if (dst != NULL)
        {
          strcopy(t, dst, 256);
          p = src;
        }
      }

      if (dst == NULL) //no variable - try to found other macro
      {
        int mchn = chn;
        
        if (*(p+1) == '@')
        {
          char num[256];
          char *pchn = p+2;
          strcpy(num, "");
          while (*pchn && (isalnum(*pchn) || *pchn == '_'
                           || *pchn == '@' || *pchn == '%'))
                               {strncat(num, pchn, 1); pchn++;}
          replace_macros(mchn, num, args, res);
          if (strlen(num) > 0) mchn = atoi(num);
          memmove(p+1, pchn, strlen(pchn)+1);
        }
        
        switch (toupper(*p))
        {
          case 'V': strcopy(t, VERSION, 256); break;
          case 'C': if (iconfig(mchn, "state") != ST_DISC)
                       strcopy(t, sconfig(mchn, "cwit"), 256);
                    else strcopy(t, sconfig(mchn, "call"), 256);
                    normalize_call(t);
                    break;
          case 'N': name = get_var(mchn, "STN_NAME");
                    if (name != NULL) strcopy(t, name, 256);
                    else {strcopy(t, sconfig("no_name"), 256); replace_macros(mchn, t);}
                    break;
          case 'Y': strcopy(t, sconfig(mchn, "call"), 256);
                    normalize_call(t);
                    break;
          case 'K': sprintf(t, "%i", mchn); break;
          case 'T': strcopy(t, time_stamp(), 256); break;
          case 'D': strcopy(t, date_stamp(), 256); break;
          case 'B': strcpy(t, "\a"); break;
          case 'Z': strcopy(t, sconfig("timezone"), 256); break;
          case 'U': strcopy(t, sconfig("no_name"), 256);
                    replace_macros(mchn, t);
                    break;
          case 'P': sprintf(t, "%i", iconfig(mchn, "port")); break;
          case 'M': n=0;
                    for (i=1; i<=iconfig("maxchn"); i++)
                      if (iconfig(i, "state") != ST_DISC) n++;
                    sprintf(t, "%i", n);
                    break;
          case 'A': tm = time(NULL);
                    sprintf(t, "%li", tm - iconfig("last_act"));
                    break;
          case '_': strcpy(t, "\r"); break;
          case '<': strcopy(t, lastline[mchn], 256);
                    strcpy(lastline[mchn], "");
                    break;
          case '!': if (res != NULL) strcopy(t, res, 256);
                                else strcpy(t, "");
                    break;
          case '#': char num[256];
                    strcpy(num, "");
                    p++;
                    while (isdigit(*p))
                    {
                      strncat(num, p, 1);
                      p++;
                    }
                    sprintf(t, "%c", atoi(num));
                    break;
          case '(': z = 1;
                    strcpy(temp_expr, "");
                    while (z > 0)
                    {
                      p++;
                      if (*p == '\0') z = 0;
                      if (*p == '(') z++;
                      if (*p == ')') z--;
                      if (z > 0 || *p == '\0') strncat(temp_expr, p, 1);
                    }
                    finished = replace_macros(mchn, temp_expr, args, res);
                    if (finished) //lower level finished, now it's our turn
                    {
                      strcpy(expr, temp_expr);
                      strcpy(t, "%!");
                    }
                    else
                    {
                      sprintf(t, "%%(%s)", temp_expr);
                    }
                    done = false;
                    break;
          case '[': char ex1[256], ex2[256];
                    strcpy(ex1, "");
                    p++;
                    while (*p && *p != ']')
                    {
                      strncat(ex1, p, 1);
                      p++;
                    }
                    finished = replace_macros(mchn, ex1, args, res);
                    if (finished)
                    {
                      Infix2Postfix(ex1, ex2);
                      sprintf(t, "%i", EvaluatePostfix(ex2));
                    }
                    else
                    {
                      sprintf(t, "%%[%s]", ex1); //delay evaluation for next round
                      done = false;
                    }
                    break;
          case 'R': if (args != NULL)
                      sprintf(t, "%i", args->argc());
                    else
                      strcpy(t, "0");
                    break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9': n = *p - '0';
                    if (args != NULL)
                    {
                      if (n < args->argc()) strcopy(t, args->argv(n), 256);
                      else strcpy(t, "");
                    }
                    break;
          case '*': strcpy(t, "");
                    for (i = 1; i < args->argc(); i++)
                    {
                      strcat(t, args->argv(i));
                      if (i < args->argc()-1) strcat(t, " ");
                    }
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
  return done;
}

void Cooker::handle_event(const Event &ev)
{
  if (ev.type == EV_TEXT_ARGS)
  {
     args.fromBuffer(ev.x, (char *)ev.data);
  }

  if (ev.type == EV_TEXT_RAW)
  {
    cook_task *newtask = new cook_task;
    strcopy(newtask->str, (char *)ev.data, 256);
    newtask->x = ev.x;
    if (ev.x != FLAG_MACRO) args.clearit();

    bool finished = replace_macros(ev.chn, newtask->str, &args);
    if (!finished)
    {
      strcopy(newtask->expr, expr, 256);
      newtask->handle = newhandle++;
      newtask->args = new Arguments;
      newtask->args->fromBuffer(args.length(), args.toBuffer());
      tasks.push_back(newtask);
      emit(ev.chn, EV_WANT_RESULT, newtask->handle, newtask->expr);
    }
    else
    {
      static char re_str[256];
      strcopy(re_str, newtask->str, 256);
      Event re = ev;
      re.type = EV_TEXT_COOKED;
      re.data = re_str;
      emit(re);
      delete newtask;
    }
    args.clearit();
  }
  if (ev.type == EV_CMD_RESULT)
  {
    std::vector <cook_task *>::iterator it;
    for (it = tasks.begin(); it < tasks.end(); it++)
      if ((*it)->handle == ev.x) break;
    if (it < tasks.end())
    {
      bool finished = replace_macros(ev.chn, (*it)->str, (*it)->args, (char *)ev.data);
      if (!finished)
      {
        strcopy((*it)->expr, expr, 256);
        emit(ev.chn, EV_WANT_RESULT, (*it)->handle, (*it)->expr);
      }
      else
      {
        static char re_str[256];
        strcopy(re_str, (*it)->str, 256);
        emit(ev.chn, EV_TEXT_COOKED, (*it)->x, re_str);
        delete (*it)->args;
        delete *it;
        tasks.erase(it);
      }
    }
  }
  if (ev.type == EV_LINE_RECV)
  {
    strcopy(lastline[ev.chn], reinterpret_cast<char *>(ev.data), 256);
    if (lastline[ev.chn][strlen(lastline[ev.chn])-1] == '\n')
      lastline[ev.chn][strlen(lastline[ev.chn])-1] = '\0';
  }
}

Cooker::~Cooker()
{
  std::vector <cook_task *>::iterator it;
  for (it = tasks.begin(); it < tasks.end(); it++)
  {
    delete (*it)->args;
    delete *it;
    tasks.erase(it);
  }
}

