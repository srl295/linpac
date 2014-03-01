#! /usr/local/bin/perl
#----------------------------------------------------------------------------
# LinPac - packet radio terminal for Linux
#
# pack - delete messages marked for deletion
#
# Version 0.01
#
# (c) Radek Burget OK2JBG <xburge01@stud.fee.vutbr.cz> 1999
#
# Usage: pack <BBS_CALL>
#----------------------------------------------------------------------------

# Normal settings:
#List path
$LIST_PATH = "/var/ax25/ulistd";

#Bulletin path
$BULLETIN_PATH = "/var/ax25/mail";

#Axgetmail config path (for determining mail path)
$CONFIG_PATH = "/etc/ax25/axgetmail.conf";

#----------------------------------------------------------------------------

if (@ARGV != 1)
{
  print "Usage: pack <BBS_CALL>\n";
  exit 0;
}

#Read axgetmail config
if (open CONFIG, $CONFIG_PATH)
{
  while (<CONFIG>)
  {
    chop;
    if ($_ !~ /^#/)
    {
      ($name, $value) = split /\s+/;
      if ($name =~ /HOMEDIR/)
      {
        $homedir = $value;
        break;
      }
    }
  }
  close CONFIG;
}

#Number of deleted messages
$deleted = 0;

$bbsname = uc $ARGV[0];

# Open files
$bbs_dir = $BULLETIN_PATH . "/" . $bbsname;
die "The bulletin directory for $bbsname ($bbs_dir) doesn't exist\n" unless -d $bbs_dir;

$list_name = $LIST_PATH . "/" . $bbsname;
open LIST, $list_name or die "Cannot open list file for $bbsname ($list_name)\n";

while (<LIST>)
{
  chop;

  # Read and split message info
  @binfo = split /\s+/;
  $num = $binfo[0];    #first entry - message number
  $flag = $binfo[1];   #second entry - flags

  if ($flag =~ /D/)
  {
    if ($homedir) #private messages
    {
      unlink($ENV{'HOME'}."/".$homedir."/".$bbsname."/".$num) and $deleted++;
    }
    unlink($bbs_dir."/".$num) and $deleted++; #bulletins
  }
}

close LIST;
print "$deleted message(s) deleted.\n";

#### END OF SCRIPT #########################################################
