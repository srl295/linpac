#ifndef CALLS_H
#define CALLS_H

/* Convert callsign into uppercase, strip zero SSID. */
char *AXnormalize_call(char *call);

/* returns the callsign w/o SSID */
char *AXcall_call(const char *src);

/* Returns 1 if the callsigns (without SSID) match. */
int AXcompare_call(char *c1, char *c2);

/* return the first callsign in the path */
char *AXfirst_call(char *dest,const char *src);

#endif

