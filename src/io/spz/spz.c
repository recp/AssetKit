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

#include "spz.h"
#include "../common/util.h"
#include "../common/postscript.h"
#include "../../platform/dylib.h"
#include "../../id.h"
#include "../../../include/ak/path.h"

AK_HIDE
bool
spz_decoderOpen(struct AkGLTFSPZLib *sp) {
  AkGaussianSplatDecoderCreateFn create;
  const char                   *path;

  path = (const char *)ak_opt_get(AK_OPT_GLTF_GSPLAT_DECODER_PATH);
  if (path)
    sp->lib = ak_dylib_open(path);

  if (!sp->lib && ak_opt_get(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD))
    sp->lib = ak_dylib_openName("assetkit_spz");

  if (!sp->lib)
    return false;

  create = (AkGaussianSplatDecoderCreateFn)ak_dylib_sym(sp->lib, "assetkit_gsplat_create");
  if (!create || create(&sp->decoder) != 0 || !sp->decoder.decodeBytes) {
    spz_decoderClose(sp);
    return false;
  }

  return true;
}

AK_HIDE
void
spz_decoderClose(struct AkGLTFSPZLib *sp) {
  if (sp->decoder.close)
    sp->decoder.close(sp->decoder.userdata);

  if (sp->lib)
    ak_dylib_close(sp->lib);

  memset(sp, 0, sizeof(*sp));
}

AK_HIDE
AkResult
spz_doc(AkDoc **dest, const char *filepath) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  void              *bytes;
  struct AkGLTFSPZLib sp;
  size_t             size;
  AkResult           result;

  memset(&sp, 0, sizeof(sp));

  *dest  = NULL;
  doc    = NULL;
  bytes  = NULL;
  size   = 0;
  result = AK_ERR;

  if (ak_readfile(filepath, NULL, &bytes, &size) != AK_OK || !size)
    goto cleanup;

  if (!spz_decoderOpen(&sp))
    goto cleanup;

  if (!(heap = ak_heap_new(NULL, NULL, NULL)))
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
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->ftype         = AK_FILE_TYPE_SPZ;
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
      || !(prim = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*prim))))
    goto cleanup;

  prim->type           = AK_PRIMITIVE_POINTS;
  prim->indexStride    = 1;
  prim->mesh           = mesh;
  mesh->primitive      = prim;
  mesh->primitiveCount = 1;

  if (sp.decoder.decodeBytes(heap, prim, bytes, size) != 0 || !prim->pos)
    goto cleanup;

  prim->nPolygons = prim->pos->accessor->count;
  io_postscript(doc);

  *dest  = doc;
  result = AK_OK;

cleanup:
  spz_decoderClose(&sp);

  if (bytes)
    ak_releasefile(bytes, size);

  if (result != AK_OK && doc)
    ak_free(doc);

  return result;
}
