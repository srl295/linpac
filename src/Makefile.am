## Process this file with automake to produce Makefile.in

SUBDIRS = applications

s_libs = data.cc calls.cc sources.cc commands.cc event.cc names.cc screen.cc \
         windows.cc editor.cc tools.cc watch.cc evaluate.cc infoline.cc \
         status.cc sounds.cc evqueue.cc comp.cc keyboard.cc \
         hash.c
s_linpac = linpac.cc	 

pkglibexec_PROGRAMS = linpac
linpac_SOURCES = $(s_libs) $(s_linpac)		 

EXTRA_DIST = calls.h commands.h comp.h constants.h data.h editor.h \
	    evaluate.h event.h evqueue.h hash.h infoline.h keyboard.h \
            names.h screen.h sounds.h sources.h status.h tevent.h \
            tools.h t_queue.h t_stack.h version.h watch.h windows.h
