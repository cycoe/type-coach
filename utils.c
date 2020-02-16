#include "utils.h"
#include <malloc.h>
#include <wchar.h>

int wstrwidth(wchar_t const *string) {
  int len = 0;
  while (*string) {
    len += *string > 256 ? 2 : 1;
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

int push_table(Node **head, unsigned offset, wchar_t ch) {
  Node *node = create_node(offset, ch);
  if (node == NULL)
    return 0;

  return push_node(head, node);
}
