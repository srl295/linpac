/*
  Callsign transformation routines
*/

int call_ssid(const char *src);   /* return the SSID from the call */
char *call_call(const char *src); /* return the callsign without ssid */
char *strip_ssid(char *src);      /* remove the SSID */
char *normalize_call(char *call); /* uppercase and remove zero SSID */
int compare_call(const char *call1, const char *call2);/* comare calls only (no ssid) */
int compare_call_ssid(const char *call1, const char *call2); /*compare call and ssid */

