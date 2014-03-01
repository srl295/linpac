/*
   Associative array routines
   (c) 2001 Radek Burget <radkovo@centrum.cz>
*/

#ifndef HASH_H
#define HASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* basic element structure */
struct h_element
{
   char *name;
   char *data;
   struct h_element *left;
   struct h_element *right;
};

/* hash structure */
struct hash
{
   struct h_element *start[256];
};

/* linked list of element names */
struct h_name_list
{
   char *name;
   struct h_name_list *next;
};

/************************* BASIC ELEMENT OPERATIONS ************************/

/* create a leaf element with specified name & data */
struct h_element *create_element(const char *name, const char *data);

/* destroy an element (do not care about its children) */
void destroy_element(struct h_element *element);

/************************** TREE NODE OPERATIONS **************************/

void add_elements_recursive(struct hash *hash, struct h_element *root);

void del_elements_recursive(struct h_element *root);

/**************************** HASH OPERATIONS *****************************/

/* create an empty hash */
struct hash *create_hash();

/* find an element by name, returns NULL if not found */
struct h_element *find_element(struct hash *hash, const char *name);

/* add a new element */
void add_element(struct hash *hash, struct h_element *newelem);

/* find and delete an element */
void delete_element(struct hash *hash, const char *name);

/* destroy all the elements */
void destroy_hash(struct hash *hash);

/********************************* TOOLS *********************************/

/* modify the value or add a new element */
void hash_set(struct hash *hash, const char *name, const char *value);

/* get the element's value (NULL if not found) */
char *hash_get(struct hash *hash, const char *name);

/******************************* NAME LIST ******************************/

/* create a list of names of known elements */
struct h_name_list *get_name_list(struct hash *hash);

/* destroy the list of names */
void destroy_name_list(struct h_name_list *start);

#ifdef __cplusplus
}
#endif

#endif

