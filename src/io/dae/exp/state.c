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

#include "state.h"
#include "../../common/path.h"

#include <stdlib.h>
#include <string.h>

AK_HIDE
bool
dae_state_init(DAEExpState          * __restrict st,
               AkDoc                * __restrict doc,
               FILE                 * __restrict file,
               const char           * __restrict filepath,
               AkDaeExportIndexMode              indexMode,
               AkDaeExportVersion                versionMode) {
  memset(st, 0, sizeof(*st));
  st->doc          = doc;
  st->outDir       = io_path_output_dir_dup(filepath);
  st->w.file       = file;
  st->w.result     = AK_OK;
  st->indexMode    = indexMode == AK_DAE_EXPORT_INDEX_AUTO
                     ? AK_DAE_EXPORT_INDEX_MULTI : indexMode;
  st->versionMode           = versionMode;
  st->prepareOk             = true;
  st->geometries            = rb_newtree_ptr();
  st->materials             = rb_newtree_ptr();
  st->images                = rb_newtree_ptr();
  st->cameras               = rb_newtree_ptr();
  st->lights                = rb_newtree_ptr();
  st->nodes                 = rb_newtree_ptr();
  st->visualNodes           = rb_newtree_ptr();
  st->sceneNodes            = rb_newtree_ptr();
  st->nodeIds               = rb_newtree_str();
  st->nodeTransforms        = rb_newtree_ptr();
  st->skins                 = rb_newtree_ptr();
  st->skinGeometries        = rb_newtree_ptr();
  st->skinInstances         = rb_newtree_ptr();
  st->skinMorphs            = rb_newtree_ptr();
  st->morphs                = rb_newtree_ptr();
  st->morphGeometries       = rb_newtree_ptr();
  st->morphVertexGeometries = rb_newtree_ptr();
  st->morphInstances        = rb_newtree_ptr();
  st->animations            = rb_newtree_ptr();

  return st->outDir
         && st->geometries
         && st->materials
         && st->images
         && st->cameras
         && st->lights
         && st->nodes
         && st->visualNodes
         && st->sceneNodes
         && st->nodeIds
         && st->nodeTransforms
         && st->skins
         && st->skinGeometries
         && st->skinInstances
         && st->skinMorphs
         && st->morphs
         && st->morphGeometries
         && st->morphVertexGeometries
         && st->morphInstances
         && st->animations;
}

static
void
dae_object_ref_list_free(DAEExpObjectRef *ref) {
  while (ref) {
    DAEExpObjectRef *next;

    next = ref->next;
    free(ref);
    ref = next;
  }
}

AK_HIDE
void*
dae_scratch(DAEExpState * __restrict st, size_t size) {
  void *mem;

  if (!st || size == 0)
    return NULL;

  if (size <= st->scratchSize)
    return st->scratch;

  mem = realloc(st->scratch, size);
  if (!mem)
    return NULL;

  st->scratch     = mem;
  st->scratchSize = size;

  return mem;
}

AK_HIDE
void
dae_state_destroy(DAEExpState * __restrict st) {
  DAEExpGeometryRef *geomRef;
  DAEExpMorphRef    *morphRef;
  uint32_t i;

  geomRef = st ? st->extraGeometries : NULL;
  while (geomRef) {
    DAEExpGeometryRef *next;

    next = geomRef->next;
    free(geomRef);
    geomRef = next;
  }

  if (st) {
    dae_object_ref_list_free(st->extraMaterials);
    dae_object_ref_list_free(st->extraImages);
    dae_object_ref_list_free(st->extraCameras);
    dae_object_ref_list_free(st->extraLights);
  }

  morphRef = st ? st->extraMorphs : NULL;
  while (morphRef) {
    DAEExpMorphRef *next;

    next = morphRef->next;
    free(morphRef);
    morphRef = next;
  }

  if (st->imageExportUris) {
    for (i = 0; i < st->imageCount; i++)
      free(st->imageExportUris[i]);
    free(st->imageExportUris);
  }

  if (st->geometries)            rb_destroy(st->geometries);
  if (st->materials)             rb_destroy(st->materials);
  if (st->images)                rb_destroy(st->images);
  if (st->cameras)               rb_destroy(st->cameras);
  if (st->lights)                rb_destroy(st->lights);
  if (st->nodes)                 rb_destroy(st->nodes);
  if (st->visualNodes)           rb_destroy(st->visualNodes);
  if (st->sceneNodes)            rb_destroy(st->sceneNodes);
  if (st->nodeIds)               rb_destroy(st->nodeIds);
  if (st->nodeTransforms)        rb_destroy(st->nodeTransforms);
  if (st->skins)                 rb_destroy(st->skins);
  if (st->skinGeometries)        rb_destroy(st->skinGeometries);
  if (st->skinInstances)         rb_destroy(st->skinInstances);
  if (st->skinMorphs)            rb_destroy(st->skinMorphs);
  if (st->morphs)                rb_destroy(st->morphs);
  if (st->morphGeometries)       rb_destroy(st->morphGeometries);
  if (st->morphVertexGeometries) rb_destroy(st->morphVertexGeometries);
  if (st->morphInstances)        rb_destroy(st->morphInstances);
  if (st->animations)            rb_destroy(st->animations);

  free(st->scratch);
  free(st->outDir);
}
