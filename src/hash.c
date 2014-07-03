/*
   Associative array routines
   (c) 2001 Radek Burget <radkovo@centrum.cz>
*/

#include <stdlib.h>
#include <string.h>

#include "hash.h"


/************************* BASIC ELEMENT OPERATIONS ************************/

/* create a leaf element with specified name & data */
struct h_element *create_element(const char *name, const char *data)
{
   struct h_element *newelem;
   newelem = (struct h_element *) malloc(sizeof(struct h_element));
   newelem->name = strdup(name);
   newelem->data = strdup(data);
   newelem->left = NULL;
   newelem->right = NULL;
   return newelem;
}

/* destroy an element (do not care about its children) */
void destroy_element(struct h_element *element)
{
   if (element)
   {
      if (element->name) free(element->name);
      if (element->data) free(element->data);
      free(element);
   }
}

/************************** TREE NODE OPERATIONS **************************/

void add_elements_recursive(struct hash *hash, struct h_element *root)
{
   if (root)
   {
      if (root->left) add_elements_recursive(hash, root->left);
      if (root->right) add_elements_recursive(hash, root->right);
      add_element(hash, root);
   }
}

void del_elements_recursive(struct h_element *root)
{
   if (root)
   {
      if (root->left) del_elements_recursive(root->left);
      if (root->right) del_elements_recursive(root->right);
      destroy_element(root);
   }
}

/**************************** HASH OPERATIONS *****************************/

/* create an empty hash */
struct hash *create_hash()
{
   struct hash *newhash;
   int i;
   
   newhash = (struct hash *) malloc(sizeof(struct hash));
   for (i = 0; i < 256; i++) newhash->start[i] = NULL;
   return newhash;
}

struct h_element *find_element(struct hash *hash, const char *name)
{
   struct h_element *el;

   el = hash->start[(int)name[0]];
   while (el && strcmp(name, el->name) != 0)
   {
      if (strcmp(name, el->name) < 0) el = el->left;
                                 else el = el->right;
   }
   return el;
}

void add_element(struct hash *hash, struct h_element *newelem)
{
   if (!hash->start[(int)newelem->name[0]])
   {
      hash->start[(int)newelem->name[0]] = newelem;
      newelem->left = newelem->right = NULL;
   }
   else
   {
      struct h_element *el;
      int inserted = 0;
      
      el = hash->start[(int)newelem->name[0]];
      while (!inserted)
      {
         if (strcmp(newelem->name, el->name) < 0)
         {
            if (el->left) el = el->left;
            else
            {
               el->left = newelem;
               newelem->left = newelem->right = NULL;
               inserted = 1;
            }
         }
         else
         {
            if (el->right) el = el->right;
            else
            {
               el->right = newelem;
               newelem->left = newelem->right = NULL;
               inserted = 1;
            }
         }
      }
   }
}

void delete_element(struct hash *hash, const char *name)
{
   struct h_element *el, *pre = NULL;

   el = hash->start[(int)name[0]];
   if (!el) return;

   while (el && strcmp(name, el->name) != 0)
   {
      pre = el;
      if (strcmp(name, el->name) < 0) el = el->left;
                                 else el = el->right;
   }

   if (el && strcmp(name, el->name) == 0)
   {
      if (pre)
      {
         if (el == pre->left)
            pre->left = NULL;
         else
            pre->right = NULL;
      }
      else
         hash->start[(int)name[0]] = NULL;

      add_elements_recursive(hash, el->left);
      add_elements_recursive(hash, el->right);
      destroy_element(el);
   }
}

void destroy_hash(struct hash *hash)
{
   int i;
   for (i = 0; i < 256; i++)
   {
      del_elements_recursive(hash->start[i]);
      hash->start[i] = NULL;
   }
   free(hash);
}

/********************************* TOOLS *********************************/

void hash_set(struct hash *hash, const char *name, const char *value)
{
   struct h_element *el;
   el = find_element(hash, name);
   if (el)
   {
      free(el->data);
      el->data = strdup(value);
   }
   else
      add_element(hash, create_element(name, value));
}

char *hash_get(struct hash *hash, const char *name)
{
   struct h_element *el;
   el = find_element(hash, name);
   if (el) return el->data;
   return NULL;
}

struct h_name_list *h_name_list_recursive(struct h_element *root,
                                          struct h_name_list *previous)
{
   if (root)
   {
      struct h_name_list *newname, *stepl, *stepr;

      newname = (struct h_name_list *) malloc(sizeof(struct h_name_list));
      newname->name = root->name;
      if (previous) previous->next = newname;

      stepl = h_name_list_recursive(root->left, newname);
      stepr = h_name_list_recursive(root->right, stepl);

      return stepr;
   }
   return previous;
}

struct h_name_list *get_name_list(struct hash *hash)
{
   struct h_name_list start, *last;
   int i;

   last = &start;
   for (i = 0; i < 256; i++)
      last = h_name_list_recursive(hash->start[i], last);
   last->next = NULL;
   
   return start.next;
}

void destroy_name_list(struct h_name_list *start)
{
   struct h_name_list *act = start;
   while (act)
   {
      struct h_name_list *next = act->next;
      free(act);
      act = next;
   }
}

