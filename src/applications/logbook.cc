/*
   LinPac Logbook
   (c) 1999 by Radek Burget OK2JBG

   Usage: logbook <start|stop> <source_callsign> <dest_callsign> <qrg>
*/
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <unistd.h>

char *time_stamp(int utc)
{
  static char tt[32];
  struct tm *cas;
  time_t secs = time(NULL);
  if (utc) cas = gmtime(&secs);
      else cas = localtime(&secs);
  sprintf(tt, "%2i:%02i", cas->tm_hour, cas->tm_min);
  //strftime(tt, 30, "%X", cas);
  return tt;
}

char *date_stamp(int utc)
{
  static char tt[32];
  struct tm *cas;
  time_t secs = time(NULL);
  if (utc) cas = gmtime(&secs);
      else cas = localtime(&secs);
  //sprintf(tt, "%i.%i.%i", cas->tm_mday, cas->tm_mon+1, cas->tm_year+1900);
  strftime(tt, 30, "%x", cas);
  return tt;
}

void start_record(char *logname, char *dest, char *qrg)
{
  FILE *f;

  f = fopen(logname, "a");
  if (f != NULL)
  {
    fprintf(f, "%s  %s - **:** UTC  %15s  %s\n", date_stamp(1),
              time_stamp(1), qrg, dest);
    fclose(f);
  }
  else
    fprintf(stderr, "logbook: cannot open file `%s' for writing\n", logname);
}

void stop_record(char *logname, char *dest, char *qrg)
{
  FILE *f, *t;
  char tname[256], s[256];

  f = fopen(logname, "r");
  if (f == NULL)
  {
    fprintf(stderr, "logbook: cannot open file `%s'\n", logname);
    return;
  }

  tmpnam(tname);
  t = fopen(tname, "w");
  if (t == NULL)
  {
    fprintf(stderr, "logbook: cannot temp file\n");
    return;
  }

  while (!feof(f))
  {
    char old_str[256];
    char *new_str;

    strcpy(s, "");
    fgets(s, 255, f);
    if (strlen(s) == 0) continue;
    if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';

    //match qrg and dest. call
    sprintf(old_str, "UTC  %15s  %s", qrg, dest);
    new_str = strstr(s, "UTC");
    if (new_str != NULL && strcmp(new_str, old_str) == 0)
    {
      char *beg = strchr(s, '*');
      if (beg != NULL)
        sprintf(beg, "%s UTC  %15s  %s", time_stamp(1), qrg, dest);
    }
    fprintf(t, "%s\n", s);
  }

  fclose(f);
  fclose(t);
  rename(tname, logname);
}

void help()
{
  printf("logbook start|stop logfile_path callsign qrg\n");
}

int main(int argc, char **argv)
{
  char logname[256];
  char call[32];
  char qrg[32];

  setlocale(LC_ALL, "");

  if (argc != 5)
  {
    help();
    return 1;
  }

  strncpy(logname, argv[2], 255);
  strncpy(call, argv[3], 30);
  strncpy(qrg, argv[4], 30);

  if (strcmp(argv[1], "start") == 0) start_record(logname, call, qrg);
  else if (strcmp(argv[1], "stop") == 0) stop_record(logname, call, qrg);
  else
  {
    help();
    return 1;
  }

  return 0;
}

