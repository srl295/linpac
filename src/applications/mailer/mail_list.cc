/*
  LinPac Mailer
  (c) 1998 - 2000 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_list.cc - message list viewer and text viewer
*/
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <cstdlib>
#include <sys/types.h>
#include "mail_list.h"
#include "mail_comp.h"
#include "mail_help.h"
#include "mail_files.h"

#define MAIL_PATH "/var/ax25/mail"
#define LIST_PATH "/var/ax25/ulistd"

#define GETMSG "getmsg"
#define DELMSG "delmsg"
#define TAB_SIZE 8
#define CTRLX 24

int atable = 0; //actual translation table
HelpWindow *helper = NULL; //help window

bool comment;

bool isnum(char *s)
{
  char *p = s;
  if (!*p) return false;
  while (*p) if (!isdigit(*p)) return false;
             else p++;
  return true;
}

char *outbuffer = NULL;

char *extract(const char *start, const char *stop)
{
   if (outbuffer != NULL) delete[] outbuffer;
   outbuffer = new char[(stop-start)+1];
   int i = 0;
   for (char *p = (char *)start; p < stop; p++)
   {
      outbuffer[i] = *p;
      i++;
   }
   outbuffer[i] = '\0';
   return outbuffer;
}

//=========================================================================
// Class Messages (list of messages)
//=========================================================================
//Possible states
const int STATE_NORM = 0; //normal state (no actions in progress)
const int STATE_WAIT = 1; //waiting for start of download
const int STATE_DNLD = 2; //download in progress

//Folders
const int FOLDER_INCOMMING = 0;
const int FOLDER_OUTGOING = 1;
const int NUM_FOLDERS = 2;

Messages::Messages(char *bbs_name, char *call, bool all)
{
   strcpy(class_name, "MESSAGES");
   bbs = strdup(bbs_name);
   mycall = strdup(call);

   folder = FOLDER_INCOMMING;
   ndx = new IncommingIndex(bbs);
   ndx->checkPresence();
   load_list(all);

   priv = all;
   slct = last_read();
   pos = slct;
   viewer = NULL;
   state = STATE_NORM;
}

void Messages::clear_msgs()
{
   msg.erase(msg.begin(), msg.end());
}

void Messages::load_list(bool all)
{
   for (int index = 0; index < ndx->messageCount(); index++)
   {
      if ((all || readable(index)) && check_filter(index))
      {
         Msg newmsg;
         newmsg.index = index;
         newmsg.select = false;
         msg.push_back(newmsg);
      }
   }
   create_list(ndx, msg, boards, mycall);
}

void Messages::reload(bool all)
{
   msg.erase(msg.begin(), msg.end());
   load_list(all);
}

long Messages::max()
{
   std::vector <Msg>::iterator it;
   int mx = 0;
   for (it = msg.begin(); it < msg.end(); it++)
   {
      int current = ndx->getMessage(it->index)->getNum();
      if (current > mx) mx = current;
   }
   return mx;
}

long Messages::min()
{
   std::vector <Msg>::iterator it;
   int mx = -1;
   for (it = msg.begin(); it < msg.end(); it++)
   {
      int current = ndx->getMessage(it->index)->getNum();
      if (current < mx || mx == -1) mx = current;
   }
   return mx;
}

long Messages::last_read()
{
   std::vector <Msg>::iterator it;
   int m = 0;
   int i;
   for (i = 0, it = msg.begin(); it < msg.end(); it++,i++)
      if (ndx->getMessage(it->index)->isPresent()) m = i;
   return m;
}

bool Messages::present(int num)
{
   int index = ndx->msgNum(num);
   return ((index > 0) && ndx->getMessage(index)->isPresent());
}

bool Messages::readable(int index)
{
   //never display deleted messages
   if (strchr(ndx->getMessage(index)->getFlags(), 'D') != NULL) return false;
   //always display bulletins
   if (strchr(ndx->getMessage(index)->getFlags(), 'B') != NULL) return true;
   //display personal messages for the user
   unsigned len = strlen(mycall);
   char *dest = ndx->getMessage(index)->getDest();
   if ((strncmp(dest, mycall, len) == 0) &&
       (dest[len] == '@' || strlen(dest) == len)) return true;
   return false;
}

bool Messages::check_filter(int index)
{
   if (bfilter.empty()) return true;
   char *dest = strdup(ndx->getMessage(index)->getDest());
   int cmp;
   char *p = strchr(dest, '@'); if (p != NULL) *p = 0;
   for (bfit = bfilter.begin(); bfit < bfilter.end(); bfit++)
   {
      cmp = strcasecmp(bfit->name, dest);
      if (cmp == 0) return true;
      if (cmp > 0) break;
   }
   return false;
}

void Messages::del_msg(std::vector <Msg>::iterator where) //delete message
{
   msg.erase(where);
}

//========================================================================
// Class TheFile (message viewer);
//========================================================================

TheFile::TheFile(Message *themsg)
{
   strcpy(class_name, "VIEWER");
   xpos = 0;
   pos = 0;
   head = false;
   wrap = true;
   wait_reply = false;
   wait_sname = false;
   msg = themsg;
   //load_message(head);
}

TheFile::~TheFile()
{
   std::vector <char *>::iterator it;
   for (it = line.begin(); it < line.end(); it++)
      if (*it != NULL) delete [] (*it);
}

void TheFile::destroy_message()
{
   std::vector <char *>::iterator it;
   for (it = line.begin(); it < line.end(); it++)
      if (*it != NULL) delete[] (*it);
   line.erase(line.begin(), line.end());
}

void TheFile::load_message(bool head)
{
   char *p1, *p2;

   destroy_message();
   attach = 0;
   p1 = msg->getBody();

   if (p1)
   {
      char lastln[256];
      strcpy(lastln, "");

      bool in7p = false;
      char name7p[256];

      while (*p1)
      {
         p2 = p1;
         while (*p2 && *p2 != '\n') p2++; if (*p2) p2++;
         
         char *s = strdup(extract(p1, p2-1));
  
         //detect 7plus
         if (strncmp(s, " go_7+.", 7) == 0 && !in7p)
         {
            in7p = true;
         }
         if (strncmp(s, " stop_7+.", 9) == 0 && in7p)
         {
            //find name
            strcpy(name7p, "");
            char *p = strchr(s, '(');
            if (p)
            {
               p++;
               while (*p && *p != ')' && *p != '/' && strlen(p) < 255)
               {
                  char ch = tolower(*p);
                  strncat(name7p, &ch, 1);
                  p++;
               }
            }
  
            in7p = false;
            attach++;
            char msg[100];
            sprintf(msg, "--- Message attachment no. %i", attach);
            line.push_back(strdup(msg));
            sprintf(msg, "--- This part of message is 7plus encoded (%.32s)", name7p);
            line.push_back(strdup(msg));
            sprintf(msg, "--- Press X to save this part of message");
            line.push_back(strdup(msg));
            delete[] s;
            p1 = p2;
            continue;
         }
       
         if (!in7p)
         {
            //expand TABs
            char ln[256];
            strcpy(ln, lastln);
            for (unsigned i=0; i<strlen(s); i++)
            {
              if (s[i] == '\t')
                do strcat(ln, " "); while (strlen(ln)%TAB_SIZE > 0);
              else
                strncat(ln, &s[i], 1);
            }
     
            //break too long lines
            if (strncmp(s, "R:", 2) != 0)
            {
               bool broken = false;
               bool answer = false;
               while ((int)strlen(ln) > xsize-3 && wrap)
               {
                  if (ln[0] == '>') answer = true;
                  char *p = &(ln[xsize-4]);            //find the last space on line
                  while (p > ln && !isspace(*p)) p--;
                  if (p == ln) p = &(ln[xsize-3]);     //no spaces found
                  *p = '\0';
                  char *store = new char[strlen(ln)+1];//store the line
                  strcpy(store, ln);
                  line.push_back(store);
                  p++;                                 //continue next part
                  //strcpy(ln, p);
                  memmove(ln, p, strlen(p)+1);
                  broken = true;
               }
               if (broken && !answer)
               {
                  strcpy(lastln, ln);
                  strcat(lastln, " ");
               }
               else
               {
                  char *store = new char[strlen(ln)+1];
                  strcpy(store, ln);
                  line.push_back(store);
                  strcpy(lastln, "");
               }
            }
            else if (head)
            {
               char *store = new char[strlen(ln)+1];
               strcpy(store, ln);
               line.push_back(store);
            }
         }

         delete[] s;
         p1 = p2; //next line
      }
   }
   else
   {
      char *store = new char[32];
      sprintf(store, "Message %i is not present.", msg->getNum());
      line.push_back(store);
   }
}

void TheFile::extract_attach(int num)
{
   char *p1, *p2;

   p1 = msg->getBody();

   if (p1)
   {
      char lastln[256];
      strcpy(lastln, "");

      bool in7p = false;
      bool saving = false;
      FILE *outf;
      int atcnt = 0;
      char name7p[256];
      char *tmp = strdup("/tmp/lp_mail_ea.XXXXXX");
      int tmpdesc = mkstemp(tmp);
      close(tmpdesc);
      //char *tmp = strdup(tmpnam(NULL));

      while (*p1)
      {
         p2 = p1;
         while (*p2 && *p2 != '\n') p2++; if (*p2) p2++;
         
         char *s = strdup(extract(p1, p2-1));

         //detect 7plus
         if (strncmp(s, " go_7+.", 7) == 0 && !in7p)
         {
            in7p = true;
            atcnt++;
            if (atcnt == num)
            {
               saving = true;
               outf = fopen(tmp, "w");
               if (!outf) errormsg("Cannot create temp file");
            }
         }
  
         if (in7p && saving && outf)
         {
            fprintf(outf, "%s\n", s);
         }
         
         if (strncmp(s, " stop_7+.", 9) == 0 && in7p)
         {
            //find name
            strcpy(name7p, "");
            char *p = strchr(s, '(');
            if (p)
            {
               p++;
               while (*p && *p != ')' && *p != '/' && strlen(p) < 255)
               {
                  char ch = tolower(*p);
                  strncat(name7p, &ch, 1);
                  p++;
               }
            }
  
            in7p = false;
  
            if (saving)
            {
               fclose(outf);
               char *name = save_file_path(name7p);
               if (rename(tmp, name) == -1)
                 errormsg("Cannot create %s", name);
               else
                 errormsg("Attachment saved to %s", name);
               delete[] name;
               saving = false;
            }
         }
         delete[] s;
         p1 = p2; //next line
      }
   } else errormsg("Cannot open message file");
}

unsigned TheFile::max_len()
{
  unsigned r = 0;
  std::vector <char *>::iterator it;
  for (it = line.begin(); it < line.end(); it++)
    if (strlen(*it) > r) r = strlen(*it);
  return r;
}

bool TheFile::return_addr(char *result)
{
  std::vector <char *>::iterator it;

  //try to find "Reply-To:" line
  for (it = line.begin(); it < line.end(); it++)
  {
    if (strncasecmp(*it, "Reply-To:", 9) == 0)
    {
      char *p = *it + 9; //read the address
      char *q = result;
      while (*p)
      {
        if (!isspace(*p)) //skip spaces
        {
          *q = *p;
          q++;
        }
        p++;
      }
      *q = '\0';
      return true;
    }
  }

  //Reply address not found, use the address from the last R: line
  if (!head) load_message(true); //We need the header
  char rroute[40];
  it = line.begin();
  while (it < line.end() && strncmp(*it, "R:", 2) == 0) it++;
  if (it > line.begin())
  {
    it--; //return to the last R: line
    char *p = strstr(*it, "@:");
    if (p != NULL)
    {
      p += 2;
      char *q = rroute;
      while (*p && !isspace(*p))
      {
        *q = *p; p++; q++;
      }
      *q = '\0';
      sprintf(result, "%s@%s", msg->getSrc(), rroute);
      if (!head) load_message(head);
      return true;
    }
  }

  //not found anywhere
  if (!head) load_message(head);
  return false;
}

//==========================================================================
// Functions using ncurses (conflicting)
//==========================================================================
#include <ncurses.h>

void Messages::init_screen(void *pwin, int height, int width, int wy, int wx)
{
   xsize = width;
   ysize = height;
   x = wx;
   y = wy;
 
   WINDOW *win = subwin(reinterpret_cast<WINDOW *>(pwin), ysize, xsize, y, x);
   mwin = win;
   keypad(win, true);
   meta(win, true);
   nodelay(win, true);
 
   wbkgdset(win, ' ' | COLOR_PAIR(C_TEXT) | A_BOLD);
   werase(win);
   box(win, ACS_VLINE, ACS_HLINE);
   mvwprintw(win, 0, 2, "%s", bbs);
   draw();
 
   old_focus_window = focus_window;
   focus_window = win;
   old_focused = focused;
   focused = this;
 
   help(HELP_LIST);
}

void Messages::draw_line(int i)
{
   WINDOW *win = reinterpret_cast<WINDOW *>(mwin);

   if (i+pos < (int)msg.size())
   {
      long attr;
      int index = msg[i+pos].index; //index of current message

      if (slct == i+pos)
      {
         attr = COLOR_PAIR(C_SELECT);
         if (ndx->getMessage(index)->isPresent()) attr |= A_BOLD;
      }
      else
      {
         attr = COLOR_PAIR(C_UNSELECT);
         if (ndx->getMessage(index)->isPresent()) attr |= A_BOLD;
      }
      
      wbkgdset(win, ' ' | attr);
  
      int sl;
      if (msg[i+pos].select) sl = ACS_BULLET;
                        else sl = ' ';

      char subj[256];
      strncpy(subj, ndx->getMessage(index)->getSubj(), 255); subj[255] = '\0';
      for (unsigned j=0; j<strlen(subj); j++) //convert subject's charset
         if (subj[j] < tabsize) subj[j] = ttable[subj[j]];
                        
      if (folder == FOLDER_INCOMMING)
         mvwprintw(win, i+2, 2, "%6i%c%2s %5i %-6.6s %-7.7s %-6.6s %6.6s %-*.*s",
                   ndx->getMessage(index)->getNum(), sl,
                   ndx->getMessage(index)->getFlags(),
                   ndx->getMessage(index)->getSize(),
                   ndx->getMessage(index)->getDest(),
                   ndx->getMessage(index)->getDPath(),
                   ndx->getMessage(index)->getSrc(),
                   ndx->getMessage(index)->getDate().toStringShort(),
                   xsize-49, xsize-50, subj);
      if (folder == FOLDER_OUTGOING)
         mvwprintw(win, i+2, 2, "%6i%c%2s %-20.20s %-6.6s %6.6s %-*.*s",
                   ndx->getMessage(index)->getNum(), sl,
                   ndx->getMessage(index)->getFlags(),
                   ndx->getMessage(index)->getDest(),
                   ndx->getMessage(index)->getSrc(),
                   ndx->getMessage(index)->getDate().toStringShort(),
                   xsize-49, xsize-50, subj);
   }
   else
   {
      wbkgdset(win, COLOR_PAIR(C_TEXT));
      mvwprintw(win, i+2, 2, "%*s", xsize-4, "");
   }
}

void Messages::draw(bool all)
{
   WINDOW *win = reinterpret_cast<WINDOW *>(mwin);
 
   if (all)
   {
      wbkgdset(win, ' ' | COLOR_PAIR(C_TEXT) | A_BOLD);
      werase(win);
      box(win, ACS_VLINE, ACS_HLINE);

      char sf[32];
      if (folder == FOLDER_INCOMMING) strcpy(sf, "Incomming mail");
      if (folder == FOLDER_OUTGOING) strcpy(sf, "Outgoing mail");

      mvwprintw(win, 0, 2, "%s: %s", bbs, sf);
      if (state == STATE_WAIT) mvwprintw(win, 0, 25, " (starting download) ");
      if (state == STATE_DNLD) mvwprintw(win, 0, 25, " (downloading messages) ");
   }
   
   for (int i=0; i<ysize-3; i++) draw_line(i);

   wrefresh(win);
   wnoutrefresh(win);
   for (int i = 0; i < 5; i++) doupdate(); //must be done many times, don't know why
}


void Messages::handle_event(Event *ev)
{
   if (view_returned)
   {
      if (viewer != NULL) {delete viewer; viewer = NULL;}
      view_returned = false;
   }
   if (blist_returned)
   {
      if (blister != NULL) {delete blister; blister = NULL;}
      reload(priv);
      slct = last_read();
      pos = slct;
      blist_returned = false;
      //draw(true);
   }

   if (ev == NULL) return; //called just for previous code

   int cur = 0;
   if (slct < count())
      cur = msg[slct].index; //index of current message

   if (ev->type == EV_KEY_PRESS)
   {
      if (ev->x == KEY_DOWN)
      {
         if (slct < count()-1)
         {
            slct++;
            if (slct-pos >= ysize-3)
            {
               pos++;
               draw();
            }
            else
            {
               draw_line(slct-1-pos);
               draw_line(slct-pos);
               wrefresh((WINDOW *)mwin);
            }
         }
      }
      if (ev->x == KEY_UP)
      {
         if (slct > 0)
         {
            slct--;
            if (slct-pos < 0)
            {
               pos--;
               draw();
            }
            else
            {
               draw_line(slct+1-pos);
               draw_line(slct-pos);
               wrefresh((WINDOW *)mwin);
            }
         }
      }
      if (ev->x == KEY_NPAGE)
      {
         for (int j=0; j < (ysize-3); j++)
         {
            if (slct < count()-1) slct++;
            if (slct-pos >= ysize-3) pos++;
         }
      }
      if (ev->x == KEY_PPAGE)
      {
         for (int j=0; j < (ysize-3); j++)
         {
            if (slct > 0) slct--;
            if (slct-pos < 0) pos--;
         }
      }
  
      if (ev->x == ' ' && state == STATE_NORM)
         msg[slct].select = !msg[slct].select;
  
      if (toupper(ev->x) == 'P')
      {
         int anum = ndx->getMessage(cur)->getNum();
         int aofs = slct - pos;
         //reload table
         priv = !priv;
         reload(priv);
         //calculate new cursor position
         int i = 0;
         while (i < count() &&
                ndx->getMessage(msg[i].index)->getNum() < anum) i++;
         slct = i;           //new cursor position
         pos = slct - aofs;  //new window start position
         if (pos < 0) pos = 0;
      }
  
      if (toupper(ev->x) == 'Q')
      {
         //delwin(reinterpret_cast<WINDOW *>(mwin));
         focus_window = old_focus_window;
         focused = old_focused;
      }
  
      if (toupper(ev->x) == 'D' && state == STATE_NORM) //Delete messages
      {
         std::vector <Msg>::iterator it;

         //Count marked messages
         int sel = 0;
         for (it = msg.begin(); it < msg.end(); it++)
         {
            int cur = it->index;
            if (it->select && !ndx->getMessage(cur)->isPresent())
                sel++;
         }

         //Some messages selected
         if (sel > 0)
         {
            //Create the command
            char *cmd = new char[64 + sel*32];
            strcpy(cmd, GETMSG);
            for (it = msg.begin(); it < msg.end(); it++)
            {
               char num[32];
               int cur = it->index;
               if (it->select && !ndx->getMessage(cur)->isPresent())
               {
                  if (strchr(ndx->getMessage(cur)->getFlags(), 'P') != NULL)
                     sprintf(num, " P%i", ndx->getMessage(cur)->getNum()); //personal
                  else
                     sprintf(num, " %i", ndx->getMessage(cur)->getNum());  //bulletins
                  strcat(cmd, num);
               }
            }

            //Start the download
            lp_emit_event(lp_channel(), EV_DO_COMMAND, strlen(cmd), cmd);
            state = STATE_WAIT;
            if (act) draw(true);
            delete[] cmd;
         }
      }
  
      if (ev->x == CTRLX) //Ctrl-X : delete messages
      {
         std::vector <Msg>::iterator it;

         //Count marked messages
         int sel = 0;
         for (it = msg.begin(); it < msg.end(); it++)
            if (it->select) sel++;

         char *cmd;
         
         //Some messages selected
         if (sel > 0)
         {
            //Create the command
            cmd = new char[64 + sel*32];
            strcpy(cmd, DELMSG);

            for (it = msg.begin(); it < msg.end(); it++)
            {
               if (it->select)
               {
                  char num[32];
                  sprintf(num, " %i", ndx->getMessage(it->index)->getNum());  //bulletins
                  strcat(cmd, num);
               }
            }

            //delete the messages from the list
            bool deleted = true;
            while (deleted)
            {
                deleted = false;
                for (it = msg.begin(); it < msg.end(); it++)
                   if (it->select)
                   {
                      del_msg(it);
                      deleted = true;
                      break;
                   }
            }
            
            
         }
         else //no messages were selected, delete actual one
         {
            cmd = new char[128];
            it = msg.begin();
            for (int i = 0; i < slct; i++) it++;
            sprintf(cmd, "%s %i", DELMSG, ndx->getMessage(it->index)->getNum());
            del_msg(it);
         }
         lp_emit_event(lp_channel(), EV_DO_COMMAND, strlen(cmd), cmd);
         delete[] cmd;
         if (slct >= count()) slct = count()-1;
  
         if (act) draw(true);
      }
  
      if (toupper(ev->x) == 'T')
      {
         atable++;
         if (atable >= ntables) atable = 0;
         load_table(atable);
         draw();
      }

      if (toupper(ev->x) == 'I')
      {
         folder++;
         if (folder >= NUM_FOLDERS) folder = 0;
         if (folder == FOLDER_INCOMMING)
         {
            clear_msgs();
            delete ndx;
            ndx = new IncommingIndex(bbs);
            ndx->checkPresence();
            load_list(priv);
            slct = last_read();
            pos = slct;
            if (act) draw(true);
         }
         if (folder == FOLDER_OUTGOING)
         {
            clear_msgs();
            delete ndx;
            ndx = new OutgoingIndex(bbs);
            ndx->checkPresence();
            load_list(true);
            slct = last_read();
            pos = slct;
            if (act) draw(true);
         }      
      }
      else if (toupper(ev->x) == 'H')
      {
         if (helper == NULL)
            helper = new HelpWindow(reinterpret_cast<WINDOW *>(mwin), ysize, xsize, y, x);
         else
            helper->show();
      }
      else if (toupper(ev->x) == 'C')
      {
         strcpy(old_help, HELP_LIST);
         start_composer(mwin, x, y, x+xsize-1, y+ysize-1, NULL, NULL);
      }
      else if (toupper(ev->x) == 'F')
      {
         blister = new BoardList(boards);
         blister->init_screen(reinterpret_cast<WINDOW *>(mwin),
                               ysize, xsize, y, x);
      }
      else if (ev->x == '\n')
      {
         viewer = new TheFile(ndx->getMessage(msg[slct].index));
         viewer->init_screen(reinterpret_cast<WINDOW *>(mwin),
                             ysize, xsize, y, x);
      }
      else
         if (ev->x != KEY_DOWN && ev->x != KEY_UP) draw();
   }

   if (ev->type == EV_APP_MESSAGE)
   {
      char *p = reinterpret_cast<char *>(ev->data);
      if (strcmp(p, "getmsg: start") == 0 && state == STATE_WAIT)
      {
         state = STATE_DNLD;
         if (act) draw(true);
      }
      else if (strcmp(p, "getmsg: done") == 0 && state == STATE_DNLD)
      {
         state = STATE_NORM;
         //reload(priv);     //there should't be the need to reload
         //if (act) draw();
      }
      else if (strncmp(p, "getmsg: got ", 12) == 0 /*&& state == STATE_DNLD*/)
      {
         char *np = p + 12;
         int num = atoi(np);
         for (unsigned i = 0; i < msg.size(); i++)
         {
            int mnum = ndx->getMessage(msg[i].index)->getNum();
            if (mnum == num && msg[i].select)
            {
               msg[i].select = false;
               ndx->getMessage(msg[i].index)->setPresence(true);
               if (act && focused == this) draw();
            }
            if (mnum >= num) break;
          }
       }
   }
}

//------------------------------------------------------------------------

void TheFile::init_screen(void *pwin, int height, int width, int wy, int wx)
{
   xsize = width;
   ysize = height;
   x = wx;
   y = wy;
 
   load_message(head);
 
   char *subj = strdup(msg->getSubj());
   for (unsigned j=0; j<strlen(subj); j++)
      if (subj[j] < tabsize) subj[j] = ttable[subj[j]];
 
   //WINDOW *win = subwin(reinterpret_cast<WINDOW *>(pwin), ysize, xsize, y, x);
   WINDOW *win = reinterpret_cast<WINDOW *>(pwin);
   mwin = win;
   keypad(win, true);
   meta(win, true);
   nodelay(win, true);
 
   wbkgdset(win, ' ' | COLOR_PAIR(C_MSG) | A_BOLD);
   werase(win);
   box(win, ACS_VLINE, ACS_HLINE);
   mvwprintw(win, 0, 2, "Message %i:%s", msg->getNum(), subj);
   draw();
 
   old_focus_window = focus_window;
   focus_window = win;
   old_focused = focused;
   focused = this;
 
   help(HELP_VIEW);
   delete[] subj;
}

void TheFile::draw(bool all)
{
   WINDOW *win = reinterpret_cast<WINDOW *>(mwin);

   char *subj = strdup(msg->getSubj());
   for (unsigned j=0; j<strlen(subj); j++)
      if (subj[j] < tabsize) subj[j] = ttable[subj[j]];

   if (all)
   {
      wbkgdset(win, ' ' | COLOR_PAIR(C_MSG) | A_BOLD);
      werase(win);
      box(win, ACS_VLINE, ACS_HLINE);
      mvwprintw(win, 0, 2, "Message %i:%s", msg->getNum(), subj);
   }
   
   for (int i=0; i<ysize-3; i++)
   {
      char s[512];
      if (i+pos < line.size() && strlen(line[i+pos]) > xpos)
         sprintf(s, "%-*.*s", xsize-3, xsize-3, &(line[i+pos][xpos]));
      else
         sprintf(s, "%-*.*s", xsize-3, xsize-3, "");
      if (s[strlen(s)-1] == '\n') s[strlen(s)-1] = '\0';
      for (unsigned j=0; j<strlen(s); j++)
         if (s[j] < tabsize) s[j] = ttable[s[j]];
      mvwaddstr(win, i+2, 1, s);
   }
   wnoutrefresh(win);
   for (int i = 0; i < 5; i++) doupdate(); //must be done many times, don't know why
   delete[] subj;
}

void TheFile::handle_event(Event *ev)
{
   if (ev == NULL)
   {
      if (wait_sname)
      {
         wait_sname = false;
         char *name = save_file_path(ibuffer);
         FILE *f = fopen(name, "w");
         if (f != NULL)
         {
            for (unsigned i = 0; i < line.size(); i++)
            {
               fputs(line[i], f);
               fputc('\n', f);
            }
            fclose(f);
            errormsg("Message saved to %s", name);
         } else errormsg("Cannot create %s", name);
         delete[] name;
      }
      if (wait_xnum)
      {
         wait_xnum = false;
         int atnum;
         if (attach > 1) atnum = atoi(ibuffer); //more attachments
                    else atnum = 1;             //only one
         extract_attach(atnum);
      }
      return;
   }
   if (ev->type == EV_KEY_PRESS)
   {
      clear_error();
      if (wait_reply) //waiting for a reply for if to insert original message
      {
         int ch = toupper(ev->x);
         if (ch == 'Y' || ch == 'N')
         {
            //create new subject with Re: or Re^x:
            char *resubj = new char[strlen(msg->getSubj())+10];
            char *p = msg->getSubj(); while (*p && isspace(*p)) p++;
            if (strncasecmp(p, "RE", 2) == 0)
            {
               p += 2;
               while (*p && isspace(*p)) p++;
               if (*p == ':') sprintf(resubj, "Re^2:%s", p+1);
               else if (*p == '^')
               {
                  char *endptr;
                  p++;
                  int n = strtol(p, &endptr, 10);
                  sprintf(resubj, "Re^%i%s", n+1, endptr);
               }
               else sprintf(resubj, "Re:%s", msg->getSubj());
            }
            else sprintf(resubj, "Re:%s", msg->getSubj());
    
            //determine reply address
            char reply_addr[50];
            if (!comment)
            {
               if (!return_addr(reply_addr))
                  strcpy(reply_addr, msg->getSrc());
            }
            else
            {
               if (*(msg->getDPath()) == '@')
                  snprintf(reply_addr, 49, "%s%s", msg->getDest(), msg->getDPath());
               else
                  snprintf(reply_addr, 49, "%s@", msg->getDest());
               reply_addr[49] = '\0';
               //strcpy(reply_addr, msg->getDest());
            }
              
            strcpy(old_help, HELP_VIEW);
            start_composer(mwin, x, y, x+xsize-1, y+ysize-1, reply_addr, resubj);
            if (ch == 'Y')
            {
               unsigned i, j;
               bool hdr = false;
     
               //find first line of the message
               j = 0;
               for (i = 0; i < line.size(); i++)
               {
                  if (strncmp("R:", line[i], 2) == 0) j = i;
                  if (strncmp("From:", line[i], 5) == 0) hdr = true;
                  if (hdr && strlen(line[i]) == 0) {j = i; break;}
               }
     
               //insert the lines
               for (i = j; i < line.size(); i++)
               {
                  //convert charset
                  char *newl = strdup(line[i]);
                  for (unsigned u=0; u<strlen(newl); u++)
                     if (newl[u] < tabsize) newl[u] = ttable[newl[u]];
                  //insert toe ditor
                  comp_insert(newl);
               }
               comp_edredraw();
            }
    
            wait_reply = false;
         }
         return;
      }
  
      if (ev->x == KEY_DOWN)
      {
         if (pos < line.size()-ysize+3) pos++;
      }
      if (ev->x == KEY_UP)
      {
         if (pos > 0) pos--;
      }
      if (ev->x == KEY_RIGHT)
      {
         if (xpos < max_len()-xsize+4) xpos++;
      }
      if (ev->x == KEY_LEFT)
      {
         if (xpos > 0) xpos--;
      }
      if (ev->x == KEY_NPAGE || ev->x == ' ')
      {
         for (int j=0; j < (ysize-3); j++)
            if (pos < line.size()-ysize+3) pos++;
      }
      if (ev->x == KEY_PPAGE)
      {
         for (int j=0; j < (ysize-3); j++)
            if (pos > 0) pos--;
      }
  
      if (toupper(ev->x) == 'H')
      {
         head = !head;
         load_message(head);
      }
  
      if (toupper(ev->x) == 'W')
      {
         wrap = !wrap;
         load_message(head);
      }
  
      if (toupper(ev->x) == 'T')
      {
         atable++;
         if (atable >= ntables) atable = 0;
         load_table(atable);
         draw();
      }
  
      if (toupper(ev->x) == 'Q')
      {
         focused = NULL; //exit immediatelly
      }
  
      if (toupper(ev->x) == 'R')
      {
         WINDOW *win = (WINDOW *)mwin;
         wbkgdset(win, ' ' | COLOR_PAIR(C_ERROR));
         mvwprintw(win, ysize-2, 1, " Include original message in reply ?  ");
         wmove(win, ysize-2, 38);
         wbkgdset(win, ' ' | COLOR_PAIR(C_MSG));
         wrefresh(win);
         wait_reply = true;
         comment = false;
      }
      else if (toupper(ev->x) == 'C')
      {
         WINDOW *win = (WINDOW *)mwin;
         wbkgdset(win, ' ' | COLOR_PAIR(C_ERROR));
         mvwprintw(win, ysize-2, 1, " Include original message in comment ?  ");
         wmove(win, ysize-2, 39);
         wbkgdset(win, ' ' | COLOR_PAIR(C_MSG));
         wrefresh(win);
         wait_reply = true;
         comment = true;
      }
      else if (toupper(ev->x) == 'F')
      {
         strcpy(old_help, HELP_VIEW);
         char *dest = new char[strlen(msg->getDest()) +
                               strlen(msg->getDPath()) + 2];
         sprintf(dest, "%s%s", msg->getDest(), msg->getDPath());
         start_composer(mwin, x, y, x+xsize-1, y+ysize-1,
                        dest, msg->getSubj());
         delete[] dest;

         //Copy original contents
         unsigned i, j;
         bool hdr = false;
         //find first line of the message
         j = 0;
         for (i = 0; i < line.size(); i++)
         {
            if (strncmp("R:", line[i], 2) == 0) j = i;
            if (strncmp("From:", line[i], 5) == 0) hdr = true;
            if (hdr && strlen(line[i]) == 0) {j = i; break;}
         }
         //insert the lines
         for (i = j; i < line.size(); i++)
         {
            //convert charset
            char *newl = strdup(line[i]);
            for (unsigned u=0; u<strlen(newl); u++)
               if (newl[u] < tabsize) newl[u] = ttable[newl[u]];
            //insert to editor
            comp_insert(newl, false);
         }
         comp_edredraw();
      }
      else if (ev->x == '\x1b' || toupper(ev->x) == 'L' ||
               toupper(ev->x) == 'I' || ev->x == '<')
      {
         view_returned = true;
         focus_window = old_focus_window;
         focused = old_focused;
         focused->draw(true);
         help(HELP_LIST);
         destroy_message();
      }
      else if (toupper(ev->x) == 'S')
      {
         wait_sname = true;
         WINDOW *win = (WINDOW *)mwin;
         iline = new InputLine(win, 2, ysize-1, xsize-2, xsize-16,
                               "Save to file:", ibuffer, INPUT_ALL);
      }
      else if (toupper(ev->x) == 'X')
      {
         if (attach > 1) //more than one attachment
         {
            wait_xnum = true;
            WINDOW *win = (WINDOW *)mwin;
            iline = new InputLine(win, 2, ysize-1, xsize-2, 5,
                                  "Attachment no. to save:", ibuffer, INPUT_NUM);
         }
         else if (attach == 1) extract_attach(1);
         else errormsg("No attachments");
      }
      else draw();
   }
}

void TheFile::errormsg(const char *fmt, ...)
{
   va_list argptr;
   char buf[256];
 
   va_start(argptr, fmt);
   vsprintf(buf, fmt, argptr);
   va_end(argptr);
   
   WINDOW *win = (WINDOW *)mwin;
   wbkgdset(win, ' ' | COLOR_PAIR(C_ERROR));
   mvwprintw(win, ysize-2, 1, " %s ", buf);
   wbkgdset(win, ' ' | COLOR_PAIR(ED_TEXT));
   err = true;
   //wrefresh(win);
   wnoutrefresh(win);
   for (int i = 0; i < 5; i++) doupdate(); //must be done many times, don't know why
}

void TheFile::clear_error()
{
   if (err) draw(true);
}

