## Process this file with automake to produce Makefile.in

CXXFLAGS = -g -Wall -funsigned-char
LDADD = ../liblinpac/liblinpac.la
LDFLAGS = @LINPACLINK@

bin_PROGRAMS = lpapi

lpapi_SOURCES = lpapi.cc

datadir = @prefix@/share/linpac/tools
bindir = @prefix@/share/linpac/tools

EXTRA_DIST = *.h
