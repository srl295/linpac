/*
  Tool for editing the LinPac station database
  (c) 1998 by Radek Burget

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  19 Oct 2001 - some fixes by F1TVE
*/
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>
#include <stdlib.h>
#include "lpapp.h"

#define DB_NAME "station.data"

//#define USE_TMPNAM

//Station informations
struct name_record
{
    char name[32];   /* NAME = */
    char nick[32];   /* NICKNAME = */
    char type[8];    /* TYPE = */
    char loc[7];     /* LOC = */
    char lang[32];   /* LANG = */
};

typedef struct
{
    char name[256];
    char content[256];
} equation;

FILE *f;                      //database file
int alone;                    //running w/o connection
int allinfo;                  //show all info
char mycall[12], remcall[12]; //my callsign, remote callsign
struct name_record data, newrec;     //actual record, new values
equation equ[64];             //additional values
int noequ = 0;                //number of aditional values
equation new_equ[16];         //new aditional values
int nonew = 0;                //number of new aditional values

/*-------------------------------------------------------------------------*/

int call_ssid(const char *src)
{
  char *p = (char *)src;
  while (*p != '\0' && *p != '-') p++;
  if (*p) p++;
  if (*p) return atoi(p);
     else return 0;
}

char *call_call(const char *src)
{
  static char s[15];
  char *p = (char *)src;
  strcpy(s, "");
  while (*p && isalnum(*p)) {strncat(s, p, 1); p++;}
  return s;
}

char *strip_ssid(char *src)
{
  if (call_ssid(src) == 0)
  {
    char *p = src;
    while (*p != '\0' && *p != '-') p++;
    *p = '\0';
  }
  return src;
}

char *normalize_call(char *call)
{
  char c[8];
  strcpy(c, call_call(call));
  int ssid = call_ssid(call);
  if (ssid == 0) strcpy(call, c); else sprintf(call, "%s-%i", c, ssid);
  for (char *p = call; *p; p++) *p = toupper(*p);
  return call;
}

/*-------------------------------------------------------------------------*/

/* find a record in the file */
int find_data(char *stn)
{
  char s[20], srch[20];
  sprintf(srch, "[%s]\n", stn);
  rewind(f);
  while (!feof(f))
  {
    strcpy(s, "");
    fgets(s, 19, f);
    if (strcasecmp(s, srch) == 0) break;
  }
  return !feof(f);
}

/* clear a data structure */
void clear_data()
{
  strcpy(data.name, "");
  strcpy(data.nick, "");
  strcpy(data.type, "");
  strcpy(data.loc, "");
  strcpy(data.lang, "");
}

/* read actual record */
void read_data()
{
  char s[256], cmd[10], par[256];
  clear_data();
  do
  {
    char *p;
    fgets(s, 256, f);
    if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
    p = strchr(s, '=');
    if (p != NULL)
    {
      strcpy(par, p+1);
      *p = '\0';
      strcpy(cmd, s);
      if (strcasecmp(cmd, "NAME") == 0) strcpy(data.name, par);
      else if (strcasecmp(cmd, "NICKNAME") == 0) strcpy(data.nick, par);
      else if (strcasecmp(cmd, "TYPE") == 0) strcpy(data.type, par);
      else if (strcasecmp(cmd, "LOC") == 0) strcpy(data.loc, par);
      else if (strcasecmp(cmd, "LANG") == 0) strcpy(data.lang, par);
      else {strcpy(equ[noequ].name, cmd);
            strcpy(equ[noequ].content, par);
            if (noequ<64) noequ++;}
    } else break;
  } while (!feof(f));
}

/* remove one record */
void remove_call(char *stn)
{
  char tname[256];
  FILE *dest;
  char s[256], srch[20];
  int ok=1;

  #ifdef USE_TMPNAM
  tmpnam(tname);
  dest = fopen(tname, "w");
  #else
  strcpy(tname, "lp-name-XXXXXX");
  int tdesc = mkstemp(tname);
  dest = fdopen(tdesc, "w");
  #endif
  if (dest == NULL) perror("cannot open temp file");
  sprintf(srch, "[%s]\n", stn);
  rewind(f);
  while (!feof(f))
  {
    fgets(s, 255, f);
    if (strcasecmp(s, srch) == 0) ok = 0;
    else if (s[0] == '[') ok = 1;
    if (ok) fputs(s, dest);
  }
  fclose(f);
  fclose(dest);
  rename(tname, DB_NAME);
  f = fopen(DB_NAME, "r");
}

/* add new data record */
void write_data(char *call)
{
  int i;

  fclose(f);
  f = fopen(DB_NAME, "a");
  fprintf(f, "\n[%s]\n", call);
  if (strlen(data.name) > 0) fprintf(f, "NAME=%s\n", data.name);
  if (strlen(data.nick) > 0) fprintf(f, "NICKNAME=%s\n", data.nick);
  if (strlen(data.type) > 0) fprintf(f, "TYPE=%s\n", data.type);
  if (strlen(data.loc) > 0) fprintf(f, "LOC=%s\n", data.loc);
  if (strlen(data.lang) > 0) fprintf(f, "LANG=%s\n", data.lang);
  for (i=0; i<noequ; i++)
    fprintf(f, "%s=%s\n", equ[i].name, equ[i].content);
  fclose(f);
  f = fopen(DB_NAME, "r");
}

/* combine old and new record and write data */
void new_data(char *call)
{
  int i,j;
  
  if (strlen(newrec.name) > 0) strcpy(data.name, newrec.name);
  if (strlen(newrec.nick) > 0) strcpy(data.nick, newrec.nick);
  if (strlen(newrec.loc) > 0) strcpy(data.loc, newrec.loc);
  if (strlen(newrec.lang) > 0) strcpy(data.lang, newrec.lang);
  if (strlen(newrec.type) > 0) strcpy(data.type, newrec.type);
  for (i=0; i<nonew; i++)
  {
    for (j=0; j<noequ; j++)
      if (strcasecmp(new_equ[i].name, equ[j].name) == 0) break;
    if (j == noequ)
    {
      strcpy(equ[noequ].name, new_equ[i].name);
      strcpy(equ[noequ].content, new_equ[i].content);
      if (noequ < 64) noequ++;
    }
    else
    {
      strcpy(equ[j].content, new_equ[i].content);
    }
  }
  write_data(call);
}

/* show actual record */
void show_info()
{
  char s[256];
  char *p;

  do
  {
    fgets(s, 255, f);
    p = strchr(s, '=');
    if (p != NULL && !feof(f)) printf(s);
  } while (p != NULL && !feof(f));
  printf("\n");
}

/*-------------------------------------------------------------------------*/

void short_help()
{
  printf("Read and modify user information");
}

void help()
{
  printf("Tool for editing LinPac station database\n");
  printf("Usage: name [options] [station name]\n");
  printf("User options: -n nickname\n"
         "              -t type\n"
         "              -l locator\n"
         "              -L language\n"
         "              -s variable=value\n"
         "              -i show all\n");
  if (alone)
  {
    printf("Sysop Options (w/o connection):\n"
           "               -S security_level\n"
           "               -c callsign (default is your callsign)\n"
           "               -r callsign_to_remove\n");
  }
}

/*-------------------------------------------------------------------------*/

int main(int argc, char **argv)
{
  int i;
  char name[32];
  char options[20];
  char s[256];
  char *p;
  int set = 0;

  /* open database */
  f = fopen(DB_NAME, "r");
  if (f == NULL)
  {
    f = fopen(DB_NAME, "w+");
    fclose(f);
    f = fopen(DB_NAME, "r");
  }
  if (f == NULL) perror("cannot open station database");

  /* initialize app */
  alone = !lp_start_appl();
  if (alone)
  {
    strcpy(mycall, "");
  }
  else
    if (lp_chn_status(lp_channel()) > 0) /* if connected */
    {
       strcpy(mycall, lp_chn_cwit(lp_channel()));
       normalize_call(mycall);
    }
    else /* not connected - run by sysop */
    {
       strcpy(mycall, lp_chn_call(lp_channel()));
       normalize_call(mycall);
       alone = true;
    }

  allinfo = 0;
  strcpy(name, "");
  strcpy(newrec.nick, "");
  strcpy(newrec.loc, "");
  strcpy(newrec.lang, "");
  strcpy(remcall, "");
  strcpy(newrec.type, "");

  if (alone) strcpy(options, "n:t:l:L:s:c:r:ihH");
        else strcpy(options, "n:t:l:L:s:ihH");

  while ((i = getopt(argc, argv, options)) != -1)
    switch (i)
    {
      case 'n': strncpy(newrec.nick, optarg, 32); set = 1; break;
      case 't': strncpy(newrec.type, optarg, 8); set = 1; break;
      case 'l': strncpy(newrec.loc, optarg, 6); set = 1; break;
      case 'L': strncpy(newrec.lang, optarg, 32); set = 1; break;
      case 'c': strncpy(mycall, optarg, 10); break;
      case 'r': strncpy(remcall, optarg, 10); set = 1; break;
      case 's': strncpy(s, optarg, 255);
                p = strchr(s, '=');
                if (p != NULL && nonew < 16)
                {
                  *p = '\0'; p++;
                  strcpy(new_equ[nonew].name, s);
                  strcpy(new_equ[nonew].content, p);
                  nonew++;
                }
                set = 1;
                break;
      case 'i': allinfo = 1; break;
      case 'H': short_help(); return 0; break;
      case 'h': help(); return 0; break;
      case ':':
      case '?': help(); return 1; break;
    }

  if (!set && optind == argc) /* no changes - info only */
  {
    if (strlen(mycall) == 0)
      printf("Cannot determine your callsign - LinPac is not running\n");
    else
    {
      if (find_data(mycall))
      {
        if (!allinfo)
        {
          read_data();
          printf("Your name is `%s'\n", data.name);
        }
        else show_info();
      }
      else if (find_data(strip_ssid(mycall)))
      {
        if (!allinfo)
        {
          read_data();
          printf("Your name is `%s'\n", data.name);
        }
        else show_info();
      }
      else printf("Your name is unknown\n");
    }
  }
  else
  {
    for (i = optind; i < argc; i++) /* concat remaining parametres */
      if ((strlen(name) + strlen(argv[i])) < 32)
      {
        strcat(name, argv[i]);
        if (i != argc-1) strcat(name, " ");
      }
      else break;
    strcpy(newrec.name, name);
  
    if (strlen(remcall) > 0) remove_call(remcall);
    else if (strlen(mycall) > 0)
    {
      if (find_data(mycall)) {read_data(); remove_call(mycall);}
                        else clear_data();
      new_data(mycall); /* add new record */
      printf("OK, information has been saved\n");
    }
    else printf("Cannot determine your callsign - LinPac is not running\n");
  }
  fclose(f);
  return 0;
}

