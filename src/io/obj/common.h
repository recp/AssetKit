/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef wobj_commoh_h
#define wobj_commoh_h

#include "../../../include/ak/assetkit.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../json.h"

#include <string.h>
#include <stdlib.h>

typedef struct AkWOBJState {
  AkHeap       *heap;
  AkDoc        *doc;
  void         *tmpParent;
} AkWOBJState;

#endif /* wobj_commoh_h */
