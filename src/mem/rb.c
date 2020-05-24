/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * ---
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

#include "rb.h"
#include <stdio.h>

void
ak_heap_rb_printNode(AkHeapSrchCtx * __restrict srchctx,
                     AkHeapSrchNode * __restrict srchNode);

/*
 simple assertion for rbtree
 source: Eternally Confuzzled
 */
int
ak_heap_rb_assert(AkHeapSrchCtx * srchctx,
                  AkHeapSrchNode * root) {
  int lh, rh;
  if (root == srchctx->nullNode) {
    return 1;
  } else {
    struct AkHeapSrchNode *ln = root->chld[0];
    struct AkHeapSrchNode *rn = root->chld[1];

    /* Consecutive red links */
    if (AK__RB_ISRED(root)) {
      if (AK__RB_ISRED(ln) || AK__RB_ISRED(rn)) {
        puts("Red violation");
        exit(0);
        return 0;
      }
    }

    lh = ak_heap_rb_assert(srchctx, ln);
    rh = ak_heap_rb_assert(srchctx, rn);

    /* Invalid binary search tree */
    if ((ln != srchctx->nullNode
         && srchctx->cmp(ln->key, root->key) > 0)
        || (rn != srchctx->nullNode
            && srchctx->cmp(rn->key, root->key) < 0)) {
          puts("Binary tree violation");
          exit(0);
          return 0;
        }

    /* Black height mismatch */
    if (lh != 0 && rh != 0 && lh != rh) {
      puts("Black violation");
      exit(0);
      return 0;
    }

    /* Only count black links */
    if (lh != 0 && rh != 0)
      return AK__RB_ISRED(root) ? lh : lh + 1;

    return 0;
  }
}

void
ak_heap_rb_insert(AkHeapSrchCtx * __restrict srchctx,
                  AkHeapSrchNode * __restrict srchNode) {
  AkHeapSrchNode *X, *P, *G, *Q, *W;
  int sQ, sG, sP, sX;

  if (srchctx->root->chld[AK__BST_RIGHT] == srchctx->nullNode) {
    AK__RB_MKBLACK(srchNode);
    srchctx->root->chld[AK__BST_RIGHT] = srchNode;
    return;
  }

  sQ = sG = sP = sX = 1;

  W = P = G = Q = srchctx->root;
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
          Q = W;
        }

        /* case 3: X and P are opposide side */
        else {
          AK__RB_MKBLACK(X);
          AK__RB_MKBLACK(X->chld[AK__BST_LEFT]);
          AK__RB_MKBLACK(X->chld[AK__BST_RIGHT]);

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
    sX = !(srchctx->cmp(srchNode->key, X->key) < 0);
    W  = Q;
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
  int            sP, sX, cmpRet, sDel;
  int            c2b;

  if (srchNode == srchctx->nullNode)
    return;

  sX     = AK__BST_RIGHT;
  G      = srchctx->root;
  P      = G;
  X      = P->chld[AK__BST_RIGHT];
  toDel  = srchctx->nullNode;
  toDelP = srchctx->nullNode;
  sDel   = 0;

  /* step 1: examine the root */
  if (AK__RB_ISBLACK(X->chld[AK__BST_LEFT])
      && AK__RB_ISBLACK(X->chld[AK__BST_RIGHT])) {
    AK__RB_MKRED(X);
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
      if (AK__RB_ISRED(X))
        goto l0;

      G->chld[sP]  = T;
      P->chld[!sX] = T->chld[sX];
      T->chld[sX]  = P;

      AK__RB_MKRED(P);
      AK__RB_MKBLACK(T);

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
    if (AK__RB_ISBLACK(X->chld[AK__BST_LEFT])
        && AK__RB_ISBLACK(X->chld[AK__BST_RIGHT])) {

      /* case 1.a: T has two black children */
      if (T != srchctx->nullNode
          && AK__RB_ISBLACK(T->chld[AK__BST_LEFT])
          && AK__RB_ISBLACK(T->chld[AK__BST_RIGHT])) {

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

        if (toDelP == G) {
          toDelP = R;
          sDel   = sX;
        }
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
    if (toDel != srchctx->nullNode) {
      sX = toDel->chld[AK__BST_RIGHT] == srchctx->nullNode;
    } else {
      cmpRet = srchctx->cmp(srchNode->key, X->key);

      if (cmpRet != 0) {
        sX = !(cmpRet < 0);
      } else {
        toDelP = P;
        toDel  = X;
        sDel   = sP;
        sX     = toDel->chld[AK__BST_RIGHT] != srchctx->nullNode;
      }
    }

    G  = P;
    P  = X;
    X  = P->chld[sX];
    T  = P->chld[!sX];
  } while (X != srchctx->nullNode);

  /* make root black */
  AK__RB_MKBLACK(srchctx->root->chld[AK__BST_RIGHT]);

  if (toDel == srchctx->nullNode)
    return;

  /* toDel has least one child */
  if (P != toDel) {
    /* P is black, save black height */
    if (c2b)
      AK__RB_MKBLACK(P->chld[!sX]);

    /* change color */
    if (AK__RB_ISRED(toDel))
      AK__RB_MKRED(P);
    else
      AK__RB_MKBLACK(P);

    /* replace P with its left child */
    G->chld[sP] = P->chld[!sX];

    /* replace X with in-order predecessor */
    P->chld[AK__BST_RIGHT] = toDel->chld[AK__BST_RIGHT];
    P->chld[AK__BST_LEFT]  = toDel->chld[AK__BST_LEFT];
    toDelP->chld[sDel]     = P;
  }

  /* P is toDel; there is no child */
  else {
    G->chld[sP] = srchctx->nullNode;
  }
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

int
ak_heap_rb_parent(AkHeapSrchCtx * __restrict srchctx,
                  void * __restrict key,
                  AkHeapSrchNode ** dest) {
   AkHeapSrchNode *iter, *parent;
   int side, cmpRet;

   side   = AK__BST_RIGHT;
   iter   = srchctx->root->chld[side];
   parent = srchctx->root;
   cmpRet = -1;

   while (iter != srchctx->nullNode) {
      cmpRet = srchctx->cmp(iter->key, key);

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
  printf("\nAssetKit Memory Id Dump:\n");
  printf("------------------------\n");

  if(srchctx->root->chld[AK__BST_RIGHT] == srchctx->nullNode)
    printf("Empty tree\n");
  else
    ak_heap_rb_printNode(srchctx,
                         srchctx->root->chld[AK__BST_RIGHT]);

  printf("------------------------\n");
}
