## Process this file with automake to produce Makefile.in

SUBDIRS = applications

s_libs = data.cc calls.cc sources.cc commands.cc event.cc names.cc screen.cc \
         windows.cc editor.cc tools.cc watch.cc evaluate.cc infoline.cc \
         status.cc sounds.cc evqueue.cc comp.cc keyboard.cc \
         hash.c
s_linpac = linpac.cc	 

CXXFLAGS = -g -Wall -funsigned-char

rootdir = @prefix@/share/linpac
root_PROGRAMS = linpac
linpac_SOURCES = $(s_libs) $(s_linpac)		 

EXTRA_DIST = *.h
