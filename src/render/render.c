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

#include <ak/assetkit.h>
#include <ak/render.h>

#include <cglm/cglm.h>

#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AK_RENDER_MAX_ATTRIBUTES 16u
#define AK_RENDER_MAX_DEPTH      4096u

typedef struct AkRenderJobAttribute {
  AkInput  *input;
  uint32_t  semantic;
  uint32_t  set;
  uint32_t  componentCount;
} AkRenderJobAttribute;

typedef struct AkRenderJob {
  AkNode                 *node;
  AkGeometry             *geometry;
  AkMeshPrimitive        *primitive;
  AkInstanceGeometry     *instance;
  AkMaterial             *material;
  AkRenderJobAttribute    attributes[AK_RENDER_MAX_ATTRIBUTES];
  mat4                    world;
  uintptr_t               materialContext;
  uint32_t                attributeCount;
  uint32_t                vertexCount;
  uint32_t                indexCount;
  uint32_t                primitiveCount;
  uint32_t                primitiveType;
  float                   worldDeterminant;
  bool                    reverseWinding;
  bool                    worldIdentity;
  bool                    worldTranslationOnly;
  bool                    hasDirectionAttribute;
} AkRenderJob;

typedef struct AkRenderGroupBuilder {
  AkRenderJob           *jobs;
  AkRenderGroupInstance *instances;
  AkNode                *root;
  size_t                 jobCount;
  size_t                 jobCapacity;
  size_t                 instanceCount;
  size_t                 instanceCapacity;
  uint8_t                status;
} AkRenderGroupBuilder;

typedef struct AkRenderGroupMapEntry {
  AkNode  *root;
  uint32_t indexPlusOne;
} AkRenderGroupMapEntry;

typedef struct AkRenderBuildState {
  AkRenderGroupBuilder **groups;
  AkRenderGroupMapEntry *groupMap;
  size_t                 groupCount;
  size_t                 groupCapacity;
  size_t                 groupMapCapacity;
  size_t                 groupMapCount;
  uint32_t               skippedPrimitiveCount;
  AkResult               result;
} AkRenderBuildState;

static
bool
ak_render_mul_overflow(size_t a, size_t b, size_t *out) {
  if (a != 0 && b > SIZE_MAX / a)
    return true;
  *out = a * b;
  return false;
}

static
bool
ak_render_add_overflow_u32(uint32_t a, uint32_t b, uint32_t *out) {
  if (b > UINT32_MAX - a)
    return true;
  *out = a + b;
  return false;
}

static
bool
ak_render_mat4_is_translation(mat4 matrix) {
  return matrix[0][0] == 1.0f
         && matrix[0][1] == 0.0f
         && matrix[0][2] == 0.0f
         && matrix[0][3] == 0.0f
         && matrix[1][0] == 0.0f
         && matrix[1][1] == 1.0f
         && matrix[1][2] == 0.0f
         && matrix[1][3] == 0.0f
         && matrix[2][0] == 0.0f
         && matrix[2][1] == 0.0f
         && matrix[2][2] == 1.0f
         && matrix[2][3] == 0.0f
         && matrix[3][3] == 1.0f;
}

static
bool
ak_render_reserve(void **items,
                  size_t itemSize,
                  size_t *capacity,
                  size_t needed) {
  size_t newCapacity, bytes;
  void  *newItems;

  if (needed <= *capacity)
    return true;

  newCapacity = *capacity ? *capacity : 256u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }
  if (ak_render_mul_overflow(newCapacity, itemSize, &bytes))
    return false;

  newItems = realloc(*items, bytes);
  if (!newItems)
    return false;

  *items    = newItems;
  *capacity = newCapacity;
  return true;
}

static
size_t
ak_render_node_hash(const AkNode *node) {
  uintptr_t value;

  value  = (uintptr_t)node >> 4;
  value ^= value >> 33;
  value *= UINT64_C(0xff51afd7ed558ccd);
  value ^= value >> 33;
  return (size_t)value;
}

static
bool
ak_render_group_map_rehash(AkRenderBuildState * __restrict state,
                           size_t                           capacity) {
  AkRenderGroupMapEntry *oldMap, *newMap;
  size_t                 oldCapacity, i;

  newMap = calloc(capacity, sizeof(*newMap));
  if (!newMap)
    return false;

  oldMap      = state->groupMap;
  oldCapacity = state->groupMapCapacity;
  state->groupMap         = newMap;
  state->groupMapCapacity = capacity;
  state->groupMapCount    = 0;

  for (i = 0; i < oldCapacity; i++) {
    size_t slot;

    if (!oldMap[i].root)
      continue;
    slot = ak_render_node_hash(oldMap[i].root) & (capacity - 1u);
    while (newMap[slot].root)
      slot = (slot + 1u) & (capacity - 1u);
    newMap[slot] = oldMap[i];
    state->groupMapCount++;
  }
  free(oldMap);
  return true;
}

static
bool
ak_render_group_index(AkRenderBuildState * __restrict state,
                      AkNode             * __restrict root,
                      uint32_t           * __restrict index,
                      bool               * __restrict created) {
  size_t slot;

  if (!state->groupMapCapacity
      && !ak_render_group_map_rehash(state, 256u))
    return false;
  if ((state->groupMapCount + 1u) * 10u
      >= state->groupMapCapacity * 7u
      && !ak_render_group_map_rehash(state,
                                     state->groupMapCapacity * 2u))
    return false;

  slot = ak_render_node_hash(root) & (state->groupMapCapacity - 1u);
  while (state->groupMap[slot].root) {
    if (state->groupMap[slot].root == root) {
      *index   = state->groupMap[slot].indexPlusOne - 1u;
      *created = false;
      return true;
    }
    slot = (slot + 1u) & (state->groupMapCapacity - 1u);
  }

  if (state->groupCount == UINT32_MAX
      || !ak_render_reserve((void **)&state->groups,
                            sizeof(*state->groups),
                            &state->groupCapacity,
                            state->groupCount + 1u))
    return false;

  *index   = (uint32_t)state->groupCount;
  *created = true;
  state->groups[state->groupCount] = calloc(1, sizeof(*state->groups[0]));
  if (!state->groups[state->groupCount])
    return false;
  state->groups[state->groupCount]->root = root;
  state->groupCount++;

  state->groupMap[slot].root         = root;
  state->groupMap[slot].indexPlusOne = *index + 1u;
  state->groupMapCount++;
  return true;
}

static
bool
ak_render_semantic_supported(AkInputSemantic semantic) {
  switch (semantic) {
    case AK_INPUT_POSITION:
    case AK_INPUT_NORMAL:
    case AK_INPUT_COLOR:
    case AK_INPUT_TANGENT:
    case AK_INPUT_TEXCOORD:
    case AK_INPUT_UV:
      return true;
    default:
      return false;
  }
}

static
int
ak_render_attribute_compare(const void *left, const void *right) {
  const AkRenderJobAttribute *a;
  const AkRenderJobAttribute *b;

  a = left;
  b = right;
  if (a->semantic != b->semantic)
    return a->semantic < b->semantic ? -1 : 1;
  if (a->set != b->set)
    return a->set < b->set ? -1 : 1;
  if (a->componentCount != b->componentCount)
    return a->componentCount < b->componentCount ? -1 : 1;
  return 0;
}

static
bool
ak_render_job_layout(AkRenderJob * __restrict job) {
  AkInput *input;
  uint32_t i, count, vertexCount, texcoordCount;
  bool     haveDirectionAttribute, havePosition;

  count         = 0;
  vertexCount   = 0;
  texcoordCount = 0;
  haveDirectionAttribute = false;
  havePosition  = false;

  for (input = job->primitive->input; input; input = input->next) {
    AkAccessor *accessor;

    if (!ak_render_semantic_supported(input->semantic))
      continue;
    if (count == AK_RENDER_MAX_ATTRIBUTES)
      return false;

    accessor = input->accessor;
    if (!accessor
        || !accessor->buffer
        || !accessor->buffer->data
        || accessor->count == 0
        || accessor->componentCount == 0
        || accessor->componentCount > 16)
      return false;

    if (accessor->count > vertexCount)
      vertexCount = accessor->count;

    if (input->semantic == AK_INPUT_POSITION) {
      if (havePosition || accessor->componentCount < 3)
        return false;
      havePosition = true;
    }
    if (input->semantic == AK_INPUT_NORMAL
        || input->semantic == AK_INPUT_TANGENT)
      haveDirectionAttribute = true;
    if (input->semantic == AK_INPUT_TEXCOORD
        || input->semantic == AK_INPUT_UV)
      texcoordCount++;

    job->attributes[count++] = (AkRenderJobAttribute){
      .input          = input,
      .semantic       = (uint32_t)input->semantic,
      .set            = input->set,
      .componentCount = accessor->componentCount
    };
  }

  if (!havePosition || count == 0)
    return false;

  qsort(job->attributes,
        count,
        sizeof(job->attributes[0]),
        ak_render_attribute_compare);

  for (i = 1; i < count; i++) {
    if (job->attributes[i - 1].semantic == job->attributes[i].semantic
        && job->attributes[i - 1].set == job->attributes[i].set)
      return false;
  }

  job->attributeCount = count;
  job->vertexCount    = vertexCount;
  job->hasDirectionAttribute = haveDirectionAttribute;
  /*
   * One texture-coordinate stream has no ambiguous instance binding:
   * every texture maps to SceneKit channel zero. Multiple streams require
   * the exact primitive+instance material context, so keep such jobs apart.
   */
  job->materialContext = texcoordCount > 1
    ? ((uintptr_t)job->primitive ^ ((uintptr_t)job->instance >> 4))
    : 0u;
  return true;
}

static
uint32_t
ak_render_index_accessor_value(const AkAccessor * __restrict accessor,
                               uint32_t                       index,
                               bool                          *ok) {
  const uint8_t *data;
  size_t         stride, offset;

  if (!accessor
      || !accessor->buffer
      || !accessor->buffer->data
      || index >= accessor->count) {
    *ok = false;
    return 0;
  }

  stride = accessor->byteStride
    ? accessor->byteStride
    : accessor->bytesPerComponent;
  offset = accessor->byteOffset + (size_t)index * stride;
  if (offset > accessor->buffer->length
      || accessor->bytesPerComponent > accessor->buffer->length - offset) {
    *ok = false;
    return 0;
  }

  data = (const uint8_t *)accessor->buffer->data + offset;
  switch (accessor->componentType) {
    case AKT_UBYTE:
      return *data;
    case AKT_USHORT: {
      uint16_t value;
      memcpy(&value, data, sizeof(value));
      return value;
    }
    case AKT_UINT: {
      uint32_t value;
      memcpy(&value, data, sizeof(value));
      return value;
    }
    default:
      *ok = false;
      return 0;
  }
}

static
uint32_t
ak_render_job_index_value(const AkRenderJob * __restrict job,
                          uint32_t                        index,
                          bool                           *ok) {
  if (job->primitive->indices)
    return ak_indexArrayGet(job->primitive->indices, index);
  if (job->primitive->indexAccessor)
    return ak_render_index_accessor_value(job->primitive->indexAccessor,
                                          index,
                                          ok);
  if (index < job->vertexCount)
    return index;

  *ok = false;
  return 0;
}

static
bool
ak_render_job_topology(AkRenderJob * __restrict job) {
  AkMeshPrimitive *primitive;
  size_t           indexCount;
  uint32_t         indexMax, i;

  primitive = job->primitive;
  if (primitive->indexStride > 1)
    return false;

  indexCount = ak_meshPrimitiveIndexCount(primitive);
  if (indexCount == 0)
    indexCount = job->vertexCount;
  if (indexCount == 0 || indexCount > UINT32_MAX)
    return false;

  switch (primitive->type) {
    case AK_PRIMITIVE_TRIANGLES:
      if (((AkTriangles *)primitive)->mode != AK_TRIANGLES
          || indexCount % 3u != 0)
        return false;
      job->primitiveCount = (uint32_t)(indexCount / 3u);
      break;
    case AK_PRIMITIVE_LINES:
      if (((AkLines *)primitive)->mode != AK_LINES
          || indexCount % 2u != 0)
        return false;
      job->primitiveCount = (uint32_t)(indexCount / 2u);
      break;
    case AK_PRIMITIVE_POINTS:
      job->primitiveCount = (uint32_t)indexCount;
      break;
    default:
      return false;
  }

  job->indexCount    = (uint32_t)indexCount;
  job->primitiveType = (uint32_t)primitive->type;

  indexMax = primitive->indices || primitive->indexAccessor
    ? ak_meshPrimitiveIndexMax(primitive)
    : job->vertexCount - 1u;
  for (i = 0; i < job->attributeCount; i++) {
    AkAccessor *accessor;

    accessor = job->attributes[i].input->accessor;
    if (!accessor || indexMax >= accessor->count)
      return false;
  }
  return job->primitiveCount != 0;
}

static
void
ak_render_skip_geometry(AkRenderBuildState * __restrict state,
                        AkGeometry         * __restrict geometry) {
  AkMesh          *mesh;
  AkMeshPrimitive *primitive;

  if (!geometry || !geometry->gdata
      || geometry->gdata->type != AK_GEOMETRY_MESH)
    return;

  mesh = ak_objGet(geometry->gdata);
  for (primitive = mesh ? mesh->primitive : NULL;
       primitive;
       primitive = primitive->next)
    state->skippedPrimitiveCount++;
}

static
bool
ak_render_add_geometry(AkRenderBuildState * __restrict state,
                       AkRenderGroupBuilder * __restrict group,
                       AkNode             * __restrict node,
                       AkGeometry         * __restrict geometry,
                       AkInstanceGeometry * __restrict instance,
                       mat4                            world) {
  AkMesh          *mesh;
  AkMeshPrimitive *primitive;
  mat3             linear;
  float            determinant;
  bool             worldIdentity;
  bool             worldTranslationOnly;

  if (!geometry || !geometry->gdata
      || geometry->gdata->type != AK_GEOMETRY_MESH)
    return true;

  mesh = ak_objGet(geometry->gdata);
  if (!mesh)
    return true;

  worldTranslationOnly = ak_render_mat4_is_translation(world);
  worldIdentity        = worldTranslationOnly
                         && world[3][0] == 0.0f
                         && world[3][1] == 0.0f
                         && world[3][2] == 0.0f;
  if (worldTranslationOnly) {
    determinant = 1.0f;
  } else {
    glm_mat4_pick3(world, linear);
    determinant = glm_mat3_det(linear);
  }

  for (primitive = mesh->primitive;
       primitive;
       primitive = primitive->next) {
    AkRenderJob        *job;
    AkResolvedMaterial  resolved;

    if (!ak_render_reserve((void **)&group->jobs,
                           sizeof(*group->jobs),
                           &group->jobCapacity,
                           group->jobCount + 1u)) {
      state->result = AK_ENOMEM;
      return false;
    }

    job = &group->jobs[group->jobCount];
    memset(job, 0, sizeof(*job));
    job->node      = node;
    job->geometry  = geometry;
    job->primitive = primitive;
    job->instance  = instance;
    job->worldDeterminant = determinant;
    job->worldIdentity    = worldIdentity;
    job->worldTranslationOnly = worldTranslationOnly;
    glm_mat4_copy(world, job->world);

    if (!ak_render_job_layout(job)) {
      state->skippedPrimitiveCount++;
      continue;
    }
    if (!ak_render_job_topology(job)) {
      state->skippedPrimitiveCount++;
      continue;
    }

    memset(&resolved, 0, sizeof(resolved));
    if (ak_materialResolve(primitive, instance, UINT32_MAX, &resolved))
      job->material = resolved.material;

    job->reverseWinding = determinant < 0.0f
                          && primitive->type == AK_PRIMITIVE_TRIANGLES;
    group->jobCount++;
  }

  return true;
}

static
bool
ak_render_build_group(AkRenderBuildState * __restrict state,
                      uint32_t                         groupIndex);

static
bool
ak_render_group_add_instance(AkRenderBuildState * __restrict state,
                             AkRenderGroupBuilder * __restrict group,
                             AkNode             * __restrict owner,
                             uint32_t                         targetGroupIndex,
                             mat4                             matrix) {
  AkRenderGroupInstance *instance;

  if (!ak_render_reserve((void **)&group->instances,
                         sizeof(*group->instances),
                         &group->instanceCapacity,
                         group->instanceCount + 1u)) {
    state->result = AK_ENOMEM;
    return false;
  }

  instance = &group->instances[group->instanceCount++];
  memset(instance, 0, sizeof(*instance));
  instance->owner            = owner;
  instance->targetGroupIndex = targetGroupIndex;
  memcpy(instance->matrix, matrix, sizeof(instance->matrix));
  return true;
}

static
bool
ak_render_collect_group_node(AkRenderBuildState * __restrict state,
                             uint32_t                         groupIndex,
                             AkNode             * __restrict node,
                             mat4                             parentWorld,
                             bool                             parentVisible,
                             uint32_t                         depth) {
  AkRenderGroupBuilder *group;
  AkInstanceGeometry   *instance;
  AkInstanceNode       *nodeInstance;
  AkNode               *child;
  mat4                  local, world;
  bool                  visible;

  if (!node || state->result != AK_OK)
    return state->result == AK_OK;
  if (depth == AK_RENDER_MAX_DEPTH) {
    state->skippedPrimitiveCount++;
    return true;
  }

  group = state->groups[groupIndex];
  ak_transformCombine(node->transform, local[0]);
  glm_mat4_mul(parentWorld, local, world);
  visible = parentVisible && node->visible;

  for (instance = node->geometry;
       instance;
       instance = (AkInstanceGeometry *)instance->base.next) {
    AkGeometry *geometry;

    geometry = ak_instanceObject(&instance->base);
    if (!visible
        || node->gpuInstancing
        || instance->skinner
        || instance->morpher) {
      ak_render_skip_geometry(state, geometry);
      continue;
    }
    if (!ak_render_add_geometry(state,
                                group,
                                node,
                                geometry,
                                instance,
                                world))
      return false;
  }

  for (child = node->chld; child; child = child->next) {
    if (!ak_render_collect_group_node(state,
                                      groupIndex,
                                      child,
                                      world,
                                      visible,
                                      depth + 1u))
      return false;
  }

  for (nodeInstance = node->node;
       nodeInstance;
       nodeInstance = nodeInstance->next) {
    AkNode  *target;
    uint32_t targetGroupIndex;
    bool     created;

    target = ak_instanceNodeTarget(nodeInstance);
    if (!target)
      continue;
    if (!visible) {
      state->skippedPrimitiveCount++;
      continue;
    }
    if (!ak_render_group_index(state,
                               target,
                               &targetGroupIndex,
                               &created)) {
      state->result = AK_ENOMEM;
      return false;
    }
    if (state->groups[targetGroupIndex]->status == 1u) {
      state->skippedPrimitiveCount++;
      continue;
    }
    if ((created || state->groups[targetGroupIndex]->status == 0u)
        && !ak_render_build_group(state, targetGroupIndex))
      return false;

    group = state->groups[groupIndex];
    if (!ak_render_group_add_instance(state,
                                      group,
                                      node,
                                      targetGroupIndex,
                                      world))
      return false;
  }

  return true;
}

static
bool
ak_render_build_group(AkRenderBuildState * __restrict state,
                      uint32_t                         groupIndex) {
  AkRenderGroupBuilder *group;
  mat4                  identity;

  group = state->groups[groupIndex];
  if (group->status == 2u)
    return true;
  if (group->status == 1u) {
    state->skippedPrimitiveCount++;
    return true;
  }

  group->status = 1u;
  glm_mat4_identity(identity);
  if (!ak_render_collect_group_node(state,
                                    groupIndex,
                                    group->root,
                                    identity,
                                    true,
                                    0u))
    return false;
  group->status = 2u;
  return true;
}

static
int
ak_render_job_compare(const void *left, const void *right) {
  const AkRenderJob *a;
  const AkRenderJob *b;
  uint32_t           i;

  a = left;
  b = right;
  if (a->primitiveType != b->primitiveType)
    return a->primitiveType < b->primitiveType ? -1 : 1;
  if (a->attributeCount != b->attributeCount)
    return a->attributeCount < b->attributeCount ? -1 : 1;
  for (i = 0; i < a->attributeCount; i++) {
    int cmp;

    cmp = ak_render_attribute_compare(&a->attributes[i], &b->attributes[i]);
    if (cmp)
      return cmp;
  }
  if (a->material != b->material)
    return (uintptr_t)a->material < (uintptr_t)b->material ? -1 : 1;
  if (a->materialContext != b->materialContext)
    return a->materialContext < b->materialContext ? -1 : 1;
  return 0;
}

static
bool
ak_render_jobs_share_batch(const AkRenderJob * __restrict a,
                           const AkRenderJob * __restrict b) {
  uint32_t i;

  if (a->primitiveType != b->primitiveType
      || a->attributeCount != b->attributeCount
      || a->material != b->material
      || a->materialContext != b->materialContext)
    return false;

  for (i = 0; i < a->attributeCount; i++) {
    if (ak_render_attribute_compare(&a->attributes[i], &b->attributes[i]))
      return false;
  }
  return true;
}

static
void
ak_render_job_normal_matrix(AkRenderJob * __restrict job,
                            mat3                       normalMatrix) {
  glm_mat4_pick3(job->world, normalMatrix);
  if (fabsf(job->worldDeterminant) > 1e-12f) {
    glm_mat3_inv(normalMatrix, normalMatrix);
    glm_mat3_transpose(normalMatrix);
  }
}

static
void
ak_render_transform_attribute(AkRenderJob * __restrict job,
                              uint32_t                  semantic,
                              uint32_t                  componentCount,
                              float       * __restrict values,
                              mat3                       normalMatrix) {
  uint32_t i;

  if (semantic != AK_INPUT_POSITION
      && semantic != AK_INPUT_NORMAL
      && semantic != AK_INPUT_TANGENT)
    return;

  if (job->worldIdentity)
    return;

  if (job->worldTranslationOnly && semantic != AK_INPUT_POSITION)
    return;

  for (i = 0; i < job->vertexCount; i++) {
    float *value;
    vec3   in3, out3;

    value  = values + (size_t)i * componentCount;
    in3[0] = value[0];
    in3[1] = value[1];
    in3[2] = value[2];

    if (semantic == AK_INPUT_POSITION && job->worldTranslationOnly) {
      float positionW;

      positionW = componentCount > 3 ? value[3] : 1.0f;
      value[0] += job->world[3][0] * positionW;
      value[1] += job->world[3][1] * positionW;
      value[2] += job->world[3][2] * positionW;
    } else if (semantic == AK_INPUT_POSITION) {
      vec4 in4, out4;

      in4[0] = in3[0];
      in4[1] = in3[1];
      in4[2] = in3[2];
      in4[3] = componentCount > 3 ? value[3] : 1.0f;
      glm_mat4_mulv(job->world, in4, out4);
      value[0] = out4[0];
      value[1] = out4[1];
      value[2] = out4[2];
      if (componentCount > 3)
        value[3] = out4[3];
    } else {
      glm_mat3_mulv(normalMatrix, in3, out3);
      if (glm_vec3_norm2(out3) > 1e-20f)
        glm_vec3_normalize(out3);
      value[0] = out3[0];
      value[1] = out3[1];
      value[2] = out3[2];
    }
  }
}

static
void
ak_scene_render_batch_free(AkRenderBatch *batch) {
  uint32_t i;

  if (!batch)
    return;
  for (i = 0; i < batch->attributeCount; i++)
    free(batch->attributes[i].values);
  free(batch->attributes);
  free(batch->indices);
  free(batch->ranges);
  memset(batch, 0, sizeof(*batch));
}

AK_EXPORT
void
ak_sceneRenderDataFree(AkSceneRenderData *renderData) {
  uint32_t i, j;

  if (!renderData)
    return;
  for (i = 0; i < renderData->groupCount; i++) {
    AkRenderGroup *group;

    group = &renderData->groups[i];
    for (j = 0; j < group->batchCount; j++)
      ak_scene_render_batch_free(&group->batches[j]);
    free(group->batches);
    free(group->instances);
  }
  free(renderData->groups);
  free(renderData);
}

static
AkResult
ak_render_build_batch(AkRenderBatch * __restrict batch,
                      AkRenderJob   * __restrict jobs,
                      size_t                     jobCount) {
  const AkRenderJob *first;
  size_t             vertexCount, indexCount;
  size_t             vertexCursor, indexCursor;
  size_t             i, j, bytes;

  first       = jobs;
  vertexCount = 0;
  indexCount  = 0;
  for (i = 0; i < jobCount; i++) {
    if (jobs[i].vertexCount > UINT32_MAX - vertexCount
        || jobs[i].indexCount > UINT32_MAX - indexCount)
      return AK_EINVAL;
    vertexCount += jobs[i].vertexCount;
    indexCount  += jobs[i].indexCount;
  }

  batch->attributes = calloc(first->attributeCount,
                             sizeof(*batch->attributes));
  batch->ranges     = calloc(jobCount, sizeof(*batch->ranges));
  if (ak_render_mul_overflow(indexCount, sizeof(*batch->indices), &bytes))
    return AK_EINVAL;
  batch->indices = malloc(bytes);
  if (!batch->attributes || !batch->ranges || !batch->indices)
    return AK_ENOMEM;

  batch->material          = first->material;
  batch->sourceGeometry    = first->geometry;
  batch->sourcePrimitive   = first->primitive;
  batch->sourceInstance    = first->instance;
  batch->attributeCount    = first->attributeCount;
  batch->vertexCount       = (uint32_t)vertexCount;
  batch->indexCount        = (uint32_t)indexCount;
  batch->rangeCount        = (uint32_t)jobCount;
  batch->primitiveType     = first->primitiveType;

  for (j = 0; j < first->attributeCount; j++) {
    AkRenderBatchAttribute *attribute;
    size_t                  valueCount;

    attribute = &batch->attributes[j];
    if (ak_render_mul_overflow(vertexCount,
                               first->attributes[j].componentCount,
                               &valueCount)
        || valueCount > UINT32_MAX
        || ak_render_mul_overflow(valueCount, sizeof(float), &bytes))
      return AK_EINVAL;

    attribute->values         = malloc(bytes);
    attribute->valueCount     = (uint32_t)valueCount;
    attribute->semantic       = first->attributes[j].semantic;
    attribute->set            = first->attributes[j].set;
    attribute->componentCount = first->attributes[j].componentCount;
    if (!attribute->values)
      return AK_ENOMEM;
  }

  vertexCursor = 0;
  indexCursor  = 0;
  for (i = 0; i < jobCount; i++) {
    AkRenderJob        *job;
    AkRenderBatchRange *range;
    mat3                normalMatrix;
    uint32_t            firstPrimitive;

    job   = &jobs[i];
    range = &batch->ranges[i];
    if (job->hasDirectionAttribute
        && !job->worldIdentity
        && !job->worldTranslationOnly)
      ak_render_job_normal_matrix(job, normalMatrix);
    if (batch->primitiveCount > UINT32_MAX - job->primitiveCount)
      return AK_EINVAL;
    firstPrimitive        = batch->primitiveCount;
    batch->primitiveCount += job->primitiveCount;

    range->node           = job->node;
    range->geometry       = job->geometry;
    range->primitive      = job->primitive;
    range->instance       = job->instance;
    range->firstPrimitive = firstPrimitive;
    range->primitiveCount = job->primitiveCount;
    range->firstIndex     = (uint32_t)indexCursor;
    range->indexCount     = job->indexCount;

    for (j = 0; j < job->attributeCount; j++) {
      AkAccessor             *accessor;
      AkRenderBatchAttribute *attribute;
      float                  *destination;
      size_t                  valueCount, sourceValueCount;

      accessor    = job->attributes[j].input->accessor;
      attribute   = &batch->attributes[j];
      valueCount  = (size_t)job->vertexCount * attribute->componentCount;
      sourceValueCount = (size_t)accessor->count * attribute->componentCount;
      destination = attribute->values
                    + vertexCursor * attribute->componentCount;
      if (ak_accessorAsFloat(accessor,
                             destination,
                             valueCount) != sourceValueCount)
        return AK_EINVAL;
      if (sourceValueCount < valueCount) {
        memset(destination + sourceValueCount,
               0,
               (valueCount - sourceValueCount) * sizeof(*destination));
      }
      ak_render_transform_attribute(job,
                                    attribute->semantic,
                                    attribute->componentCount,
                                    destination,
                                    normalMatrix);
    }

    for (j = 0; j < job->indexCount; j++) {
      uint32_t sourceIndex, sourceOffset;
      bool     ok;

      ok           = true;
      sourceOffset = (uint32_t)j;
      if (job->reverseWinding)
        sourceOffset = (uint32_t)(j - j % 3u + (2u - j % 3u));
      sourceIndex = ak_render_job_index_value(job, sourceOffset, &ok);
      if (!ok || sourceIndex >= job->vertexCount)
        return AK_EINVAL;
      batch->indices[indexCursor + j]
        = (uint32_t)vertexCursor + sourceIndex;
    }

    vertexCursor += job->vertexCount;
    indexCursor  += job->indexCount;
  }

  return AK_OK;
}

static
void
ak_render_build_state_free(AkRenderBuildState *state) {
  size_t i;

  if (!state)
    return;
  for (i = 0; i < state->groupCount; i++) {
    if (!state->groups[i])
      continue;
    free(state->groups[i]->jobs);
    free(state->groups[i]->instances);
    free(state->groups[i]);
  }
  free(state->groups);
  free(state->groupMap);
  memset(state, 0, sizeof(*state));
}

static
AkResult
ak_render_emit_group(AkSceneRenderData    * __restrict data,
                     AkRenderGroup        * __restrict output,
                     AkRenderGroupBuilder * __restrict builder) {
  size_t   batchCount, batchIndex, begin, end;
  AkResult result;
  uint32_t included;

  if (builder->instanceCount > UINT32_MAX
      || builder->jobCount > UINT32_MAX)
    return AK_EINVAL;

  output->sourceRoot    = builder->root;
  output->instances     = builder->instances;
  output->instanceCount = (uint32_t)builder->instanceCount;
  builder->instances    = NULL;
  builder->instanceCount = builder->instanceCapacity = 0;

  if (ak_render_add_overflow_u32(data->includedPrimitiveCount,
                                 (uint32_t)builder->jobCount,
                                 &included))
    return AK_EINVAL;
  data->includedPrimitiveCount = included;
  if (builder->jobCount == 0)
    return AK_OK;

  qsort(builder->jobs,
        builder->jobCount,
        sizeof(*builder->jobs),
        ak_render_job_compare);

  batchCount = 1;
  for (begin = 1; begin < builder->jobCount; begin++) {
    if (!ak_render_jobs_share_batch(&builder->jobs[begin - 1],
                                    &builder->jobs[begin]))
      batchCount++;
  }
  if (batchCount > UINT32_MAX)
    return AK_EINVAL;

  output->batches = calloc(batchCount, sizeof(*output->batches));
  if (!output->batches)
    return AK_ENOMEM;
  output->batchCount = (uint32_t)batchCount;

  batchIndex = 0;
  begin      = 0;
  while (begin < builder->jobCount) {
    end = begin + 1;
    while (end < builder->jobCount
           && ak_render_jobs_share_batch(&builder->jobs[begin],
                                         &builder->jobs[end]))
      end++;

    result = ak_render_build_batch(&output->batches[batchIndex],
                                   &builder->jobs[begin],
                                   end - begin);
    if (result != AK_OK)
      return result;
    batchIndex++;
    begin = end;
  }

  return AK_OK;
}

AK_EXPORT
AkResult
ak_sceneBuildRenderData(AkScene            * __restrict scene,
                        AkSceneRenderData ** __restrict renderData) {
  AkRenderBuildState state;
  AkSceneRenderData *data;
  uint32_t           rootGroupIndex;
  size_t             i;
  AkResult           result;
  bool               created;

  if (!renderData)
    return AK_EINVAL;
  *renderData = NULL;
  if (!scene)
    return AK_EINVAL;

  memset(&state, 0, sizeof(state));
  state.result = AK_OK;

  data = calloc(1, sizeof(*data));
  if (!data)
    return AK_ENOMEM;
  if (!scene->node) {
    *renderData = data;
    return AK_OK;
  }

  if (!ak_render_group_index(&state,
                             scene->node,
                             &rootGroupIndex,
                             &created)) {
    ak_render_build_state_free(&state);
    free(data);
    return AK_ENOMEM;
  }
  if (!ak_render_build_group(&state, rootGroupIndex)) {
    result = state.result;
    ak_render_build_state_free(&state);
    free(data);
    return result;
  }

  if (state.groupCount > UINT32_MAX) {
    ak_render_build_state_free(&state);
    free(data);
    return AK_EINVAL;
  }

  data->groups = calloc(state.groupCount, sizeof(*data->groups));
  if (!data->groups) {
    ak_render_build_state_free(&state);
    free(data);
    return AK_ENOMEM;
  }
  data->groupCount            = (uint32_t)state.groupCount;
  data->rootGroupIndex        = rootGroupIndex;
  data->skippedPrimitiveCount = state.skippedPrimitiveCount;

  for (i = 0; i < state.groupCount; i++) {
    result = ak_render_emit_group(data,
                                  &data->groups[i],
                                  state.groups[i]);
    if (result != AK_OK) {
      ak_render_build_state_free(&state);
      ak_sceneRenderDataFree(data);
      return result;
    }
  }

  ak_render_build_state_free(&state);
  *renderData = data;
  return AK_OK;
}
