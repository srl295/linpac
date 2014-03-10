dnl Process this file with autoconf to produce a configure script.

# Prelude
AC_INIT([LinPac], [0.20])
AC_PREREQ([2.68])
AC_CONFIG_SRCDIR([src/linpac.cc])
AC_CONFIG_AUX_DIR([build-aux])
## See https://bugzilla.redhat.com/show_bug.cgi?id=901333 for more
## information about the AC_CONFIG_MACRO_DIR
AC_CONFIG_MACRO_DIR([m4])
LT_PREREQ([2.4.2])
LT_INIT
AM_INIT_AUTOMAKE([gnu -Wall -Werror 1.11.3])

# Checks for programs
AC_LANG([C++])
AC_LANG([C])
AC_PROG_CXX
AC_PROG_LN_S
AC_PATH_PROG([PERL], [perl], ["none"])
if test $PERL = "none"
then
  echo "-------------------------------------------------------------------"
  echo " WARNING: Cannot found 'perl' installed. Some tools won't work !!!"
  echo "-------------------------------------------------------------------"
  set PERL="/usr/bin/perl"
fi

# Checks for libraries

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADER([linux/ax25.h], , AC_MSG_ERROR([linux/ax25.h not found !]))

# Checks for typedefs, structures, and compiler characteristics
AC_MSG_CHECKING([for socklen_t])
AC_TRY_COMPILE([#include <sys/types.h> 
                #include <sys/socket.h>], [socklen_t a;], \
             AC_MSG_RESULT([present]),
	     AC_MSG_RESULT([not present])
	     AC_DEFINE([NO_SOCKLEN_T], 1))
AC_MSG_CHECKING([the AX.25 implementation])
AC_TRY_COMPILE([#include <sys/socket.h>
                #include <sys/types.h>
                #include <linux/ax25.h>], 
               [ax25_info_struct i; unsigned short a = i.vs;], \
             AC_MSG_RESULT([new AX.25])
             AC_DEFINE([NEW_AX25], 1),
             AC_MSG_RESULT([old AX.25]))

# Checks for library functions
AC_CHECK_LIB([ncurses], [main], , [AC_MSG_ERROR([ncurses library not found !])])

# Definitions
AH_TEMPLATE([NEW_AX25], [AX.25 version])
AH_TEMPLATE([NO_SOCKLEN_T], [Socket length type])
AH_TEMPLATE([NEW_AX25], [AX.25 version])
AH_TEMPLATE([VERINFO], [Version string])
# TODO: move VERINFO to a more prominate location or eliminated it
AC_DEFINE([VERINFO], ["Development snapshot 030813"])

# Package Options
# TODO: make sure this is the up-to-date method and that it works right
#       with modern libtools
# 2014/03/06 jdunmire - build fails if --enable-LINKSTATIC used
LINPACLINK=""
AC_ARG_ENABLE(LINKSTATIC, Use --enable-LINKSTATIC for static linking,
              LINPACLINK="-all-static")


# Output files
AC_PREFIX_DEFAULT([/usr/local])
AC_SUBST([LINPACLINK])
AC_SUBST([PERL])
AC_SUBST([VERSION])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile \
          ctt/Makefile \
	  data/Makefile \
          doc/Makefile \
	  doc/czech/Makefile \
          doc/charsets/Makefile \
	  macro/Makefile \
          macro/cz/Makefile \
	  macro/en/Makefile \
	  macro/pl/Makefile \
	  macro/fr/Makefile \
	  macro/sk/Makefile \
	  macro/de/Makefile \
	  mail/Makefile \
	  mail/delmsg.pl \
	  mail/pack.pl \
	  src/Makefile \
	  src/applications/liblinpac/Makefile \
	  src/applications/lpapi/Makefile \
	  src/applications/libaxmail/Makefile \
	  src/applications/mailer/Makefile \
          src/applications/Makefile \
          inst/Makefile \
	  plugins/Makefile])
AC_OUTPUT
