#include <stdio.h>
#include <ctype.h>
#include "parser.h"

static char *outbuffer = NULL;
static bool amp_brk = true;

void P_amp_breaks(bool yes)
{
   amp_brk = yes;
}

bool P_is_field(char c)
{
   if (amp_brk)
      return (!isspace(c) && !iscntrl(c) && c != '@');
   return (!isspace(c) && !iscntrl(c));
}

char *P_next_field_start(const char *pos)
{
   char *p = (char *) pos;
   while (*p && P_is_field(*p)) p++;
   while (*p && !P_is_field(*p) && *p != '@') p++;
   return p;
}

char *P_field_end(const char *pos)
{
   char *p = (char *) pos;
   while (*p && P_is_field(*p)) p++;
   return p;
}

char *P_string_end(const char *pos)
{
   char *p = (char *) pos;
   while (*p) p++;
   return p;
}

char *P_extract(const char *start, const char *stop)
{
   if (outbuffer != NULL) delete[] outbuffer;
   outbuffer = new char[(stop-start)+1];
   int i = 0;
   for (char *p = (char *)start; p < stop; p++)
   {
      outbuffer[i] = *p;
      i++;
   }
   outbuffer[i] = '\0';
   return outbuffer;
}

