#include <dirent.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

int wchwidth(wchar_t const ch) { return ch > 255 ? 2 : 1; }

int wstrwidth(wchar_t const *string) {
  int len = 0;
  while (*string) {
    len += *string > 255 ? 2 : 1;
    string++;
  }

  return len;
}

int wstrlen(wchar_t const *string) {
  unsigned len = 0;
  while (*string++) {
    len++;
  }

  return len;
}

int wstrcmp(wchar_t *a, wchar_t *b) {

  while (*a || *b) {
    if (*a < *b)
      return -1;
    if (*a > *b)
      return 1;

    a++;
    b++;
  }

  return 0;
}

Node *create_node(unsigned offset, wchar_t ch) {
  Node *node = (Node *)malloc(sizeof(Node));
  if (node == NULL)
    return NULL;

  node->offset = offset;
  node->ch = ch;
  node->next = NULL;

  return node;
}

int push_node(Node **head, Node *node) {
  node->next = *head;
  *head = node;

  return 1;
}

Node *pop_node(Node **head) {
  if (*head == NULL)
    return NULL;

  Node *node = *head;
  *head = node->next;
  node->next = NULL;

  return node;
}

void free_link(Node **head) {
  Node *node;
  while ((node = *head)) {
    *head = node->next;
    free(node);
  }
}

Node **create_table(unsigned len) {
  Node **table = (Node **)calloc(sizeof(Node *), len);
  return table;
}

void destroy_table(Node **head, unsigned len) {
  while (len-- > 0) {
    free_link(head + len);
  }
  free(head);
}

int push_table(Node **head, unsigned offset, wchar_t ch) {
  Node *node = create_node(offset, ch);
  if (node == NULL)
    return 0;

  return push_node(head, node);
}

  wchar_t **listdir(const char *dir_name, unsigned *nfile) {
  int i = 0, nfile_ = 0;
  unsigned strlen_;
  DIR *dr = opendir(dir_name);
  wchar_t *item, **items;
  struct dirent *dirt;

  /* 扫描共有多少个文件 */
  while ((dirt = readdir(dr))) {
    if (strcmp(dirt->d_name, ".") == 0 || strcmp(dirt->d_name, "..") == 0)
      continue;
    nfile_++;
  }

  items = (wchar_t **)malloc(sizeof(wchar_t *) * nfile_);

  rewinddir(dr);
  while ((dirt = readdir(dr))) {
    if (strcmp(dirt->d_name, ".") == 0 || strcmp(dirt->d_name, "..") == 0)
      continue;

    strlen_ = strlen(dirt->d_name);
    item = (wchar_t *)calloc(sizeof(wchar_t), strlen_ + 1);
    mbstowcs(item, dirt->d_name, strlen_);
    items[i++] = item;
  }

  closedir(dr);

  *nfile = nfile_;
  return items;
}

int destroy_listdir(wchar_t **items, unsigned nitem) {
  for (unsigned i = 0; i < nitem; i++)
    free(items[i]);
  free(items);

  return 1;
}
