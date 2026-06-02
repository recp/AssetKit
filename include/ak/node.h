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

#ifndef assetkit_node_h
#define assetkit_node_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

struct AkInstanceMorph;
struct AkAccessor;

typedef enum AkNodeFlags {
  AK_NODEF_FIXED_COORD = 1
} AkNodeFlags;

/*!
 * @brief Per-instance TRS accessors for EXT_mesh_gpu_instancing.
 */
typedef struct AkGpuInstancing {
  struct AkAccessor *translation;  /* optional, vec3 x count */
  struct AkAccessor *rotation;     /* optional, vec4 x count (quaternion) */
  struct AkAccessor *scale;        /* optional, vec3 x count */
  uint32_t           count;        /* number of instances */
} AkGpuInstancing;

typedef enum AkNodeType {
  AK_NODE_TYPE_NODE        = 1,
  AK_NODE_TYPE_CAMERA_NODE = 2,
  AK_NODE_TYPE_JOINT       = 3
} AkNodeType;

typedef struct AkListIter {
  void *prev;
  void *next;
} AkListIter;

typedef struct AkTreeWithParentIter {
  void *prev;
  void *next;
  void *parent;
  void *chld;
} AkTreeWithParentIter;

typedef struct AkTreeIter {
  void *prev;
  void *next;
  void *chld;
} AkTreeIter;

typedef struct AkNode {
  /* const char * id;  */
  /* const char * sid; */

  const char           *name;
  AkNodeFlags           flags;
  AkNodeType            nodeType;
  AkStringArray        *layer;
  struct AkTransform   *transform;
  bool                  visible;

  /* only avilable if library is forced to calculate them
     check these two matrix to avoid extra or same calculation
   */
  struct AkMatrix      *matrix;
  struct AkMatrix      *matrixWorld;
  struct AkBoundingBox *bbox;
  
  AkInstanceGeometry   *geometry;
  AkInstanceBase       *camera;
  AkInstanceBase       *light;
  AkNodeRef           *nodeRefs;

  /* EXT_mesh_gpu_instancing, NULL if not authored. */
  AkGpuInstancing      *gpuInstancing;

  AkTree               *extra;

  struct AkNode        *prev;
  struct AkNode        *next;
  struct AkNode        *docNext;
  struct AkNode        *chld;
  struct AkNode        *parent;
} AkNode;

AK_EXPORT
void
ak_addSubNode(AkNode * __restrict parent,
              AkNode * __restrict subnode,
              bool                fixCoordSys);

/*!
 * @brief Allocate a fresh AkNode in the document's heap.
 *
 * Optionally attaches the new node as a child of @p parent (via
 * ak_addSubNode without coord-sys fix-up — the caller is in control
 * of orientation). The node's name is duplicated into the heap if
 * @p name is non-NULL.
 *
 * @param[in]  doc     document the node will live in (required)
 * @param[in]  parent  parent node to attach as child of, or NULL
 *                     to leave the new node unparented
 * @param[in]  name    optional node name (deep-copied)
 *
 * @return Newly allocated AkNode, or NULL on allocation failure.
 */
AK_EXPORT
AkNode *
ak_nodeMake(AkDoc      * __restrict doc,
            AkNode     * __restrict parent,
            const char * __restrict name);

/*!
 * @brief Find a direct child of @p parent whose name matches @p name.
 *
 * Linear scan over the immediate children chain. NULL parent or NULL
 * name returns NULL. Returns the first match — names aren't unique
 * by spec, so callers that care should walk the result chain
 * themselves.
 */
AK_EXPORT
AkNode *
ak_nodeFindChildByName(AkNode     * __restrict parent,
                       const char * __restrict name);

/*!
 * @brief Find a child by name, creating it under @p parent if missing.
 *
 * Convenience wrapper for the common "ensure a designated container
 * node exists" pattern (e.g. a "User Cameras" group). Equivalent to
 * ak_nodeFindChildByName followed by ak_nodeMake when nothing matches.
 */
AK_EXPORT
AkNode *
ak_nodeFindOrMakeChild(AkDoc      * __restrict doc,
                       AkNode     * __restrict parent,
                       const char * __restrict name);

/*!
 * @brief Attach a camera to a node by creating a camera instance.
 *
 * Allocates an AkInstanceBase, sets type = AK_INSTANCE_CAMERA, and
 * chains it onto node->camera (preserving any existing camera
 * instance — multi-camera nodes are uncommon but legal).
 *
 * Does NOT register the camera in the cameras library — that's the
 * job of ak_camMakePerspective / ak_camMakeOrthographic. Pair them.
 *
 * @return The freshly allocated camera instance.
 */
AK_EXPORT
AkInstanceBase *
ak_nodeAttachCamera(AkNode   * __restrict node,
                    AkCamera * __restrict cam);

/*!
 * @brief Attach a light to a node by creating a light instance.
 *
 * Mirrors ak_nodeAttachCamera: allocates an AkInstanceBase with
 * type = AK_INSTANCE_LIGHT and chains it onto node->light. Pair
 * with ak_lightMake() which handles the lights library entry.
 *
 * @return The freshly allocated light instance.
 */
AK_EXPORT
AkInstanceBase *
ak_nodeAttachLight(AkNode  * __restrict node,
                   AkLight * __restrict light);

/*!
 * @brief Replace the node's transform with a single column-major 4×4
 *        matrix (AKT_MATRIX item).
 *
 * Allocates an `AkTransform` for the node if it didn't already have
 * one, then drops a fresh AKT_MATRIX-typed AkObject in its `item`
 * slot containing the supplied matrix. Any prior transform chain
 * (translate / rotate / scale items) is replaced — convenient for
 * runtime UI edits where the user works with a decomposed pose and
 * we serialize the composed result.
 *
 * @param[in]  node    target node (required)
 * @param[in]  matrix  16 floats in column-major order
 *                     (matches cglm / OpenGL / SCNMatrix4 layout)
 */
AK_EXPORT
void
ak_nodeSetTransformMatrix(AkNode * __restrict node,
                          const float         matrix[16]);

/*!
 * @brief Find a root-level node in a scene by name.
 *
 * Scenes use `scene->node` as a synthetic entrypoint. Its `nodeRefs` list
 * references authored root nodes and may be NULL for an empty scene. NULL
 * inputs return NULL.
 * NULL inputs return NULL.
 */
AK_EXPORT
AkNode *
ak_sceneFindRoot(struct AkScene * __restrict scene,
                 const char     * __restrict name);

/*!
 * @brief Find a root-level node by name, or create one in @p scene.
 *
 * Convenience for "ensure a top-level container exists" — e.g. a
 * "User Cameras" group placed alongside the asset's authored roots.
 * Created roots are document-library nodes attached to the scene through the
 * synthetic root's `nodeRefs` list.
 */
AK_EXPORT
AkNode *
ak_sceneFindOrMakeRoot(AkDoc          * __restrict doc,
                       struct AkScene * __restrict scene,
                       const char     * __restrict name);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_node_h */
