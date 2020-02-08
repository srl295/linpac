## Process this file with automake to produce Makefile.in

s_bbs = bbs.cc bbs_fbb.cc bbs_pbbs.cc
s_lzhuf = lzhuf.cc
s_getmsg = getmsg.cc
s_forward = forward.cc

AM_CPPFLAGS = -I$(srcdir)/../lpapi -I$(srcdir)/..

LDADD = ../liblinpac/liblinpac.la ../libaxmail/libaxmail.la
AM_LDFLAGS = @LINPACLINK@

pkglibexec_PROGRAMS = forward getmsg

forward_SOURCES = $(s_bbs) $(s_lzhuf) $(s_forward)
getmsg_SOURCES = $(s_bbs) $(s_lzhuf) $(s_getmsg)

dist_pkglibexec_SCRIPTS = delmsg getmail

EXTRA_DIST = bbs.h bbs_fbb.h bbs_pbbs.h lzhuf.h
