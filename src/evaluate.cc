/*
  Library for evaluating numerical expressions
  (c) 1998 by Radek Burget (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.
*/
#include <string.h>
#include "t_stack.h"
#include "evaluate.h"

typedef Stack <char> CStack;
typedef Stack <int> IStack;

int CalcResult;

int priority(char ch)
{
  switch (ch)
  {
    case '(': return 0;
    case '+':
    case '-': return 1;
    case '*':
    case '/': return 2;
    case '_': return 3;
    default : CalcResult = BadCharErr;
              return -1;
  }
}

bool ins_possible(CStack &s, char ch)
{
  if (s.isEmpty()) return true;
  else return (priority(s.top()) < priority(ch));
}


void Infix2Postfix(const char *ins, char *outs)
{
  CStack s;
  char *p = const_cast <char *>(ins);
  char ch;
  bool unar = true;
  bool end = false;

  strcpy(outs, "");
  if (*p) while(!end)
  {
    ch = *p;
    switch (ch)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': strncat(outs, &ch, 1); break;
      case '(': s.push(ch); break;
      case '+':
      case '-':
      case '*':
      case '/': if (unar && ch == '-') ch = '_';
                while (!ins_possible(s, ch))
                {
                  char o = s.top();
                  strncat(outs,& o, 1);
                  s.pop();
                }
                s.push(ch);
                strcat(outs, " ");
                break;
      case ')': char o;
                do
                {
                  o = s.top();
                  if (o != '(') strncat(outs,& o, 1);
                  s.pop();
                } while (!s.isEmpty() && o != '(');
                if (o != '(') CalcResult = BadCloseErr;
                break;
      case ' ': break;
      case '\0':while (!s.isEmpty())
                {
                  char o = s.top();
                  strncat(outs,& o, 1);
                  s.pop();
                }
                end = true;
                break;

      default: CalcResult = BadCharErr; break;
    }
    unar = (*p == '(');
    p++;
  }
}

int get_number(char **p)
{
  char *start = *p;
  return strtol(start, p, 10);
}

int EvaluatePostfix(const char *ins)
{
  char *p = const_cast <char *>(ins);
  IStack s;
  int op1, op2, vysl;
  char opr;

  CalcResult = CalcOK;
  while (*p)
  {
    opr = *p;
    switch (opr)
    {
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9': op1 = get_number(&p);
                s.push(op1);
                p--;
                break;
      case '+':
      case '-':
      case '*':
      case '/': if (!s.isEmpty())
                {
                  op1 = s.top(); s.pop();
                  if (!s.isEmpty())
                  {
                    op2 = s.top(); s.pop();
                    switch (opr)
                    {
                      case '+': vysl = op2 + op1; break;
                      case '-': vysl = op2 - op1; break;
                      case '*': vysl = op2 * op1; break;
                      case '/': if (op1) vysl = op2 / op1;
                                    else CalcResult = DivZeroErr;
                                break;
                    }
                    s.push(vysl);
                  } else CalcResult = OperandErr;
                } else CalcResult = OperandErr;
                break;
      case '_': if (!s.isEmpty())
                {
                  op1 = -s.top(); s.pop();
                  s.push(op1);
                } else CalcResult = OperandErr;
                break;
    }
    p++;
  }
  if (!s.isEmpty()) {vysl = s.top(); s.pop();}
           else vysl = 0;
  return vysl;
}

