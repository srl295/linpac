/*
  Round-trip-time measuring for LinPac
  by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)
*/
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

time_t atime, gtime;

int main(int argc, char **argv)
{
  if (argc == 1)
  {
    printf("//e //rtt %li Checking round trip time\n", time(NULL));
  }
  else
  {
    char *endptr;
    char s[256];
    strncpy(s, argv[1], 255);
    gtime = strtol(s, &endptr, 10);
    if (strlen(endptr) == 0)
    {
      atime = time(NULL);
      printf("LinPac: Round trip time is %li sec.\n", atime - gtime);
    }
    else
    {
      printf("LinPac: invalid test frame returned\n");
    }
  }
  return 0;
}

