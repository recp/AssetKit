/*
 * Copyright (C) 2026 Recep Aslantas
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

#include "ohana.h"

#include <math.h>
#include <string.h>

#define DAE_OHANA_MIN_JOINTS              8u
#define DAE_OHANA_AFFINE_EPSILON          1.0e-5f
#define DAE_OHANA_WORLD_RIGID_EPSILON     1.0e-4f
#define DAE_OHANA_ANCHOR_EPSILON          1.0e-4f
#define DAE_OHANA_CORRUPT_EPSILON         1.0e-3f
#define DAE_OHANA_INVERTIBLE_EPSILON      1.0e-6f

static
bool
dae_ohana_equal_parts(const char * __restrict value,
                      const char * __restrict first,
                      size_t                   firstLen,
                      const char * __restrict second) {
  size_t secondLen;

  if (!value || !first || !second)
    return false;

  secondLen = strlen(second);
  return strlen(value) == firstLen + secondLen
      && memcmp(value, first, firstLen) == 0
      && memcmp(value + firstLen, second, secondLen) == 0;
}

static
bool
dae_ohana_controller_base(const char * __restrict id,
                          size_t      * __restrict baseLen) {
  static const char prefix[] = "mesh_";
  static const char suffix[] = "_ctrl_id";
  size_t            idLen;
  size_t            i;

  if (!id || !baseLen)
    return false;

  idLen = strlen(id);
  if (idLen <= sizeof(prefix) - 1u + sizeof(suffix) - 1u
      || memcmp(id, prefix, sizeof(prefix) - 1u) != 0
      || memcmp(id + idLen - (sizeof(suffix) - 1u),
                suffix,
                sizeof(suffix) - 1u) != 0)
    return false;

  i = sizeof(prefix) - 1u;
  if (id[i] < '0' || id[i] > '9')
    return false;
  do {
    i++;
  } while (i < idLen && id[i] >= '0' && id[i] <= '9');

  *baseLen = idLen - (sizeof(suffix) - 1u);
  return i < *baseLen - 1u && id[i] == '_';
}

static
bool
dae_ohana_matrix_finite(AkFloat4x4 matrix) {
  uint32_t column, row;

  for (column = 0u; column < 4u; column++) {
    for (row = 0u; row < 4u; row++) {
      if (!isfinite(matrix[column][row]))
        return false;
    }
  }
  return true;
}

static
bool
dae_ohana_matrix_affine(AkFloat4x4 matrix) {
  return fabsf(matrix[0][3]) <= DAE_OHANA_AFFINE_EPSILON
      && fabsf(matrix[1][3]) <= DAE_OHANA_AFFINE_EPSILON
      && fabsf(matrix[2][3]) <= DAE_OHANA_AFFINE_EPSILON
      && fabsf(matrix[3][3] - 1.0f) <= DAE_OHANA_AFFINE_EPSILON;
}

static
float
dae_ohana_rigid_error(AkFloat4x4 matrix) {
  float    error;
  uint32_t left, right, row;

  error = 0.0f;
  for (left = 0u; left < 3u; left++) {
    for (right = 0u; right < 3u; right++) {
      float dot;
      float expected;
      float delta;

      dot = 0.0f;
      for (row = 0u; row < 3u; row++)
        dot += matrix[left][row] * matrix[right][row];

      expected = left == right ? 1.0f : 0.0f;
      delta    = fabsf(dot - expected);
      if (delta > error)
        error = delta;
    }
  }
  return error;
}

static
float
dae_ohana_relative_matrix_error(AkFloat4x4 left, AkFloat4x4 right) {
  float    difference;
  float    scale;
  uint32_t column, row;

  difference = 0.0f;
  scale      = 1.0f;
  for (column = 0u; column < 4u; column++) {
    for (row = 0u; row < 4u; row++) {
      float value;

      value = fabsf(left[column][row] - right[column][row]);
      if (value > difference)
        difference = value;
      value = fabsf(right[column][row]);
      if (value > scale)
        scale = value;
    }
  }
  return difference / scale;
}

static
float
dae_ohana_bind_error(AkFloat4x4 world, AkFloat4x4 inverseBind) {
  mat4     product;
  float    error;
  uint32_t column, row;

  glm_mat4_mul(world, inverseBind, product);
  error = 0.0f;
  for (column = 0u; column < 4u; column++) {
    for (row = 0u; row < 4u; row++) {
      float expected;
      float delta;

      expected = column == row ? 1.0f : 0.0f;
      delta    = fabsf(product[column][row] - expected);
      if (delta > error)
        error = delta;
    }
  }
  return error;
}

static
bool
dae_ohana_identity(AkFloat4x4 matrix) {
  uint32_t column, row;

  if (!dae_ohana_matrix_finite(matrix))
    return false;

  for (column = 0u; column < 4u; column++) {
    for (row = 0u; row < 4u; row++) {
      float expected;

      expected = column == row ? 1.0f : 0.0f;
      if (fabsf(matrix[column][row] - expected)
          > DAE_OHANA_AFFINE_EPSILON)
        return false;
    }
  }
  return true;
}

static
bool
dae_ohana_accessor_bounds(AkAccessor * __restrict accessor,
                          size_t                    itemSize) {
  size_t stride;
  size_t lastOffset;

  if (!accessor || !accessor->buffer || !accessor->buffer->data
      || accessor->count == 0u)
    return false;

  stride = accessor->byteStride ? accessor->byteStride : itemSize;
  if (stride < itemSize || accessor->byteOffset > accessor->buffer->length)
    return false;
  if ((size_t)(accessor->count - 1u)
      > (SIZE_MAX - accessor->byteOffset) / stride)
    return false;

  lastOffset = accessor->byteOffset
             + (size_t)(accessor->count - 1u) * stride;
  return lastOffset <= accessor->buffer->length
      && itemSize <= accessor->buffer->length - lastOffset;
}

static
const char*
dae_ohana_joint_name(AkAccessor * __restrict accessor, size_t index) {
  const char *name;
  size_t      stride;
  size_t      offset;

  if (!accessor || index >= accessor->count)
    return NULL;

  stride = accessor->byteStride ? accessor->byteStride : sizeof(name);
  offset = accessor->byteOffset + index * stride;
  memcpy(&name, (char *)accessor->buffer->data + offset, sizeof(name));
  return name;
}

static
bool
dae_ohana_source_profile(AkSkinDAE  * __restrict skindae,
                         const char * __restrict base,
                         size_t                   baseLen,
                         AkAccessor ** __restrict jointsAccessor) {
  DaeSource *source;
  DaeSource *jointsSource;
  DaeSource *bindSource;
  DaeSource *weightsSource;
  AkAccessor *jointAcc;
  AkAccessor *bindAcc;
  AkAccessor *weightAcc;
  uint32_t    sourceCount;

  if (!skindae
      || !skindae->joints.joints
      || !skindae->joints.invBindMatrix
      || !skindae->weights.joints
      || !skindae->weights.weights
      || skindae->inputCount != 2u
      || skindae->weights.joints->indexOffset != 0u
      || skindae->weights.weights->indexOffset != 1u)
    return false;

  jointAcc  = skindae->joints.joints->accessor;
  bindAcc   = skindae->joints.invBindMatrix->accessor;
  weightAcc = skindae->weights.weights->accessor;
  if (!jointAcc || !bindAcc || !weightAcc
      || skindae->weights.joints->accessor != jointAcc
      || jointAcc->componentType != AKT_NAME
      || jointAcc->componentCount != 1u
      || jointAcc->bytesPerComponent != sizeof(const char *)
      || bindAcc->componentType != AKT_FLOAT
      || bindAcc->componentCount != 16u
      || bindAcc->count != jointAcc->count
      || weightAcc->componentType != AKT_FLOAT
      || weightAcc->componentCount != 1u
      || !dae_ohana_accessor_bounds(jointAcc, sizeof(const char *)))
    return false;

  jointsSource = NULL;
  bindSource = NULL;
  weightsSource = NULL;
  sourceCount = 0u;
  for (source = skindae->source; source; source = source->next) {
    const char *id;

    sourceCount++;
    id = ak_getId(source);
    if (dae_ohana_equal_parts(id,
                              base,
                              baseLen,
                              "_ctrl_joint_names_id")) {
      if (jointsSource)
        return false;
      jointsSource = source;
    } else if (dae_ohana_equal_parts(id,
                                     base,
                                     baseLen,
                                     "_ctrl_inv_bind_poses_id")) {
      if (bindSource)
        return false;
      bindSource = source;
    } else if (dae_ohana_equal_parts(id,
                                     base,
                                     baseLen,
                                     "_ctrl_weights_id")) {
      if (weightsSource)
        return false;
      weightsSource = source;
    } else {
      return false;
    }
  }

  if (sourceCount != 3u
      || !jointsSource || jointsSource->accessor != jointAcc
      || !bindSource || bindSource->accessor != bindAcc
      || !weightsSource || weightsSource->accessor != weightAcc
      || !jointsSource->buffer
      || !dae_ohana_equal_parts(ak_getId(jointsSource->buffer),
                                base,
                                baseLen,
                                "_ctrl_joint_names_array_id")
      || !bindSource->buffer
      || !dae_ohana_equal_parts(ak_getId(bindSource->buffer),
                                base,
                                baseLen,
                                "_ctrl_inv_bind_poses_array_id")
      || !weightsSource->buffer
      || !dae_ohana_equal_parts(ak_getId(weightsSource->buffer),
                                base,
                                baseLen,
                                "_ctrl_weights_array_id"))
    return false;

  *jointsAccessor = jointAcc;
  return true;
}

static
AkInstanceController*
dae_ohana_instance_profile(DAEState    * __restrict dst,
                           AkController * __restrict controller,
                           const char   * __restrict base,
                           size_t                     baseLen) {
  FListItem           *item;
  AkInstanceController *match;
  uint32_t             count;

  match = NULL;
  count = 0u;
  for (item = dst->instCtlrs; item; item = item->next) {
    AkInstanceController *instance;

    instance = item->data;
    if (instance && ak_instanceObject(&instance->base) == controller) {
      match = instance;
      count++;
    }
  }

  if (count != 1u || !match || !match->base.node
      || !match->base.url.url
      || strlen(match->base.url.url) != strlen(ak_getId(controller)) + 1u
      || match->base.url.url[0] != '#'
      || strcmp(match->base.url.url + 1u, ak_getId(controller)) != 0)
    return NULL;

  /* The owner uses "vsn_" + mesh base, with only its id carrying "_id". */
  {
    const char *ownerId;
    const char *ownerName;
    size_t      ownerIdLen;
    size_t      ownerNameLen;

    ownerId = ak_getId(match->base.node);
    ownerName = match->base.node->name;
    ownerIdLen = ownerId ? strlen(ownerId) : 0u;
    ownerNameLen = ownerName ? strlen(ownerName) : 0u;
    if (!ownerId || ownerIdLen != sizeof("vsn_") - 1u + baseLen + 3u
        || memcmp(ownerId, "vsn_", sizeof("vsn_") - 1u) != 0
        || memcmp(ownerId + sizeof("vsn_") - 1u, base, baseLen) != 0
        || memcmp(ownerId + sizeof("vsn_") - 1u + baseLen, "_id", 3u) != 0
        || !ownerName
        || ownerNameLen != sizeof("vsn_") - 1u + baseLen
        || memcmp(ownerName, "vsn_", sizeof("vsn_") - 1u) != 0
        || memcmp(ownerName + sizeof("vsn_") - 1u, base, baseLen) != 0)
      return NULL;
  }

  if (!match->reserved || match->reserved->next || !match->reserved->data)
    return NULL;

  return match;
}

static
bool
dae_ohana_repair_controller(DAEState    * __restrict dst,
                            AkController * __restrict controller) {
  const char           *controllerId;
  const char           *geometryId;
  const char           *firstJointName;
  const char           *skeletonUrl;
  AkSkin               *skin;
  AkSkinDAE            *skindae;
  AkGeometry           *geometry;
  AkAccessor           *jointsAccessor;
  AkInstanceController *instance;
  AkFloat4x4           *repaired;
  size_t                baseLen;
  size_t                anchors;
  size_t                i;
  bool                  corruptEvidence;

  if (!controller || controller->type != AK_CONTROLLER_SKIN
      || !(controllerId = ak_getId(controller))
      || !dae_ohana_controller_base(controllerId, &baseLen)
      || !(skin = controller->data)
      || !(skindae = ak_userData(skin))
      || skin->nJoints < DAE_OHANA_MIN_JOINTS
      || !skin->joints || !skin->invBindPoses
      || !dae_ohana_identity(skin->bindShapeMatrix)
      || !dae_ohana_source_profile(skindae,
                                   controllerId,
                                   baseLen,
                                   &jointsAccessor)
      || jointsAccessor->count != skin->nJoints)
    return false;

  geometry = ak_getObjectByUrl(&skindae->baseGeom);
  geometryId = geometry && ak_typeid(geometry) == AKT_GEOMETRY
             ? ak_getId(geometry)
             : NULL;
  if (!dae_ohana_equal_parts(geometryId, controllerId, baseLen, "_id"))
    return false;

  instance = dae_ohana_instance_profile(dst,
                                        controller,
                                        controllerId,
                                        baseLen);
  if (!instance)
    return false;

  if (skin->nJoints > SIZE_MAX / sizeof(*repaired))
    return false;
  repaired = ak_heap_aligned_alloc(dst->heap,
                                   dst->tempmem,
                                   AK_ALIGNOF(AkFloat4x4),
                                   skin->nJoints * sizeof(*repaired));
  if (!repaired)
    return false;

  anchors = 0u;
  corruptEvidence = false;
  firstJointName = NULL;
  for (i = 0u; i < skin->nJoints; i++) {
    const char *jointName;
    const char *jointId;
    const char *jointSid;
    AkNode     *joint;
    AkFloat4x4  world;
    float       determinant;
    float       relativeError;

    jointName = dae_ohana_joint_name(jointsAccessor, i);
    joint = skin->joints[i];
    jointId = joint ? ak_getId(joint) : NULL;
    jointSid = joint ? ak_sid_get(joint) : NULL;
    if (!jointName || !jointName[0]
        || !joint || joint->nodeType != AK_NODE_TYPE_JOINT
        || !dae_ohana_equal_parts(jointId,
                                  jointName,
                                  strlen(jointName),
                                  "_bone_id")
        || !jointSid || strcmp(jointSid, jointName) != 0
        || !joint->name || strcmp(joint->name, jointName) != 0)
      return false;

    if (i == 0u)
      firstJointName = jointName;

    ak_transformCombineWorld(joint, world[0]);
    determinant = glm_mat4_det(world);
    if (!dae_ohana_matrix_finite(world)
        || !dae_ohana_matrix_affine(world)
        || !isfinite(determinant)
        || fabsf(determinant) <= DAE_OHANA_INVERTIBLE_EPSILON
        || fabsf(determinant - 1.0f) > DAE_OHANA_WORLD_RIGID_EPSILON
        || dae_ohana_rigid_error(world)
             > DAE_OHANA_WORLD_RIGID_EPSILON
        || !dae_ohana_matrix_finite(skin->invBindPoses[i])
        || !dae_ohana_matrix_affine(skin->invBindPoses[i]))
      return false;

    glm_mat4_inv(world, repaired[i]);
    if (!dae_ohana_matrix_finite(repaired[i]))
      return false;

    relativeError = dae_ohana_relative_matrix_error(skin->invBindPoses[i],
                                                     repaired[i]);
    if (relativeError <= DAE_OHANA_ANCHOR_EPSILON)
      anchors++;
    if (relativeError >= DAE_OHANA_CORRUPT_EPSILON
        && dae_ohana_rigid_error(skin->invBindPoses[i])
             >= DAE_OHANA_CORRUPT_EPSILON
        && dae_ohana_bind_error(world, skin->invBindPoses[i])
             >= DAE_OHANA_CORRUPT_EPSILON)
      corruptEvidence = true;
  }

  skeletonUrl = instance->reserved->data;
  if (!firstJointName
      || strlen(skeletonUrl) != 1u + strlen(firstJointName)
                              + sizeof("_bone_id") - 1u
      || skeletonUrl[0] != '#'
      || memcmp(skeletonUrl + 1u,
                firstJointName,
                strlen(firstJointName)) != 0
      || strcmp(skeletonUrl + 1u + strlen(firstJointName), "_bone_id") != 0
      || skin->skeleton != skin->joints[0]
      || anchors <= skin->nJoints / 2u
      || !corruptEvidence)
    return false;

  memcpy(skin->invBindPoses,
         repaired,
         skin->nJoints * sizeof(*skin->invBindPoses));
  return true;
}

AK_HIDE
void
dae_bugfix_ohana_inverse_bind_poses(DAEState * __restrict dst) {
  AkController *controller;

  if (!dst || !dst->doc || !ak_opt_get(AK_OPT_BUGFIXES))
    return;

  for (controller = dst->controllers; controller; controller = controller->next)
    (void)dae_ohana_repair_controller(dst, controller);
}
