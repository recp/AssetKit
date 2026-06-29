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

#ifndef assetkit_dae_exp_common_h
#define assetkit_dae_exp_common_h

#include "dae.h"
#include "writer.h"

#include "../../../../include/ak/controller.h"
#include "../../../../include/ak/instance.h"
#include "../../../../include/ak/options.h"
#include "../../../../include/ak/transform.h"

#include <ds/rb.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define DAE_EXP_RAD_TO_DEG 57.2957795130823208768f

typedef struct DAEExpGeometryRef {
  struct DAEExpGeometryRef *next;
  AkGeometry               *geom;
} DAEExpGeometryRef;

typedef struct DAEExpObjectRef {
  struct DAEExpObjectRef *next;
  void                  *object;
} DAEExpObjectRef;

typedef struct DAEExpMorphRef {
  struct DAEExpMorphRef *next;
  AkMorph               *morph;
} DAEExpMorphRef;

typedef struct DAEExpState {
  AkDoc        *doc;
  char         *outDir;
  DAEExpWriter w;
  RBTree       *geometries;
  RBTree       *materials;
  RBTree       *images;
  RBTree       *cameras;
  RBTree       *lights;
  RBTree       *nodes;
  RBTree       *visualNodes;
  RBTree       *sceneNodes;
  RBTree       *nodeIds;
  RBTree       *nodeTransforms;
  RBTree       *skins;
  RBTree       *skinGeometries;
  RBTree       *skinInstances;
  RBTree       *skinMorphs;
  RBTree       *morphs;
  RBTree       *morphGeometries;
  RBTree       *morphVertexGeometries;
  RBTree       *morphInstances;
  DAEExpGeometryRef *extraGeometries;
  DAEExpGeometryRef *lastExtraGeometry;
  DAEExpObjectRef *extraMaterials;
  DAEExpObjectRef *lastExtraMaterial;
  DAEExpObjectRef *extraImages;
  DAEExpObjectRef *lastExtraImage;
  DAEExpObjectRef *extraCameras;
  DAEExpObjectRef *lastExtraCamera;
  DAEExpObjectRef *extraLights;
  DAEExpObjectRef *lastExtraLight;
  DAEExpMorphRef *extraMorphs;
  DAEExpMorphRef *lastExtraMorph;
  char        **imageExportUris;
  void         *scratch;
  size_t        scratchSize;
  uint32_t      geometryCount;
  uint32_t      materialCount;
  uint32_t      imageCount;
  uint32_t      cameraCount;
  uint32_t      lightCount;
  uint32_t      nodeCount;
  uint32_t      visualNodeCount;
  uint32_t      skinCount;
  uint32_t      morphCount;
  AkDaeExportIndexMode indexMode;
  AkDaeExportVersion versionMode;
  bool          useCollada150;
  bool          prepareOk;
  bool          imageRefsOnly;
} DAEExpState;

#define DAE_EXP_DUPLICATE_NODE_ID ((void *)(uintptr_t)1u)

AK_HIDE
AkGeometry*
dae_instance_geometry_object(AkInstanceGeometry * __restrict inst);

AK_HIDE
AkCamera*
dae_instance_camera_object(AkInstanceBase * __restrict inst);

AK_HIDE
AkLight*
dae_instance_light_object(AkInstanceBase * __restrict inst);

AK_HIDE
uint32_t
dae_map_index(RBTree * __restrict map, void * __restrict key);

AK_HIDE
bool
dae_w_node_id_ref(DAEExpState * __restrict st,
                  AkNode      * __restrict node);

AK_HIDE
AkNode*
dae_node_for_transform(DAEExpState * __restrict st,
                       AkObject    * __restrict transform);

AK_HIDE
const char*
dae_transform_sid(AkObject * __restrict transform);

AK_HIDE
void
dae_w_transform_sid(DAEExpWriter * __restrict w,
                    AkObject     * __restrict transform);

AK_HIDE
void
dae_w_node_id(DAEExpWriter * __restrict w, uint32_t nodeIdx);

AK_HIDE
void
dae_w_vnode_id(DAEExpWriter * __restrict w, uint32_t vnodeIdx);

AK_HIDE
bool
dae_prepare_extra_object(RBTree             * __restrict map,
                         uint32_t           * __restrict count,
                         DAEExpObjectRef   ** __restrict first,
                         DAEExpObjectRef   ** __restrict last,
                         void               * __restrict object);

AK_HIDE
void*
dae_scratch(DAEExpState * __restrict st, size_t size);

AK_HIDE
void
dae_w_geom_prim_id(DAEExpWriter * __restrict w,
                   uint32_t                  geomIdx,
                   uint32_t                  primIdx,
                   DAEExpName                suffix);

AK_HIDE
void
dae_w_geom_id(DAEExpWriter * __restrict w, uint32_t geomIdx);

AK_HIDE
void
dae_w_prim_material_symbol(DAEExpWriter * __restrict w, uint32_t primIdx);

AK_HIDE
void
dae_write_float_elem(DAEExpWriter * __restrict w,
                     DAEExpName                tag,
                     float                     val);

AK_HIDE
void
dae_w_matrix4x4_dae(DAEExpWriter * __restrict w,
                    const AkFloat             matrix[4][4]);

AK_HIDE
void
dae_quat_axis_angle_deg(AkQuaternion * __restrict quat,
                        float                      axis[3],
                        float        * __restrict angleDeg);

AK_HIDE
void
dae_w_identity4x4(DAEExpWriter * __restrict w);

#endif /* assetkit_dae_exp_common_h */
