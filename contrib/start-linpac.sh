#!/bin/bash

# /usr/local/sbin/start-linpac.sh
# dranch@arrl.net

# 11/29/15 - Removed screen logging as it doesn't work well for Ncurses UIs
# 12/11/14 - Enabled screen logging to a specific file
# 08/30/14 - disabled screen logging
# 02/06/14 - Added screen loggin
# 06/28/13 - Added screen resume
# 11/19/12
#

#Linpac cannot play notification bells via the PC speaker nor listen to axlistend 
# w/o being root If you do NOT want to run as root, you can use axspyd to allow 
# monitoring
#
#   Don't enable screen logging (-L) as it just creates ANSI trash in the logs

if [ -n "`ps aux | grep -v grep | grep -i screen | grep -i linpac`" ]; then
   echo -e "\nLinpac already running under Screen.  Resuming"
   screen -d -r linpac
  else
   echo -e "\nPrevious Linpac not found.  Starting"
   date > /var/log/linpac.log
   screen -S linpac sudo linpac
fi

