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

#include "lgsc.h"
#include "../common/postscript.h"
#include "../../id.h"
#include "../../../include/ak/path.h"

AK_HIDE
AkResult
lgsc_doc(AkDoc **dest, const char *filepath) {
  static const AkInputSemantic semantics[] = {AK_INPUT_POSITION, AK_INPUT_ROTATION,
                                              AK_INPUT_SCALE, AK_INPUT_OPACITY};
  static const char *names[] = {"POSITION", "ROTATION", "SCALE", "OPACITY"};
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *acc;
  AkInput         *input;
  void            *bytes;
  size_t           size;
  AkLGSCData       data;
  uint32_t         i;
  AkResult         result;

  *dest  = NULL;
  doc    = NULL;
  bytes  = NULL;
  size   = 0;
  result = AK_ERR;

  if (ak_readfile(filepath, NULL, &bytes, &size) != AK_OK || !size
      || !(heap = ak_heap_new(NULL, NULL, NULL)))
    goto cleanup;

  if (!(doc = ak_heap_calloc(heap, NULL, sizeof(*doc)))) {
    ak_heap_destroy(heap);
    goto cleanup;
  }

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  if (!(doc->inf = ak_heap_calloc(heap, doc, sizeof(*doc->inf))))
    goto cleanup;

  doc->inf->name          = ak_heap_strdup(heap, doc->inf, filepath);
  doc->inf->dir           = ak_path_dir(heap, doc->inf, filepath);
  doc->inf->dirlen        = doc->inf->dir ? strlen(doc->inf->dir) : 0;
  doc->inf->ftype         = AK_FILE_TYPE_LGSC;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP;

  if (!(scene = ak_heap_calloc(heap, doc, sizeof(*scene)))
      || !(scene->node = ak_heap_calloc(heap, scene, sizeof(*scene->node)))
      || !(node = ak_heap_calloc(heap, doc, sizeof(*node))))
    goto cleanup;

  ak_setypeid(scene->node, AKT_NODE);
  ak_setypeid(node, AKT_NODE);
  scene->node->visible = true;
  node->visible        = true;
  AK_LIB_PREPEND(doc->lib.nodes, node, docNext);
  ak_addSubNode(scene->node, node, false);
  AK_LIB_PREPEND(doc->lib.scenes, scene, next);
  doc->scene = scene;

  if (!(mesh = ak_allocMeshEx(heap, doc, &geom, false)))
    goto cleanup;

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);

  if (!ak_nodeAttachGeometry(node, geom)
      || !(prim = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*prim)))
      || !(prim->gsplat = ak_heap_calloc(heap, prim, sizeof(*prim->gsplat))))
    goto cleanup;

  prim->type           = AK_PRIMITIVE_POINTS;
  prim->indexStride    = 1;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  /* Standalone v1.0 stores all degree-three fields and has no SH-degree
     or coordinate metadata. Its source convention is INRIA/Graphdeco RDF. */
  if (!lgsc_decode(heap, doc, bytes, size, 3, 0, true, &data))
    goto cleanup;

  AK_LIB_PREPEND(doc->lib.buffers, data.buffer, next);

  for (i = 0; i < 20; i++) {
    if (!(acc = ak_heap_calloc(heap, doc, sizeof(*acc)))
        || !(input = ak_heap_calloc(heap, prim, sizeof(*input))))
      goto cleanup;

    ak_setypeid(acc, AKT_ACCESSOR);
    lgsc_accessor(acc, &data, i);
    AK_LIB_PREPEND(doc->lib.accessors, acc, next);

    input->accessor    = acc;
    input->semantic    = i < 4 ? semantics[i] : AK_INPUT_SH;
    input->semanticRaw = i < 4 ? names[i] : "SH";
    input->set         = i < 4 ? 0 : i - 4;
    input->next        = prim->input;
    prim->input        = input;
    prim->inputCount++;

    if (!i)
      prim->pos = input;
  }

  prim->gsplat->kernel       = AK_GSPLAT_KERNEL_ELLIPSE;
  prim->gsplat->colorSpace   = AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
  prim->gsplat->decodedCount = data.count;
  prim->gsplat->shDegree     = 3;
  prim->nPolygons            = data.count;
  io_postscript(doc);

  *dest  = doc;
  result = AK_OK;

cleanup:
  if (bytes)
    ak_releasefile(bytes, size);
  if (result != AK_OK && doc)
    ak_free(doc);

  return result;
}
