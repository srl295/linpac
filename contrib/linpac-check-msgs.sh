#!/bin/bash

# /usr/local/sbin/linpac-check-msgs.sh
# dranch@trinnet.net
# 12/10/14

# What does this script do:
#
#    Essentially, it looks for new files in a specific directory and if one
#    exists, it emails it to the specified email address.  This script is
#    currently tailored to check for new Linpac messages but can be adapted 
#    to anything.
#
#    This file also assumes the user is using the packetrig.sh script
#    which creates the listen-date.log
#
#
#  To enable execution of this scriptf for Centos Linux.  Adapt to your own
#  distro to suit:
#
#    1. Write this script to /usr/local/sbin/linpac-check-msgs.sh
#    2. chmod 700 /usr/local/sbin/linpac-check-msgs.sh
#    3. cd /etc/cron.5min
#    4. ln -s /usr/local/sbin/linpac-check-msgs.sh .


# Errata
# ------
# 12/10/14 - Now using a sorted ls listing for better results, trying to troubleshoot
#            possible wrapping issue at message 299
# 06/10/13 - Added more logic for abandoned messages that are zero bytes
#          - Updated the REMOTESENDER variable to only show the last callsign
# 05/31/13 - Avoid empty notifications due to messages being actively written 
#            during polling period
# 05/25/13 - improved remote station callsign identification
# 05/06/13 - moved to mutt getting the msg via a pipe vs indirection
#          - Added support for multiple messages within the polling period
# 05/04/13 - Attempt to identify remote sender
#          - Added 
#          - better logging
# 04/16/13 - first version


# ------------
# Dependencies
# ------------
# This script requires:
#
#   mutt
#   diff
#   tail
#   awk
#   find



#-----------------------
# Variables
#-----------------------
#Username running the Linpac process and the homedir are the packet messages stored
LINPACUSER=root

#Path to Linpac messages - do NOT have a trailing /
MSGPATH="/$LINPACUSER/LinPac/mail"

#Msg status file
OLDMSGLIST="/var/tmp/linpac-old-msg-list.txt"
NEWMSGLIST="/var/tmp/linpac-new-msg-list.txt"

#DOESN'T seem to work : Source email address if you don't want it to be "root"
EMAIL="dranch@hampacket2.trinnet.net"

#Where to send messages - supports two destinations: one for email, another for 
# an email to SMS gateway
DESTEMAILADDR="change-me@gmail.com"
DESTSMSADDR="change-me@vtext.com"

#Callsign that accepts messages - it's important to LEAVE the two trailing asterisks 
# for proper regex matching of your call with or without a SSID
LOCALCALL="N0CALL**"

#Current Local listen log
LISTENLOG=`ls -1t /var/log/listen-* | head -n1`

#programs
MUTT="/usr/bin/mutt"

#Log file
LOGFILE="/var/log/packet.log"

#enable for debugging
DEBUG=0

# ------------------------------------------------------------------------

#----------------------------
# Variables
#----------------------------
#Initialize some variables
NEWMSG=""


#----------------------------
# Main
#----------------------------
#echo "DEBUG: Path to New messages: $MSGPATH"

#New installation - startup corner case
if [ ! -f $OLDMSGLIST ]; then
   echo -e "New installation.. seeding $OLDMSGLIST"
   ls -1 $MSGPATH | sort -g > $OLDMSGLIST
   if [ $? -ne 0 ]; then
      echo -e "Error: could not write $OLDMSGLIST"
      exit 1
   fi
fi

if [ ! -d $MSGPATH ]; then
   echo -e "Error: could not read $MSGPATH or path doesn't exist"
   exit 1
fi

#Do this all inline to minimize writing to the HD when not required
#echo -e "DEBUG: diff list"
NEWMSG="`ls -1 $MSGPATH | sort -g | diff $OLDMSGLIST - | tail -n +2 | awk '{print $2}'`"

#echo -e "DEBUG: NEWMSG is '$NEWMSG'"

#A "-n" test doesn't work here
if [ "$NEWMSG" != '' ]; then
   #echo -e "There is a new message: $NEWMSG"
   #Update the message list
   #  Disabled for testing only - needs to be active

   # Check the state of the msg:
   #
   #   If a user is actively writing a message, we don't want to send the notification
   #   until they are done BUT if the message gets abandoned, we also dont want a ZERO
   #   byte message to stall the notification system either.
   #
   #     Delay check: 10 minutes or less
   #
   #Only dealing with one message right now, need to improve to support multiple
   if [ -n "`find $MSGPATH -empty -mmin -10 | grep -v mail.groups`" ]; then
      echo "`date` - packet message being written but isn't complete.  Wait next period" >> $LOGFILE
      exit 1
   fi
      
   ls -1 $MSGPATH | sort -g > $OLDMSGLIST
   if [ $? -ne 0 ]; then
      echo -e "Error: could not write $OLDMSGLIST"
      exit 1
   fi

   #--------------------------
   # Determine remote callsign
   #--------------------------
   #This logic is heavily flawed considering if we are going to accept multiple
   #connections and even multiple concurrent incoming messages within the same 
   #polling period.  The proper solution is to fix linpac's logging of the remote 
   #callsign as recorded in messages.local
   REMOTESENDER="`grep -iB1 "\/\/sp" $LISTENLOG | grep -i "to $LOCALCALL" | awk '{print $3}' | tail -n1`"

   #Need to deal when multiple messages come in within the polling period
   for I in $NEWMSG; do
     echo -e "\n`date` - New packet message from $REMOTESENDER : path: $MSGPATH : # $I" >> $LOGFILE
     #Indirection might be unreliable
     #$MUTT -s "New packet message from $REMOTESENDER" $DESTEMAILADDR < $MSGPATH/$NEWMSG
     # Try direct pipeline instead
     cat $MSGPATH/$I | $MUTT -s "New packet message from $REMOTESENDER" $DESTEMAILADDR
     $MUTT -s "New packet message from $REMOTESENDER" $DESTSMSADDR
     ENDRESULT=$?
     if [ $DEBUG -eq 1 ]; then
        echo "DEBUG: Last end result was: $ENDRESULT"
        echo "DEBUG: MUTT is $MUTT - `ls $MUTT`"
        echo "DEBUG: REMOSTSENDER: $REMOTESENDER" 
        echo "DEBUG: DESTEMAILADDR: $DESTEMAILADDR"
        echo "DEBUG: MESSAGE:  $MSGPATH/$NEWMSG"
        echo "DEBUG: CURRENT MESSAGE:  $MSGPATH/$I"
     fi
    #else
     #Left commented out or your get syslog emails every 5 minutes
     #echo -e "No new messages"
   done
fi
