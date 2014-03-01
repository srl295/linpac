/*
   catch - utility for examining data streams for known patterns
   (c) 1998 - 2000 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage: catch -m <message_to_user> -b <max_pattern_length> pattern command

   Pattern can contain wildcards ? (one character) and * (0-n) characters
   Command can contain $0 = whole pattern
                       $n = string responding to n-th wildcard (1..n)
   Maximal pattern length is by default 1024 bytes.
*/

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "lpapp.h"

//max. number of wildcards
#define MAX_WILDC 32

char message[256];  //Displayed message
char pattern[256];  //Search pattern
char origptrn[254]; //Original search pattern (w/o initial and terminal *)
char command[256];  //Command to execute

bool datain = false;
bool dataout = false;
bool dataloc = false;

char *buf;
long blen = 1024;
char *wildcard[MAX_WILDC];
int numw = 0;

bool abort_all = false;
bool done = false;

// Find the needle in haystack, needle can contain ? wildcards
char *str_wstr(const char *haystack, const char *needle)
{
  const char *p = haystack;
  while (strlen(p) >= strlen(needle))
  {
    bool fits = true;
    const char *q = p;
    const char *r = needle;
    while (*r)
    {
      if (*r != *q && *r != '?') { fits = false; break; }
      r++; q++;
    }
    if (fits) return (char *)p;
    p++;
  }
  return NULL;
}

// Scan the buffer, find the wildcards and check if the buffer matches
bool scan_buffer()
{
  char *p = pattern;
  char *b = buf;
  bool result = true;

  numw = 0;
  for (int i = 0; i < MAX_WILDC; i++)
    if (wildcard[i] != NULL) delete[] wildcard[i];

  while (*p)
  {
    while (*p && *b && *p == *b) { p++; b++; }
    if (*p == '*')
    {
      p++;
      //find the terminating sequence
      char *s = new char[blen+1];
      strcpy(s, "");
      char *q = p; while (*q && *q != '*') {strncat(s, q, 1); q++;}
      if (strlen(s) > 0)
        q = str_wstr(b, s);   //copy to terminating string
      else
        q = buf+strlen(buf);  //copy to end of string
      delete[] s;
      
      //not found
      if (q == NULL) { result = false; break; }
      //found
      if (numw < MAX_WILDC)
      {
        wildcard[numw] = new char[q - b + 1];
        strcpy(wildcard[numw], "");
        while (b < q) { strncat(wildcard[numw], b, 1); b++; }
        numw++;
      }
      else b = q;
    }
    else if (*p == '?')
    {
      p++;
      if (*b)
      {
        if (numw < MAX_WILDC)
        {
          wildcard[numw] = new char[2];
          wildcard[numw][0] = *b;
          wildcard[numw][1] = '\0';
          numw++;
        }
        b++;
      }
      else {result = false; break;}
    }
    else if (*p != '\0' || *b != '\0') {result = false; break;}
  }
  return result;
}

// Add next char to the buffer and check again
void check_char(char ch)
{
  if ((long)strlen(buf) == blen) memmove(buf, buf+1, blen);
  strncat(buf, &ch, 1);
  //fprintf(stderr, "pattern=`%s' buf=`%s'\n", pattern, buf);
  if (scan_buffer()) done = true;
}

// User event handler
void my_handler(Event *ev)
{
  char *p = (char *)ev->data;
  if (ev->chn == lp_channel())
    switch (ev->type)
    {
      case EV_ABORT: abort_all = 1;
                     break;
      case EV_DATA_INPUT: if (datain)
                         for (int i = 0; i < ev->x; i++)
                           check_char(p[i]);
                       break;
      case EV_DATA_OUTPUT: if (dataout)
                         for (int i = 0; i < ev->x; i++)
                           check_char(p[i]);
                        break;
      case EV_LOCAL_MSG: if (dataloc)
                         for (int i = 0; i < ev->x; i++)
                           check_char(p[i]);
                        break;
    }
}

// Add the parametres to the command and execute it
void exec_command()
{
  int wc = 1;
  char *whole = new char[blen]; //whole pattern
  strcpy(whole, "");

  //create whole pattern
  char *p = origptrn;
  while (*p)
  {
    if (*p == '?' || *p == '*') { strcat(whole, wildcard[wc]); wc++; }
    else strncat(whole, p, 1);
    p++;
  }
  
  //create the command
  char *cmd = new char[strlen(command) + blen];
  strcpy(cmd, "");
  p = command;
  while (*p)
  {
    if (*p == '$')
    {
      char num[5];
      strcpy(num, "");
      p++;
      while (*p && isdigit(*p) && strlen(num) < 4) { strncat(num, p, 1); p++; }
      int n = atoi(num);
      if (n == 0) strcat(cmd, whole);
      else if (n > 0 && n < numw-1) strcat(cmd, wildcard[n]); //omit last *
    }
    else
    {
      strncat(cmd, p, 1);
      p++;
    }
  }

  //execute
  lp_emit_event(lp_channel(), EV_TEXT_COOKED, 0, cmd);
  delete[] cmd;
  delete[] whole;
}

void help()
{
  printf("catch: scans selected stream(s) for matching patterns\n");
  printf("Usage: catch -iol [-m <message_to_user>] [-b <buffer_length>] pattern command\n");
  printf("\n");
  printf("Switches: -i scan input data\n");
  printf("          -o scan output data\n");
  printf("          -l scan local messages\n");
  printf("\n");
  printf("Pattern can contain wildcards ? (one character) and * (0-n) characters\n");
  printf("Command can contain $0 = whole pattern\n");
  printf("                    $n = string responding to n-th wildcard (1..n)\n");
  printf("Default buffer length is 1024 bytes\n");
}

int main(int argc, char **argv)
{
  int i;
  
  while ((i = getopt(argc, argv, "m:iolh")) != -1)
  {
    switch (i)
    {
      case 'm': strncpy(message, optarg, 255); break;
      case 'b': blen = atol(optarg); break;
      case 'i': datain = true; break;
      case 'o': dataout = true; break;
      case 'l': dataloc = true; break;
      case 'h': help();
      default: help(); return 0; break;
    }
  }

  if (optind != argc-2) { help(); return 0; }
  if (blen < 1) blen = 1;
  if (!datain && !dataout && !dataloc)
  {
    printf("No data stream specified - use -i / -o / -l switches\n");
    return 1;
  }

  strncpy(origptrn, argv[optind], 253);
  strncpy(command, argv[optind+1], 255);
  sprintf(pattern, "*%s*", origptrn);

  buf = new char[blen+1];
  if (buf == NULL) { printf("Cannot allocate buffers\n"); return 1; }
  memset(buf, '\0', blen+1);
  for (int i = 0; i < MAX_WILDC; i++) wildcard[i] = NULL;

  if (lp_start_appl())
  {
    lp_set_event_handler(my_handler);
    if (strlen(message) > 0) lp_statline(message);
    do pause(); while (!done && !abort_all);
    if (done) exec_command();
    lp_end_appl();
  }
  else
  {
    printf("LinPac is not running\n");
    return 1;
  }

  return 0;
}

