/*
  Library for evaluating numerical expressions
  (c) 1998 by Radek Burget (xburge01@stud.fee.vutbr.cz)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the license, or (at your option) any later version.
*/

const int CalcOK = 0;
const int BadCloseErr = 1;        // Found ')' with no matching '('
const int BadOpenErr = 2;         // Found '(' with no matching ')'
const int BadCharErr = 3;         // unknown character
const int DivZeroErr = 4;         // division by zero
const int RangeErr = 5;           // out of range
const int OperandErr = 6;         // lack of operands
const int BadOperator = 7;        // unknown operator

extern int CalcResult;

void Infix2Postfix(const char *ins, char *outs);
int EvaluatePostfix(const char *ins);

