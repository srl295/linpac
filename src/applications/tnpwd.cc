/*
   TheNet password checking for LinPac
   (c) 1998 - 1999 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "lpapp.h"

#define KEYLEN 5 //key length (5 chars from password)
#define REPLYLEN 40 //reply length

int error = 0; //bad password request
bool nohide = false; //do not hide the reply in the random string

void randomize()
{
  int t;
  t = (int)time(NULL);
  srand(t);
}

int rnd(int range)
{
  float f = rand();
  float g = RAND_MAX+1.0;
  int i = (int)(range * f / g);
  return i;
}


void thenet_reply(unsigned char *passwd, int *key, unsigned char *reply)
{
  int i, pos;

  if (nohide) //short answers
  {
    reply[KEYLEN] = '\0';
    pos = 0;
  }
  else
  {
    randomize();
    for (i = 0; i < REPLYLEN; i++)
      reply[i] = passwd[rnd(strlen((char *)passwd))];
    reply[i] = '\0';
    pos = rnd(REPLYLEN - KEYLEN);
  }
  for (i = 0; i < KEYLEN; i++)
    if ((unsigned)key[i] > strlen((char *)passwd))
    {
      reply[i+pos] = 'a';
      error = 1;
    }
    else reply[i+pos] = passwd[key[i]-1];
}

int main(int argc, char **argv)
{
  if (lp_start_appl())
  {
     unsigned char code[REPLYLEN+1];
     int key[KEYLEN]; // 5 key numbers
     char *p;
     int param = 1;
     
     if (argc < 3)
     {
       printf("Usage: tnpwd [-n] <password> <prompt>\n");
       return 1;
     }

     if (strcmp(argv[param], "-n") == 0)
     {
       nohide = true;
       param++;
     }

     if ((nohide && argc != 4) || (!nohide && argc != 3))
     {
       printf("Usage: tnpwd [-n] <password> <prompt>\n");
       return 1;
     }
     
     //printf("prompt is :%s\n",argv[param+1]);
     
     p = strrchr(argv[param+1], '>');
     if (p == NULL)
     {
       printf("Invalid prompt (%s) : missing '>'\n",p);
       return 2;
     }
     p++; 

     //printf("prompt after > is :%s\n",p);
    
     //extract the key from the prompt
     int nums = sscanf(p, "%i %i %i %i %i", &key[0], &key[1], &key[2], &key[3], &key[4]);
     if (nums != 5)
     {
       printf("Invalid prompt (%s) : 5(%d) digits required after '>'\n",p,nums);
       return 2;
     }
          
     thenet_reply((unsigned char *)argv[param], key, code);
     printf("%s\n", code);
     if (error) lp_set_var(lp_channel(), "TNPWD", "0");
           else lp_set_var(lp_channel(), "TNPWD", "1");
     lp_end_appl();
  }
  else
  {
    printf("LinPac is not running\n");
    return 1;
  }
  return 0;
}

