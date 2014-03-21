/*
  axmail - library for manipulating the message database
  (c) 2001-2002 Radek Burget <radkovo@centrum.cz>
*/

#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "calls.h"
#include "parser.h"
#include "axmail.h"

//-----------------------------------------------------------------------
// LibrarySettings
//-----------------------------------------------------------------------

char *list_path = NULL;      //path to the lists
char *bulletin_path = NULL;  //path to bulletin directories
char *personal_path = NULL;  //path to personal mail directories
char *outgoing_path = NULL;  //path to outgoing mail directory

char *current_bbs = NULL;    //current BBS callsign

void axmail_init()
{
   if (list_path != NULL) delete[] list_path;
   list_path = strdup("/var/ax25/ulistd");
   if (bulletin_path != NULL) delete[] bulletin_path;
   bulletin_path = strdup("/var/ax25/mail");
   if (personal_path != NULL) delete[] personal_path;
   char *p = getenv("HOME");
   if (p == NULL) personal_path = strdup(bulletin_path);
   else
   {
      personal_path = new char[strlen(p)+20];
      strcpy(personal_path, p);
      strcat(personal_path, "/LinPac/mail");
   }
   if (outgoing_path != NULL) delete[] outgoing_path;
   outgoing_path = strdup(personal_path);

   current_bbs = strdup("AXMAIL");
}

void axmail_init(const char *listPath,
                 const char *bulletinPath,
                 const char *personalPath,
                 const char *outgoingPath)
{
   if (list_path != NULL) delete[] list_path;
   list_path = strdup(listPath);
   if (bulletin_path != NULL) delete[] bulletin_path;
   bulletin_path = strdup(bulletinPath);
   if (personal_path != NULL) delete[] personal_path;
   personal_path = strdup(personalPath);
   if (outgoing_path != NULL) delete[] outgoing_path;
   outgoing_path = strdup(outgoingPath);

   current_bbs = strdup("AXMAIL");
}

void default_bbs(const char *call)
{
   if (current_bbs != NULL) delete[] current_bbs;
   char *tmp = strdup(call);
   AXnormalize_call(tmp);
   current_bbs = strdup(AXcall_call(tmp));
   delete[] tmp;
}

char *AXstrdupl(const char *s)
{
   if (s == NULL) return NULL;
   else return strdup(s);
}

bool AXisnum(char *s)
{
  char *p = s;
  if (!*p) return false;
  while (*p) if (!isdigit(*p)) return false;
             else p++;
  return true;
}

//-----------------------------------------------------------------------
// MsgDate
//-----------------------------------------------------------------------

MsgDate::MsgDate()
{
}

MsgDate::MsgDate(bool isShort, const char *src)
{
   if (isShort)
   {
      if (strlen(src) != 6) return; //only yymmdd strings accepted
      char buf[3]; buf[2] = '\0';
      strncpy(buf, src,   2); year = atoi(buf);
      strncpy(buf, src+2, 2); month = atoi(buf);
      strncpy(buf, src+4, 2); day = atoi(buf);
   }
   else
   {
      if (strlen(src) != 11 && strlen(src) != 12) return; //Z is not necessarry
      char buf[3]; buf[2] = '\0';
      char buf2[5]; buf2[4] = '\0';
      strncpy(buf, src,   2); year = atoi(buf);
      strncpy(buf, src+2, 2); month = atoi(buf);
      strncpy(buf2, src+4, 4); day = atoi(buf2);
      strncpy(buf, src+7, 2); hour = atoi(buf);
      strncpy(buf2, src+9, 4); min = atoi(buf2);
   }   
}

char *MsgDate::toStringShort(char *s)
{
   sprintf(s, "%02i%02i%02i", year, month, day);
   return s;
}

char *MsgDate::toStringShort()
{
   static char buf[8];
   sprintf(buf, "%02i%02i%02i", year, month, day);
   return buf;
}

char *MsgDate::toStringLong(char *s)
{
   sprintf(s, "%02i%02i%02i/%02i%02iZ", year, month, day, hour, min);
   return s;
}

char *MsgDate::toStringLong()
{
   static char buf[14];
   sprintf(buf, "%02i%02i%02i/%02i%02iZ", year, month, day, hour, min);
   return buf;
}

bool MsgDate::operator ==(const MsgDate &src)
{
   return (src.year == year && src.month == month && src.day == day &&
           src.hour == hour && src.min == min);
}

bool MsgDate::operator <(const MsgDate &src)
{
   return (year < src.year ||
           (year == src.year && month < src.month) ||
           (year == src.year && month == src.month && day < src.day) ||
           (year == src.year && month == src.month && day == src.day && hour < src.hour) ||
           (year == src.year && month == src.month && day == src.day && hour == src.hour && min < src.min));
}

bool MsgDate::check()
{
   bool r = true;
   if (year < 0 || year > 99) r = false;
   if (month < 1 || month > 12) r = false;
   if (day < 1 || day > 31) r = false; //this is silly...
   if (hour < 0 || hour > 23) r = false;
   if (min < 0 || min > 59) r = false;
   return r;
}

//-----------------------------------------------------------------------
// Message
//-----------------------------------------------------------------------

Message::Message(long pnum, const char *pflags, int psize, const char *pdest,
                 const char *pdpath, const char *psrc, const char *pdate,
                 const char *psubject)
{
   num = pnum;
   size = psize;
   flags = strdup(pflags);
   dest = strdup(pdest);
   dpath = strdup(pdpath);
   src = strdup(psrc);
   date = new MsgDate(true, pdate);
   subject = strdup(psubject);
   bid = NULL;
   present = false;
   modified = false;
   deleted = false;

   outgoing = false;
   text = NULL;

   priv = (strchr(flags, 'P') != NULL);
   if (priv)
   {
      path = new char[strlen(personal_path)+strlen(current_bbs)+20];
      sprintf(path, "%s/%s/%i", personal_path, current_bbs, num);
   }
   else
   {
      path = new char[strlen(bulletin_path)+strlen(current_bbs)+20];
      sprintf(path, "%s/%s/%i", bulletin_path, current_bbs, num);
   }
   nothing[0] = '\0';
}

Message::Message(long pnum, const char *pflags, const char *pbid,
                 const char *psrc, const char *pdest, const char *pdate,
                 const char *psubject)
{
   num = pnum;
   size = -1;
   flags = strdup(pflags);
   dest = strdup(pdest);
   dpath = NULL;
   src = strdup(psrc);
   date = new MsgDate(false, pdate);
   subject = strdup(psubject);
   bid = strdup(pbid);
   present = true; //outgoing messages should be present!
   modified = false;
   deleted = false;

   outgoing = true;
   text = NULL;

   priv = (strchr(flags, 'P') != NULL);
   path = new char[strlen(outgoing_path)+20];
   sprintf(path, "%s/%i", outgoing_path, num);
   nothing[0] = '\0';
}

Message::Message(const Message &msg)
{
   num = msg.num;
   size = msg.size;
   flags = AXstrdupl(msg.flags);
   dest = AXstrdupl(msg.dest);
   dpath = AXstrdupl(msg.dpath);
   src = AXstrdupl(msg.src);
   date = new MsgDate(*msg.date);
   subject = AXstrdupl(msg.subject);
   bid = AXstrdupl(msg.bid);
   present = msg.present;
   outgoing = msg.outgoing;
   text = AXstrdupl(msg.text);
   priv = msg.priv;
   path = AXstrdupl(msg.path);
   deleted = msg.deleted;
   modified = msg.modified;
   nothing[0] = '\0';
}

Message::~Message()
{
   if (flags != NULL) delete[] flags;
   if (dest != NULL) delete[] dest;
   if (dpath != NULL) delete[] dpath;
   if (src != NULL) delete[] src;
   if (date != NULL) delete date;
   if (subject != NULL) delete[] subject;
   if (bid != NULL) delete[] bid;
   if (path != NULL) delete[] path;
   if (text != NULL) delete[] text;
}

Message &Message::operator = (const Message &msg)
{
   num = msg.num;
   size = msg.size;
   flags = AXstrdupl(msg.flags);
   dest = AXstrdupl(msg.dest);
   dpath = AXstrdupl(msg.dpath);
   src = AXstrdupl(msg.src);
   date = new MsgDate(*msg.date);
   subject = AXstrdupl(msg.subject);
   bid = AXstrdupl(msg.bid);
   present = msg.present;
   outgoing = msg.outgoing;
   text = AXstrdupl(msg.text);
   priv = msg.priv;
   path = AXstrdupl(msg.path);
   deleted = msg.deleted;
   modified = msg.modified;
   return *this;
}

void Message::setBBS(const char *bbs_call)
{
   char *tmp = strdup(bbs_call);
   AXnormalize_call(tmp);
   char *call = strdup(AXcall_call(tmp));

   if (path != NULL) delete[] path;
   if (outgoing)
   {
      path = new char[strlen(outgoing_path)+20];
      sprintf(path, "%s/%i", outgoing_path, num);
   }
   else
   {
      if (priv)
      {
         path = new char[strlen(personal_path)+strlen(call)+20];
         sprintf(path, "%s/%s/%i", personal_path, call, num);
      }
      else
      {
         path = new char[strlen(bulletin_path)+strlen(call)+20];
         sprintf(path, "%s/%s/%i", bulletin_path, call, num);
      }
   }

   delete[] tmp;
   delete[] call;
}

void Message::update()
{
   if (deleted)
   {
      unlink(path);
      deleted = false;
      present = false;
   }
   else if (modified && text != NULL)
   {
      FILE *f = fopen(path, "w");
      if (f)
      {
         fputs(text, f);
         fclose(f);
         modified = false;
      }
   }
}

void Message::setPresence(bool b)
{
   present = b;
}

bool Message::checkPresence()
{
   struct stat st;
   present = (stat(path, &st) != -1 && !S_ISDIR(st.st_mode));
   return present;
}

char *Message::getBody(bool reload)
{
   if (reload || text == NULL)
   {
      if (text != NULL) delete[] text;
   
      FILE *f = fopen(path, "r");
      if (f)
      {
         if (outgoing)          //outgoing message: read the subject
         {
            char buf[256];
            if (fgets(buf, 255, f) == NULL)
            {
                buf[0] = 0;
            }
         }
         long pos = ftell(f);
         fseek(f, 0, SEEK_END);
         long size = ftell(f);
         fseek(f, pos, SEEK_SET);
         text = new char[size+1];
         int n = fread(text, 1, size, f);
         text[n] = '\0';
         fclose(f);
      }
      else text = NULL;
   }
   return text;
}

void Message::setBody(const char *body)
{
   text = strdup(body);
   modified = true;
}

//-----------------------------------------------------------------------
// MessageIndex
//-----------------------------------------------------------------------

void MessageIndex::updateList()
{
   std::vector <Message *>::iterator it;
   for (it = messages.begin(); it < messages.end(); it++)
   {
      if ((*it)->isDel())
      {
         (*it)->update();
         delete *it;
         messages.erase(it);
      } else (*it)->update();
   }
}

void MessageIndex::clearList()
{
   std::vector <Message *>::iterator it;
   for (it = messages.begin(); it < messages.end(); it++)
      delete *it;
   messages.erase(messages.begin(), messages.end());
}

Message *MessageIndex::getMessage(int index)
{
   if (index < 0) index = 0;
   if (index >= (int)messages.size()) index = messages.size() - 1;
   return messages[index];
}

void MessageIndex::removeMessage(int index)
{
   if (index >= 0 && index < (int)messages.size())
      messages[index]->markDel();
}

bool MessageIndex::addMessage(const Message &msg)
{
   if (msg.getNum() > lastnum)
   {
      Message *newmsg = new Message(msg);
      newmsg->setBBS(call);
      messages.push_back(newmsg);
      lastnum = msg.getNum();
      return true;
   }
   return false;
}

int MessageIndex::msgNum(int num)
{
   int left, right, mid;
   left = 0; right = messages.size() - 1; mid = (left + right) / 2;
   if (right == -1) return -1;
   while (messages[mid]->getNum() != num && left != right && left+1 != right)
   {
      mid = (left + right) / 2;
      if (num < messages[mid]->getNum()) right = mid;
      else left = mid;
   }
   if (messages[mid]->getNum() == num) return mid;
   if (messages[right]->getNum() == num) return right;
   return -1;
}

void MessageIndex::checkPresence()
{
   DIR *dir;
   struct dirent *dd;
   char *msgdir;

   //search in bulletins
   msgdir = new char[strlen(bulletin_path)+20];
   sprintf(msgdir, "%s/%s", bulletin_path, call);
   dir = opendir(msgdir);
   if (dir != NULL)
   {
      do
      {
         dd = readdir(dir);
         if (dd != NULL && AXisnum(dd->d_name))
         {
            int m = atoi(dd->d_name);
            int i = msgNum(m);
            if (i != -1) messages[i]->setPresence(true);
         }
      } while (dd != NULL);
      closedir(dir);
   }
   delete[] msgdir;
   
   //search in private mail
   msgdir = new char[strlen(personal_path)+20];
   sprintf(msgdir, "%s/%s", personal_path, call);
   dir = opendir(msgdir);
   if (dir != NULL)
   {
      do
      {
         dd = readdir(dir);
         if (dd != NULL && AXisnum(dd->d_name))
         {
            int m = atoi(dd->d_name);
            int i = msgNum(m);
            if (i != -1) messages[i]->setPresence(true);
         }
      } while (dd != NULL);
      closedir(dir);
   }
   delete[] msgdir;
}

//-----------------------------------------------------------------------
// IncommingIndex
//-----------------------------------------------------------------------

IncommingIndex::IncommingIndex(const char *bbs_call)
{
   char *tmp = strdup(bbs_call);
   AXnormalize_call(tmp);
   call = strdup(AXcall_call(tmp));

   path = new char[strlen(list_path)+strlen(call)+2];
   sprintf(path, "%s/%s", list_path, call);

   lastnum = 0;
   reload();
}

IncommingIndex::~IncommingIndex()
{
   std::vector <Message *>::iterator it;
   for (it = messages.begin(); it < messages.end(); it++)
      delete *it;
}

void IncommingIndex::reload()
{
   FILE *list;
   int num;
   char *flags;
   int size;
   char *dest;
   char *dpath;
   char *src;
   char *date;
   char *subject;
   
   P_amp_breaks(true); //apersand means end of field
   clearList(); //clear the list
   list = fopen(path, "r");
   if (list)
   {
     while (!feof(list))
     {
        char line[1024];
        strcpy(line, "");
        if (fgets(line, 1023, list) != NULL)
        {
            //remove trailing \n and spaces
            if (strlen(line) > 0 && line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
            while (strlen(line) > 0 && line[strlen(line)-1] == ' ') line[strlen(line)-1] = '\0';
            if (strlen(line) == 0) continue;
        }
        else
        {
            continue;
        }

        char *p, *q;
        //number
        p = line; q = P_field_end(p);
        num = atoi(P_extract(p, q));
        if (lastnum < num) lastnum = num;
        //flags
        p = P_next_field_start(p); q = P_field_end(p);
        flags = strdup(P_extract(p, q));
        if (strcmp(flags, "#") == 0 || strcmp(flags, "!") == 0)
        {
           delete[] flags;
           continue;
        }

        //size
        p = P_next_field_start(p); q = P_field_end(p);
        size = atoi(P_extract(p, q));
        //destination
        p = P_next_field_start(p); q = P_field_end(p);
        dest = strdup(P_extract(p, q));
        //path
        p = P_next_field_start(p); q = P_field_end(p+1);
        if (*p == '@')
        {
           dpath = strdup(P_extract(p, q));
           p = P_next_field_start(p+1); q = P_field_end(p);
        } else dpath = strdup("");
        //source
        src = strdup(P_extract(p, q));
        //date
        p = P_next_field_start(p); q = P_field_end(p);
        date = strdup(P_extract(p, q));
        //subject
        p = P_next_field_start(p); q = P_string_end(p);
        subject = strdup(P_extract(p, q));
        
        Message *newmsg = new Message(num, flags, size, dest, dpath,
                                      src, date, subject);
        newmsg->setBBS(call);
        messages.push_back(newmsg);

        delete[] flags;
        delete[] dest;
        delete[] dpath;
        delete[] src;
        delete[] date;
        delete[] subject;        
     }
     fclose(list);
   }
     else
         fprintf(stderr, "mail: cannot open message index %s\n", path);
}

void IncommingIndex::writeIndex()
{
   FILE *list;

   list = fopen(path, "w");
   if (list)
   {
      updateList(); //delete removed messages
      if (!messages.empty())
      {
         int actnum = messages[0]->getNum();
         std::vector <Message *>::iterator it;
         for (it = messages.begin(); it < messages.end(); it++)
         {
            if ((*it)->getNum() > actnum+1)
              for (int i = actnum+1; i < (*it)->getNum(); i++)
                 if (i <= lastnum) fprintf(list, "%i  #\n", i);
            actnum = (*it)->getNum();
            char sdate[8];
            (*it)->getDate().toStringShort(sdate);
            fprintf(list, "%i  %s %6i %-6s%-7s %-6s %-6s %s\n", (*it)->getNum(),
                                                         (*it)->getFlags(),
                                                         (*it)->getSize(),
                                                         (*it)->getDest(),
                                                         (*it)->getDPath(),
                                                         (*it)->getSrc(),
                                                         sdate,
                                                         (*it)->getSubj());
         }
         actnum++;
         while (actnum <= lastnum)
         {
            fprintf(list, "%i  #\n", actnum);
            actnum++;
         }
      }
      else if (lastnum > 0)
         fprintf(list, "%i  #\n", lastnum);
      fclose(list);
   }
}

//-----------------------------------------------------------------------
// OutgoingIndex
//-----------------------------------------------------------------------

OutgoingIndex::OutgoingIndex(const char *bbs_call)
{
   char *tmp = strdup(bbs_call);
   AXnormalize_call(tmp);
   call = strdup(AXcall_call(tmp));

   path = new char[strlen(outgoing_path)+16];
   sprintf(path, "%s/messages.local", outgoing_path);

   lastnum = 0;
   reload();
}

OutgoingIndex::~OutgoingIndex()
{
   std::vector <Message *>::iterator it;
   for (it = messages.begin(); it < messages.end(); it++)
      delete *it;
}

void OutgoingIndex::reload()
{
   FILE *list;
   int num;
   char *flags;
   char *bid;
   char *src;
   char *dest;
   char *date;
   char *subject;
   
   P_amp_breaks(false); //ampersand doesn't mean end of the field
   clearList(); //clear the list
   list = fopen(path, "r");
   if (list)
   {
      while (!feof(list))
      {
         char line[1024];
         strcpy(line, "");
         if (fgets(line, 1023, list) != NULL)
         {
            //remove trailing \n and spaces
            if (strlen(line) > 0 && line[strlen(line)-1] == '\n') line[strlen(line)-1] = '\0';
            while (strlen(line) > 0 && line[strlen(line)-1] == ' ') line[strlen(line)-1] = '\0';
            if (strlen(line) == 0) continue;
         }
         else
         {
             continue;
         }
 
         char *p, *q;
         //number
         p = line; q = P_field_end(p);
         num = atoi(P_extract(p, q));
         if (lastnum < num) lastnum = num;
         //flags
         p = P_next_field_start(p); q = P_field_end(p);
         flags = strdup(P_extract(p, q));
         //BID
         p = P_next_field_start(p); q = P_field_end(p);
         bid = strdup(P_extract(p, q));
         //source
         p = P_next_field_start(p); q = P_field_end(p);
         src = strdup(P_extract(p, q));
         //destination
         p = P_next_field_start(q); q = P_field_end(p);
         dest = strdup(P_extract(p, q));
         //date
         p = P_next_field_start(q); q = P_field_end(p);
         date = strdup(P_extract(p, q));
         //subject
         p = P_next_field_start(p); q = P_string_end(p);
         subject = strdup(P_extract(p+1, q));
         
         Message *newmsg = new Message(num, flags, bid, src, dest,
                                       date, subject);
         newmsg->setBBS(call);
         messages.push_back(newmsg);
 
         delete[] flags;
         delete[] bid;
         delete[] src;
         delete[] dest;
         delete[] date;
         delete[] subject;        
      }
      fclose(list);
   }
   else
       fprintf(stderr, "mail: cannot open message index %s\n", path);
}

void OutgoingIndex::writeIndex()
{
   FILE *list;

   list = fopen(path, "w");
   if (list)
   {
      updateList(); //delete removed messages
      if (!messages.empty())
      {
         int actnum = messages[0]->getNum();
         std::vector <Message *>::iterator it;
         for (it = messages.begin(); it < messages.end(); it++)
         {
            if ((*it)->getNum() > actnum+1)
              for (int i = actnum+1; i < (*it)->getNum(); i++)
                 if (i <= lastnum) fprintf(list, "%i  #\n", i);
            actnum = (*it)->getNum();
            char sdate[16];
            (*it)->getDate().toStringLong(sdate);
            if (!(*it)->getDate().check())
               fprintf(stderr, "Illegal date in msg %i\n", (*it)->getNum());
            fprintf(list, "%i\t%s\t%s\t%s\t%s\t%s\t|%s\n", (*it)->getNum(),
                                                       (*it)->getFlags(),
                                                       (*it)->getBid(),
                                                       (*it)->getSrc(),
                                                       (*it)->getDest(),
                                                       sdate,
                                                       (*it)->getSubj());
         }
         actnum++;
         while (actnum <= lastnum)
         {
            fprintf(list, "%i  #\n", actnum);
            actnum++;
         }
      }
      else if (lastnum > 0)
         fprintf(list, "%i  #\n", lastnum);
      fclose(list);
   }
}

