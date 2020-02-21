#ifndef _UTILS_H
#define _UTILS_H

#include <wchar.h>

typedef struct Node {
  unsigned offset;
  wchar_t ch;
  struct Node *next;
} Node;

int wchwidth(wchar_t const ch);
int wstrwidth(wchar_t const *string);
int wstrlen(wchar_t const *string);
int wstrcmp(wchar_t *a, wchar_t *b);
Node *create_node(unsigned offset, wchar_t ch);
Node *pop_node(Node **head);
void free_link(Node **head);
Node **create_table(unsigned len);
void destroy_table(Node **head, unsigned len);
int push_table(Node **head, unsigned offset, wchar_t ch);
wchar_t **listdir(const char *dir_name, unsigned *nfile);
int destroy_listdir(wchar_t **items, unsigned nitem);

#endif
