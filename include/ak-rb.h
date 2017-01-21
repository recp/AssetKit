/*
 * Copyright (c), Recep Aslantas.
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#ifndef redblack_h
#define redblack_h

#include <stdint.h>

typedef int  (*RBCmpFn)(void *, void *);
typedef void (*RBPrintFn)(void *);

typedef struct RBNode {
  void          *key;
  void          *val;
  struct RBNode *chld[2];
  uint8_t        color;
} RBNode;

typedef struct RBTree {
  RBNode   *root;
  RBNode   *nullNode;
  RBCmpFn   cmp;
  RBPrintFn print;
} RBTree;

RBTree*
rb_newtree(RBCmpFn cmp, RBPrintFn print);

RBTree*
rb_newtree_str();

RBTree*
rb_newtree_ptr();

void
rb_insert(RBTree *tree,
          void *key,
          void *val);

void
rb_remove(RBTree *tree, void *key);

void *
rb_find(RBTree *tree, void *key);

int
rb_parent(RBTree *tree,
          void *key,
          RBNode ** dest);

void
rb_print(RBTree *tree);

int
rb_assert(RBTree *tree, RBNode *root);

void
rb_destroy(RBTree *tree);

#endif /* redblack_h */
