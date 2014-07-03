/*==========================================================================
   LinPac: Packet Radio Terminal for Linux
   (c) Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz) 1998

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version
   2 of the license, or (at your option) any later version.

   calls.cc

   Functions for callsign conversions
   Some of this functions are based on the 'axutils.c' from ax25-utils
   
   Last update 20.5.1998
  =========================================================================*/
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "calls.h"

void norm_call(char *peer, const ax25_address *a)
{
	static char buf[11];
	char c, *s;
	int n;

	for (n = 0, s = buf; n < 6; n++) {
		c = (a->ax25_call[n] >> 1) & 0x7F;

		if (c != ' ') *s++ = c;
	}
	
	*s++ = '-';

	if ((n = ((a->ax25_call[6] >> 1) & 0x0F)) > 9) {
		*s++ = '1';
		n -= 10;
	}
	
	*s++ = n + '0';
	*s++ = '\0';

	strcpy(peer, buf);
}

int convert_call(char *name, char *buf)
{
  char *p = name;
  int index = 0;
  int ssid = 0;

  while (index < 6)
  {
    if (*p == '-' || *p == '\0') break;
    if (islower(*p)) *p = toupper(*p);
    if (!isalnum(*p)) return -1;
    buf[index] = (*p << 1);
    p++; index++;
  }
  while (index < 6) buf[index++] = ' ' << 1;
  if (*p == '-')
  {
     p++;
     if (sscanf(p, "%d", &ssid) != 1 || ssid < 0 || ssid > 15) return -1;
  }
  buf[6] = ((ssid+'0') << 1) & 0x1E;
  return 0;
}

int convert_path(char *call, struct full_sockaddr_ax25 *sax)
{
  int len = 0;
  char *bp, *np;
  char *addrp;
  int n = 0;
  char *tmp = strdup(call);

  if (tmp == NULL) return -1;
  bp = tmp;
  len = sizeof (struct sockaddr_ax25);
  addrp = sax->fsa_ax25.sax25_call.ax25_call;
  do
  {
    while (*bp && isspace(*bp)) bp++;
    np = bp;
    while (*np && !isspace(*np)) np++;
    if (*np) *np++ = '\0';
    if (n==1 && (strcasecmp(bp,"V")==0 || strcasecmp(bp,"VIA")==0) )
    {
      bp = np;
      continue;
    }
    if (convert_call(bp, addrp) == -1) return -1;
    bp=np;
    addrp = sax->fsa_digipeater[n].ax25_call;
    n++;
    if (n==2)
    len = sizeof(struct full_sockaddr_ax25);
  } while (n<AX25_MAX_DIGIS && *bp);
  free (tmp);
  sax->fsa_ax25.sax25_ndigis = n-1;
  sax->fsa_ax25.sax25_family = AF_AX25;
  return len;
}

