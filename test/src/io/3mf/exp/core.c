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

static AkDoc *
ak_test_make_3mf_triangle_doc(void) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    2.0f, 3.0f, 4.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!heap || !doc)
    return NULL;
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  if (!scene || !root || !node)
    return NULL;

  node->name  = "3MF Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  if (!geom)
    return NULL;
  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  if (!ak_nodeAttachGeometry(node, geom))
    return NULL;

  return doc;
}

static AkDoc *
ak_test_make_3mf_color_triangle_doc(void) {
  AkDoc          *doc;
  AkHeap         *heap;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInput        *colorInput;
  AkGeometry     *geom;
  const uint8_t colors[12] = {
    255u, 0u,   0u,   255u,
    0u,   255u, 0u,   255u,
    0u,   0u,   255u, 128u
  };

  doc = ak_test_make_3mf_triangle_doc();
  if (!doc)
    return NULL;

  heap = ak_heap_getheap(doc);
  geom = doc->lib.geometries.first;
  mesh = geom && geom->gdata ? ak_objGet(geom->gdata) : NULL;
  prim = mesh ? mesh->primitive : NULL;
  if (!prim)
    return NULL;

  colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
  if (!colorInput)
    return NULL;

  colorInput->semantic = AK_INPUT_COLOR;
  colorInput->set      = 0;
  colorInput->index    = 0;
  colorInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                     colorInput,
                                                     colors,
                                                     4,
                                                     3);
  if (!colorInput->accessor)
    return NULL;
  colorInput->accessor->normalized = false;
  colorInput->next = prim->input;
  prim->input      = colorInput;
  prim->inputCount++;

  return doc;
}

TEST_IMPL(three_mf_export_triangle_roundtrip) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  struct stat      st;
  const char      *outDir  = "./assetkit_export_3mf_triangle_roundtrip";
  const char      *mfPath  = "./assetkit_export_3mf_triangle_roundtrip/model.3mf";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_3mf_triangle_doc();
  ASSERT(doc != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(stat(mfPath, &st) == 0);
  ASSERT(st.st_size > 0);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->coordSys == AK_ZUP);
  ASSERT(roundTrip->unit != NULL);
  ASSERT(fabs(roundTrip->unit->dist - 0.001) < 0.000001);
  ASSERT(roundTrip->lib.geometries.count == 1);

  geom = roundTrip->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->count == 3);
  ASSERT(prim->indices != NULL);
  ASSERT(prim->indices->count == 3);
  ASSERT(ak_indexArrayGet(prim->indices, 0) == 0);
  ASSERT(ak_indexArrayGet(prim->indices, 1) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 2) == 2);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_export_color_triangle_roundtrip) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInput        *input;
  AkInput        *colorInput;
  AkAccessor     *colorAcc;
  const char     *outDir  = "./assetkit_export_3mf_color_triangle_roundtrip";
  const char     *mfPath  = "./assetkit_export_3mf_color_triangle_roundtrip/model.3mf";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_3mf_color_triangle_doc();
  ASSERT(doc != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_3MF) == AK_OK);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->materialProperties.count == 1);
  ASSERT(roundTrip->materialProperties.sets != NULL);
  ASSERT(roundTrip->materialProperties.sets->type == AK_MATERIAL_PROPERTY_COLOR);
  ASSERT(roundTrip->materialProperties.sets->count == 3);
  ASSERT(roundTrip->materialProperties.sets->properties[0].baseColor != NULL);
  ASSERT(roundTrip->materialProperties.sets->properties[0].metallic != NULL);
  ASSERT(roundTrip->materialProperties.sets->properties[0].roughness != NULL);

  geom = roundTrip->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);

  colorInput = NULL;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_COLOR) {
      colorInput = input;
      break;
    }
  }
  ASSERT(colorInput != NULL);
  colorAcc = colorInput->accessor;
  ASSERT(colorAcc != NULL);
  ASSERT(colorAcc->componentType == AKT_UBYTE);
  ASSERT(colorAcc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(colorAcc->count == 3);
  ASSERT(prim->material != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}
