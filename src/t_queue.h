//-----------------------------------------------------------------------------
// t_queue.h
// Template for a queue (implemented as single linked list)
// Petr Kneblik (xknebl00@stud.fee.vutbr.cz), 2000
// Copying policy : GPL
//-----------------------------------------------------------------------------

#ifndef __T_QUEUE_H
#define __T_QUEUE_H

#include <stdlib.h>

//-----------------------------------------------------------------------------
// template for a queue
template <class T>
class Queue {
//-----------------------------------------------------------------------------
private:
  struct queue_item {  // one item of the queue
    queue_item *next;  // next item
    T value;           // value
  };
  
  queue_item *fst;  // first element of the list
  queue_item *lst;  // last element of the list
  long max_len;     // maximal length of the queue (-1 .. unconstrained)
  long act_len;     // actual length of the queue
  T error_val;      // returned as an error result
  
public:
//=============================================================================
  // constructs empty queue
  // max_length .. maximal length of the queue (default is -1, not constrained)
  Queue(long max_length = -1) { fst = lst = NULL; max_len = max_length; act_len = 0; }
  virtual ~Queue();
  
  // inserts the first and the last element
  // returns FALSE when memory is exhausted or the queue reached its maximal capacity
  // returns TRUE on success
  bool insertFirst(T &v);
  bool insertLast(T &v);
  
  // returns the first and the last element
  // when (isEmpty() == true) both functions return the value defined by setErrorValue()
  // if no error value is set, undefined value is returned
  T  &first() { return (fst != NULL) ? fst->value : error_val; }
  T  &last()  { return (lst != NULL) ? lst->value : error_val; }
  
  // works like first() and last() and the returned element is also removed
  T  removeFirst();
  T  removeLast();

  // returns reference to the item at given position
  // if index is out of range 0..size()-1, the value defined by setErrorValue() is
  // returned - if no error value is set, undefined value is returned
  T& at(long pos);
  T& operator[](long pos) { return at(pos); }
  
  // returns TRUE when the queue is empty, otherwise FALSE
  bool isEmpty() { return (act_len == 0); }
  
  // returns number of elements in the queue
  long size() { return act_len; }
  
  // sets value returned on error by functions first(), last() ...
  void setErrorValue(T v) { error_val = v; }

  // removes all items
  void clear() { while (!isEmpty()) removeFirst(); }
//=============================================================================  
};
  
//-----------------------------------------------------------------------------
template <class T>
Queue<T>::~Queue()
//-----------------------------------------------------------------------------
{
  queue_item *tmp;
  while (fst != NULL) {
    tmp = fst;
    fst = fst->next;
    delete tmp;
  }
}

//-----------------------------------------------------------------------------
template <class T>
bool Queue<T>::insertFirst(T &v)
//-----------------------------------------------------------------------------
{
  if ((max_len >= 0)&&(act_len >= max_len)) return false;

  queue_item *tmp;
  if ((tmp = new queue_item) == NULL) return false;
  tmp->value = v;
  tmp->next = fst;
  fst = tmp;
  if (lst == NULL) lst = tmp;
  act_len++;
  return true;
}
  
//-----------------------------------------------------------------------------
template <class T>
bool Queue<T>::insertLast(T &v)
//-----------------------------------------------------------------------------
{
  if ((max_len >= 0)&&(act_len >= max_len)) return false;

  queue_item *tmp;
  if ((tmp = new queue_item) == NULL) return false;
  if (lst != NULL) lst->next = tmp;
  lst = tmp;
  tmp->next = NULL;
  tmp->value = v;
  if (fst == NULL) fst = tmp;
  act_len++;
  return true;
}

//-----------------------------------------------------------------------------
template <class T>
T Queue<T>::removeFirst()
//-----------------------------------------------------------------------------
{
  if (fst == NULL) return error_val;

  queue_item *tmp = fst;
  T val = fst->value;
  fst = fst->next;
  delete tmp;
  if (fst == NULL) lst = NULL;
  act_len--;
  return val;
}

//-----------------------------------------------------------------------------
template <class T>
T Queue<T>::removeLast()
//-----------------------------------------------------------------------------
{
  if (lst == NULL) return error_val;

  T val = lst->value;
  if (fst == lst) {
    delete lst;
    fst = lst = NULL;
  }
  else {
    queue_item *tmp = fst;
    while (tmp->next != lst) tmp = tmp->next;
    tmp->next = NULL;
    delete lst;
    lst = tmp;
  }
  act_len--;
  return val;
}

//-----------------------------------------------------------------------------
template <class T>
T& Queue<T>::at(long pos)
//-----------------------------------------------------------------------------
{
  if ((pos < 0) || (pos >= act_len)) return error_val;

  queue_item *tmp = fst;
  for (long i = 0; i < pos; i++) tmp = tmp->next;
  return tmp->value;
}

#endif
// end of file
