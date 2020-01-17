/*
   BBS access implementation for FBB
   (c) 2020 by Martin Cooper KD6YAM
   (c) 1998 - 1999 by Radek Burget OK2JBG

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lpapp.h"
#include "lzhuf.h"
#include "bbs.h"
#include "bbs_fbb.h"

#define NUL 0
#define SOH 1
#define STX 2
#define EOT 4

FBB::FBB(char* homebbs, char* homecall, char* mailhome)
    : BBS(homebbs, homecall, mailhome)
{
}

FBB::~FBB()
{
}

/* read and store one message */
int FBB::get_one_message(FILE *fin, char **buf, int *bsize, char *title)
{
  int eot;
  int ch;
  char c;
  char ofset[7];
  signed char sum, chksum;
  int flen;
  int i;
  int bpoz;
  int size;

  size = 1;
  *buf = (char *) malloc(sizeof(char) * size);
  bpoz = 0;
  
  ch = safe_fgetc(fin);
  if (ch != SOH)
  {
     fprintf(stderr, "getmsg: bad starting character (0x%x)\n", ch);
     return 2; /* format violated */
  }
  (void)safe_fgetc(fin); // read and discard header length

  lp_statline("Reading message");

  /* read title */
  strcpy(title, "");
  do
  {
    ch = safe_fgetc(fin);
    c = (char) ch;
    if (ch != NUL) strncat(title, &c, 1);
  } while (ch != NUL && !abort_all);

  if (abort_all)
  {
    fprintf(stderr, "getmsg: aborted\n");
    return 3;
  }

  lp_statline("Reading message: `%s'", title);

  /* read ofset */
  strcpy(ofset, "");
  do
  {
    ch = safe_fgetc(fin);
    c = (char) ch;
    if (ch != NUL) strncat(ofset, &c, 1);
  } while (ch != NUL && !abort_all);

  if (abort_all)
  {
    fprintf(stderr, "getmsg: aborted\n");
    return 3;
  }

  /* read data frames */
  sum = 0;
  eot = 0;
  while (!eot)
  {
    ch = safe_fgetc(fin);
    if (ch == STX) /* data frame */
    {
      flen = safe_fgetc(fin);
      if (flen == 0) flen = 256;
      if (bpoz+flen >= size) /* buffer exceeded -> expand */
      {
        size += flen;
        *buf = (char *) realloc(*buf, sizeof(char) * size);
      }

      if (abort_all)
      {
        fprintf(stderr, "getmsg: aborted\n");
        return 3;
      }
      /*fread(buf+bpoz, 1, flen, fin);
      for (i=0; i<flen; i++)
      {
        sum += (int) (*buf)[bpoz];
        bpoz++;
      }*/
      for (i=0; i<flen; i++)
      {
        ch = safe_fgetc(fin);
        (*buf)[bpoz] = (char) ch; bpoz++;
        sum += ch;
        if (abort_all)
        {
          fprintf(stderr, "getmsg: aborted\n");
          return 3;
        }
      }
    }
    else if (ch == EOT)/* end of transfer */
    {
      chksum = safe_fgetc(fin);
      if (chksum + sum != 0 && !(chksum == -128 && sum == -128))
      {
        fprintf(stderr, "getmsg: Checksum error\n");
        return 1;
      }
      eot = 1;
    }
    else
    {
      fprintf(stderr, "getmsg: Protocol violation (0x%x)\n", ch);
      return 2;
    }
  }
  *bsize = bpoz;
  return 0;
}

/* save decompressed message (and convert to Unix text) */
void FBB::save_msg(int num, bool pers, char* data, unsigned long length)
{
    LZHufPacker pck;
    unsigned long clen = ((data[3]*256 + data[2])*256 + data[1])*256 + data[0];
    pck.Decode(clen, data+4, length-4);

    char fname[256];
    char* outdata = (char*)pck.GetData();
    unsigned long outlen = pck.GetLength();

    get_msg_path(num, pers, fname);
    FILE *f = fopen(fname, "w");
    if (f != NULL)
    {
        unsigned long i;
        char *p = outdata;
        for (i=0; i<outlen; i++, p++)
            if (*p != '\r') putc(*p, f);
        fclose(f);
    }
    else fprintf(stderr, "getmsg: cannot write to file %s\n", fname);
}

/* wait for prompt from BBS */
void FBB::wait_prompt(FILE *fin)
{
  int old_char=0, new_char=0;
  do
  {
    old_char = new_char;
    new_char = safe_fgetc(fin);
  } while ((old_char != '>' || new_char != '\r') && !abort_all);
}

/* send LinPac tag */
void FBB::send_tag()
{
   printf("[LinPac-%s-$]\r", LPAPP_VERSION);
}

void FBB::send_request(int num)
{
  printf("F< %i\r", num);
}
