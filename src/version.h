#ifndef VERSION_H
#define VERSION_H

#ifdef HAVE_CONFIG_H

#include "config.h"

#else

#define PACKAGE "LinPac"
#define VERSION "0.16"
#define VERINFO "Author's work version"

/* socklen_t is suposed to be defined */
//#define NO_SOCKLEN_T

/* uncomment this to enable new ax.25 implementation support */
//#define NEW_AX25

#endif

/* always support debugging since this is the development release */
#define LINPAC_DEBUG

/* check for memory lacks in first 64 objects - very slow ! */
//#define MEMCHECK 64

/* shared configuration structure */
#define SHARED_CONFIG

/* enable ax25spyd support */
#define WITH_AX25SPYD

#endif

