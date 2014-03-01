/*
   Automatic FlexNet password generator for LinPac
   (c) 1998 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   Usage: flexpwd <magic_num>
   Flexpwd reads the prompt from stdin
*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

int authorize(char *magic, char *request)
{
  char *p, *q;
  int result = 0;

  p = magic;
  q = request;
  while (strlen(q) > strlen(p)) q++;
  while (strlen(p) > strlen(q)) p++;
  while (*q && *p)
  {
    result += ((*q)-'0') * ((*p)-'0');
    p++;
    q++;
  }
  return result;
}


int main(int argc, char **argv)
{
  char req[20];
  char magic[10];
  char *p;
  int ch;

  if (argc != 2)
  {
    printf("Usage: flexpwd <magic_num>\n");
    return 1;
  }

  strncpy(magic, argv[1], 9);

  p = req;
  do
  {
    ch = getchar();
    if (ch != '>') {*p = (char)ch; p++;}
  } while (ch != '>');
  *p = '\0';
  while (p > req && isdigit(*(p-1))) p--;
  printf("%i\n", authorize(magic, p));
  return 0;
}

