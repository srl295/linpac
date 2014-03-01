/*
   MD2 password checking for LinPac
   (c) 1998 - 1999 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   This software uses the RSA Data Security, Inc. MD2 Message Digest
   Algorithm.
*/

#include <stdio.h>
#include <string.h>
#include "md2.h"

void md2_reply(char *passwd, char *prompt, char *reply)
{
  unsigned char buff[256];  //working buffer
  unsigned char digest[16]; //message digest (always 16 bytes)
  MD2_CTX context;
  int i, len;
  
  //conact the prompt with the password
  sprintf((char *)buff, "%s%s", prompt, passwd);
  //create the message digest using MD2
  MD2Init(&context);
  len = strlen((char *)buff);
  for (i = 0; i < len; i += 16)
    MD2Update(&context, buff+i, ((len - i) > 16) ? 16 : (len - i));
  MD2Final(digest, &context);

  //print the result in hex
  strcpy(reply, "");
  for (i = 0; i < 16; i++)
  {
    sprintf((char *)buff, "%02x", digest[i]);
    strcat(reply, (char *)buff);
  }
}

int main(int argc, char **argv)
{
  char code[30];
  char key[15];
  char *p;
  
  if (argc != 3)
  {
    printf("Usage: md2pwd <password> <prompt>\n");
    return 1;
  }

  p = strchr(argv[2], '[');
  if (p == NULL)
  {
    printf("Invalid prompt\n");
    return 2;
  }
  p++;

  //extract the key from the prompt
  memset(key, '\0', 15);
  strncpy(key, p, 10);
  
  md2_reply(argv[1], key, code);
  printf("%s\n", code);
  return 0;
}

