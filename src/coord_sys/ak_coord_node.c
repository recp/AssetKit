/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"
#include "../ak_memory_common.h"
#include "../../include/ak-coord-util.h"
#include "ak_coord_common.h"
#include <cglm.h>

bool
_assetkit_hide
ak_fixInstanceTransform(AkNode         *node,
                        AkInstanceBase *inst,
                        AkCoordSys     *instCoordSys,
                        AkCoordSys     *coordSys);

bool
_assetkit_hide
ak_fixInstanceTransform(AkNode         *node,
                        AkInstanceBase *inst,
                        AkCoordSys     *instCoordSys,
                        AkCoordSys     *coordSys) {
  AkHeap *heap;
  AkNode *subNode;

  heap    = ak_heap_getheap(inst);
  subNode = ak_instanceMoveToSubNodeIfNeeded(node, inst);

  if (!subNode)
    return false;

  if (!subNode->transform)
    subNode->transform = ak_heap_calloc(heap,
                                        subNode,
                                        sizeof(*subNode->transform));

  /* fixup transform[s] */
  ak_coordFindTransform(subNode->transform,
                        instCoordSys,
                        coordSys);

  return true;
}

AK_EXPORT
void
ak_fixNodeCoordSys(AkNode * __restrict node) {
  AkHeap         *heap;
  void           *parentObject;
  AkCoordSys     *coordSys, *parentCoordSys, *instCoordSys, *coordSysToFix;
  AkInstanceBase *inst;
  AkInstanceBase *instArray[] = {(AkInstanceBase *)node->geometry,
                                 (AkInstanceBase *)node->node,
                                 (AkInstanceBase *)node->controller,
                                 node->camera,
                                 node->light};
  int             i, instArrayLen, instTransCount;
  bool            hasCoordSys;

  hasCoordSys    = ak_hasCoordSys(node);
  coordSys       = ak_getCoordSys(node);
  instArrayLen   = AK_ARRAY_LEN(instArray);
  parentCoordSys = NULL;
  coordSysToFix  = NULL;
  instTransCount = 0;

  /* fix instance transforms */
  for (i = 0; i < instArrayLen; i++) {
    inst = instArray[i];
    while (inst) {
      void *instObj;
      instObj = ak_instanceObject(inst);
      if (instObj && ak_hasCoordSys(instObj)) {
        instCoordSys = ak_getCoordSys(instObj);
        if (instCoordSys != coordSys) {
          bool movedToSub;
          movedToSub = ak_fixInstanceTransform(node,
                                               inst,
                                               instCoordSys,
                                               coordSys);
          if (!movedToSub) {
            coordSysToFix = instCoordSys;
            instTransCount++;
          }
        }
      }
      inst = inst->next;
    }
  }

  heap = ak_heap_getheap(node);
  switch (ak_opt_get(AK_OPT_COORD_CONVERT_TYPE)) {
    case AK_COORD_CVT_FIX_TRANSFORM: {
      if (instTransCount == 1 && coordSysToFix)
        coordSys = coordSysToFix;

      parentObject = node->parent;
      if (!parentObject)
        parentObject = ak_mem_parent(node);

      if (parentObject)
        parentCoordSys = ak_getCoordSys(parentObject);
      else
        parentCoordSys = AK_YUP;

      if (parentCoordSys == coordSys)
        return;

      if (hasCoordSys
          /* don't change nodes in library_nodes */
          || (!node->parent && ak_typeid(parentObject) == AKT_SCENE)) {

        if (!node->transform)
          node->transform = ak_heap_calloc(heap,
                                           node,
                                           sizeof(*node->transform));

        /* fixup transform[s] */
        ak_coordFindTransform(node->transform,
                              coordSys,
                              parentCoordSys);
      }
      break;
    }
    case AK_COORD_CVT_ALL:
      if (node->transform)
        ak_coordCvtNodeTransforms(ak_heap_data(heap), node);
      break;
    default:
      break;
  }
}
