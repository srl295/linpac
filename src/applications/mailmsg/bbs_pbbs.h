/*
   BBS access API for PBBS
   (c) 2020 by Martin Cooper KD6YAM

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.
*/

class PBBS : public BBS {
    public:
        PBBS(char* homebbs, char* homecall, char* mailhome);
        virtual ~PBBS();

        virtual int get_one_message(FILE *fin, char **buf, int *bsize, char *title);
        virtual void wait_prompt(FILE *fin);
        virtual void send_request(int num);
        virtual void set_limit(FILE *fin);
        virtual void sign_off(FILE *fin);

    private:
        void read_line(char *s, int size, FILE *stream);

        bool at_prompt;
        bool have_eol;
};

