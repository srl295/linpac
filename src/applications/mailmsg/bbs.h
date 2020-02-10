/*
   BBS access API
   (c) 2020 by Martin Cooper KD6YAM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

#define MAILDIR "/var/ax25/mail"
#define PERSONAL //storing messages to home directory supported

class BBS
{
    public:
        BBS(char* homebbs, char* homecall, char* mailhome);
        virtual ~BBS();

        // Usually not overridden
        virtual void connect_bbs(char *addr);
        virtual void disc_bbs();
        virtual void stop_it();
        virtual void abort();
        virtual void set_kill_cmd(char* killcmd);

        // Sometimes overridden but defaults available
        virtual void save_msg(int num, bool pers, char* data, unsigned long length);
        virtual void delete_message(int num);
        virtual void send_tag();
        virtual void set_limit(FILE *fin);
        virtual void sign_off(FILE *fin);

        // Must be implemented by subclass
        virtual int get_one_message(FILE *fin, char **buf, int *bsize, char *title) = 0;
        virtual void send_request(int num) = 0;
        virtual void wait_prompt(FILE *fin) = 0;

    protected:
        // Helper for save_msg
        virtual void get_msg_path(int num, bool pers, char* path);

        char* homebbs;
        char* homecall;
        char* mailhome;
        char* killcmd;
        bool abort_all;
};
