#ifndef PARSER_H
#define PARSER_H

void P_amp_breaks(bool yes);
bool P_is_field(char c);
char *P_next_field_start(const char *p);
char *P_field_end(const char *p);
char *P_string_end(const char *p);
char *P_extract(const char *start, const char *stop);

#endif

