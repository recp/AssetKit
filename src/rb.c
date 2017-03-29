/*
 * Copyright (c), Recep Aslantas.
 * MIT License (MIT), http://opensource.org/licenses/MIT
 *
 * Insertion and Deletion are Top-Down
 *
 * X:  Current Node
 * P:  Parent Node         (Parent of X)
 * T:  X's Sibling Node
 * G:  Grand Parent Node   (Parent of P)
 * Q:  Great Parent Node   (Parent of Grand Parent)
 *
 * Y:  X's left child
 * Z:  X's right child
 *
 * sX: side of X           (if X is left then sX=0, right sX=1)
 * sP: side of P
 * sG: side of GD
 * sN: side of Next X      (Side of Next Current Node)
 *
 * cX: color of X
 */

#include "../include/ak-rb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define RB_RED        1
#define RB_BLCK       0
#define RB_RIGHT      1
#define RB_LEFT       0
#define RB_MKBLCK(R)  R->color = RB_BLCK
#define RB_MKRED(R)   R->color = RB_RED
#define RB_ISRED(R)   (R->color == RB_RED)
#define RB_ISBLCK(R)  (R->color == RB_BLCK)

static const char *rb_empty = "";

void
rb_printi(RBTree *tree, RBNode *node);

static
void
rb_walki(RBTree * __restrict tree,
         RBWalkFn            walkFn,
         RBNode * __restrict node);

static
int
rb_def_cmp_str(void * key1, void *key2) {
  return strcmp((char *)key1, (char *)key2);
}

static
int
rb_def_cmp_ptr(void *key1, void *key2) {
  if (key1 > key2)
    return 1;
  else if (key1 < key2)
    return -1;
  return 0;
}

static
void
rb_def_print_str(void *key) {
  printf("\t'%s'\n", (const char *)key);
}

static
void
rb_def_print_ptr(void *key) {
  printf("\t'%p'\n", key);
}

static
void
rb_free(RBTree *tree, RBNode *node) {
  if(node != tree->nullNode) {
    rb_free(tree, node->chld[RB_LEFT]);
    rb_free(tree, node->chld[RB_RIGHT]);
    free(node);
  }
}

RBTree*
rb_newtree(RBCmpFn cmp, RBPrintFn print) {
  RBTree *tree;
  RBNode *rootNode, *nullNode;

  tree     = malloc(sizeof(*tree));
  rootNode = calloc(sizeof(*rootNode), 1);
  nullNode = calloc(sizeof(*rootNode), 1);

  assert(tree && rootNode && nullNode);

  nullNode->key            = (void *)rb_empty;
  nullNode->chld[RB_LEFT]  = nullNode;
  nullNode->chld[RB_RIGHT] = nullNode;

  rootNode->key            = (void *)rb_empty;
  rootNode->chld[RB_LEFT]  = nullNode;
  rootNode->chld[RB_RIGHT] = nullNode;

  tree->root     = rootNode;
  tree->nullNode = nullNode;

  tree->cmp   = cmp   ? cmp   : rb_def_cmp_str;
  tree->print = print ? print : rb_def_print_str;

  return tree;
}

RBTree*
rb_newtree_str() {
  return rb_newtree(NULL, NULL);
}

RBTree*
rb_newtree_ptr() {
  return rb_newtree(rb_def_cmp_ptr, rb_def_print_ptr);
}

void
rb_destroy(RBTree *tree) {
  RBNode *node;

  node = tree->root->chld[RB_RIGHT];
  if (node != tree->nullNode)
    rb_free(tree, tree->root->chld[RB_RIGHT]);

  free(tree->root);
  free(tree->nullNode);
  free(tree);
}

void
rb_insert(RBTree *tree,
          void   *key,
          void   *val) {
  RBNode *newnode;
  RBNode *X, *P, *G, *Q, *W;
  int sQ, sG, sP, sX;

  newnode = malloc(sizeof(*newnode));
  newnode->chld[RB_LEFT]  = tree->nullNode;
  newnode->chld[RB_RIGHT] = tree->nullNode;
  newnode->key = key;
  newnode->val = val;

  if (tree->root->chld[RB_RIGHT] == tree->nullNode) {
    RB_MKBLCK(newnode);
    tree->root->chld[RB_RIGHT] = newnode;
    return;
  }

  sQ = sG = sP = sX = 1;

  W = P = G = Q = tree->root;
  X = P->chld[RB_RIGHT];

  /* Top-Down Insert */
  do {
    /* main case : two children are red */
    if (RB_ISRED(X->chld[RB_LEFT])
        && RB_ISRED(X->chld[RB_RIGHT])) {

      /* case 1: P is black */
      if (!RB_ISRED(P)) {
        /* make X red */
        RB_MKRED(X);

        /* make two children black */
        RB_MKBLCK(X->chld[RB_LEFT]);
        RB_MKBLCK(X->chld[RB_RIGHT]);
      }

      /* P is red */
      else {
        RB_MKRED(G);

        /* case 2: X and P are both left/right children */
        if (sX == sP){
          /* single rotation */

          RB_MKRED(X);
          RB_MKBLCK(P);
          RB_MKBLCK(X->chld[RB_LEFT]);
          RB_MKBLCK(X->chld[RB_RIGHT]);

          Q->chld[sG]  = P;
          G->chld[sP]  = P->chld[!sP];
          P->chld[!sP] = G;

          G = Q;
          Q = W;
        }

        /* case 3: X and P are opposide side */
        else {
          RB_MKBLCK(X);
          RB_MKBLCK(X->chld[RB_LEFT]);
          RB_MKBLCK(X->chld[RB_RIGHT]);

          Q->chld[sG] = X;
          P->chld[sX] = X->chld[sP];
          G->chld[sP] = X->chld[sX];
          X->chld[sP] = P;
          X->chld[sX] = G;

          G  = W;
          P  = Q;
          sX = sG;
          sP = sQ;
        }
      }
    }

    sQ = sG;
    sG = sP;
    sP = sX;
    sX = !(tree->cmp(key, X->key) < 0);
    W  = Q,
    Q  = G;
    G  = P;
    P  = X;
    X  = X->chld[sX];
  } while (X != tree->nullNode);

  X = P->chld[sX] = newnode;

  /* make current red */
  RB_MKRED(X);

  /* check for red violation, we know uncle is black */
  if (RB_ISRED(P)) {
    RB_MKRED(G);

    /* double rotation */
    if (sX != sP){
      RB_MKBLCK(X);

      Q->chld[sG]  = X;
      P->chld[sX]  = X->chld[!sX];
      G->chld[sP]  = X->chld[sX];
      X->chld[!sX] = P;
      X->chld[sX]  = G;
    }

    /* single rotation */
    else {
      RB_MKBLCK(P);

      G->chld[sP]  = P->chld[!sP];
      P->chld[!sP] = G;
      Q->chld[sG]  = P;
    }
  }

  /* make root black */
  RB_MKBLCK(tree->root->chld[RB_RIGHT]);
}

void
rb_remove(RBTree *tree, void *key) {
  RBNode *X, *P, *T, *G, *toDel, *toDelP;
  int     sP, sX, cmpRet, sDel;
  int     c2b;

  if (!key || key == rb_empty)
    return;

  sX     = RB_RIGHT;
  G      = tree->root;
  P      = G;
  X      = P->chld[RB_RIGHT];
  toDel  = tree->nullNode;
  toDelP = tree->nullNode;
  sDel   = 0;

  /* step 1: examine the root */
  if (RB_ISBLCK(X->chld[RB_LEFT])
      && RB_ISBLCK(X->chld[RB_RIGHT])) {
    RB_MKRED(X);
    c2b = 0;
  } else {
    /* step 2B */
    c2b = 1;
  }

  goto l0;

  /* Top-Down Deletion */
  do {
    /* case 2b continue: check new X */
    if (c2b) {
      c2b = 0;

      /* if new X is red continue down again */
      if (RB_ISRED(X))
        goto l0;

      G->chld[sP]  = T;
      P->chld[!sX] = T->chld[sX];
      T->chld[sX]  = P;

      RB_MKRED(P);
      RB_MKBLCK(T);

      if (toDelP == G) {
        toDelP = T;
        sDel = sX;
      }

      G  = T;
      T  = P->chld[!sX];
      sP = sX;
      /* if new X is black back to case 2 */
    }

    /* case 2: X has two black children */
    if (RB_ISBLCK(X->chld[RB_LEFT])
        && RB_ISBLCK(X->chld[RB_RIGHT])) {

      /* case 1.a: T has two black children */
      if (T != tree->nullNode
          && RB_ISBLCK(T->chld[RB_LEFT])
          && RB_ISBLCK(T->chld[RB_RIGHT])) {

        /* color flip */
        RB_MKRED(X);
        RB_MKRED(T);
        RB_MKBLCK(P);
      }

      /* case 1.b: T's left child is red */
      else if (RB_ISRED(T->chld[sX])) {
        RBNode *R;

        R = T->chld[sX];

        /* double rotate:
           rotate R around T, then R around P
         */
        T->chld[sX]  = R->chld[!sX];
        P->chld[!sX] = R->chld[sX];
        R->chld[sX]  = P;
        R->chld[!sX] = T;
        G->chld[sP]  = R;

        RB_MKRED(X);
        RB_MKBLCK(P);

        if (toDelP == G) {
          toDelP = R;
          sDel   = sX;
        }
      }

      /* case 1.c: T's right child is red */
      else if (RB_ISRED(T->chld[!sX])) {
        RBNode *R;

        R = T->chld[!sX];

        /* single rotate
           rotate T around P
         */
        P->chld[!sX] = T->chld[sX];
        T->chld[sX]  = P;
        G->chld[sP]  = T;

        RB_MKRED(X);
        RB_MKRED(T);
        RB_MKBLCK(P);
        RB_MKBLCK(R);

        if (toDelP == G) {
          toDelP = T;
          sDel   = sX;
        }
      }
    } else {
      /* case 2b: X's one child is red, advence to next level */
      c2b = 1;
    }

  l0:
    sP = sX;
    if (toDel != tree->nullNode) {
      sX = toDel->chld[RB_RIGHT] == tree->nullNode;
    } else {
      cmpRet = tree->cmp(key, X->key);

      if (cmpRet != 0) {
        sX = !(cmpRet < 0);
      } else {
        toDelP = P;
        toDel  = X;
        sDel   = sP;
        sX     = toDel->chld[RB_RIGHT] != tree->nullNode;
      }
    }

    G  = P;
    P  = X;
    X  = P->chld[sX];
    T  = P->chld[!sX];
  } while (X != tree->nullNode);

  /* make root black */
  RB_MKBLCK(tree->root->chld[RB_RIGHT]);

  if (toDel == tree->nullNode)
    return;

  /* toDel has least one child */
  if (P != toDel) {
    /* P is black, save black height */
    if (c2b)
      RB_MKBLCK(P->chld[!sX]);

    /* change color */
    if (RB_ISRED(toDel))
      RB_MKRED(P);
    else
      RB_MKBLCK(P);

    /* replace P with its left child */
    G->chld[sP] = P->chld[!sX];

    /* replace X with in-order predecessor */
    P->chld[RB_RIGHT]  = toDel->chld[RB_RIGHT];
    P->chld[RB_LEFT]   = toDel->chld[RB_LEFT];
    toDelP->chld[sDel] = P;
  }

  /* P is toDel; there is no child */
  else {
    G->chld[sP] = tree->nullNode;
  }

  free(toDel);
}

void *
rb_find(RBTree *tree, void *key) {
  RBNode *found;
  found = rb_find_node(tree, key);
  if (found == NULL)
    return NULL;

  return found->val;
}

RBNode*
rb_find_node(RBTree *tree, void *key) {
  RBNode *iter;

  iter = tree->root->chld[RB_RIGHT];

  while (iter != tree->nullNode) {
    int cmpRet;

    cmpRet = tree->cmp(iter->key, key);

    if (cmpRet == 0)
      break;

    iter = iter->chld[cmpRet < 0];
  }

  if (!iter || iter == tree->nullNode)
    return NULL;
  
  return iter;
}

int
rb_parent(RBTree *tree, void *key, RBNode **dest) {
   RBNode *iter, *parent;
   int side, cmpRet;

   side   = RB_RIGHT;
   iter   = tree->root->chld[side];
   parent = tree->root;
   cmpRet = -1;

   while (iter != tree->nullNode) {
      cmpRet = tree->cmp(iter->key, key);

      if (cmpRet == 0)
         break;

      side   = cmpRet < 0;
      parent = iter;
      iter   = iter->chld[side];
   }

   if (cmpRet != 0)
      *dest = NULL;
   else
      *dest = parent;

   return side;
}

void
rb_printi(RBTree *tree, RBNode *node) {
  if(node != tree->nullNode) {
    rb_printi(tree, node->chld[RB_LEFT]);
    tree->print(node->key);
    rb_printi(tree, node->chld[RB_RIGHT]);
  }
}

void
rb_print(RBTree *tree) {
  printf("Rb Id Dump:\n");
  printf("------------------------\n");

  if(tree->root->chld[RB_RIGHT] == tree->nullNode)
    printf("Empty tree\n");
  else
    rb_printi(tree, tree->root->chld[RB_RIGHT]);

  printf("------------------------\n");
}

static
void
rb_walki(RBTree * __restrict tree,
         RBWalkFn            walkFn,
         RBNode * __restrict node) {
  if(node == tree->nullNode)
    return;

  rb_walki(tree,
           walkFn,
           node->chld[RB_LEFT]);

  walkFn(tree, node);

  rb_walki(tree,
           walkFn,
           node->chld[RB_RIGHT]);
}

void
rb_walk(RBTree *tree, RBWalkFn walkFn) {
  RBNode *rootNode;

  rootNode = tree->root->chld[RB_RIGHT];
  if(rootNode == tree->nullNode)
    return;

  rb_walki(tree,
           walkFn,
           rootNode);
}

/*
 simple assertion for rbtree
 source: Eternally Confuzzled
 */
int
rb_assert(RBTree *tree, RBNode *root) {
  int lh, rh;
  if (root == tree->nullNode) {
    return 1;
  } else {
    RBNode *ln, *rn;

    ln = root->chld[0];
    rn = root->chld[1];

    /* Consecutive red links */
    if (RB_ISRED(root)) {
      if (RB_ISRED(ln) || RB_ISRED(rn)) {
        puts("Red violation");
        return 0;
      }
    }

    lh = rb_assert(tree, ln);
    rh = rb_assert(tree, rn);

    /* Invalid binary search tree */
    if ((ln != tree->nullNode
         && tree->cmp(ln->key, root->key) > 0)
        || (rn != tree->nullNode
            && tree->cmp(rn->key, root->key) < 0)) {
          puts("Binary tree violation");
          return 0;
        }

    /* Black height mismatch */
    if (lh != 0 && rh != 0 && lh != rh) {
      puts("Black violation");
      return 0;
    }

    /* Only count black links */
    if (lh != 0 && rh != 0)
      return RB_ISRED(root) ? lh : lh + 1;
    
    return 0;
  }
}
