/*
  Simple message composer for LinPac
  (c) 1998-2001 by Radek Burget (OK2JBG)

  Usage: compose [p|b] dest_addr [subject] [source_file]
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "lpapp.h"
#include "version.h"

//uncomment this to add a path to yourself into each message
//#define PATH_TO_OWN

char mycall[20];
char *maildir;
char *homebbs;
char *homeaddr;
int msgnum;
char mode = 'b'; /* p = personal, b = bulletin */
char title[256];
char from[256], to[256];
char mid[20], date[30];
long size;

char *strupr(char *s)
{
  char *p = s;
  while (*p) { *p = toupper(*p); p++; }
  return s;
}

void striplf(char *s)
{
  if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
}

char *call_call(const char *src)
{
  static char s[15];
  char *p = (char *)src;
  strcpy(s, "");
  while (*p && isalnum(*p) && strlen(s) < 6)
  {
    char c = toupper(*p);
    strncat(s, &c, 1);
    p++;
  }
  return s;
}

/* -------------------------------------------------------------------- */

/* return the number of last local message */
int last_msg_number()
{
  FILE *index;
  char fname[256], s[256];
  int msgnum = 0;

  sprintf(fname, "%s/messages.local", maildir);
  index = fopen(fname, "r");
  if (index == NULL)
  {
    printf("Cannot open message database %s\n", fname);
    exit(2);
  }
  while (!feof(index))
  {
    strcpy(s, "");
    safe_fgets(s, 255, index);
    if (strlen(s) > 0) sscanf(s, "%i", &msgnum);
  }
  fclose(index);
  return msgnum;
}

void read_message(int msgnum)
{
  FILE *f;
  char fname[256], s[256];
  int stp;
  struct tm *tim;
  time_t secs = time(NULL);
  tim = gmtime(&secs);

  sprintf(fname, "%s/%i", maildir, msgnum);
  f = fopen(fname, "w");
  if (f == NULL)
  {
    printf("Cannot create message file %s\n", fname);
    exit(1);
  }
  fprintf(f, "%s\n", title);

  sprintf(date, "%02i%02i%02i/%02i%02i", (tim->tm_year+1900)%100,
           tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min);
#ifdef PATH_TO_OWN
  fprintf(f, "R:%sZ @:%s.\n", date, mycall);
  fprintf(f, "\n");
  fprintf(f, "From: %s\n", from);
  fprintf(f, "To: %s\n", to);
  fprintf(f, "\n");
#endif

  printf("Enter message for %s\nEnd with /EX or (Ctrl-Z) in first column\n", to);
  stp = 0;
  do
  {
    safe_fgets(s, 255, stdin);
    if (s[0] == 26 || strncasecmp(s, "/ex", 3) == 0) stp = 1;
    else fprintf(f, s);
  } while (!stp);
  size = ftell(f);
  fclose(f);

  sprintf(mid, "%06i%s", msgnum, mycall);
  while (strlen(mid) < 12) strcat(mid, "0");
  printf("\nMessage written OK.  Mid: %s  Size: %li\n", mid, size);
}

void copy_message(int msgnum, char *srcname)
{
  FILE *f, *src;
  char fname[256], s[256];
  struct tm *tim;
  time_t secs = time(NULL);
  tim = gmtime(&secs);

  src = fopen(srcname, "r");
  if (src == NULL)
  {
     printf("Cannot open source file\n");
     exit(1);
  }

  sprintf(fname, "%s/%i", maildir, msgnum);
  f = fopen(fname, "w");
  if (f == NULL)
  {
    printf("Cannot create message file %s\n", fname);
    exit(1);
  }
  fprintf(f, "%s\n", title);

  sprintf(date, "%02i%02i%02i/%02i%02i", (tim->tm_year+1900)%100,
           tim->tm_mon+1, tim->tm_mday, tim->tm_hour, tim->tm_min);
#ifdef PATH_TO_OWN
  fprintf(f, "R:%sZ @:%s.\n", date, mycall);
  fprintf(f, "\n");
  fprintf(f, "From: %s\n", from);
  fprintf(f, "To: %s\n", to);
  fprintf(f, "\n");
#endif

  while (!feof(src))
  {
     strcpy(s, "");
     safe_fgets(s, 255, src);
     fputs(s, f);
  }
  size = ftell(f);
  fclose(f);

  sprintf(mid, "%06i%s", msgnum, mycall);
  while (strlen(mid) < 12) strcat(mid, "0");
  printf("\nMessage written OK.  Mid: %s  Size: %li\n", mid, size);
}

void add_to_index(int msgnum)
{
  FILE *index;
  char fname[256], flags[10];

  sprintf(fname, "%s/messages.local", maildir);
  index = fopen(fname, "a");
  if (index == NULL)
  {
    printf("Cannot open message database %s\n", fname);
    exit(2);
  }
  if (mode == 'p') strcpy(flags, "PO");
              else strcpy(flags, "BO");

  fprintf(index, "%i\t%s\t%s\t%s@%s\t%s\t%sZ\t|%s\n", msgnum, flags, mid,
              from, homeaddr, to, date, title);
  fclose(index);
}

/* --------------------------------------------------------------------- */

int main(int argc, char **argv)
{
  bool fromfile = false;
  char *subj = NULL;
  char *fname = NULL;
  
  if (argc < 3 && argc > 5)
  {
    printf("use: compose [p|b] <dest_addr> [<subject>] [<file_name>]");
    return 3;
  }
  if (argc > 3) subj = argv[3];
  if (argc == 5)
  {
     fname = argv[4];
     fromfile = true;
  }

  mode = argv[1][0];

  if (lp_start_appl())
  {
    //lp_event_handling_off();
    maildir = lp_get_var(0, "MAIL_PATH");
    if (maildir == NULL)
    {
      printf("No MAIL_PATH@0 specified\n");
      return 1;
    }
  
    homebbs = lp_get_var(0, "HOME_BBS");
    if (homebbs == NULL)
    {
      printf("No HOME_BBS@0 specified\n");
      return 1;
    }
  
    homeaddr = lp_get_var(0, "HOME_ADDR");
    if (homebbs == NULL)
    {
      printf("No HOME_ADDR@0 specified\n");
      return 1;
    }
  
    msgnum = last_msg_number();
    if (!fromfile) printf("Last message number is %i\n", msgnum);
    msgnum++;
  
    strcpy(mycall, call_call(lp_chn_call(lp_channel())));
    strcpy(from, mycall);
    strcpy(to, argv[2]);
    if (strchr(to, '@') == NULL)
    {
      strcat(to, "@");
      strcat(to, homeaddr);
    }
    strupr(to);
    
    if (subj == NULL)
    {
       if (mode == 'p') printf("Enter subject of PRIVATE MESSAGE (max 80 characters)\n");
                   else printf("Enter subject of BULLETIN (max 80 characters)\n");
       safe_fgets(title, 80, stdin);
    } else strncpy(title, subj, 80);
    striplf(title);
    if (strlen(title) == 0)
    {
      printf("\nMessage cancelled\n");
      return 0;
    }

    if (!fromfile) read_message(msgnum);
    else copy_message(msgnum, fname);
    
    add_to_index(msgnum);
    lp_end_appl();
  }
  else printf("LinPac is not running\n");
  return 0;
}

