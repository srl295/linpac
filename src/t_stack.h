//-----------------------------------------------------------------------------
// t_stack.h
// Template for a stack (implemented as single linked list)
// Petr Kneblik (xknebl00@stud.fee.vutbr.cz), 2000
// Copying policy : GPL
//-----------------------------------------------------------------------------

#ifndef __T_STACK_H
#define __T_STACK_H

#include <stdlib.h>

//-----------------------------------------------------------------------------
// template for stack
template <class T>
class Stack {
//-----------------------------------------------------------------------------
private:
  class stack_item {  // one item of the list
  public:
    stack_item *next; // next item
    T value;          // value
  };
  stack_item *first;  // head of the list
  T error_val;        // returned by top() with empty stack
  
public:
//=============================================================================
  Stack() { first = NULL; }
  virtual ~Stack();

  // push value on the stack
  // returns TRUE on success, FALSE if memory is exhausted
  bool push(T v);

  // remove item from the top of stack
  void pop();
  
  // returns TRUE when the stack is empty, otherwise FALSE
  bool isEmpty()    { return (first == NULL); }
  
  // get item from the top of stack
  // when (isEmpty() == true) top() returns the value defined by setErrorValue()
  // if no error value is set, undefined value is returned
  T top()  { return first->value;  }

  // sets value returned on error by function top()
  void setErrorValue(T v) { error_val = v; }
//=============================================================================  
};

//-----------------------------------------------------------------------------
template <class T>
Stack<T>::~Stack()
//-----------------------------------------------------------------------------
{
  stack_item *tmp;
  while (first != NULL) {
    tmp = first;
    first = first->next;
    delete tmp;
  }
}

//-----------------------------------------------------------------------------
template <class T>
bool Stack<T>::push(T v)
//-----------------------------------------------------------------------------
{
  stack_item *tmp;
  
  if ((tmp = new stack_item) == NULL) return false;
  else {
    tmp->next = first;
    tmp->value = v;
    first = tmp;
  }
  return true;
}

//-----------------------------------------------------------------------------
template <class T>
void Stack<T>::pop()
//-----------------------------------------------------------------------------
{
  stack_item *tmp;
  
  if (first != NULL) {
    //dispValue(first->value);
    tmp = first;
    first = first->next;
    delete tmp;
  }
}

#endif
// end of file
