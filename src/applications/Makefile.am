## Process this file with automake to produce Makefile.in

SUBDIRS = liblinpac lpapi libaxmail mailer

s_lzhuf = lzhuf.cc
s_getmsg = getmsg.cc
s_forward = forward.cc
s_crc = crc.cc
s_dostime = dostime.cc
s_autobin = autobin.cc
s_bell = bell.cc
s_compose = compose.cc
s_lpbck = lpbck.cc
s_name = name.cc
s_rawio = rawio.cc
s_rtt = rtt.cc
s_save7pl = save7pl.cc
s_testapp = testapp.c
s_yapp = yapp.cc
s_logbook = logbook.cc
s_flexpwd = flexpwd.cc
s_md2pwd = md2pwd.cc md2.cc
s_md5pwd = md5pwd.cc md5.cc
s_tnpwd = tnpwd.cc
s_catch = catch.cc
s_join = join.c

LDADD = ./liblinpac/liblinpac.la

bin_PROGRAMS = autobin bell compose forward getmsg logbook lpbck \
               name rawio rtt save7pl testapp yapp flexpwd md2pwd \
               tnpwd md5pwd catch join

autobin_SOURCES = $(s_crc) $(s_autobin)
bell_SOURCES = $(s_bell)
compose_SOURCES = $(s_compose)
forward_SOURCES = $(s_lzhuf) $(s_forward)
getmsg_SOURCES = $(s_lzhuf) $(s_getmsg)
logbook_SOURCES = $(s_logbook)
lpbck_SOURCES = $(s_lpbck)
name_SOURCES = $(s_name)
rawio_SOURCES = $(s_rawio) 
rtt_SOURCES = $(s_rtt)
save7pl_SOURCES = $(s_crc) $(s_save7pl)
testapp_SOURCES = $(s_testapp)
yapp_SOURCES = $(s_yapp)
flexpwd_SOURCES = $(s_flexpwd)
md2pwd_SOURCES = $(s_md2pwd)
md5pwd_SOURCES = $(s_md5pwd)
tnpwd_SOURCES = $(s_tnpwd)
catch_SOURCES = $(s_catch)
join_SOURCES = $(s_join)

datadir = @prefix@/share/linpac/bin
bindir = @prefix@/share/linpac/bin

data_DATA = commands
bin_SCRIPTS = mheard shell w kill_chn delmsg pack sendfile

EXTRA_DIST = $(data_DATA) $(bin_SCRIPTS) *.h
