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

#ifndef assetkit_render_h
#define assetkit_render_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkScene;
struct AkNode;
struct AkGeometry;
struct AkMeshPrimitive;
struct AkInstanceGeometry;
struct AkMaterial;

/*
 * One tightly-packed float attribute owned by an AkSceneRenderData.
 * semantic is an AkInputSemantic value and primitiveType below is an
 * AkMeshPrimitiveType value. They are stored as uint32_t so this header can
 * also be included independently of assetkit.h.
 */
typedef struct AkRenderBatchAttribute {
  float    *values;
  uint32_t  valueCount;
  uint32_t  semantic;
  uint32_t  set;
  uint32_t  componentCount;
} AkRenderBatchAttribute;

/*
 * Maps a contiguous primitive range in a batch back to the source scene.
 * Consumers can binary-search firstPrimitive/primitiveCount after a render
 * hit without expanding the source hierarchy into separate draw objects.
 */
typedef struct AkRenderBatchRange {
  struct AkNode             *node;
  struct AkGeometry         *geometry;
  struct AkMeshPrimitive    *primitive;
  struct AkInstanceGeometry *instance;
  uint32_t                   firstPrimitive;
  uint32_t                   primitiveCount;
  uint32_t                   firstIndex;
  uint32_t                   indexCount;
} AkRenderBatchRange;

/*
 * A static render batch has one material and one canonical vertex layout.
 * source* fields identify a representative source item for consumers that
 * need to resolve texture-coordinate bindings while creating the material.
 */
typedef struct AkRenderBatch {
  AkRenderBatchAttribute     *attributes;
  uint32_t                   *indices;
  AkRenderBatchRange         *ranges;
  struct AkMaterial         *material;
  struct AkGeometry         *sourceGeometry;
  struct AkMeshPrimitive    *sourcePrimitive;
  struct AkInstanceGeometry *sourceInstance;
  uint32_t                    attributeCount;
  uint32_t                    vertexCount;
  uint32_t                    indexCount;
  uint32_t                    primitiveCount;
  uint32_t                    rangeCount;
  uint32_t                    primitiveType;
} AkRenderBatch;

/*
 * One reference to another render group. matrix is column-major and maps the
 * target group's local batch coordinates into this group.
 */
typedef struct AkRenderGroupInstance {
  struct AkNode *owner;
  float          matrix[16];
  uint32_t       targetGroupIndex;
} AkRenderGroupInstance;

/*
 * One compact source-node prototype. Direct child geometry is statically
 * batched; instance_node targets remain shared group references rather than
 * being expanded into duplicated vertex buffers.
 */
typedef struct AkRenderGroup {
  AkRenderBatch         *batches;
  AkRenderGroupInstance *instances;
  struct AkNode         *sourceRoot;
  uint32_t               batchCount;
  uint32_t               instanceCount;
} AkRenderGroup;

typedef struct AkSceneRenderData {
  AkRenderGroup *groups;
  uint32_t       groupCount;
  uint32_t       rootGroupIndex;
  uint32_t       includedPrimitiveCount;
  uint32_t       skippedPrimitiveCount;
} AkSceneRenderData;

/*
 * Build an optional, immutable static rendering view of scene.
 *
 * The source AkScene and AkDoc are not changed. POSITION values are baked to
 * their compact group's coordinates; NORMAL and TANGENT values use the
 * inverse-transpose normal matrix. instance_node DAG edges remain references
 * to shared groups. Only fully supported static primitives are included.
 *
 * A consumer that requires an all-or-nothing fast path should use the result
 * only when skippedPrimitiveCount == 0.
 */
AK_EXPORT
AkResult
ak_sceneBuildRenderData(struct AkScene    * __restrict scene,
                        AkSceneRenderData ** __restrict renderData);

AK_EXPORT
void
ak_sceneRenderDataFree(AkSceneRenderData *renderData);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_render_h */
