/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (radkovo@centrum.cz) 1998 - 2002

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   watch.cc

   Objects for automatic command execution

   Last update 21.6.2002
  =========================================================================*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "constants.h"
#include "data.h"
#include "tools.h"
#include "watch.h"

//--------------------------------------------------------------------------
// Class Watch
//--------------------------------------------------------------------------
Watch::Watch()
{
  strcpy(class_name, "Watch");
  for (int i=0; i<=MAX_CHN; i++)
  {
    buf[i][0] = '\r';
    buflen[i] = 1;
    disabled[i] = false;
  }
  strcpy(lastcall, "");
  strcpy(cinit, ":cinit\n");
  strcpy(ctext, ":ctext\n");
  strcpy(cexit, ":cexit\n");
}

void Watch::handle_event(const Event &ev)
{
  if (ev.type == EV_DATA_INPUT && !disabled[ev.chn])
  {
    char *data = reinterpret_cast <char *>(ev.data);
    if (buflen[ev.chn] + ev.x >= WATCH_BUFFER_SIZE) //make place for incoming data
    {
      memmove(buf[ev.chn], &(buf[ev.chn][ev.x]), buflen[ev.chn] - ev.x);
      buflen[ev.chn] -= ev.x;
    }
    for (int i = 0; i < ev.x; i++)
    {
      buf[ev.chn][buflen[ev.chn]] = data[i];
      buflen[ev.chn]++;
    }
    //try to find all keys in bufer
    std::vector <autorun_entry>::iterator it;
    for (it = watch.begin(); it < watch.end(); it++)
    {
      if ((it->chn == ev.chn || it->chn == 0) && key_found(ev.chn, it->key))
      {
        priority_max(); //go to catch the trigger frame
        emit(ev.chn, EV_TEXT_RAW, 0, it->command);
      }
    }
  }

  if (ev.type == EV_ADD_WATCH)
  {
    autorun_entry *entry = reinterpret_cast<autorun_entry *>(ev.data);
    watch.push_back(*entry);
  }

  if (ev.type == EV_DISABLE_SCREEN) disabled[ev.chn] = true;
  if (ev.type == EV_ENABLE_SCREEN) disabled[ev.chn] = false;

  if (ev.type == EV_DISC_FM || ev.type == EV_DISC_TIME || ev.type == EV_FAILURE)
  {
    if (disabled[ev.chn]) emit(ev.chn, EV_ENABLE_SCREEN, 0, NULL);
    char cmd[256];
    sprintf(cmd, "%s \"%s\"", cexit, (char *)ev.data);
    emit(ev.chn, EV_TEXT_COOKED, 0, cmd); //run exit script
  }

  if (ev.type == EV_CONN_TO || ev.type == EV_RECONN_TO) //when connection ensures
  {
    char cmd[256];
    sprintf(cmd, "%s \"%s\" \"%s\" \"%s\"", cinit, (char *)ev.data, lastcall,
            (ev.type == EV_CONN_TO || ev.x == 1)?"connect":"reconnect");
    strcopy(lastcall, (char *)ev.data, 15);
    emit(ev.chn, EV_TEXT_COOKED, 0, cmd);
    //when the other station initiated connection send ctext
    if (ev.type == EV_CONN_TO && ev.x != 0)
         emit(ev.chn, EV_TEXT_COOKED, 0, ctext);
  }

  //check for connection commands
  if (ev.type == EV_TEXT_COOKED)
  {
    char *s = reinterpret_cast<char *>(ev.data);
    if (com_is(s, "Connect") || com_is(s, "Mailbox") || com_is(s, "BBs"))
      csent[ev.chn] = true;
  }

  if (ev.type == EV_LINE_RECV)
  {
    char *p;
    char s[256];
    strcopy(s, reinterpret_cast<char *>(ev.data), 256);
    for (p = s; *p; p++) *p = toupper(*p);
    if ((p=strstr(s, "RECONNECTED TO ")) != NULL)
    {
      p += 15;
      char newc[12];
      extract_call(p, newc, 10);
      normalize_call(newc);
      setSConfig(ev.chn, "cwit", newc);
      emit(ev.chn, EV_RECONN_TO, 0, newc);
      csent[ev.chn] = false;
    }
    else if (csent[ev.chn] && (p=strstr(s, "CONNECTED TO ")) != NULL)
    {
      p += 13;
      char newc[12];
      extract_call(p, newc, 10);
      normalize_call(newc);
      setSConfig(ev.chn, "cwit", newc);
      emit(ev.chn, EV_RECONN_TO, 0, newc);
      csent[ev.chn] = false;
    }
  }
}

bool Watch::key_found(int chn, char *key)
{
  int pos = 0;
  int keypos = 0;
  int ret_pos = 0;
  int keylen = strlen(key);

  while (pos < buflen[chn] && keypos < keylen)
  {
    if (buf[chn][pos] == key[keypos])
    {
      pos++; keypos++;
    }
    else
    {
      ret_pos++;
      pos = ret_pos; keypos = 0;
    }
  }

  if (keypos >= keylen)
  {
    buf[chn][pos-1] = '\0'; //we don't want to find it next time
    return true;
  }
  return false;
}

int Watch::com_is(char *s1, char *s2)
{
  unsigned i,j,k;

  for (i=0, k=0; i<strlen(s1); i++)
    if (isalnum(s1[i])) k++; else break;
  if (k>strlen(s2)) return 0;
  //How many capital letters
  for (i=0, j=0; i<strlen(s2); i++)
    if (s2[i]==toupper(s2[i])) j++;
  if (k<j) return 0;
  //Match all capitals
  for (i=0, j=1; i<k; i++)
    if (toupper(s1[i])!=toupper(s2[i])) j=0;
  return j;
}

void Watch::extract_call(const char *from, char *to, int len)
{
    char *buf = new char[strlen(from)+1];
    strcpy(buf, from);
    char *p = buf;
    while (*p && isspace(*p)) p++;
    char *q = p;
    while (*q && !isspace(*q)) q++;
    *q = '\0';
    char *col = strchr(p, ':');
    if (col)
    {
        col++;
        strcopy(to, col, len);
    }
    else strcopy(to, p, len);
    delete[] buf;
}

