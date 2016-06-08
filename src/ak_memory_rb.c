/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
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

#include "ak_memory_rb.h"
#include <stdio.h>

void
ak_heap_rb_insert(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode) {
  AkHeapSrchNode *X, *P, *G, *Q;
  int sG, sP, sX;

  sG = sP = sX = 1;
  
  P = G = Q = srchctx->root;
  X = P->chld[AK__BST_RIGHT];

  /* Top-Down Insert */
  while (X != srchctx->nullNode) {
    sG = sP;
    sP = sX;
    sX = !(srchctx->cmp(srchNode->key, X->key) < 0);

    Q = G;
    G = P;
    P = X;
    X = X->chld[sX];

  case1:
    /*
     * case 1: two children are red
     *   color flip
     */
    if (AK__RB_ISRED(X->chld[AK__BST_LEFT])
        && AK__RB_ISRED(X->chld[AK__BST_RIGHT])) {
      /* make current red */
      AK__RB_MKRED(X);

      /* make two children black */
      AK__RB_MKBLACK(X->chld[AK__BST_LEFT]);
      AK__RB_MKBLACK(X->chld[AK__BST_RIGHT]);

      /* case 2: both X and P are red: red violation */
      if (AK__RB_ISRED(P)) {
        AK__RB_MKRED(G);

        /*
         * case 2a: double rotation
         *          X and P are opposide side
         *          current < grand != current < parent
         */
        if (sX != sP){
          AkHeapSrchNode *newX;
          int sN;

          /* move X down to avoid re-compare to P or G
             we are already know that new X will be one of children of X
           */
          sN   = !(srchctx->cmp(srchNode->key, X->key) < 0);
          newX = X->chld[sN];

          P->chld[sX]  = X->chld[!sX];
          G->chld[sP]  = X->chld[sX];
          X->chld[!sX] = P;
          X->chld[sX]  = G;
          Q->chld[sG]  = X;

          AK__RB_MKBLACK(X);

          P = X->chld[sN];
          X = newX;
          G = X;

          sP = sN;
          sX = !sP;

          goto case1;
        }

        /* single rotation */
        else {
          G->chld[sP]  = P->chld[!sX];
          P->chld[!sX] = G;
          Q->chld[sG]  = P;

          AK__RB_MKBLACK(P);

          G = Q;
        }
      }
    }
  }

  if (X != srchctx->nullNode)
    return;

  /* Red Black Node is pre-allocated in memory/heap tree */
  X = P->chld[sX] = srchNode;

  /* make current red */
  AK__RB_MKRED(X);

  /*
   * check case 2 for new node
   * case 2: both X and P are red: red violation
   */
  if (AK__RB_ISRED(P)) {
    AK__RB_MKRED(G);

    /* double rotation */
    if (sX != sP){
      P->chld[sX]  = X->chld[!sX];
      G->chld[sP]  = X->chld[sX];
      X->chld[!sX] = P;
      X->chld[sX]  = G;
      Q->chld[sG]  = X;

      AK__RB_MKBLACK(X);
    }

    /* single rotation */
    else {
      G->chld[sP]  = P->chld[!sX];
      P->chld[!sX] = G;
      Q->chld[sG]  = P;

      AK__RB_MKBLACK(P);
    }
  }

  /* make root black */
  AK__RB_MKBLACK(srchctx->root->chld[AK__BST_RIGHT]);
}

void
ak_heap_rb_remove(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode) {
  AkHeapSrchNode *X, *P, *T, *G;
  int            sG, sP, sX, cX, cmpRet;

  sG = sP = sX = 1;
  cX = 0; /* root is black */

  G = P = srchctx->root;
  X = P->chld[AK__BST_RIGHT];
  T = srchctx->nullNode;

  /* step 1: make root red */
  AK__RB_MKRED(srchctx->root->chld[AK__BST_RIGHT]);

  /* Top-Down Deletion */
  while (X != srchctx->nullNode) {
    cmpRet = srchctx->cmp(srchNode->key, X->key);

    sG = sP;
    sP = sX;
    sX = !(cmpRet < 0);
    G  = P;
    P  = X;
    T  = P->chld[!sX];
    X  = P->chld[sX];

    if (X == srchctx->nullNode)
      goto step3;

    cX = AK__RB_ISRED(X);

    /* step 2: make X red */
    AK__RB_MKRED(X);

    /* case 2: X is black T is red
     *         (prev X has one red child and new X is black) 
     */
    if (cX == AK_HEAP_NODE_FLAGS_RED
        && AK__RB_ISRED(T)) {
      AkHeapSrchNode *R;

      R = T->chld[!sX];

      P->chld[!sX] = T->chld[sX];
      T->chld[sX]  = P;
      G->chld[sP]  = T;

      AK__RB_MKRED(P);
      AK__RB_MKBLACK(T);

      G = T;
      T = R;

      sP = sX;
    }

    /* case 1: X has two black children */
    if (!AK__RB_ISRED(X->chld[AK__BST_LEFT])
        && !AK__RB_ISRED(X->chld[AK__BST_RIGHT])) {

      /* case 1.a: T has two black children */
      if (T != srchctx->nullNode
          && !AK__RB_ISRED(T->chld[AK__BST_LEFT])
          && !AK__RB_ISRED(T->chld[AK__BST_RIGHT])) {

        /* color flip */
        AK__RB_MKRED(X);
        AK__RB_MKRED(T);
        AK__RB_MKBLACK(P);
      }

      /* case 1.b: T's left child is red */
      else if (AK__RB_ISRED(T->chld[sX])) {
        AkHeapSrchNode *R;

        R = T->chld[sX];

        /* double rotate:
           rotate R around T, then R around P
         */
        T->chld[sX]  = R->chld[!sX];
        P->chld[!sX] = R->chld[sX];
        R->chld[sX]  = P;
        R->chld[!sX] = T;
        G->chld[sP]  = R;

        AK__RB_MKRED(X);
        AK__RB_MKBLACK(P);

        G = R;
        T = P->chld[!sX];

        sP = sX;
      }

      /* case 1.c: T's right child is red */
      else if (AK__RB_ISRED(T->chld[!sX])) {
        AkHeapSrchNode *R;

        R = T->chld[!sX];

        /* single rotate
           rotate T around P
         */
        P->chld[!sX] = T->chld[sX];
        T->chld[sX]  = P;
        G->chld[sP]  = T;

        AK__RB_MKRED(X);
        AK__RB_MKRED(T);
        AK__RB_MKBLACK(P);
        AK__RB_MKBLACK(R);

        G = T;
        T = P->chld[!sX];

        sG = sP;
        sP = sX;
      }
    }

  step3:
    /* found
         X is P, P will be removed as X
         attach X's left to X's right->left(min) if exists
     */
    if (cmpRet == 0) {
      AkHeapSrchNode *Z;

      Z = P->chld[AK__BST_RIGHT];
      if (Z != srchctx->nullNode) {
        while (Z->chld[AK__BST_LEFT] != srchctx->nullNode)
          Z = Z->chld[AK__BST_LEFT];

        G->chld[sP] = P->chld[AK__BST_RIGHT];
        Z->chld[AK__BST_LEFT] = P->chld[AK__BST_LEFT];
      } else {
        G->chld[sP] = P->chld[AK__BST_LEFT];
      }

      break;
    }
  } /* while */

  /* step4: make root black */
  AK__RB_MKBLACK(srchctx->root->chld[AK__BST_RIGHT]);
}

AkHeapSrchNode *
ak_heap_rb_find(AkHeapSrchCtx * __restrict srchctx,
                void * __restrict key) {
  AkHeapSrchNode *iter;

  iter = srchctx->root->chld[AK__BST_RIGHT];

  while (iter != srchctx->nullNode) {
    int cmpRet;

    cmpRet = srchctx->cmp(iter->key, key);

    if (cmpRet == 0)
      break;

    iter = iter->chld[cmpRet < 0];
  }

  return iter;
}

void
ak_heap_rb_printNode(AkHeapSrchCtx * __restrict srchctx,
                     AkHeapSrchNode * __restrict srchNode) {
  if(srchNode != srchctx->nullNode) {
    ak_heap_rb_printNode(srchctx,
                         srchNode->chld[AK__BST_LEFT]);
    printf("%s\n", srchNode->key);
    ak_heap_rb_printNode(srchctx,
                         srchNode->chld[AK__BST_RIGHT]);
  }
}

void
ak_heap_rb_print(AkHeapSrchCtx * __restrict srchctx) {
  printf("Asset Kit Memory Id Dump:\n");
  printf("------------------------\n");

  if(srchctx->root->chld[AK__BST_RIGHT] == srchctx->nullNode)
    printf("Empty tree\n");
  else
    ak_heap_rb_printNode(srchctx,
                         srchctx->root->chld[AK__BST_RIGHT]);

  printf("------------------------\n");
}
