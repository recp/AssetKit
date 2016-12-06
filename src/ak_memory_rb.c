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

  if (srchctx->root->chld[AK__BST_RIGHT] == srchctx->nullNode) {
    AK__RB_MKBLACK(srchNode);
    srchctx->root->chld[AK__BST_RIGHT] = srchNode;
    return;
  }

  sG = sP = sX = 1;
  
  P = G = Q = srchctx->root;
  X = P->chld[AK__BST_RIGHT];

  /* Top-Down Insert */
  do {
    /* main case : two children are red */
    if (AK__RB_ISRED(X->chld[AK__BST_LEFT])
        && AK__RB_ISRED(X->chld[AK__BST_RIGHT])) {

      /* case 1: P is black */
      if (!AK__RB_ISRED(P)) {
        /* make X red */
        AK__RB_MKRED(X);

        /* make two children black */
        AK__RB_MKBLACK(X->chld[AK__BST_LEFT]);
        AK__RB_MKBLACK(X->chld[AK__BST_RIGHT]);
      }

      /* P is red */
      else {
        AK__RB_MKRED(G);

        /* case 2: X and P are both left/right children */
        if (sX == sP){
          /* single rotation */

          AK__RB_MKRED(X);
          AK__RB_MKBLACK(P);
          AK__RB_MKBLACK(X->chld[AK__BST_LEFT]);
          AK__RB_MKBLACK(X->chld[AK__BST_RIGHT]);

          Q->chld[sG]  = P;
          G->chld[sP]  = P->chld[!sP];
          P->chld[!sP] = G;

          G = Q;
        }

        /* case 3: X and P are opposide side */
        else {
          AK__RB_MKBLACK(X);
          AK__RB_MKBLACK(X->chld[AK__BST_LEFT]);
          AK__RB_MKBLACK(X->chld[AK__BST_RIGHT]);

          Q->chld[sG]  = X;
          P->chld[sX]  = X->chld[!sX];
          G->chld[sP]  = X->chld[sX];
          X->chld[!sX] = P;
          X->chld[sX]  = G;

          X = P;
          P = Q->chld[sG];
          G = Q;

          sP = sG;
          sX = sP;
        }
      }
    }

    sG = sP;
    sP = sX;
    sX = !(srchctx->cmp(srchNode->key, X->key) < 0);
    Q  = G;
    G  = P;
    P  = X;
    X  = X->chld[sX];
  } while (X != srchctx->nullNode);

  X = P->chld[sX] = srchNode;

  /* make current red */
  AK__RB_MKRED(X);

  /* check for red violation, we know uncle is black */
  if (AK__RB_ISRED(P)) {
    AK__RB_MKRED(G);

    /* double rotation */
    if (sX != sP){
      AK__RB_MKBLACK(X);

      Q->chld[sG]  = X;
      P->chld[sX]  = X->chld[!sX];
      G->chld[sP]  = X->chld[sX];
      X->chld[!sX] = P;
      X->chld[sX]  = G;
    }

    /* single rotation */
    else {
      AK__RB_MKBLACK(P);

      G->chld[sP]  = P->chld[!sP];
      P->chld[!sP] = G;
      Q->chld[sG]  = P;
    }
  }

  /* make root black */
  AK__RB_MKBLACK(srchctx->root->chld[AK__BST_RIGHT]);
}

void
ak_heap_rb_remove(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode) {
  AkHeapSrchNode *X, *P, *T, *G, *toDel, *toDelP;
  int            sP, sX, cmpRet, sDel, sDelP;
  int            c2b, fnd;

  sP     = AK__BST_RIGHT;
  G      = srchctx->root;
  P      = G;
  X      = P->chld[AK__BST_RIGHT];
  c2b    = 0;
  fnd    = 0;
  toDel  = NULL;
  toDelP = NULL;
  sDel   = 0;
  sDelP  = 0;

  /* step 1: examine the root */
  cmpRet = srchctx->cmp(srchNode->key, X->key);
  if (cmpRet == 0) {
    toDelP = P;
    toDel  = X;
    sDel   = sP;
    sDelP  = sP;
    sX     = AK__BST_LEFT;

    G      = P;
    P      = X;
    X      = P->chld[sX];
    T      = P->chld[!sX];
    sX     = AK__BST_RIGHT;
  } else {
    sX  = !(cmpRet < 0);
  }

  if (!AK__RB_ISRED(X->chld[AK__BST_LEFT])
      && !AK__RB_ISRED(X->chld[AK__BST_RIGHT])) {
    AK__RB_MKRED(srchctx->root->chld[AK__BST_RIGHT]);
    G = P;
    P = X;
    X = P->chld[sX];
    T = P->chld[!sX];

    if (X == srchctx->nullNode)
      goto step3;

    goto case2;
  } else {
    /* step 2B */
    c2b = 1;
  }

  /* Top-Down Deletion */
  while (X != srchctx->nullNode) {
    if (cmpRet != 0) {
      cmpRet = srchctx->cmp(srchNode->key, X->key);
      if (cmpRet == 0) {
        toDelP = P;
        toDel  = X;
        sDel   = sX;
        sDelP  = sP;
        sX     = AK__BST_LEFT;

        G      = P;
        P      = X;
        X      = P->chld[sX];
        T      = P->chld[!sX];
        sX     = AK__BST_RIGHT;
      } else {
        sP = sX;
        sX = !(cmpRet < 0);
      }
    }

    G  = P;
    P  = X;
    T  = P->chld[!sX];
    X  = P->chld[sX];

    if (X == srchctx->nullNode)
      break;

    /* case 2b continue: check new X */
    if (c2b) {
      AkHeapSrchNode *R;

      /* if new X is red continue down again */
      if (AK__RB_ISRED(X)) {
        c2b = 0;
        continue;
      }

      /* new X is black T is red */
      R = T->chld[!sX];

      P->chld[!sX] = T->chld[sX];
      T->chld[sX]  = P;
      G->chld[sP]  = T;

      AK__RB_MKRED(P);
      AK__RB_MKBLACK(T);

      G = T;
      T = P->chld[!sX];

      sP  = sX;
      c2b = 0;
    }

    /* case 2: X has two black children */
    if (!AK__RB_ISRED(X->chld[AK__BST_LEFT])
        && !AK__RB_ISRED(X->chld[AK__BST_RIGHT])) case2: {

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
        /* T = P->chld[!sX]; */

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
        /* T = P->chld[!sX]; */

        /* sG = sP; */
        sP = sX;
      }
    } else {
      /* case 2b: X's one child is red, advence to next level */
      c2b = 1;
    }
  } /* while */

step3:
  if (cmpRet == 0) {
    /* P is node which will take toDel place */
    if (toDel->chld[AK__BST_LEFT] != srchctx->nullNode) {
      P->chld[AK__BST_RIGHT] = toDel->chld[AK__BST_RIGHT];

      if (P != toDel->chld[AK__BST_LEFT])
        P->chld[AK__BST_LEFT] = toDel->chld[AK__BST_LEFT];

      G->chld[sP]         = srchctx->nullNode;
      toDelP->chld[sDelP] = P;
    } else {
      toDelP->chld[sDelP] = toDel->chld[AK__BST_RIGHT];
    }
  }

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
    srchctx->print(srchNode->key);
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
