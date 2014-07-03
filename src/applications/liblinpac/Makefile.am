## process this file with automake to produce Makefile.in

includedir=@includedir@/linpac
#libdir=/usr/local/lib
LIBS=
include_HEADERS = lpapp.h tevent.h
lib_LTLIBRARIES = liblinpac.la
liblinpac_la_SOURCES = lpapp.c hash.c
liblinpac_la_LDFLAGS = -version-info 7:2:1
EXTRA_DIST = hash.h
