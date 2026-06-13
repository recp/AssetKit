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
 */

#include "postscript.h"
#include "mesh_fixup.h"
#include "ak/coord-util.h"

AK_HIDE
void
gltf_setskinners(RBTree *tree, RBNode *rbnode);

AK_HIDE
void
gltf_postscript(AkGLTFState * __restrict gst) {
  AkCoordCvtType coordCvtType;
  AkCoordSys    *sourceCoordSys, *targetCoordSys;
  AkScene       *vscn;
  bool           fixTransform;

  coordCvtType   = (AkCoordCvtType)ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  sourceCoordSys = gst->doc ? gst->doc->coordSys : NULL;
  targetCoordSys = (void *)ak_opt_get(AK_OPT_COORD);
  fixTransform   = coordCvtType == AK_COORD_CVT_FIX_TRANSFORM
                   && sourceCoordSys
                   && targetCoordSys
                   && sourceCoordSys != targetCoordSys
                   && !ak_coordOrientationIsEq(sourceCoordSys, targetCoordSys);

  gltf_mesh_fixup(gst);

  if (coordCvtType != AK_COORD_CVT_DISABLED)
    gst->doc->coordSys = targetCoordSys;

  if (gst->doc && gst->doc->lib.scenes.first) {
    for (vscn = gst->doc->lib.scenes.first;
         vscn;
         vscn = vscn->next) {
      if (fixTransform)
        ak_fixSceneCoordSys(vscn);
    }
  }

  rb_walk(gst->skinBound, gltf_setskinners);
}

AK_HIDE
void
gltf_setskinners(RBTree *tree, RBNode *rbnode) {
  char                skinid[16];
  AkGLTFState        *gst;
  AkInstanceSkin     *skinner;
  AkNode             *node;
  AkInstanceGeometry *instGeom;
  int32_t             i32val;

  gst               = tree->userData;
  node              = rbnode->key;
  instGeom          = node->geometry;
  i32val            = (int32_t)(intptr_t)rbnode->val;
  
  gltf_imp_skin_id(skinid, i32val);
  
  skinner           = ak_heap_calloc(gst->heap, node, sizeof(*skinner));
  skinner->skin     = ak_getObjectById(gst->doc, skinid);
  instGeom->skinner = skinner;
}
