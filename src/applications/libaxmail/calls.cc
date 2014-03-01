#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "calls.h"

char *AXstrupr(char *s)
{
   for (char *p = s; *p; p++)
      *p = toupper(*p);
   return s;
}

/*======================== Callsign transformations =======================*/
/* Convert callsign into uppercase, strip zero SSID. */
char *AXnormalize_call(char *call)
{
  char *p;

  AXstrupr(call);
  p=strchr(call,'-');
  if(p){
    if(!atoi(p+1)) *p='\0';
  }
  return(call);
}

/* returns the callsign w/o SSID */
char *AXcall_call(const char *src)
{
  static char s[7];
  sscanf(src, "%6[A-Za-z0-9]", s);
  return s;
}

/* Returns 1 if the callsigns (without SSID) match. */
int AXcompare_call(char *c1, char *c2)
{
  for(;*c1&*c2;c1++,c2++){
    if((*c1=='-')&&(*c2=='-')) return(1);
    if(toupper(*c1)!=toupper(*c2)) return(0);
  }
  return(1);
}

/* return the first callsign in the path */
char *AXfirst_call(char *dest,const char *src)
{
  sscanf(src,"%10[A-Za-z0-9-]",dest);
  return(dest);
}

