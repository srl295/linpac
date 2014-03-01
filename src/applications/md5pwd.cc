/*
   MD5 password checking for LinPac
   (c) 1998 - 2000 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   This software uses the RSA Data Security, Inc. MD5 Message Digest
   Algorithm.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "md5.h"

void md5_reply(char *passwd, char *prompt, char *reply)
{
  unsigned char buff[256];  //working buffer
  MD5_CTX context;
  int i, len;
  
  //conact the prompt with the password
  sprintf((char *)buff, "%s%s", prompt, passwd);
  //create the message digest using MD5
  MD5Init(&context);
  len = strlen((char *)buff);
  for (i = 0; i < len; i += 16)
    MD5Update(&context, buff+i, ((len - i) > 16) ? 16 : (len - i));
  MD5Final(&context);

  //print the result in hex
  strcpy(reply, "");
  for (i = 0; i < 16; i++)
  {
    sprintf((char *)buff, "%02x", context.digest[i]);
    strcat(reply, (char *)buff);
  }
}

void md5_reply_num(char *passwd, long prompt, char *reply)
{
  unsigned char buff[256];  //working buffer
  MD5_CTX context;
  
  //create the message digest using MD5
  MD5Init(&context);
  MD5Update(&context, (unsigned char *)&prompt, sizeof(prompt));
  MD5Update(&context, (unsigned char *)passwd, strlen(passwd));
  MD5Final(&context);

  //print the result in hex
  strcpy(reply, "");
  for (int i = 0; i < 16; i++)
  {
    sprintf((char *)buff, "%02x", context.digest[i]);
    strcat(reply, (char *)buff);
  }
}

int main(int argc, char **argv)
{
  char code[30];
  char *p;
  bool num = false;
  int parg = 2;
  
  if (argc != 3 && (argc != 4 || strcmp(argv[1], "-num") != 0))
  {
    printf("Usage: md5pwd [-num] <password> <prompt>\n");
    return 1;
  }

  if (argc == 4) {parg = 3; num = true;}

  p = strchr(argv[parg], '[');
  if (p == NULL)
  {
    printf("Invalid prompt\n");
    return 2;
  }
  p++;

  //extract the key from the prompt
  if (!num)
  {
    char key[15];
    memset(key, '\0', 15);
    strncpy(key, p, 10);
    md5_reply(argv[1], key, code);
  }
  else
  {
    long key = 0L;
    while (isalnum(*p))
    {
      key <<= 4;
      key += (*p > '9') ? (tolower(*p) - 'a' + 10) : *p - '0';
      p++;
    }
    md5_reply_num(argv[2], key, code);
  }

  printf("%s\n", code);
  return 0;
}

