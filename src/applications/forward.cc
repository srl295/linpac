/*
  Mail forward for LinPac
  (c) 1998 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.
*/

#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "lzhuf.h"
#include "lpapp.h"
#include "version.h"

#include <unistd.h>

#define ABORT_ADDR "forward"

#define EXIT_OK 0
#define EXIT_FILE 1
#define EXIT_LINK 2
#define EXIT_ABORT 3
#define EXIT_OTHER 4

/* max. size of one message block [kB] */
#define MSG_BLOCK_SIZE 10240

#define NUL 0
#define SOH 1
#define STX 2
#define EOT 4

/* one forwarded message entry */
typedef struct
        {
          int ok;               /* this record is usable */
          int send;             /* this record is to be sent */
          int let;              /* let this message marked as not sent */
          int num;              /* message number */
          long offset;          /* starting offset */
          char proposal[256];   /* proposal line - FA .... */
          char subject[256];    /* message title */
          char *body;           /* message body */
          size_t size;          /* message size */
        } msgrec;

int bbs_connected = 0;   /* BBS is connected */

FILE *indexf;            /* index file */
char mycall[20];         /* my callsign */
char *maildir;           /* mail directory */
char *homebbs;           /* path to home BBS - port:CALL [CALL ...] */
char *homeaddr;          /* hierarchical address of home BBS */
int msgnum;              /* actual message number */
char mode = 'b';         /* p = personal, b = bulletin */
char title[256];         /* message data */
char from[256], to[256];
char mid[20], date[30], flags[10];
long size;               /* current message size */
long block_size;         /* current block size */
msgrec record[5];        /* msg. block - FBB allows max. 5 records */
int records;             /* number of records in actual block (max 5) */

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

/* fgets_cr - works like fgets, but uses CR as EOLN */
char *fgets_cr(char *s, int size, FILE *stream)
{
  int cnt = 0;
  int ch;
  while (cnt < size)
  {
    do ch = fgetc(stream); while (ch == EOF && errno == EINTR);
    if (ch == EOF) {s[cnt++] = '\0'; break;}
    s[cnt++] = ch;
    if (ch == '\r') {s[cnt++] = '\0'; break;}
  }
  return s;
}

/* strip SSID from callsign */
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

/* return the call from full address <call>@<bbs> */
char *user(const char *src)
{
  static char s[15];
  char *p = (char *)src;
  strcpy(s, "");
  while (*p && *p != '@' && strlen(s) < 6)
  {
    char c = toupper(*p);
    strncat(s, &c, 1);
    p++;
  }
  return s;
}

/* return the bbs address from full address <call>@<bbs> */
char *bbs(const char *src)
{
  static char addr[30];
  char *p;

  p = strchr(src, '@');
  if (p == NULL) strcpy(addr, "");
            else strcpy(addr, p+1);
  return addr;
}


/* -------------------------------------------------------------------- */

/* wait for BBS prompt ">" */
void wait_prompt()
{
  int old_char=0, new_char=0;
  do
  {
    old_char = new_char;
    new_char = fgetc(stdin);
  } while (old_char != '>' || new_char != '\r');
}

/* connect the BBS and switch to forward mode */
void connect_bbs(char *addr)
{
  bbs_connected = 1;
  lp_statline("Forward: connecting BBS");

  char cmd[255];
  sprintf(cmd, "c %s", addr);
  lp_wait_connect_prepare(lp_channel());
  lp_emit_event(lp_channel(), EV_DO_COMMAND, 0, cmd);

  char *call = strchr(addr, ':');
  if (call == NULL) call = addr; else call++;
  lp_wait_connect(lp_channel(), call);
  lp_statline("Forward: starting transfer");
  wait_prompt();
  printf("[LinPac-%s-BF$]\r", LPAPP_VERSION);
}

/* send FQ to disconnect BBS */
void disc_bbs()
{
  lp_statline("Forward: disconnecting BBS");
  printf("FQ\r");
  bbs_connected = 0;
  lp_emit_event(lp_channel(), EV_DISC_LOC, 0, NULL);
  lp_wait_event(lp_channel(), EV_DISC_FM);
}

/* -------------------------------------------------------------------- */

/* sends compressed data from file to output */
/* using FBB compressed protocol version 0 */
int send_data(char *subject, char *body, size_t size, FILE *tofile)
{
  const int BSIZE=250;
  signed char sum;
  unsigned pos, r;
  char *p;

  p = body;
  pos = 0;

  lp_disable_screen();

  striplf(subject);
  /* header */
  fputc(SOH, tofile);
  fputc(strlen(subject)+8, tofile);
  fprintf(tofile, "%s", subject);
  fputc(NUL, tofile);
  fprintf(tofile, "     0");
  fputc(NUL, tofile);
  /* data */
  sum = 0;
  while (pos < size)
  {
    r = (pos + BSIZE >= size) ? size-pos : BSIZE;
    fputc(STX, tofile);
    fputc(r, tofile);
    for (unsigned i=0; i < r; i++)
    {
      sum += *p;
      fwrite(p, sizeof(char), 1, tofile);
      p++;
    }
    pos += r;
  }
  fputc(EOT, tofile);
  fputc(-sum, tofile);

  lp_enable_screen();
  return 1;
}

/* mark a record in index file as forwarded */
void mark_record(int msgnum)
{
  long poz, last_line;
  int done = 0;
  char s[256], flags[256];
  int num;
  char *p;

  poz = ftell(indexf);
  rewind(indexf);

  while (!done && !feof(indexf))
  {
    last_line = ftell(indexf);
    fgets(s, 255, indexf);
    sscanf(s, "%i %s", &num, flags);
    p = strchr(flags, 'O');
    if (num == msgnum && p != NULL)
    {
      *p = 'F';
      fseek(indexf, last_line, SEEK_SET);
      fprintf(indexf, "%i\t%s", num, flags);
      break;
    }
  }
  fseek(indexf, poz, SEEK_SET);
}

/* send a block of messages */
void send_block()
{
  int i, blocks;
  char s[256];
  char *p;

  /* count messages */
  blocks = 0;
  for (i=0; i<5; i++)
    if (record[i].ok) blocks++;

  if (blocks == 0) return;
  if (!bbs_connected) connect_bbs(homebbs);

  lp_statline("forward: transfering messages");
  
  /* send proposals */
  for (i=0; i<5; i++)
    if (record[i].ok) printf(record[i].proposal);
  printf("F>\r");
  fgets_cr(s, 255, stdin);

  /* decode answer */
  p = s+3; /* 'FS ' */
  for (i=0; i<5; i++)
    if (record[i].ok)
    {
      if (*p == '+' || *p =='Y') {record[i].send = 1; record[i].let = 0; p++;}
      else if (*p == '-' || *p =='N') {record[i].send = 0; record[i].let = 0; p++;}
      else if (*p == '=' || *p =='L') {record[i].send = 0; record[i].let = 1; p++;}
      else p++;
    }
    
  /* send data */
  for (i=0; i<5; i++)
    if (record[i].ok && record[i].send)
    {
      //FILE *f = fopen("/root/out", "w");
      send_data(record[i].subject, record[i].body, record[i].size, stdout);
      //send_data(record[i].subject, record[i].body, record[i].size, f);
      //fclose(f);
    }

  fgets_cr(s, 255, stdin); /* wait for FF */

  /* mark sent messages */
  for (i=0; i<5; i++)
    if (record[i].ok)
    {
      if (!record[i].let) mark_record(record[i].num);
      record[i].ok = 0;
    }
  
  records = 0;
  block_size = 0;
}

/* prepare a message for sending in next block */
int forward_message(int num, char type, char *mid, char *from, char *to)
{
  FILE *msg;
  char s[256];
  char *body, *cbody;
  long size, csize;
  
  lp_statline("Forward: preparing message %i", num);

  sprintf(s, "%s/%i", maildir, num);
  msg = fopen(s, "r");
  if (msg == NULL)
  {
    fprintf(stderr, "forward: cannot open file %s\n", s);
    return 0;
  }

  fgets(title, 255, msg); /* read title */
  long beg = ftell(msg);  /* compute size of body */
  size = fseek(msg, 0, SEEK_END);
  size = ftell(msg) - beg;
  fseek(msg, beg, SEEK_SET);

  /* read the body and add CR */
  body = new char[size*2];
  int i = 0;
  while (!feof(msg))
  {
    fread(&body[i], 1, 1, msg);
    i++;
    if (body[i-1] == '\n')
    {
      body[i-1] = '\r';
      body[i] = '\n';
      i++;
      size++;
    }
  }
  //fread(body, 1, size, msg);
  fclose(msg);

  /* compress message */
  LZHufPacker pck;
  pck.Encode(size, body);
  csize = pck.GetLength();
  cbody = new char[csize+4]; //leave space for size info
  memcpy(&cbody[4], pck.GetData(), csize);
  delete[] body;
  /* add size of uncompressed file */
  long sz = size;
  for (i=0; i<4; i++)
  {
    cbody[i] = sz%256;
    sz /= 256;
  }
  csize += 4;

  /* if block is large enough, send it */
  if (block_size + size > MSG_BLOCK_SIZE) send_block();
  
  strcpy(s, user(from));
  sprintf(record[records].proposal, "FA %c %s %s %s %s %li\r",
          toupper(type), s, bbs(to), user(to), mid, size);
  strcpy(record[records].subject, title);
  record[records].body = cbody;
  record[records].size = csize;
  record[records].offset = 0;
  record[records].num = msgnum;
  record[records].ok = 1;
  records++;
  block_size += csize;
  if (records > 5) send_block();
  return 1;
}
  
/* scan message index and prepare messages marked for forward */
void start_forward()
{
  char fname[256], s[256];
  int num;

  records = 0;
  sprintf(fname, "%s/messages.local", maildir);
  indexf = fopen(fname, "r+");
  if (indexf == NULL)
  {
    printf("Cannot open message database %s\r", fname);
    exit(EXIT_FILE);
  }
  while (!feof(indexf))
  {
    int n;
    n = fscanf(indexf, "%i %s %s %s %s %s", &num, flags, mid, from, to, date);
    msgnum = num;
    fgets(s, 255, indexf);
    if (n == 6)
    {
      if (strchr(flags, 'O') != NULL)
        forward_message(num, flags[0], mid, from, to);
    }
  }
  send_block();
  fclose(indexf);
}

/* --------------------------------------------------------------------- */

void my_event_handler(Event *ev)
{
  switch (ev->type)
  {
    case EV_ABORT:  if (ev->data != NULL && ev->chn == lp_channel() &&
                        strcasecmp((char *)ev->data, ABORT_ADDR) == 0)
                    {
                      if (bbs_connected) lp_emit_event(lp_channel(), EV_DISC_LOC, 0, NULL);
                      lp_enable_screen();
                      exit(EXIT_ABORT);
                    }
                    break;
    case EV_DISC_FM:
    case EV_DISC_TIME:
    case EV_FAILURE: if (ev->chn == lp_channel() && bbs_connected)
                     {
                        lp_emit_event(ev->chn, EV_ENABLE_SCREEN, 0, NULL);
                        bbs_connected = 0;
                        printf("Forward: Link failure with BBS\n");
                        exit(EXIT_LINK);
                     }
                     break;
  }
}


int main(int argc, char **argv)
{
  if (lp_start_appl())
  {
    lp_set_event_handler(my_event_handler);
    maildir = lp_get_var(0, "MAIL_PATH");
    if (maildir == NULL)
    {
      printf("No MAIL_PATH@0 specified\r");
      return 1;
    }
  
    homebbs = lp_get_var(0, "HOME_BBS");
    if (homebbs == NULL)
    {
      printf("No HOME_BBS@0 specified\r");
      return 1;
    }
  
    homeaddr = lp_get_var(0, "HOME_ADDR");
    if (homeaddr == NULL)
    {
      printf("No HOME_ADDR@0 specified\r");
      return 1;
    }

    strcpy(mycall, call_call(lp_chn_call(lp_channel())));
    strcpy(from, mycall);
    /*maildir = "/root/myprog/packet/mail";
    homebbs = "kiss:CZ6PIG CZ6JBG";
    homeaddr = "CZ6PIG.#MOR.CZE.EU";
    strcpy(mycall, "CZ6JBG");
    strcpy(from, mycall);*/

    start_forward();
    if (bbs_connected) disc_bbs();
    lp_remove_statline();
    lp_end_appl();
  }
  else printf("LinPac is not running\r");
  return 0;
}

