/*
  axmail - library for manipulating the message database
  (c) 2001 Radek Burget <radkovo@centrum.cz>
*/

#ifndef AXMAIL_H
#define AXMAIL_H

#include <vector>

//Library initialization with default paths
void axmail_init();

//Library initialization with user paths
void axmail_init(const char *listPath,
                 const char *bulletinPath,
                 const char *personalPath,
                 const char *outgoingPath);

//Set default BBS for new messages
void default_bbs(const char *call);

//MsgDate - represents the date of message creation or arrival
class MsgDate
{
   protected:
      int day, month, year;
      int hour, min;
   
   public:
      MsgDate();
      MsgDate(bool isShort, const char *); //accept date in format yymmdd
                                           //(short) or yymmdd/hhmmZ (long)
      char *toStringShort(char *buffer);   //convert to string
      char *toStringShort();               //convert to string with temporary buffer
      char *toStringLong(char *buffer);
      char *toStringLong();
      bool operator ==(const MsgDate &src);
      bool operator <(const MsgDate &src);

      bool check();  //check if the date is valid (very stupid)

      int d() { return day; }
      int m() { return month; }
      int y() { return year; }
      int H() { return hour; }
      int M() { return min; }
};

//Message - a message
class Message
{
   protected:
      //message data
      int num;             //msg number on BBS
      int size;            //msg size
      char *flags;         //flags
      char *dest;          //msg destination
      char *dpath;         //destination path (@WW etc.)
      char *src;           //msg source
      char *subject;       //subject
      MsgDate *date;       //msg date
      char *bid;           //message BID (outgoing)
      char *text;          //message text, NULL if not read yet

      //internal data
      bool outgoing;       //outgoing message (first line is subject)
      bool priv;           //private message
      char *path;          //path to message
      bool present;        //message is present
      bool modified;       //the body has been modified
      bool deleted;        //message will be deleted

      //empty string for returning nothing
      char nothing[2];

   public:
      //Constructor for incomming messages
      Message(long pnum,            //message number on the BBS
              const char *pflags,   //flags
              int psize,            //size in bytes
              const char *pdest,    //destination addr
              const char *pdpath,   //destination path
              const char *psrc,     //source addr
              const char *pdate,    //date yymmdd
              const char *psubject); //subject

      //Constructor for outgoing messages
      Message(long pnum,            //message number (local)
              const char *pflags,   //flags
              const char *pbid,     //BID/MID
              const char *psrc,     //source addr
              const char *pdest,    //destination addr
              const char *pdate,    //date yymmdd/hhmmZ
              const char *psubject); //subject

      Message(const Message &msg);  //copy constructor
      
      ~Message();
      Message &operator = (const Message &);

      void setBBS(const char *call);

      //Message properties
      int getNum() const { return num; }
      int getSize() { return size; }
      char *getFlags() { return flags; }
      char *getSrc() { return src; }
      char *getDest() { return dest; }
      char *getDPath() { return dpath?dpath:nothing; }
      char *getSubj() { return subject; }
      char *getBid() { return bid?bid:nothing; }
      MsgDate &getDate() { return *date; }

      char *getBody(bool reload = false); //read the body (if present)
      void setBody(const char *body);     //set a new body

      void markDel(bool del = true) { deleted = del; } //mark as deleted
      bool isDel() { return deleted; } // is deleted?
      void update();  // delete the message if marked for delete,
                      // write the body if modified

      bool isPrivate() { return priv; }
      bool isPresent() { return present; }  //check if message is present

      bool checkPresence();     //check if message is present
      void setPresence(bool);   //set the presence
};

//MessageIndex - a prototype of message index
class MessageIndex
{
   protected:
      char *call;                   //BBS callsign w/o ssid
      char *path;                   //index path
      int lastnum;                  //highest known message number
      std::vector <Message *> messages;  //list of messages

      void updateList();            //remove deleted messages
      void clearList();             //clear the list of messages

   public:
      //number of messages
      int messageCount() { return messages.size(); }
      //get a message
      Message *getMessage(int index);
      //mark a message for removal
      void removeMessage(int index);
      //add new message, returns true if successfull
      bool addMessage(const Message &message);

      //return the index of a message with number num, -1 if no such message
      int msgNum(int num);
      //return next free index
      int newIndex() { return lastnum+1; }

      //check the presence of all the messages (optimized)
      void checkPresence();
};

//IncommingIndex - a list of messages available on the BBS
class IncommingIndex : public MessageIndex
{
   public:
      IncommingIndex(const char *bbs_call);
      ~IncommingIndex();

      void reload();
      void writeIndex();
};

//OutgoingIndex - a list of messages available on the BBS
class OutgoingIndex : public MessageIndex
{
   public:
      OutgoingIndex(const char *bbs_call);
      ~OutgoingIndex();

      void reload();
      void writeIndex();
};

#endif

