/*
   BBS access implementation for PBBS
   (c) 2020 by Martin Cooper KD6YAM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "lpapp.h"
#include "bbs.h"
#include "bbs_pbbs.h"

const char EOL = 0x0A;

PBBS::PBBS(char* homebbs, char* homecall, char* mailhome)
    : BBS(homebbs, homecall, mailhome)
{
}

PBBS::~PBBS()
{
}

int PBBS::get_one_message(FILE *fin, char **buf, int *bsize, char *title)
{
    char *buffer;
    int buffer_size, buffer_remain, buffer_inc, line_len;
    char line[256];
    bool in_headers = true;

    buffer_size = buffer_remain = buffer_inc = 1024;
    buffer = (char *) malloc(buffer_size);
    if (buffer == NULL)
    {
        fprintf(stderr, "getmsg: Out of memory\n");
        return MSG_RC_NO_MEMORY;
    }
    buffer[0] = '\0';
    buffer_remain--;

    lp_statline("Reading message");
    do
    {
        read_line(line, sizeof(line), fin);
        if (abort_all)
        {
            fprintf(stderr, "getmsg: aborted\n");
            return MSG_RC_ABORTED;
        }
        line_len = strlen(line);

        if (in_headers)
        {
            if (line_len == 1 && line[0] == EOL)
            {
                in_headers = false;
            }
            else if (strncmp(line, "SUBJECT: ", 9) == 0)
            {
                strcpy(title, line+9);
                title[strlen(title) - 1] = '\0';  // remove trailing EOL
                lp_statline("Reading message: `%s'", title);
            }
            else if (strncmp(line, "MESSAGE # NOT FOUND", 19) == 0)
            {
                return MSG_RC_MISSING;
            }
        }
        else
        {
            // Detect end of message
            if (at_prompt)
                break;
  
            if (line_len > buffer_remain)
            {
                buffer = (char *) realloc(buffer, buffer_size + buffer_inc);
                if (buffer == NULL)
                {
                    fprintf(stderr, "getmsg: Out of memory\n");
                    return MSG_RC_NO_MEMORY;
                }
                buffer_size += buffer_inc;
                buffer_remain += buffer_inc;
            }
            strcat(buffer, line);
            buffer_remain -= (line_len);
        }
    } while (true);

    *buf = buffer;
    *bsize = buffer_size - buffer_remain;
    return MSG_RC_SUCCESS;
}

void PBBS::wait_prompt(FILE *fin)
{
    char line[256];

    while (!at_prompt)
        read_line(line, sizeof(line), fin);
}

void PBBS::send_request(int num)
{
    printf("R %i\r", num);
}

void PBBS::set_limit(FILE *fin)
{
    wait_prompt(fin);
    printf("NL 99999\r");
    at_prompt = false; // Force read of next prompt
}

void PBBS::sign_off(FILE *fin)
{
    wait_prompt(fin);
    printf("B\r");
}

void PBBS::read_line(char *s, int size, FILE *stream)
{
    const char EOL_IN = 0x0D;
    bool had_eol = have_eol;
    int cnt = 0;
    int ch;

    have_eol = false;
    at_prompt = false;

    while (cnt < size - 1)
    {
        do ch = fgetc(stream); while (ch == EOF && errno == EINTR);
        if (ch == EOF)
            break;
        if (ch == EOL_IN) {
            s[cnt++] = EOL;
            have_eol = true;
            break;
        }
        s[cnt++] = ch;
    }
    s[cnt] = '\0';

    if (have_eol && had_eol)
    {
        int line_len = strlen(s);
        // Complete line (beginning to end); check for prompt
        if (strstr(s, "ENTER COMMAND: ") == s
                && s[line_len-2] == '>'
                && s[line_len-1] == EOL)
            at_prompt = true;
    }
}
