/*
   BBS access API - base implemention
   (c) 2020 by Martin Cooper KD6YAM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

#include <string.h>
#include "lpapp.h"
#include "bbs.h"

BBS::BBS(char* homebbs_, char* homecall_, char* mailhome_)
{
    homebbs = homebbs_;
    homecall = homecall_;
    mailhome = mailhome_;
    abort_all = false;
}

BBS::~BBS()
{
}

void BBS::connect_bbs(char *addr)
{
    char cmd[255];
    sprintf(cmd, "c %s", addr);
    lp_wait_connect_prepare(lp_channel());
    lp_emit_event(lp_channel(), EV_DO_COMMAND, 0, cmd);
 
    char *call = strchr(addr, ':');
    if (call == NULL) call = addr; else call++;
    lp_wait_connect(lp_channel(), call);
    wait_prompt(stdin);
}

void BBS::disc_bbs()
{
    lp_emit_event(lp_channel(), EV_DISC_LOC, 0, NULL);
    lp_wait_event(lp_channel(), EV_DISC_LOC);
}

void BBS::stop_it()
{
    disc_bbs();
}

void BBS::abort()
{
    abort_all = true;
}

void BBS::set_kill_cmd(char* killcmd_)
{
    killcmd = killcmd_;
}

void BBS::save_msg(int num, bool pers, char* data, unsigned long length)
{
    char fname[256];

    get_msg_path(num, pers, fname);

    FILE *f = fopen(fname, "w");
    if (f != NULL)
    {
        fwrite(data, 1, length, f);
        fclose(f);
    }
    else
        fprintf(stderr, "getmsg: cannot write to file %s\n", fname);
}

void BBS::send_tag()
{
    // Do nothing by default
}

void BBS::set_limit(FILE *fin)
{
    // Do nothing by default
}

void BBS::sign_off(FILE *fin)
{
    // Do nothing by default
}

void BBS::delete_message(int num)
{
    char pref[64];
    char suf[64];
    strcpy(pref, killcmd); // Preconfigured kill command
    strcpy(suf, "");

    char *p = strchr(pref, '#');
    if (p != NULL)
    {
        *p = '\0';
        p++;
        strcpy(suf, p);
        printf("%s%i%s\r", pref, num, suf);
    }
    else
        printf("%s\n", pref);
}

void BBS::get_msg_path(int num, bool pers, char* path)
{
    #ifdef PERSONAL
    if (pers)
        sprintf(path, "%s/%s/%i", mailhome, homecall, num);
    else
        sprintf(path, "%s/%s/%i", MAILDIR, homecall, num);
    #else
    sprintf(path, "%s/%s/%i", MAILDIR, homecall, num);
    #endif
}
