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

#include "test_common.h"
#include "../../src/mem/common.h"
#include "../../src/mem/lt.h"

#include <limits.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

extern AkHeapAllocator ak__allocator;

TEST_IMPL(heap) {
  AkHeap  *heap, *other, staticHeap;
  uint32_t heapid, data;

  heap = ak_heap_new(NULL, NULL, NULL);
  ASSERT(heap->allocator == &ak__allocator);
  ASSERT(ak_heap_allocator(heap) == &ak__allocator);

  heapid = heap->heapid;
  ASSERT(heapid > 0);
  ASSERT(ak_heap_lt_find(heap->heapid) == heap);

  other = ak_heap_new(NULL, NULL, NULL);

  ak_heap_attach(heap, other);
  ASSERT(heap->chld == other);

  ak_heap_dettach(heap, other);
  ASSERT(heap->chld == NULL);

  ak_heap_attach(heap, other);
  ASSERT(heap->chld == other);

  ak_heap_setdata(heap, &data);
  ASSERT(ak_heap_data(heap) == &data);

  ak_heap_destroy(heap);
  ASSERT(ak_heap_lt_find(heapid) == NULL);

  ak_heap_init(&staticHeap, NULL, NULL, NULL);
  ASSERT(staticHeap.heapid > 0);

  ak_heap_lt_remove(staticHeap.heapid);
  ASSERT(ak_heap_lt_find(staticHeap.heapid) == NULL);

  TEST_SUCCESS
}

TEST_IMPL(heap_multiple) {
  AkHeap  *heap, *root;
  uint32_t i;

  root = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc, leak */
  for (i = 0; i < 1000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc 2, leak */
  for (i = 0; i < 1000; i++)
    heap = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc-free 1 */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc-free 2 */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_destroy(heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  /* multiple alloc, attach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
  }

  ak_heap_destroy(root);

  root = ak_heap_new(NULL, NULL, NULL);

  /* multiple alloc, attach-detach to parent */
  for (i = 0; i < 1000; i++) {
    heap = ak_heap_new(NULL, NULL, NULL);
    ak_heap_attach(root, heap);
    ak_heap_dettach(root, heap);
  }

  TEST_SUCCESS
}

typedef struct AkTestIndexStats {
  uint32_t primitiveCount;
  uint32_t ownedCount;
  uint32_t accessorCount;
  uint32_t u8Count;
  uint32_t u16Count;
  uint32_t u32Count;
} AkTestIndexStats;

static
bool
ak_test_write_obj(const char *path) {
  FILE     *file;
  uint32_t  i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  for (i = 0; i < 300; i++)
    fprintf(file, "v %u 0 0\n", i);
  fputs("f 1 2 3\n", file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply(const char *path) {
  FILE     *file;
  uint32_t  i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 300\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "element face 1\n"
        "property list uchar int vertex_indices\n"
        "end_header\n",
        file);
  for (i = 0; i < 300; i++)
    fprintf(file, "%u 0 0\n", i);
  fputs("3 0 1 2\n", file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae(const char *path) {
  FILE     *file;
  uint32_t  i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"geom\"><mesh>\n"
        "<source id=\"pos\"><float_array id=\"pos-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common>"
        "<accessor source=\"#pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/>"
        "<param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>\n"
        "<vertices id=\"verts\"><input semantic=\"POSITION\" source=\"#pos\"/></vertices>\n"
        "<triangles count=\"100\"><input semantic=\"VERTEX\" source=\"#verts\" offset=\"0\"/><p>",
        file);

  for (i = 0; i < 100; i++)
    fputs("0 1 2 ", file);

  fputs("</p></triangles>\n"
        "</mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\"><node id=\"node\">"
        "<instance_geometry url=\"#geom\"/></node></visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf(const char *path, const char *binPath) {
  FILE    *file;
  uint8_t  buffer[76];
  float    positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  float    normals[9] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f
  };

  memset(buffer, 0, sizeof(buffer));
  memcpy(buffer, positions, sizeof(positions));
  buffer[36] = 0;
  buffer[37] = 1;
  buffer[38] = 2;
  memcpy(buffer + 40, normals, sizeof(normals));

  file = fopen(binPath, "wb");
  if (!file)
    return false;
  if (fwrite(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
    fclose(file);
    return false;
  }
  if (fclose(file) != 0)
    return false;

  file = fopen(path, "wb");
  if (!file)
    return false;
  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"buffers\":[{\"uri\":\"index_stats.bin\",\"byteLength\":76}],"
        "\"bufferViews\":["
        "{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36},"
        "{\"buffer\":0,\"byteOffset\":36,\"byteLength\":3},"
        "{\"buffer\":0,\"byteOffset\":40,\"byteLength\":36}],"
        "\"accessors\":["
        "{\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"},"
        "{\"bufferView\":1,\"componentType\":5121,\"count\":3,\"type\":\"SCALAR\"},"
        "{\"bufferView\":2,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\"}],"
        "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0,\"NORMAL\":2},\"indices\":1}]}],"
        "\"nodes\":[{\"mesh\":0}],"
        "\"scenes\":[{\"nodes\":[0]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
void
ak_test_collect_index_stats(AkDoc *doc, AkTestIndexStats *stats) {
  AkGeometry *geom;

  memset(stats, 0, sizeof(*stats));

  for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (!mesh)
      continue;

    for (prim = mesh->primitive; prim; prim = prim->next) {
      AkTypeId componentType;

      componentType = ak_meshPrimitiveIndexComponentType(prim);
      stats->primitiveCount++;
      if (prim->indices)
        stats->ownedCount++;
      if (prim->indexAccessor)
        stats->accessorCount++;

      switch (componentType) {
        case AKT_UBYTE:  stats->u8Count++;  break;
        case AKT_USHORT: stats->u16Count++; break;
        case AKT_UINT:   stats->u32Count++; break;
        default:                         break;
      }
    }
  }
}

static
bool
ak_test_load_index_stats(const char       *path,
                         AkTestIndexStats *stats) {
  AkDoc *doc;

  doc = NULL;
  if (ak_load(&doc, path, AK_FILE_TYPE_AUTO) != AK_OK || !doc)
    return false;

  ak_test_collect_index_stats(doc, stats);
  ak_free(doc);

  return true;
}

TEST_IMPL(index_stats_corpus) {
  AkTestIndexStats stats;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             daePath[PATH_MAX];
  char             objPath[PATH_MAX];
  char             plyPath[PATH_MAX];
  char             gltfPath[PATH_MAX];
  char             binPath[PATH_MAX];
  const char      *tmpBase;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-index-stats-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath,  sizeof(daePath),  "%s/index_stats.dae",  tmpdir);
  snprintf(objPath,  sizeof(objPath),  "%s/index_stats.obj",  tmpdir);
  snprintf(plyPath,  sizeof(plyPath),  "%s/index_stats.ply",  tmpdir);
  snprintf(gltfPath, sizeof(gltfPath), "%s/index_stats.gltf", tmpdir);
  snprintf(binPath,  sizeof(binPath),  "%s/index_stats.bin",  tmpdir);

  ASSERT(ak_test_write_dae(daePath));
  ASSERT(ak_test_write_obj(objPath));
  ASSERT(ak_test_write_ply(plyPath));
  ASSERT(ak_test_write_gltf(gltfPath, binPath));

  ASSERT(ak_test_load_index_stats(daePath, &stats));
  ASSERT(stats.primitiveCount == 1);
  ASSERT(stats.ownedCount == 1);
  ASSERT(stats.accessorCount == 0);
  ASSERT(stats.u8Count == 1);

  ASSERT(ak_test_load_index_stats(objPath, &stats));
  ASSERT(stats.primitiveCount == 1);
  ASSERT(stats.ownedCount == 1);
  ASSERT(stats.accessorCount == 0);
  ASSERT(stats.u8Count == 1);

  ASSERT(ak_test_load_index_stats(plyPath, &stats));
  ASSERT(stats.primitiveCount == 1);
  ASSERT(stats.ownedCount == 1);
  ASSERT(stats.accessorCount == 0);
  ASSERT(stats.u8Count == 1);

  ASSERT(ak_test_load_index_stats(gltfPath, &stats));
  ASSERT(stats.primitiveCount == 1);
  ASSERT(stats.ownedCount == 0);
  ASSERT(stats.accessorCount == 1);
  ASSERT(stats.u8Count == 1);

  unlink(daePath);
  unlink(objPath);
  unlink(plyPath);
  unlink(gltfPath);
  unlink(binPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
