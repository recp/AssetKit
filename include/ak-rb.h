/*
 * Copyright (c), Recep Aslantas.
 * MIT License (MIT), http://opensource.org/licenses/MIT
 */

#ifndef redblack_h
#define redblack_h
#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

struct RBTree;
struct RBNode;

typedef int  (*RBCmpFn)(void *, void *);
typedef void (*RBPrintFn)(void *);
typedef void (*RBWalkFn)(struct RBTree *tree, struct RBNode *node);
typedef void (*RBFreeFn)(void *);

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
  RBWalkFn  freeNode;
  RBFreeFn  freeFn;
  size_t    count;
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

RBNode*
rb_find_node(RBTree *tree, void *key);

int
rb_parent(RBTree *tree,
          void *key,
          RBNode ** dest);

void
rb_print(RBTree *tree);

void
rb_walk(RBTree *tree, RBWalkFn walkFn);

int
rb_assert(RBTree *tree, RBNode *root);

void
rb_destroy(RBTree *tree);

#ifdef __cplusplus
}
#endif
#endif /* redblack_h */
