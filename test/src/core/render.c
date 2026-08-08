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

#include "../test_export_common.h"

#include <math.h>

static
bool
ak_test_write_render_instance_dae(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"tri\"><mesh>"
        "<source id=\"pos\"><float_array id=\"pos-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<vertices id=\"verts\"><input semantic=\"POSITION\" source=\"#pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#verts\" offset=\"0\"/>"
        "<p>0 1 2</p></triangles></mesh></geometry></library_geometries>\n"
        "<library_nodes><node id=\"shared\" name=\"Shared\">"
        "<translate>2 0 0</translate><instance_geometry url=\"#tri\"/>"
        "</node></library_nodes>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><translate>3 0 0</translate>"
        "<instance_node url=\"#shared\"/><instance_node url=\"#shared\"/>"
        "</node></visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_render_transform_dae(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"tri\"><mesh>"
        "<source id=\"pos\"><float_array id=\"pos-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"normal\"><float_array id=\"normal-array\" count=\"9\">"
        "0.577350269 0.577350269 0.577350269 "
        "0.577350269 0.577350269 0.577350269 "
        "0.577350269 0.577350269 0.577350269"
        "</float_array><technique_common><accessor source=\"#normal-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"tangent\"><float_array id=\"tangent-array\" count=\"9\">"
        "0.707106781 0.707106781 0 "
        "0.707106781 0.707106781 0 "
        "0.707106781 0.707106781 0"
        "</float_array><technique_common><accessor source=\"#tangent-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<vertices id=\"verts\"><input semantic=\"POSITION\" source=\"#pos\"/></vertices>"
        "<triangles count=\"1\">"
        "<input semantic=\"VERTEX\" source=\"#verts\" offset=\"0\"/>"
        "<input semantic=\"NORMAL\" source=\"#normal\" offset=\"1\"/>"
        "<input semantic=\"TANGENT\" source=\"#tangent\" offset=\"2\"/>"
        "<p>0 0 0 1 1 1 2 2 2</p></triangles></mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"identity\"><instance_geometry url=\"#tri\"/></node>"
        "<node id=\"translated\"><translate>5 6 7</translate><instance_geometry url=\"#tri\"/></node>"
        "<node id=\"mirrored\"><scale>-2 3 4</scale><instance_geometry url=\"#tri\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
AkRenderBatchAttribute*
ak_test_render_attribute(AkRenderBatch *batch, uint32_t semantic) {
  uint32_t i;

  for (i = 0; i < batch->attributeCount; i++) {
    if (batch->attributes[i].semantic == semantic)
      return &batch->attributes[i];
  }
  return NULL;
}

TEST_IMPL(scene_render_data_keeps_instance_dag_compact) {
  AkSceneRenderData *renderData;
  AkRenderGroup     *rootGroup;
  AkRenderGroup     *sharedGroup;
  AkRenderBatch     *batch;
  AkRenderBatchAttribute *position;
  AkDoc             *doc;
  char               dirTemplate[PATH_MAX];
  char              *tmpdir;
  char               daePath[PATH_MAX];
  const char        *tmpBase;
  uint32_t           sharedGroupIndex;
  uint32_t           i;

  doc      = NULL;
  tmpBase  = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-render-data-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  snprintf(daePath, sizeof(daePath), "%s/render.dae", tmpdir);
  ASSERT(ak_test_write_render_instance_dae(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);
  ASSERT(doc->scene != NULL);

  renderData = NULL;
  ASSERT(ak_sceneBuildRenderData(doc->scene, &renderData) == AK_OK);
  ASSERT(renderData != NULL);
  ASSERT(renderData->skippedPrimitiveCount == 0);
  ASSERT(renderData->includedPrimitiveCount == 1);
  ASSERT(renderData->groupCount == 2);
  ASSERT(renderData->rootGroupIndex < renderData->groupCount);

  rootGroup = &renderData->groups[renderData->rootGroupIndex];
  ASSERT(rootGroup->batchCount == 0);
  ASSERT(rootGroup->instanceCount == 2);
  ASSERT(fabsf(rootGroup->instances[0].matrix[12] - 3.0f) < 1e-6f);
  ASSERT(fabsf(rootGroup->instances[1].matrix[12] - 3.0f) < 1e-6f);
  ASSERT(rootGroup->instances[0].targetGroupIndex
         == rootGroup->instances[1].targetGroupIndex);

  sharedGroupIndex = rootGroup->instances[0].targetGroupIndex;
  ASSERT(sharedGroupIndex < renderData->groupCount);
  sharedGroup = &renderData->groups[sharedGroupIndex];
  ASSERT(sharedGroup->batchCount == 1);
  ASSERT(sharedGroup->instanceCount == 0);

  batch = &sharedGroup->batches[0];
  ASSERT(batch->primitiveType == AK_PRIMITIVE_TRIANGLES);
  ASSERT(batch->primitiveCount == 1);
  ASSERT(batch->vertexCount == 3);
  ASSERT(batch->indexCount == 3);
  ASSERT(batch->rangeCount == 1);
  ASSERT(batch->indices[0] == 0);
  ASSERT(batch->indices[1] == 1);
  ASSERT(batch->indices[2] == 2);

  position = NULL;
  for (i = 0; i < batch->attributeCount; i++) {
    if (batch->attributes[i].semantic == AK_INPUT_POSITION) {
      position = &batch->attributes[i];
      break;
    }
  }
  ASSERT(position != NULL);
  ASSERT(position->componentCount == 3);
  ASSERT(position->valueCount == 9);
  ASSERT(fabsf(position->values[0] - 2.0f) < 1e-6f);
  ASSERT(fabsf(position->values[1]) < 1e-6f);
  ASSERT(fabsf(position->values[2]) < 1e-6f);

  ak_sceneRenderDataFree(renderData);
  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(scene_render_data_preserves_identity_and_affine_transforms) {
  AkSceneRenderData      *renderData;
  AkRenderGroup          *group;
  AkRenderBatch          *batch;
  AkRenderBatchAttribute *position;
  AkRenderBatchAttribute *normal;
  AkRenderBatchAttribute *tangent;
  AkDoc                  *doc;
  char                    dirTemplate[PATH_MAX];
  char                   *tmpdir;
  char                    daePath[PATH_MAX];
  const char             *tmpBase;
  uint32_t                jobIndex;
  uint32_t                identityCount;
  uint32_t                translatedCount;
  uint32_t                mirroredCount;

  doc      = NULL;
  tmpBase  = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-render-transform-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  snprintf(daePath, sizeof(daePath), "%s/render.dae", tmpdir);
  ASSERT(ak_test_write_render_transform_dae(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);
  ASSERT(doc->scene != NULL);

  renderData = NULL;
  ASSERT(ak_sceneBuildRenderData(doc->scene, &renderData) == AK_OK);
  ASSERT(renderData != NULL);
  ASSERT(renderData->groupCount == 1);
  ASSERT(renderData->includedPrimitiveCount == 3);
  ASSERT(renderData->skippedPrimitiveCount == 0);

  group = &renderData->groups[renderData->rootGroupIndex];
  ASSERT(group->batchCount == 1);
  batch = &group->batches[0];
  ASSERT(batch->rangeCount == 3);
  ASSERT(batch->vertexCount == 9);
  ASSERT(batch->indexCount == 9);
  ASSERT(batch->primitiveCount == 3);

  position = ak_test_render_attribute(batch, AK_INPUT_POSITION);
  normal   = ak_test_render_attribute(batch, AK_INPUT_NORMAL);
  tangent  = ak_test_render_attribute(batch, AK_INPUT_TANGENT);
  ASSERT(position != NULL && position->componentCount == 3);
  ASSERT(normal != NULL && normal->componentCount == 3);
  ASSERT(tangent != NULL && tangent->componentCount == 3);

  identityCount = 0;
  translatedCount = 0;
  mirroredCount = 0;
  for (jobIndex = 0; jobIndex < 3u; jobIndex++) {
    const float *p;
    const float *n;
    const float *t;
    uint32_t     baseVertex;
    uint32_t     firstIndex;

    baseVertex = jobIndex * 3u;
    firstIndex = batch->ranges[jobIndex].firstIndex;
    p = position->values + (size_t)baseVertex * 3u;
    n = normal->values + (size_t)baseVertex * 3u;
    t = tangent->values + (size_t)baseVertex * 3u;

    if (fabsf(p[0]) < 1e-6f && fabsf(p[3] - 1.0f) < 1e-6f) {
      identityCount++;
      ASSERT(fabsf(p[1]) < 1e-6f && fabsf(p[2]) < 1e-6f);
      ASSERT(fabsf(p[4]) < 1e-6f && fabsf(p[5]) < 1e-6f);
      ASSERT(fabsf(p[6]) < 1e-6f && fabsf(p[7] - 1.0f) < 1e-6f);
      ASSERT(fabsf(p[8]) < 1e-6f);
      ASSERT(fabsf(n[0] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(n[1] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(n[2] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(t[0] - 0.707106781f) < 1e-6f);
      ASSERT(fabsf(t[1] - 0.707106781f) < 1e-6f);
      ASSERT(fabsf(t[2]) < 1e-6f);
      ASSERT(batch->indices[firstIndex] == baseVertex);
      ASSERT(batch->indices[firstIndex + 1u] == baseVertex + 1u);
      ASSERT(batch->indices[firstIndex + 2u] == baseVertex + 2u);
    } else if (fabsf(p[0] - 5.0f) < 1e-6f) {
      translatedCount++;
      ASSERT(fabsf(p[1] - 6.0f) < 1e-6f);
      ASSERT(fabsf(p[2] - 7.0f) < 1e-6f);
      ASSERT(fabsf(p[3] - 6.0f) < 1e-6f);
      ASSERT(fabsf(p[4] - 6.0f) < 1e-6f);
      ASSERT(fabsf(p[5] - 7.0f) < 1e-6f);
      ASSERT(fabsf(p[6] - 5.0f) < 1e-6f);
      ASSERT(fabsf(p[7] - 7.0f) < 1e-6f);
      ASSERT(fabsf(p[8] - 7.0f) < 1e-6f);
      ASSERT(fabsf(n[0] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(n[1] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(n[2] - 0.577350269f) < 1e-6f);
      ASSERT(fabsf(t[0] - 0.707106781f) < 1e-6f);
      ASSERT(fabsf(t[1] - 0.707106781f) < 1e-6f);
      ASSERT(fabsf(t[2]) < 1e-6f);
      ASSERT(batch->indices[firstIndex] == baseVertex);
      ASSERT(batch->indices[firstIndex + 1u] == baseVertex + 1u);
      ASSERT(batch->indices[firstIndex + 2u] == baseVertex + 2u);
    } else {
      float normalLength;
      float tangentLength;

      mirroredCount++;
      ASSERT(fabsf(p[0]) < 1e-6f);
      ASSERT(fabsf(p[1]) < 1e-6f);
      ASSERT(fabsf(p[2]) < 1e-6f);
      ASSERT(fabsf(p[3] + 2.0f) < 1e-6f);
      ASSERT(fabsf(p[4]) < 1e-6f && fabsf(p[5]) < 1e-6f);
      ASSERT(fabsf(p[6]) < 1e-6f && fabsf(p[7] - 3.0f) < 1e-6f);
      ASSERT(fabsf(p[8]) < 1e-6f);

      normalLength = sqrtf(0.25f + 1.0f / 9.0f + 1.0f / 16.0f);
      ASSERT(fabsf(n[0] - (-0.5f / normalLength)) < 1e-6f);
      ASSERT(fabsf(n[1] - ((1.0f / 3.0f) / normalLength)) < 1e-6f);
      ASSERT(fabsf(n[2] - (0.25f / normalLength)) < 1e-6f);

      tangentLength = sqrtf(0.25f + 1.0f / 9.0f);
      ASSERT(fabsf(t[0] - (-0.5f / tangentLength)) < 1e-6f);
      ASSERT(fabsf(t[1] - ((1.0f / 3.0f) / tangentLength)) < 1e-6f);
      ASSERT(fabsf(t[2]) < 1e-6f);

      ASSERT(batch->indices[firstIndex] == baseVertex + 2u);
      ASSERT(batch->indices[firstIndex + 1u] == baseVertex + 1u);
      ASSERT(batch->indices[firstIndex + 2u] == baseVertex);
    }
  }
  ASSERT(identityCount == 1);
  ASSERT(translatedCount == 1);
  ASSERT(mirroredCount == 1);

  ak_sceneRenderDataFree(renderData);
  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
