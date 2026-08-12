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
#include "../../../../../src/string_fast.h"

#include <ak/string.h>

typedef enum AkTestDaeSkinWeightsCase {
  AK_TEST_DAE_SKIN_DENSE_SIX,
  AK_TEST_DAE_SKIN_SPARSE_INPUTS,
  AK_TEST_DAE_SKIN_UNSUPPORTED_INPUT,
  AK_TEST_DAE_SKIN_INVALID_FLOAT,
  AK_TEST_DAE_SKIN_SHORT_V,
  AK_TEST_DAE_SKIN_HUGE_OFFSET,
  AK_TEST_DAE_SKIN_SHORT_WEIGHT_BUFFER,
  AK_TEST_DAE_SKIN_HUGE_DECLARED_WEIGHT_ARRAY,
  AK_TEST_DAE_SKIN_DUPLICATE_WEIGHT_OVERFLOW,
  AK_TEST_DAE_SKIN_INVALID_REQUIRED_OFFSET,
  AK_TEST_DAE_SKIN_PARTIAL_PRIMITIVE_FAILURE
} AkTestDaeSkinWeightsCase;

static const char *ak_test_dae_identity4x4 =
  "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1";

static
bool
ak_test_write_dae_skin_weights_case(const char                  *path,
                                    AkTestDaeSkinWeightsCase     testCase) {
  const char *inputs;
  const char *weights;
  const char *vcount;
  const char *v;
  const char *primitives;
  uint32_t    weightArrayCount;
  uint32_t    weightAccessorCount;
  FILE       *file;
  uint32_t    i;

  inputs = "<input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
           "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>";
  weights = "0.05 0.1 0.15 0.2 0.23 0.27";
  weightArrayCount = 6;
  weightAccessorCount = 6;
  vcount = "6 1 1";
  v = "0 0 1 1 2 2 3 3 4 4 5 5 0 5 0 5";
  primitives =
    "<triangles count=\"1\"><input semantic=\"VERTEX\" "
    "source=\"#verts\" offset=\"0\"/><p>0 1 2</p></triangles>";

  switch (testCase) {
    case AK_TEST_DAE_SKIN_SPARSE_INPUTS:
      inputs =
        "<input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"2\"/>";
      vcount = "1 1 1";
      v = "0 99 0 1 99 1 2 99 2";
      break;
    case AK_TEST_DAE_SKIN_UNSUPPORTED_INPUT:
      inputs =
        "<input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<input semantic=\"CUSTOM\" source=\"#skin-weights\" offset=\"7\"/>";
      vcount = "1 1 1";
      v = "0 0 99 99 99 99 99 0 "
          "1 1 99 99 99 99 99 0 "
          "2 2 99 99 99 99 99 0";
      break;
    case AK_TEST_DAE_SKIN_INVALID_FLOAT:
      weights = "-1 NaN 2";
      weightArrayCount = 3;
      weightAccessorCount = 3;
      vcount = "1 1 1";
      v = "0 0 0 1 0 2";
      break;
    case AK_TEST_DAE_SKIN_SHORT_V:
      vcount = "2 2 2";
      v = "0 0 1 1 2 2";
      break;
    case AK_TEST_DAE_SKIN_HUGE_OFFSET:
      inputs =
        "<input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<input semantic=\"CUSTOM\" source=\"#skin-weights\" "
        "offset=\"4294967295\"/>";
      vcount = "1 1 1";
      v = "0 0 1 1 2 2";
      break;
    case AK_TEST_DAE_SKIN_SHORT_WEIGHT_BUFFER:
      weights = "1";
      weightArrayCount = 1;
      weightAccessorCount = 3;
      vcount = "1 1 1";
      v = "0 0 0 1 0 2";
      break;
    case AK_TEST_DAE_SKIN_HUGE_DECLARED_WEIGHT_ARRAY:
      weights = "1";
      weightArrayCount = UINT32_MAX;
      weightAccessorCount = 3;
      vcount = "1 1 1";
      v = "0 0 0 1 0 2";
      break;
    case AK_TEST_DAE_SKIN_DUPLICATE_WEIGHT_OVERFLOW:
      weights = "3e38 3e38 1";
      weightArrayCount = 3;
      weightAccessorCount = 3;
      vcount = "2 1 1";
      v = "0 0 0 1 0 2 0 2";
      break;
    case AK_TEST_DAE_SKIN_INVALID_REQUIRED_OFFSET:
      inputs =
        "<input semantic=\"JOINT\" source=\"#skin-joints\" "
        "offset=\"4294967295\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"0\"/>";
      vcount = "1 1 1";
      v = "0 0 0 1 0 2";
      break;
    case AK_TEST_DAE_SKIN_PARTIAL_PRIMITIVE_FAILURE:
      vcount = "1 1 1";
      v = "0 0 1 1 2 2";
      primitives =
        "<triangles count=\"1\"><input semantic=\"VERTEX\" "
        "source=\"#verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "<triangles count=\"1\"><p>0 1 2</p></triangles>";
      break;
    case AK_TEST_DAE_SKIN_DENSE_SIX:
      break;
  }

  file = fopen(path, "wb");
  if (!file)
    return false;

  fprintf(file,
          "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
          "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
          "version=\"1.4.1\">"
          "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis>"
          "</asset>"
          "<library_geometries><geometry id=\"geom\"><mesh>"
          "<source id=\"pos\"><float_array id=\"pos-array\" count=\"9\">"
          "0 0 0 1 0 0 0 1 0</float_array><technique_common>"
          "<accessor source=\"#pos-array\" count=\"3\" stride=\"3\">"
          "<param name=\"X\" type=\"float\"/>"
          "<param name=\"Y\" type=\"float\"/>"
          "<param name=\"Z\" type=\"float\"/>"
          "</accessor></technique_common></source>"
          "<vertices id=\"verts\"><input semantic=\"POSITION\" "
          "source=\"#pos\"/></vertices>"
          "%s"
          "</mesh></geometry></library_geometries>"
          "<library_controllers><controller id=\"skin\"><skin source=\"#geom\">"
          "<bind_shape_matrix>%s</bind_shape_matrix>"
          "<source id=\"skin-joints\"><Name_array id=\"skin-joints-array\" "
          "count=\"6\">j0 j1 j2 j3 j4 j5</Name_array><technique_common>"
          "<accessor source=\"#skin-joints-array\" count=\"6\" stride=\"1\">"
          "<param name=\"JOINT\" type=\"Name\"/>"
          "</accessor></technique_common></source>"
          "<source id=\"skin-bind\"><float_array id=\"skin-bind-array\" "
          "count=\"96\">",
          primitives,
          ak_test_dae_identity4x4);

  for (i = 0; i < 6; i++)
    fprintf(file, "%s%s", i ? " " : "", ak_test_dae_identity4x4);

  fprintf(file,
          "</float_array><technique_common><accessor source=\"#skin-bind-array\" "
          "count=\"6\" stride=\"16\"><param name=\"TRANSFORM\" "
          "type=\"float4x4\"/></accessor></technique_common></source>"
          "<source id=\"skin-weights\"><float_array id=\"skin-weights-array\" "
          "count=\"%u\">%s</float_array><technique_common>"
          "<accessor source=\"#skin-weights-array\" count=\"%u\" stride=\"1\">"
          "<param name=\"WEIGHT\" type=\"float\"/>"
          "</accessor></technique_common></source>"
          "<joints><input semantic=\"JOINT\" source=\"#skin-joints\"/>"
          "<input semantic=\"INV_BIND_MATRIX\" source=\"#skin-bind\"/>"
          "</joints><vertex_weights count=\"3\">%s<vcount>%s</vcount>"
          "<v>%s</v></vertex_weights></skin></controller></library_controllers>"
          "<library_visual_scenes><visual_scene id=\"Scene\">"
          "<node id=\"root\" name=\"root\">",
          weightArrayCount,
          weights,
          weightAccessorCount,
          inputs,
          vcount,
          v);

  for (i = 0; i < 6; i++) {
    fprintf(file,
            "<node id=\"node%u\" sid=\"j%u\" name=\"joint%u\" "
            "type=\"JOINT\"/>",
            i,
            i,
            i);
  }

  fputs("</node><node id=\"meshNode\"><instance_controller url=\"#skin\">"
        "<skeleton>#root</skeleton></instance_controller></node>"
        "</visual_scene></library_visual_scenes>"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_short_angle_buffer(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
        "version=\"1.4.1\"><asset><unit name=\"meter\" meter=\"1\"/>"
        "<up_axis>Y_UP</up_axis></asset><library_animations>"
        "<animation id=\"angle\"><source id=\"time\">"
        "<float_array id=\"time-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#time-array\" count=\"2\" "
        "stride=\"1\"><param name=\"TIME\" type=\"float\"/>"
        "</accessor></technique_common></source><source id=\"angle-output\">"
        "<float_array id=\"angle-array\" count=\"1\">90</float_array>"
        "<technique_common><accessor source=\"#angle-array\" count=\"2\" "
        "stride=\"1\"><param name=\"ANGLE\" type=\"float\"/>"
        "</accessor></technique_common></source><sampler id=\"angle-sampler\">"
        "<input semantic=\"INPUT\" source=\"#time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#angle-output\"/>"
        "</sampler><channel source=\"#angle-sampler\" "
        "target=\"node/rotate.ANGLE\"/></animation></library_animations>"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\"><rotate sid=\"rotate\">0 0 1 0</rotate></node>"
        "</visual_scene></library_visual_scenes>"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene></COLLADA>",
        file);

  return fclose(file) == 0;
}

static
AkDoc*
ak_test_load_dae_skin_case(const char *path, AkResult *result) {
  AkDoc    *doc;
  uintptr_t previousCoordCvtType;

  doc = NULL;
  previousCoordCvtType = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  *result = ak_load(&doc, path, AK_FILE_TYPE_DAE);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, previousCoordCvtType);

  return doc;
}

static
AkBoneWeights*
ak_test_dae_first_skin_weights(AkSkin *skin) {
  if (!skin || !skin->weights || skin->nPrims == 0)
    return NULL;
  return skin->weights[0];
}

TEST_IMPL(float_array_invalid_token_alignment) {
  static const char *invalidCases[] = {
    "-1 NaN 2",
    "-1 +NaN 2",
    "-1 INF 2",
    "-1 1foo 2",
    "-1 1eNaN 2",
    "-1 1e 2",
    "-1 1e+ 2",
    "-1 1#IND 2",
    "-1 1foo'bar 2",
    "-1 1foo\"bar 2"
  };
  char  bounded[32];
  char  unbounded[32];
  float values[3];
  size_t i;

  for (i = 0; i < sizeof(invalidCases) / sizeof(invalidCases[0]); i++) {
    snprintf(bounded, sizeof(bounded), "%s", invalidCases[i]);
    snprintf(unbounded, sizeof(unbounded), "%s", invalidCases[i]);

    values[0] = values[1] = values[2] = 99.0f;
    ASSERT(ak_strtof(bounded, strlen(bounded), 3, values) == 0);
    ASSERT(values[0] == -1.0f);
    ASSERT(values[1] == 0.0f);
    ASSERT(values[2] == 2.0f);

    values[0] = values[1] = values[2] = 99.0f;
    ASSERT(ak_strtof(unbounded, 0, 3, values) == 0);
    ASSERT(values[0] == -1.0f);
    ASSERT(values[1] == 0.0f);
    ASSERT(values[2] == 2.0f);
  }

  snprintf(bounded, sizeof(bounded), "%s", "-1 1e+2 2");
  ASSERT(ak_strtof(bounded, strlen(bounded), 3, values) == 0);
  ASSERT(values[0] == -1.0f);
  ASSERT(values[1] == 100.0f);
  ASSERT(values[2] == 2.0f);

  {
    static const char junkToken[] = "1foo";
    static const char exponentToken[] = "1e";
    static const char junkQuoted[] = "1foo\"";
    static const char validToken[] = "1";
    const char *after;
    float       value;

    ASSERT(!ak_str_parse_float_token_fast(junkToken,
                                           junkToken + 4,
                                           &value,
                                           &after));
    ASSERT(!ak_str_parse_float_token_fast(exponentToken,
                                           exponentToken + 2,
                                           &value,
                                           &after));
    ASSERT(!ak_str_parse_float_quoted_fast(junkQuoted,
                                           junkQuoted + 5,
                                           '\"',
                                           &value,
                                           &after));
    ASSERT(ak_str_parse_float_token_fast(validToken,
                                         validToken + 1,
                                         &value,
                                         &after));
    ASSERT(after == validToken + 1 && value == 1.0f);
  }

  TEST_SUCCESS
}

TEST_IMPL(dae_skin_vertex_weight_tolerance) {
  AkDoc         *doc;
  AkSkin        *skin;
  AkBoneWeights *weights;
  AkResult       result;
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
                           "assetkit-dae-skin-weights-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "skin.dae"));

  ASSERT(ak_test_write_dae_skin_weights_case(daePath,
                                             AK_TEST_DAE_SKIN_DENSE_SIX));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc && doc->lib.skins.count == 1);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(skin->nMaxJoints == 6);
  ASSERT(weights && weights->nVertex == 3 && weights->nWeights == 8);
  ASSERT(weights->counts[0] == 6);
  ASSERT(weights->weights[weights->indexes[0] + 5].joint == 5);
  ASSERT(weights->weights[weights->indexes[0] + 5].weight == 0.27f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(daePath,
                                             AK_TEST_DAE_SKIN_SPARSE_INPUTS));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(skin->nMaxJoints == 1);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->counts[0] == 1 && weights->counts[1] == 1
         && weights->counts[2] == 1);
  ASSERT(weights->weights[weights->indexes[0]].joint == 0);
  ASSERT(weights->weights[weights->indexes[1]].joint == 1);
  ASSERT(weights->weights[weights->indexes[2]].joint == 2);
  ASSERT(weights->weights[weights->indexes[2]].weight == 0.15f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_UNSUPPORTED_INPUT));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(skin->nMaxJoints == 1);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->weights[weights->indexes[0]].joint == 0);
  ASSERT(weights->weights[weights->indexes[1]].joint == 1);
  ASSERT(weights->weights[weights->indexes[2]].joint == 2);
  ASSERT(weights->weights[weights->indexes[2]].weight == 0.15f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(daePath,
                                             AK_TEST_DAE_SKIN_INVALID_FLOAT));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->weights[weights->indexes[0]].weight == -1.0f);
  ASSERT(weights->weights[weights->indexes[1]].weight == 0.0f);
  ASSERT(weights->weights[weights->indexes[2]].weight == 2.0f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(daePath,
                                             AK_TEST_DAE_SKIN_SHORT_V));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(skin->nMaxJoints == 2);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->counts[0] == 2);
  ASSERT(weights->counts[1] == 1);
  ASSERT(weights->counts[2] == 0);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(daePath,
                                             AK_TEST_DAE_SKIN_HUGE_OFFSET));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(skin->nMaxJoints == 0);
  ASSERT(weights && weights->nWeights == 0);
  ASSERT(weights->counts[0] == 0 && weights->counts[1] == 0
         && weights->counts[2] == 0);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_SHORT_WEIGHT_BUFFER));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->weights[weights->indexes[0]].weight == 1.0f);
  ASSERT(weights->weights[weights->indexes[1]].weight == 0.0f);
  ASSERT(weights->weights[weights->indexes[2]].weight == 0.0f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_HUGE_DECLARED_WEIGHT_ARRAY));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(weights && weights->nWeights == 3);
  ASSERT(weights->weights[weights->indexes[0]].weight == 1.0f);
  ASSERT(weights->weights[weights->indexes[1]].weight == 0.0f);
  ASSERT(weights->weights[weights->indexes[2]].weight == 0.0f);
  ak_free(doc);

  ASSERT(ak_test_write_dae_short_angle_buffer(daePath));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  ASSERT(doc->lib.animations.first != NULL);
  ASSERT(doc->lib.animations.first->sampler != NULL);
  ASSERT(doc->lib.animations.first->sampler->outputInput != NULL);
  ASSERT(doc->lib.animations.first->sampler->outputInput->accessor != NULL);
  ASSERT(doc->lib.animations.first->sampler->outputInput->accessor->buffer
         != NULL);
  ASSERT(doc->lib.animations.first->sampler->outputInput->accessor
           ->buffer->length == sizeof(float));
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_INVALID_REQUIRED_OFFSET));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 0 && doc->lib.skins.first == NULL);
  {
    AkGeometry *geom;
    AkMesh     *mesh;

    geom = doc->lib.geometries.first;
    mesh = geom ? ak_objGet(geom->gdata) : NULL;
    ASSERT(mesh && mesh->skins == NULL);
  }
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_PARTIAL_PRIMITIVE_FAILURE));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  ASSERT(doc->lib.skins.count == 0 && doc->lib.skins.first == NULL);
  {
    AkGeometry *geom;
    AkMesh     *mesh;

    geom = doc->lib.geometries.first;
    mesh = geom ? ak_objGet(geom->gdata) : NULL;
    ASSERT(mesh && mesh->primitiveCount == 2 && mesh->skins == NULL);
  }
  ak_free(doc);

  ASSERT(ak_test_write_dae_skin_weights_case(
    daePath,
    AK_TEST_DAE_SKIN_DUPLICATE_WEIGHT_OVERFLOW));
  doc = ak_test_load_dae_skin_case(daePath, &result);
  ASSERT(result == AK_OK && doc);
  skin = doc->lib.skins.first;
  weights = ak_test_dae_first_skin_weights(skin);
  ASSERT(weights && weights->counts[0] == 2);
  {
    AkGeometry      *geom;
    AkMesh          *mesh;
    AkMeshPrimitive *prim;
    unsigned char   *interleaved;
    float           *rowWeights;
    void            *buffer;
    size_t           bytes;

    geom = doc->lib.geometries.first;
    mesh = geom ? ak_objGet(geom->gdata) : NULL;
    prim = mesh ? mesh->primitive : NULL;
    buffer = NULL;
    bytes = ak_skinInterleave(skin, prim, 0u, 2u, &buffer);
    ASSERT(bytes == 3u * (2u * sizeof(uint16_t) + 2u * sizeof(float)));
    ASSERT(buffer != NULL);
    interleaved = buffer;
    rowWeights = (float *)(interleaved + 2u * sizeof(uint16_t));
    ASSERT(isfinite(rowWeights[0]) && isfinite(rowWeights[1]));
    ASSERT(fabsf(rowWeights[0] - 1.0f) < 1e-6f);
    ASSERT(rowWeights[1] == 0.0f);
    ak_free(buffer);
  }
  ak_free(doc);

  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
