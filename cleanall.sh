#!/bin/bash
#
# cleanall.sh - remove all build products
#
# This is a temporary script for use while I am working on the autotools
# setup. It will be replaced by a make target.
#
rm -f Makefile.in
rm -f aclocal.m4
rm -rf autom4te.cache/
rm -f m4/*
rm -rf build-aux/
rm -f config.guess
rm -f config.h.in
rm -f config.sub
rm -f configure
rm -f ctt/Makefile.in
rm -f data/Makefile.in
rm -f depcomp
rm -f doc/Makefile.in
rm -f doc/charsets/Makefile.in
rm -f doc/czech/Makefile.in
rm -f inst/Makefile.in
rm -f install-sh
rm -f ltmain.sh
rm -f macro/Makefile.in
rm -f macro/cz/Makefile.in
rm -f macro/de/Makefile.in
rm -f macro/en/Makefile.in
rm -f macro/fr/Makefile.in
rm -f macro/pl/Makefile.in
rm -f macro/sk/Makefile.in
rm -f mail/Makefile.in
rm -f missing
rm -f plugins/Makefile.in
rm -f src/Makefile.in
rm -f src/applications/Makefile.in
rm -f src/applications/libaxmail/Makefile.in
rm -f src/applications/liblinpac/Makefile.in
rm -f src/applications/lpapi/Makefile.in
rm -f src/applications/mailer/Makefile.in
