## Process this file with automake to produce Makefile.in

s_mail = mail.cc mail_call.cc mail_comp.cc mail_data.cc \
         mail_edit.cc mail_files.cc mail_filt.cc \
         mail_help.cc mail_input.cc mail_list.cc mail_route.cc \
         mail_screen.cc

LDADD = ../liblinpac/liblinpac.la ../libaxmail/libaxmail.la
AM_LDFLAGS = @LINPACLINK@

bin_PROGRAMS = mail

mail_SOURCES = $(s_mail) 

datadir = @prefix@/share/linpac/bin
bindir = @prefix@/share/linpac/bin

data_DATA =
bin_SCRIPTS =

EXTRA_DIST = $(data_DATA) $(bin_SCRIPTS) *.h
