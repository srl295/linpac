/*
  LinPac Mailer
  (c) 1998 - 1999 by Radek Burget OK2JBG (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.

  mail_route.h - mail routes database
*/
#include <algorithm>
#include <vector.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>

#define OUT_FILE "/var/ax25/mail_routes"

#define HOME_COUNT 5

//-----------------------------------------------------------------------
// class addr
//-----------------------------------------------------------------------
class route_addr
{
  public:
    char call[10];
    char route[36];
    int count;
    int pcount;
    time_t ttime;

    route_addr();
    route_addr &operator = (const route_addr &);
};

route_addr &route_addr::operator = (const route_addr &src)
{
  strcpy(call, src.call);
  strcpy(route, src.route);
  count = src.count;
  pcount = src.pcount;
  ttime = src.ttime;
  return *this;
}

route_addr::route_addr()
{
  strcpy(call, "");
  strcpy(route, "");
  count = 0;
  pcount = 0;
  ttime = 0;
}


vector <route_addr> routes;
    
//-----------------------------------------------------------------------

//read the database
void load_routes()
{
  FILE *f = fopen(OUT_FILE, "r");
  if (f == NULL) return;
  while (!feof(f))
  {
    char line[256];
    char call[256];
    char route[256];
    int count, pcount;
    time_t ttime;
    route_addr newaddr;

    strcpy(line, "");
    fgets(line, 255, f);
    int n = sscanf(line, "%s %s %i %i %li", call, route, &count, &pcount, &ttime);
    if (n == 5)
    {
      strncpy(newaddr.call, call, 10);
      strncpy(newaddr.route, route, 35);
      newaddr.count = count;
      newaddr.ttime = ttime;
      newaddr.pcount = pcount;
      routes.push_back(newaddr);
    }
  }
  return;
}


bool find_route(char *call, char *result)
{
  vector <route_addr> cands;
  vector <route_addr>::iterator it, home;
  bool found = false;

  //select candidates
  for (it = routes.begin(); it < routes.end(); it++)
    if (strcmp(call, it->call) == 0) cands.push_back(*it);

  if (cands.empty()) return false;

  //find the forced home (time = -1)
  for (it = cands.begin(); it < cands.end(); it++)
    if (it->ttime == -1)
    {
      home = it;
      found = true;
    }
  //find the bbs with PCOUNT > HOME_COUNT
  if (!found)
    for (it = cands.begin(); it < cands.end(); it++)
      if (it->pcount > HOME_COUNT)
      {
        home = it;
        found = true;
      }
  //if not found return the most used BBS
  if (!found)
  {
    home = cands.begin();
    for (it = cands.begin(); it < cands.end(); it++)
      if (it->count > home->count) home = it;
  }

  strcpy(result, home->route);
  return true;
}

