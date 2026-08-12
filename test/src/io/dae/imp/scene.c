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

#include "../../../test_export_common.h"

#include <cglm/cglm.h>

static char ak_test_dae_image_load_path[PATH_MAX];

static
bool
ak_test_dae_float_near(float a, float b) {
  return fabsf(a - b) < 0.00001f;
}

static
const float*
ak_test_dae_accessor_float_row(AkAccessor *accessor, uint32_t row) {
  size_t stride;

  if (!accessor
      || accessor->componentType != AKT_FLOAT
      || accessor->bytesPerComponent != sizeof(float)
      || !accessor->buffer
      || !accessor->buffer->data
      || row >= accessor->count)
    return NULL;

  stride = accessor->byteStride
             ? accessor->byteStride
             : (size_t)accessor->componentCount * sizeof(float);

  return (const float *)((const unsigned char *)accessor->buffer->data
                         + accessor->byteOffset
                         + (size_t)row * stride);
}

static
bool
ak_test_dae_accessor_near(AkAccessor *a, AkAccessor *b) {
  uint32_t row, component;

  if (!a || !b
      || a->count != b->count
      || a->componentCount != b->componentCount)
    return false;

  for (row = 0; row < a->count; row++) {
    const float *arow, *brow;

    arow = ak_test_dae_accessor_float_row(a, row);
    brow = ak_test_dae_accessor_float_row(b, row);
    if (!arow || !brow)
      return false;

    for (component = 0; component < a->componentCount; component++) {
      if (!ak_test_dae_float_near(arow[component], brow[component]))
        return false;
    }
  }

  return true;
}

static
bool
ak_test_dae_matrix_near(const float a[4][4], const float b[4][4]) {
  uint32_t column, row;

  for (column = 0; column < 4; column++) {
    for (row = 0; row < 4; row++) {
      if (!ak_test_dae_float_near(a[column][row], b[column][row]))
        return false;
    }
  }

  return true;
}

static
AkChannel*
ak_test_dae_find_channel(AkAnimation *animation, const char *target) {
  for (; animation; animation = animation->next) {
    AkChannel *channel;

    for (channel = animation->channel; channel; channel = channel->next) {
      if (channel->target && strcmp(channel->target, target) == 0)
        return channel;
    }

    if (animation->animation) {
      channel = ak_test_dae_find_channel(animation->animation, target);
      if (channel)
        return channel;
    }
  }

  return NULL;
}

static
AkAnimSampler*
ak_test_dae_channel_sampler(AkChannel *channel) {
  AkAnimSampler *sampler;

  if (!channel)
    return NULL;

  sampler = channel->source.ptr;
  if (!sampler)
    sampler = ak_getObjectByUrl(&channel->source);

  return sampler;
}

static
AkAccessor*
ak_test_dae_sampler_accessor(AkAnimSampler *sampler,
                             AkInputSemantic semantic) {
  AkInput *input;

  if (!sampler)
    return NULL;

  switch (semantic) {
    case AK_INPUT_OUTPUT:
      if (sampler->outputInput)
        return sampler->outputInput->accessor;
      break;
    case AK_INPUT_IN_TANGENT:
      if (sampler->inTangentInput)
        return sampler->inTangentInput->accessor;
      break;
    case AK_INPUT_OUT_TANGENT:
      if (sampler->outTangentInput)
        return sampler->outTangentInput->accessor;
      break;
    default:
      break;
  }

  for (input = sampler->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input->accessor;
  }

  return NULL;
}

static
bool
ak_test_dae_pointer_add_unique(const void **items,
                               uint32_t    *count,
                               uint32_t     capacity,
                               const void  *item) {
  uint32_t i;

  if (!item)
    return true;
  for (i = 0; i < *count; i++) {
    if (items[i] == item)
      return true;
  }
  if (*count >= capacity)
    return false;
  items[(*count)++] = item;
  return true;
}

static
bool
ak_test_dae_animation_counts_walk(AkAnimation *animation,
                                  uint32_t    *samplerCount,
                                  const void **accessors,
                                  uint32_t    *accessorCount,
                                  const void **buffers,
                                  uint32_t    *bufferCount,
                                  uint32_t     capacity) {
  for (; animation; animation = animation->next) {
    AkAnimSampler *sampler;

    for (sampler = animation->sampler;
         sampler;
         sampler = (AkAnimSampler *)sampler->base.next) {
      AkInput *input;

      (*samplerCount)++;
      for (input = sampler->input; input; input = input->next) {
        AkAccessor *accessor;

        accessor = input->accessor;
        if (!ak_test_dae_pointer_add_unique(accessors,
                                            accessorCount,
                                            capacity,
                                            accessor)
            || !ak_test_dae_pointer_add_unique(buffers,
                                               bufferCount,
                                               capacity,
                                               accessor
                                                 ? accessor->buffer : NULL))
          return false;
      }
    }

    if (animation->animation
        && !ak_test_dae_animation_counts_walk(animation->animation,
                                              samplerCount,
                                              accessors,
                                              accessorCount,
                                              buffers,
                                              bufferCount,
                                              capacity))
      return false;
  }
  return true;
}

static
bool
ak_test_dae_animation_counts(AkAnimation *animation,
                             uint32_t    *samplerCount,
                             uint32_t    *accessorCount,
                             uint32_t    *bufferCount) {
  const void *accessors[256];
  const void *buffers[256];

  *samplerCount = *accessorCount = *bufferCount = 0u;
  return ak_test_dae_animation_counts_walk(animation,
                                           samplerCount,
                                           accessors,
                                           accessorCount,
                                           buffers,
                                           bufferCount,
                                           256u);
}

static
AkImageData*
ak_test_dae_image_loader(AkHeap     * __restrict heap,
                         AkImage    * __restrict image,
                         const char * __restrict path,
                         bool                    flipVertically) {
  size_t pathLen;

  (void)heap;
  (void)image;
  (void)flipVertically;

  pathLen = path ? strlen(path) : 0;
  if (pathLen >= sizeof(ak_test_dae_image_load_path))
    pathLen = sizeof(ak_test_dae_image_load_path) - 1u;
  if (pathLen)
    memcpy(ak_test_dae_image_load_path, path, pathLen);
  ak_test_dae_image_load_path[pathLen] = '\0';

  return NULL;
}

TEST_IMPL(dae_scene_roots_are_child_nodes) {
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *rootA, *rootB;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  const char  *tmpBase;
  uint32_t     rootCount;
  AkNode      *root;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-roots-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/roots.dae", tmpdir);
  ASSERT(ak_test_write_dae_two_roots(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(scene->node->next == NULL);
  ASSERT(doc->lib.nodes.count == 2);

  rootA = ak_sceneFindRoot(scene, "RootA");
  rootB = ak_sceneFindRoot(scene, "RootB");
  ASSERT(rootA != NULL);
  ASSERT(rootB != NULL);
  ASSERT(rootA != rootB);
  ASSERT(rootA->name && strcmp(rootA->name, "RootA") == 0);
  ASSERT(rootB->name && strcmp(rootB->name, "RootB") == 0);
  ASSERT(rootA->parent == scene->node);
  ASSERT(rootB->parent == scene->node);

  rootCount = 0;
  for (root = scene->node->chld; root; root = root->next)
    rootCount++;
  ASSERT(rootCount == 2);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_same_file_external_refs_are_internal) {
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *geoNode;
  AkNode             *camNode;
  AkNode             *lightNode;
  AkInstanceGeometry *geomInst;
  char                dirTemplate[PATH_MAX];
  char               *tmpdir;
  char                daePath[PATH_MAX];
  char                outDir[PATH_MAX];
  char                outDaePath[PATH_MAX];
  const char         *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-same-file-refs-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/self.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDaePath, sizeof(outDaePath), "%s/self.dae", outDir);

  ASSERT(ak_test_write_dae_same_file_refs(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);

  geoNode   = ak_sceneFindRoot(scene, "GeoNode");
  camNode   = ak_sceneFindRoot(scene, "CamNode");
  lightNode = ak_sceneFindRoot(scene, "LightNode");
  ASSERT(geoNode != NULL && camNode != NULL && lightNode != NULL);
  ASSERT(geoNode->geometry != NULL);
  ASSERT(camNode->camera != NULL);
  ASSERT(lightNode->light != NULL);

  geomInst = geoNode->geometry;
  ASSERT(ak_instanceObject(&geomInst->base) == doc->lib.geometries.first);
  ASSERT(ak_instanceObject(camNode->camera) == doc->lib.cameras.first);
  ASSERT(ak_instanceObject(lightNode->light) == doc->lib.lights.first);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDaePath, "<library_geometries>"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_geometry url=\"#geom_0\">"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_camera url=\"#camera_0\"/>"));
  ASSERT(ak_test_file_contains(outDaePath, "<instance_light url=\"#light_0\"/>"));

  ak_free(doc);
  ak_test_export_cleanup(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_skin_idref_joints_populate_default_joints) {
  AkDoc              *doc;
  AkSkin             *skin;
  AkNode             *meshNode;
  AkInstanceGeometry *geomInst;
  char                dirTemplate[PATH_MAX];
  char               *tmpdir;
  char                daePath[PATH_MAX];
  const char         *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-joints-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_write_dae_skin_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 1);

  skin = doc->lib.skins.first;
  ASSERT(skin != NULL);
  ASSERT(skin->nJoints == 2);
  ASSERT(skin->joints != NULL);
  ASSERT(skin->joints[0] != NULL);
  ASSERT(skin->joints[1] != NULL);
  ASSERT(strcmp((const char *)ak_getId(skin->joints[0]), "joint0") == 0);
  ASSERT(strcmp((const char *)ak_getId(skin->joints[1]), "joint1") == 0);

  meshNode = ak_getObjectById(doc, "meshNode");
  ASSERT(meshNode != NULL);
  geomInst = meshNode->geometry;
  ASSERT(geomInst != NULL);
  ASSERT(geomInst->skinner != NULL);
  ASSERT(geomInst->skinner->overrideJoints != NULL);
  ASSERT(geomInst->skinner->overrideJoints[0] == skin->joints[0]);
  ASSERT(geomInst->skinner->overrideJoints[1] == skin->joints[1]);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_skin_multi_source_primitives_keep_weight_offsets) {
  AkDoc         *doc;
  AkSkin        *skin;
  AkBoneWeights *weights0;
  AkBoneWeights *weights1;
  char           dirTemplate[PATH_MAX];
  char          *tmpdir;
  char           daePath[PATH_MAX];
  const char    *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-skin-offsets-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));
  ASSERT(ak_test_write_dae_skin_multi_source_primitives(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  ASSERT(doc->lib.skins.count == 1);
  skin = doc->lib.skins.first;
  ASSERT(skin != NULL);
  ASSERT(skin->nPrims == 2);
  ASSERT(skin->weights != NULL);

  weights0 = skin->weights[0];
  weights1 = skin->weights[1];
  ASSERT(weights0 != NULL);
  ASSERT(weights1 != NULL);
  ASSERT(weights0->nVertex == 3);
  ASSERT(weights1->nVertex == 3);
  ASSERT(weights0->counts[0] == 1);
  ASSERT(weights1->counts[0] == 1);
  ASSERT(weights0->weights[weights0->indexes[0]].joint == 0);
  ASSERT(weights1->weights[weights1->indexes[0]].joint == 1);
  ASSERT(weights0->weights[weights0->indexes[0]].weight == 1.0f);
  ASSERT(weights1->weights[weights1->indexes[0]].weight == 1.0f);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_coord_all_converts_skin_and_animation_once) {
  AkDoc          *expected, *actual, *roundtrip;
  AkGeometry     *expectedGeom, *actualGeom;
  AkMesh         *expectedMesh, *actualMesh;
  AkSkin         *expectedSkin, *actualSkin;
  AkNode         *expectedJoint, *actualJoint;
  AkChannel      *expectedChannel, *actualChannel;
  AkAnimSampler  *expectedSampler, *actualSampler;
  AkAccessor     *expectedAccessor, *actualAccessor;
  mat4            expectedJointMatrix, actualJointMatrix;
  mat4            bindProduct;
  const float    *firstPosition;
  char            dirTemplate[PATH_MAX];
  char           *tmpdir;
  char            daePath[PATH_MAX];
  char            exportDir[PATH_MAX];
  char            exportDaePath[PATH_MAX];
  const char     *tmpBase;
  uintptr_t       previousCoord, previousCoordCvtType;
  AkResult        expectedResult, actualResult;
  uint32_t        column, row;
  uint32_t        samplerCount, accessorCount, bufferCount;
  uint32_t        samplerCountAfter, accessorCountAfter, bufferCountAfter;

  expected = actual = roundtrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-coord-all-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "coord-all.dae"));
  ASSERT(ak_test_path_join(exportDir, sizeof(exportDir), tmpdir, "export"));
  ASSERT(ak_test_path_join(exportDaePath,
                           sizeof(exportDaePath),
                           exportDir,
                           "coord-all.dae"));
  ASSERT(ak_test_write_dae_coord_all_skin_animation(daePath));

  previousCoord        = ak_opt_get(AK_OPT_COORD);
  previousCoordCvtType = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ak_opt_set(AK_OPT_COORD, (uintptr_t)AK_YUP);

  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  expectedResult = ak_load(&expected, daePath, AK_FILE_TYPE_AUTO);

  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_ALL);
  actualResult = ak_load(&actual, daePath, AK_FILE_TYPE_AUTO);

  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, previousCoordCvtType);
  ak_opt_set(AK_OPT_COORD, previousCoord);

  ASSERT(expectedResult == AK_OK && expected);
  ASSERT(actualResult == AK_OK && actual);
  ASSERT(expected->coordSys == AK_ZUP);
  ASSERT(actual->coordSys == AK_YUP);
  ASSERT(ak_getCoordSys(actual) == AK_YUP);

  /* The public document conversion is the canonical whole-payload result. */
  ak_changeCoordSys(expected, AK_YUP);
  ASSERT(expected->coordSys == AK_YUP);
  ASSERT(ak_getCoordSys(expected) == AK_YUP);

  expectedGeom = ak_getObjectById(expected, "geom");
  actualGeom   = ak_getObjectById(actual, "geom");
  ASSERT(expectedGeom && actualGeom);
  expectedMesh = ak_objGet(expectedGeom->gdata);
  actualMesh   = ak_objGet(actualGeom->gdata);
  ASSERT(expectedMesh && actualMesh);
  ASSERT(expectedMesh->primitive && actualMesh->primitive);
  ASSERT(expectedMesh->primitive->pos && actualMesh->primitive->pos);
  ASSERT(ak_test_dae_accessor_near(expectedMesh->primitive->pos->accessor,
                                   actualMesh->primitive->pos->accessor));

  firstPosition = ak_test_dae_accessor_float_row(
    actualMesh->primitive->pos->accessor,
    0);
  ASSERT(firstPosition != NULL);
  ASSERT(ak_test_dae_float_near(firstPosition[0], 1.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], 3.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[2], -2.0f));

  expectedSkin = expected->lib.skins.first;
  actualSkin   = actual->lib.skins.first;
  ASSERT(expectedSkin && actualSkin);
  ASSERT(expectedSkin->nJoints == 1 && actualSkin->nJoints == 1);
  ASSERT(expectedSkin->invBindPoses && actualSkin->invBindPoses);
  ASSERT(ak_test_dae_matrix_near(expectedSkin->bindShapeMatrix,
                                 actualSkin->bindShapeMatrix));
  ASSERT(ak_test_dae_matrix_near(expectedSkin->invBindPoses[0],
                                 actualSkin->invBindPoses[0]));
  ASSERT(ak_test_dae_float_near(actualSkin->bindShapeMatrix[3][0], 1.0f));
  ASSERT(ak_test_dae_float_near(actualSkin->bindShapeMatrix[3][1], 3.0f));
  ASSERT(ak_test_dae_float_near(actualSkin->bindShapeMatrix[3][2], -2.0f));
  ASSERT(ak_test_dae_float_near(actualSkin->invBindPoses[0][3][0], -4.0f));
  ASSERT(ak_test_dae_float_near(actualSkin->invBindPoses[0][3][1], 6.0f));
  ASSERT(ak_test_dae_float_near(actualSkin->invBindPoses[0][3][2], -5.0f));

  expectedJoint = ak_getObjectById(expected, "joint0");
  actualJoint   = ak_getObjectById(actual, "joint0");
  ASSERT(expectedJoint && actualJoint);
  ak_transformCombine(expectedJoint->transform, expectedJointMatrix[0]);
  ak_transformCombine(actualJoint->transform, actualJointMatrix[0]);
  ASSERT(ak_test_dae_matrix_near(expectedJointMatrix,
                                 actualJointMatrix));

  /* The authored static joint transform and IBM remain mutual inverses. */
  glm_mat4_mul(actualJointMatrix,
               actualSkin->invBindPoses[0],
               bindProduct);
  for (column = 0; column < 4; column++) {
    for (row = 0; row < 4; row++) {
      ASSERT(ak_test_dae_float_near(bindProduct[column][row],
                                    column == row ? 1.0f : 0.0f));
    }
  }

#define AK_TEST_DAE_COMPARE_ANIM(TARGET, TYPE, SEMANTIC)                     \
  expectedChannel = ak_test_dae_find_channel(expected->lib.animations.first, \
                                              (TARGET));                     \
  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,     \
                                            (TARGET));                       \
  ASSERT(expectedChannel && actualChannel);                                  \
  ASSERT(expectedChannel->targetType == (TYPE));                             \
  ASSERT(actualChannel->targetType == (TYPE));                               \
  expectedSampler = ak_test_dae_channel_sampler(expectedChannel);            \
  actualSampler = ak_test_dae_channel_sampler(actualChannel);                \
  ASSERT(expectedSampler && actualSampler);                                  \
  expectedAccessor = ak_test_dae_sampler_accessor(expectedSampler,           \
                                                   (SEMANTIC));              \
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, (SEMANTIC));  \
  ASSERT(expectedAccessor && actualAccessor);                                \
  ASSERT(ak_test_dae_accessor_near(expectedAccessor, actualAccessor))

  AK_TEST_DAE_COMPARE_ANIM("joint0/translation",
                           AK_TARGET_POSITION,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/translation",
                           AK_TARGET_POSITION,
                           AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/translation",
                           AK_TARGET_POSITION,
                           AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotation",
                           AK_TARGET_ROTATE,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotation",
                           AK_TARGET_ROTATE,
                           AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotation",
                           AK_TARGET_ROTATE,
                           AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/scalingShared",
                           AK_TARGET_SCALE,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/scaling",
                           AK_TARGET_SCALE,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/translation.Y",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/translation.Y",
                           AK_TARGET_FLOAT,
                           AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/translation.Y",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/scaling.Y",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotation.Y",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform",
                           AK_TARGET_FLOAT,
                           AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform(1)(3)",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform(1)(3)",
                           AK_TARGET_FLOAT,
                           AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/matrixTransform(1)(3)",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotationAngleValueA.ANGLE",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_ANIM("joint0/rotationAngleNamedA.ANGLE",
                           AK_TARGET_FLOAT,
                           AK_INPUT_OUTPUT);

#undef AK_TEST_DAE_COMPARE_ANIM

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/translation");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition != NULL);
  ASSERT(ak_test_dae_float_near(firstPosition[0], 1.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], 3.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[2], -2.0f));

  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_IN_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && actualAccessor->componentCount == 6u);
  ASSERT(ak_test_dae_float_near(firstPosition[0], -0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], 1.5f));
  ASSERT(ak_test_dae_float_near(firstPosition[2], -0.3f));
  ASSERT(ak_test_dae_float_near(firstPosition[3], 3.5f));
  ASSERT(ak_test_dae_float_near(firstPosition[4], -0.2f));
  ASSERT(ak_test_dae_float_near(firstPosition[5], -2.5f));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/rotation");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_IN_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && actualAccessor->componentCount == 8u);
  ASSERT(ak_test_dae_float_near(firstPosition[0], -0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], 1.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[2], -0.3f));
  ASSERT(ak_test_dae_float_near(firstPosition[3], 3.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[4], -0.2f));
  ASSERT(ak_test_dae_float_near(firstPosition[5], -2.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[6], -0.4f));
  ASSERT(ak_test_dae_float_near(firstPosition[7], glm_rad(9.0f)));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/scalingShared");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], 1.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], 3.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[2], 2.0f));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/matrixTransform");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition != NULL);
  ASSERT(ak_test_dae_float_near(firstPosition[12], 7.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[13], 9.0f));
  ASSERT(ak_test_dae_float_near(firstPosition[14], -8.0f));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/translation.Y");
  ASSERT(actualChannel && actualChannel->resolvedTarget);
  ASSERT(actualChannel->resolvedTarget->isPartial);
  ASSERT(actualChannel->resolvedTarget->off == 2u);
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -2.0f));
  ASSERT(ak_typeid(actualAccessor) == AKT_ACCESSOR);

  {
    AkAccessor *negativeAccessor;

    negativeAccessor = actualAccessor;
    actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                              "joint0/translation.Z");
    actualSampler = ak_test_dae_channel_sampler(actualChannel);
    actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                  AK_INPUT_OUTPUT);
    ASSERT(actualAccessor && actualAccessor != negativeAccessor);
    ASSERT(actualAccessor->buffer != negativeAccessor->buffer);
    ASSERT(ak_typeid(actualAccessor) == AKT_ACCESSOR);
  }

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/matrixTransform(1)(3)");
  ASSERT(actualChannel && actualChannel->resolvedTarget);
  ASSERT(actualChannel->resolvedTarget->isPartial);
  ASSERT(actualChannel->resolvedTarget->off == 14u);
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -4.0f));
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_IN_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], -3.5f));

  actualChannel = ak_test_dae_find_channel(
                    actual->lib.animations.first,
                    "joint0/rotationAngleValueA.ANGLE");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0],
                                                 glm_rad(30.0f)));
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_IN_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], glm_rad(25.0f)));

  actualChannel = ak_test_dae_find_channel(
                    actual->lib.animations.first,
                    "joint0/rotationAngleValueB.ANGLE");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0],
                                                 glm_rad(30.0f)));

  actualChannel = ak_test_dae_find_channel(
                    actual->lib.animations.first,
                    "joint0/rotationAngleNamedA.ANGLE");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0],
                                                 glm_rad(45.0f)));

  ASSERT(ak_test_dae_animation_counts(actual->lib.animations.first,
                                      &samplerCount,
                                      &accessorCount,
                                      &bufferCount));
  ak_changeCoordSys(actual, AK_ZUP);
  ak_changeCoordSys(actual, AK_YUP);
  ASSERT(actual->coordSys == AK_YUP && ak_getCoordSys(actual) == AK_YUP);
  ASSERT(ak_test_dae_animation_counts(actual->lib.animations.first,
                                      &samplerCountAfter,
                                      &accessorCountAfter,
                                      &bufferCountAfter));
  ASSERT(samplerCountAfter == samplerCount);
  ASSERT(accessorCountAfter == accessorCount);
  ASSERT(bufferCountAfter == bufferCount);

  ASSERT(ak_export(actual, exportDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(exportDaePath,
                               "target=\"joint0/translation.Z\""));
  ASSERT(ak_test_file_contains(exportDaePath,
                               "target=\"joint0/translation.Y\""));
  ASSERT(!ak_test_file_contains(exportDaePath,
                                "target=\"joint0/scaling.Y\""));
  ASSERT(ak_test_file_contains(exportDaePath,
                               "target=\"joint0/scaling.Z\""));
  ASSERT(ak_test_file_contains(exportDaePath,
                               "target=\"joint0/matrixTransform(2)(3)\""));
  ASSERT(ak_test_file_contains(exportDaePath,
                               "target=\"joint0/rotationAngleValueA.ANGLE\""));
  ASSERT(ak_test_file_contains(exportDaePath,
                               "<param name=\"TIME\" type=\"float\"/><param name=\"VALUE\" type=\"float\"/>"));

  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  actualResult = ak_load(&roundtrip, exportDaePath, AK_FILE_TYPE_AUTO);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, previousCoordCvtType);
  ASSERT(actualResult == AK_OK && roundtrip);

#define AK_TEST_DAE_COMPARE_MATRIX_ROUNDTRIP(SEMANTIC)                       \
  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,     \
                                            "joint0/matrixTransform");       \
  expectedChannel = ak_test_dae_find_channel(roundtrip->lib.animations.first,\
                                              "joint0/matrixTransform");     \
  ASSERT(actualChannel && expectedChannel);                                  \
  actualSampler = ak_test_dae_channel_sampler(actualChannel);                \
  expectedSampler = ak_test_dae_channel_sampler(expectedChannel);            \
  ASSERT(actualSampler && expectedSampler);                                  \
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, (SEMANTIC));  \
  expectedAccessor = ak_test_dae_sampler_accessor(expectedSampler,           \
                                                   (SEMANTIC));              \
  ASSERT(actualAccessor && expectedAccessor);                                \
  ASSERT(ak_test_dae_accessor_near(actualAccessor, expectedAccessor))

  AK_TEST_DAE_COMPARE_MATRIX_ROUNDTRIP(AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_MATRIX_ROUNDTRIP(AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_MATRIX_ROUNDTRIP(AK_INPUT_OUT_TANGENT);

#undef AK_TEST_DAE_COMPARE_MATRIX_ROUNDTRIP

#define AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP(ACTUAL_TARGET, EXPORTED_TARGET, SEMANTIC) \
  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,            \
                                            (ACTUAL_TARGET));                        \
  expectedChannel = ak_test_dae_find_channel(roundtrip->lib.animations.first,       \
                                              (EXPORTED_TARGET));                    \
  ASSERT(actualChannel && expectedChannel);                                          \
  actualSampler = ak_test_dae_channel_sampler(actualChannel);                        \
  expectedSampler = ak_test_dae_channel_sampler(expectedChannel);                    \
  ASSERT(actualSampler && expectedSampler);                                           \
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, (SEMANTIC));           \
  expectedAccessor = ak_test_dae_sampler_accessor(expectedSampler, (SEMANTIC));       \
  ASSERT(actualAccessor && expectedAccessor);                                         \
  ASSERT(ak_test_dae_accessor_near(actualAccessor, expectedAccessor))

  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/translation",
                                       "joint0/translation",
                                       AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/translation",
                                       "joint0/translation",
                                       AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/rotation",
                                       "joint0/rotation",
                                       AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/rotation",
                                       "joint0/rotation",
                                       AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/matrixTransform(1)(3)",
                                       "joint0/matrixTransform(2)(3)",
                                       AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/matrixTransform(1)(3)",
                                       "joint0/matrixTransform(2)(3)",
                                       AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/matrixTransform(1)(3)",
                                       "joint0/matrixTransform(2)(3)",
                                       AK_INPUT_OUT_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/rotationAngleValueA.ANGLE",
                                       "joint0/rotationAngleValueA.ANGLE",
                                       AK_INPUT_OUTPUT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/rotationAngleValueA.ANGLE",
                                       "joint0/rotationAngleValueA.ANGLE",
                                       AK_INPUT_IN_TANGENT);
  AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP("joint0/rotationAngleNamedA.ANGLE",
                                       "joint0/rotationAngleNamedA.ANGLE",
                                       AK_INPUT_OUTPUT);

#undef AK_TEST_DAE_COMPARE_TARGET_ROUNDTRIP

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/translation.Y");
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 1);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -5.0f));
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_IN_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], -1.5f));
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler,
                                                AK_INPUT_OUT_TANGENT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], 0.1f));
  ASSERT(ak_test_dae_float_near(firstPosition[1], -2.5f));

  /* The same scalar sampler is also shared by source Z, whose conversion has
     positive sign.  It must not undo the one negative source-Y conversion. */
  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/translation.Z");
  ASSERT(actualChannel && actualChannel->resolvedTarget);
  ASSERT(actualChannel->resolvedTarget->isPartial);
  ASSERT(actualChannel->resolvedTarget->off == 1u);
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], 2.0f));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/scaling.Y");
  ASSERT(actualChannel && actualChannel->resolvedTarget);
  ASSERT(actualChannel->resolvedTarget->isPartial);
  ASSERT(actualChannel->resolvedTarget->off == 2u);
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], 2.0f));

  actualChannel = ak_test_dae_find_channel(actual->lib.animations.first,
                                            "joint0/rotation.Y");
  ASSERT(actualChannel && actualChannel->resolvedTarget);
  ASSERT(actualChannel->resolvedTarget->isPartial);
  ASSERT(actualChannel->resolvedTarget->off == 2u);
  actualSampler = ak_test_dae_channel_sampler(actualChannel);
  actualAccessor = ak_test_dae_sampler_accessor(actualSampler, AK_INPUT_OUTPUT);
  firstPosition = ak_test_dae_accessor_float_row(actualAccessor, 0);
  ASSERT(firstPosition && ak_test_dae_float_near(firstPosition[0], -0.25f));

  ak_free(actual);
  ak_free(expected);
  ak_free(roundtrip);
  ak_test_export_cleanup(exportDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_load_utf16le) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         gltfPath[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-utf16le-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/utf16.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(gltfPath, sizeof(gltfPath), "%s/utf16.gltf", outDir);

  ASSERT(ak_test_write_dae_utf16le_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->scene != NULL);
  ASSERT(doc->scene->node != NULL);
  ASSERT(doc->scene->node->chld != NULL);
  ASSERT(doc->scene->node->chld->name
         && strcmp(doc->scene->node->chld->name, "Root") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\""));

  ak_free(doc);
  unlink(gltfPath);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae14_missing_surface_sampler_resolves_image) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         texPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;
  FILE        *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-sampler-surface-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/missing_surface.dae", tmpdir);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/missing_surface.dae", outDir);

  ASSERT(ak_test_write_dae14_missing_surface_texture(daePath));
  file = fopen(texPath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("TGADATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "duckCM.tga"));
  ASSERT(ak_test_file_contains(outDae, "<texture texture=\"sampler_0\""));

  ak_free(doc);
  unlink(outDae);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", outDir);
  unlink(texPath);
  snprintf(texPath, sizeof(texPath), "%s/image_0_duckCM.tga", outDir);
  unlink(texPath);
  rmdir(outDir);
  snprintf(texPath, sizeof(texPath), "%s/duckCM.tga", tmpdir);
  unlink(texPath);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae14_broken_texture_ref_recovers_unique_image_and_path) {
  AkDoc           *doc;
  AkMaterial      *material;
  AkTextureRef    *textureRef;
  AkImage         *recoverImage;
  AkImage         *exactImage;
  AkImage         *image;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  char             recoveredTexturePath[PATH_MAX];
  char             exactTexturePath[PATH_MAX];
  char             exactCandidatePath[PATH_MAX];
  char             pathCandidateA[PATH_MAX];
  char             pathCandidateB[PATH_MAX];
  const char      *tmpBase;
  FILE            *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-broken-texture-ref-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/broken_texture_ref.dae", tmpdir);
  snprintf(recoveredTexturePath,
           sizeof(recoveredTexturePath),
           "%s/bbr_metal-2048.dds",
           tmpdir);
  snprintf(exactTexturePath, sizeof(exactTexturePath), "%s/exact.dds", tmpdir);
  snprintf(exactCandidatePath,
           sizeof(exactCandidatePath),
           "%s/bbr_wood-2048.dds",
           tmpdir);
  snprintf(pathCandidateA, sizeof(pathCandidateA), "%s/path_a.dds", tmpdir);
  snprintf(pathCandidateB, sizeof(pathCandidateB), "%s/path_b.dds", tmpdir);

  ASSERT(ak_test_write_dae14_broken_texture_refs(daePath));
  file = fopen(recoveredTexturePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(exactCandidatePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(pathCandidateA, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(pathCandidateB, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);
  file = fopen(exactTexturePath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("DDSDATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  recoverImage = ak_getObjectById(doc, "bbr_metal-2048_dds");
  ASSERT(recoverImage != NULL);
  material = ak_getObjectById(doc, "mat_recover");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == recoverImage);
  ASSERT(recoverImage->source != NULL);
  ASSERT(strcmp(recoverImage->source->uri, "metal-2048.dds") == 0);
  ASSERT(recoverImage->source->resolvedPath != NULL);
  ASSERT(strcmp(recoverImage->source->resolvedPath, recoveredTexturePath) == 0);

  ak_test_dae_image_load_path[0] = '\0';
  ak_imageInitLoader(ak_test_dae_image_loader, NULL);
  ak_imageLoad(recoverImage);
  ak_imageInitLoader(NULL, NULL);
  ASSERT(strcmp(ak_test_dae_image_load_path, recoveredTexturePath) == 0);

  exactImage = ak_getObjectById(doc, "wood-2048_dds");
  ASSERT(exactImage != NULL);
  material = ak_getObjectById(doc, "mat_exact");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == exactImage);
  ASSERT(exactImage->source != NULL);
  ASSERT(exactImage->source->resolvedPath == NULL);
  ASSERT(strcmp(ak_imageResolvePath(exactImage), exactTexturePath) == 0);
  ASSERT(strcmp(exactImage->source->resolvedPath, exactTexturePath) == 0);

  material = ak_getObjectById(doc, "mat_ambiguous");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  image = textureRef->texture->image;
  ASSERT(image == NULL);

  image = ak_getObjectById(doc, "path_a_dds");
  ASSERT(image != NULL);
  material = ak_getObjectById(doc, "mat_path_ambiguous");
  ASSERT(material && material->surface && material->surface->baseColor);
  textureRef = ak_materialInputTexture(material->surface->baseColor);
  ASSERT(textureRef && textureRef->texture);
  ASSERT(textureRef->texture->image == image);
  ASSERT(image->source != NULL);
  ASSERT(image->source->resolvedPath == NULL);

  ak_free(doc);
  unlink(pathCandidateB);
  unlink(pathCandidateA);
  unlink(exactCandidatePath);
  unlink(exactTexturePath);
  unlink(recoveredTexturePath);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae14_nested_init_from_ref_image) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         texDir[PATH_MAX];
  char         texPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outTexDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;
  FILE        *file;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae14-image-ref-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/nested_ref.dae", tmpdir);
  snprintf(texDir, sizeof(texDir), "%s/Textures", tmpdir);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", texDir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outTexDir, sizeof(outTexDir), "%s/Textures", outDir);
  snprintf(outDae, sizeof(outDae), "%s/nested_ref.dae", outDir);

  ASSERT(mkdir(texDir, 0777) == 0);
  ASSERT(ak_test_write_dae14_nested_ref_image(daePath));
  file = fopen(texPath, "wb");
  ASSERT(file != NULL);
  ASSERT(fputs("PNGDATA", file) >= 0);
  ASSERT(fclose(file) == 0);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "WoodFloor-01.png"));

  ak_free(doc);
  unlink(outDae);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", outTexDir);
  unlink(texPath);
  rmdir(outTexDir);
  snprintf(texPath, sizeof(texPath), "%s/image_0_WoodFloor-01.png", outDir);
  unlink(texPath);
  rmdir(outDir);
  snprintf(texPath, sizeof(texPath), "%s/WoodFloor-01.png", texDir);
  unlink(texPath);
  rmdir(texDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
