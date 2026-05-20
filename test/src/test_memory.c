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
  AkHeapStats stats;
  uint32_t heapid, data;

  heap = ak_heap_new(NULL, NULL, NULL);
  ASSERT(heap->allocator == &ak__allocator);
  ASSERT(ak_heap_allocator(heap) == &ak__allocator);
  ak_heap_getStats(heap, &stats);
  ASSERT(stats.allocCalls == 0);
  ASSERT(stats.peakNodes == 0);

  heapid = heap->heapid;
  ASSERT(heapid > 0);
  ASSERT(ak_heap_lt_find(heap->heapid) == heap);

  data = 0;
  {
    void *mem;

    mem = ak_heap_calloc(heap, NULL, 16);
    ak_heap_getStats(heap, &stats);
    ASSERT(stats.allocCalls == 1);
    ASSERT(stats.callocCalls == 1);
    ASSERT(stats.allocBytes == 16);
    ASSERT(stats.callocBytes == 16);
    ASSERT(stats.liveNodes == 1);
    ASSERT(stats.peakNodes == 1);

    ak_free(mem);
    ak_heap_getStats(heap, &stats);
    ASSERT(stats.freeCalls == 1);
    ASSERT(stats.liveNodes == 0);
  }

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
bool
ak_test_write_u16le(FILE *file, uint16_t value) {
  uint8_t bytes[2];

  bytes[0] = (uint8_t)(value & 0xffu);
  bytes[1] = (uint8_t)((value >> 8) & 0xffu);

  return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static
bool
ak_test_write_u32le(FILE *file, uint32_t value) {
  uint8_t bytes[4];

  bytes[0] = (uint8_t)(value & 0xffu);
  bytes[1] = (uint8_t)((value >> 8) & 0xffu);
  bytes[2] = (uint8_t)((value >> 16) & 0xffu);
  bytes[3] = (uint8_t)((value >> 24) & 0xffu);

  return fwrite(bytes, 1, sizeof(bytes), file) == sizeof(bytes);
}

static
bool
ak_test_write_f32le(FILE *file, float value) {
  uint32_t bits;

  memcpy(&bits, &value, sizeof(bits));

  return ak_test_write_u32le(file, bits);
}

static
bool
ak_test_write_stl_binary_zero_header(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  uint8_t  attr[2] = {0, 0};
  float    tri[12] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  bool     ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 1);
  for (i = 0; i < 12; i++)
    ok = ok && ak_test_write_f32le(file, tri[i]);
  ok = ok && fwrite(attr, 1, sizeof(attr), file) == sizeof(attr);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_binary_solid(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  uint8_t  attr[2] = {0, 0};
  float    tri[12] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  bool     ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));
  memcpy(header, "solid binary", 12);

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 1);
  for (i = 0; i < 12; i++)
    ok = ok && ak_test_write_f32le(file, tri[i]);
  ok = ok && fwrite(attr, 1, sizeof(attr), file) == sizeof(attr);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_binary_square(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  uint8_t  attr[2] = {0, 0};
  float    tris[24] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,

    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  bool     ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));
  memcpy(header, "binary square", 13);

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 2);
  for (i = 0; i < 12; i++)
    ok = ok && ak_test_write_f32le(file, tris[i]);
  ok = ok && fwrite(attr, 1, sizeof(attr), file) == sizeof(attr);
  for (; i < 24; i++)
    ok = ok && ak_test_write_f32le(file, tris[i]);
  ok = ok && fwrite(attr, 1, sizeof(attr), file) == sizeof(attr);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_binary_color_trailing(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  uint8_t  trailing[4] = {1, 2, 3, 4};
  float    tri[12] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  bool     ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));
  memcpy(header, "COLOR=", 6);
  header[6] = 128;
  header[7] = 64;
  header[8] = 32;
  header[9] = 255;

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 1);
  for (i = 0; i < 12; i++)
    ok = ok && ak_test_write_f32le(file, tri[i]);
  ok = ok && ak_test_write_u16le(file, 0x001fu);
  ok = ok && fwrite(trailing, 1, sizeof(trailing), file) == sizeof(trailing);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_binary_viscam_color(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  float    tri[12] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  bool     ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));
  memcpy(header, "viscam color", 12);

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 1);
  for (i = 0; i < 12; i++)
    ok = ok && ak_test_write_f32le(file, tri[i]);
  ok = ok && ak_test_write_u16le(file, 0xFC00u);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_binary_truncated(const char *path) {
  FILE    *file;
  uint8_t  header[80];
  bool     ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  memset(header, 0, sizeof(header));
  memcpy(header, "binary bad", 10);

  ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
  ok = ok && ak_test_write_u32le(file, 1);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_stl_ascii_one(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("solid one\r\n"
        "  FACET normal 0 0 1\n"
        "    # exporter comment inside facet\n"
        "    outer loop\n"
        "      vertex 0 0 0\n"
        "      vertex 1 0 0\n"
        "      vertex 0 1 0\n"
        "    endloop\n"
        "  ENDFACET\n"
        "endsolid one\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_extra_attrs(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0 1\n"
        "v 1 0 0 0.25 0.50 0.75\n"
        "v 0 1 0 0.10 0.20 0.30 0.40\n"
        "vt 0.1 0.2 0.3\n"
        "f 1/1 2/1 3/1\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_lines_points(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "l 1 2 3\n"
        "g pts\n"
        "p 1 4\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_sparse_lines_points(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0 1 0 0\n"
        "v 1 0 0 0.25 0.50 0.75\n"
        "v 2 0 0 1 1 0\n"
        "v 3 0 0 0 1 0\n"
        "v 4 0 0 0 0 1\n"
        "v 5 0 0 0.50 0.25 1\n"
        "l 2 4 2\n"
        "g sparse_points\n"
        "p 6 2 6 2 6 2 6\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_mixed_face(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 0 0 1\n"
        "vt 0 0\n"
        "vn 0 0 1\n"
        "f 1/1/1 2//1 3/1/1\n"
        "f 1 3/1/1 4//1\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_repeated_materials(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 1 1 0\n"
        "usemtl mat_a\n"
        "f 1 2 3\n"
        "usemtl mat_b\n"
        "f 2 4 3\n"
        "usemtl mat_a\n"
        "f 1 3 4\n"
        "usemtl mat_b\n"
        "f 1 4 2\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_smoothing_groups(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 1 1 0\n"
        "usemtl mat_a\n"
        "s 1\n"
        "f 1 2 3\n"
        "s off\n"
        "f 2 4 3\n"
        "s 1\n"
        "f 1 3 4\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_material_carry(const char *path, const char *mtlPath) {
  FILE *file;

  file = fopen(mtlPath, "wb");
  if (!file)
    return false;
  fputs("newmtl mat_a\n"
        "Kd 0.8 0.7 0.6\n",
        file);
  if (fclose(file) != 0)
    return false;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("mtllib material_carry.mtl\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 1 1 0\n"
        "usemtl mat_a\n"
        "o first\n"
        "f 1 2 3\n"
        "o second\n"
        "f 2 4 3\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_empty_usemtl(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 1 1 0\n"
        "usemtl \n"
        "f 1 2 3\n"
        "f 1 3 4\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_invalid_faces(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "v 1 1 0\n"
        "vt 0 0\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/1/1 3/1/1\n"
        "f 1 4\n"
        "f 1 2 1\n"
        "f 999/1/1 2/1/1 3/1/1\n"
        "f 1/999/1 2/1/1 3/1/1\n"
        "f 1/1/999 2/1/1 3/1/1\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_invalid_syntax(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "vt 0 0\n"
        "vn 0 0 1\n"
        "f 1/1/1 2/1/1 3/1/1\n"
        "v\n"
        "v 1 2\n"
        "v a b c\n"
        "vt\n"
        "vt vt\n"
        "vn 1 2\n"
        "vn vn\n"
        "f f\n"
        "f a b c\n"
        "f 1/1/1/1 2/1/1/1 3/1/1/1\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_duplicate_faces(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 1 1 0\n"
        "v 0 1 0\n"
        "f 1 2 3 1\n"
        "f 1 2 3 4 1 2\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_vertices_only(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_shuffled(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 3\n"
        "property float z\n"
        "property float x\n"
        "property float y\n"
        "element face 1\n"
        "property ushort confidence\n"
        "property list int int other_stuff\n"
        "property list uchar int vertex_indices\n"
        "end_header\n"
        "0 0 0\n"
        "0 1 0\n"
        "0 0 1\n"
        "60000 2 100 200 3 0 1 2\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_points(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 2\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "property uchar red\n"
        "property uchar green\n"
        "property uchar blue\n"
        "property uchar alpha\n"
        "end_header\n"
        "0 0 0 255 0 0 128\n"
        "1 0 0 0 255 0 255\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_tristrips(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 4\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "element tristrips 1\n"
        "property list uchar int vertex_indices\n"
        "end_header\n"
        "0 0 0\n"
        "1 0 0\n"
        "0 1 0\n"
        "1 1 0\n"
        "4 0 1 2 3\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_binary_tristrips_large(const char *path) {
  FILE     *file;
  uint32_t  i;
  bool      ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = fputs("ply\n"
             "format binary_little_endian 1.0\n"
             "element vertex 400\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "property uchar red\n"
             "property uchar green\n"
             "property uchar blue\n"
             "element tristrips 1\n"
             "property list int int vertex_indices\n"
             "end_header\n",
             file) >= 0;

  for (i = 0; ok && i < 400; i++) {
    ok = ok && ak_test_write_f32le(file, (float)i);
    ok = ok && ak_test_write_f32le(file, 0.0f);
    ok = ok && ak_test_write_f32le(file, 0.0f);
    ok = ok && fputc((int)(i & 0xffu), file) != EOF;
    ok = ok && fputc(128, file) != EOF;
    ok = ok && fputc(255, file) != EOF;
  }

  ok = ok && ak_test_write_u32le(file, 400);
  for (i = 0; ok && i < 400; i++)
    ok = ok && ak_test_write_u32le(file, i);

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_ply_edges(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 3\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "element face 0\n"
        "property list uchar int vertex_indices\n"
        "element edge 2\n"
        "property int vertex1\n"
        "property int vertex2\n"
        "property uchar red\n"
        "end_header\n"
        "0 0 0\n"
        "1 0 0\n"
        "0 1 0\n"
        "0 1 255\n"
        "1 2 128\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_face_edges(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("ply\n"
        "format ascii 1.0\n"
        "element vertex 4\n"
        "property float x\n"
        "property float y\n"
        "property float z\n"
        "element face 1\n"
        "property list uchar int vertex_indices\n"
        "element edge 1\n"
        "property int vertex1\n"
        "property int vertex2\n"
        "end_header\n"
        "0 0 0\n"
        "1 0 0\n"
        "0 1 0\n"
        "1 1 0\n"
        "3 0 1 2\n"
        "1 3\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_ply_binary_edges(const char *path) {
  FILE    *file;
  uint8_t  red0, red1;
  bool     ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = fputs("ply\n"
             "format binary_little_endian 1.0\n"
             "element vertex 3\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "element edge 2\n"
             "property int vertex1\n"
             "property int vertex2\n"
             "property uchar red\n"
             "end_header\n",
             file) >= 0;

  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_u32le(file, 0xbf80000au);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 1.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);

  red0 = 255;
  red1 = 128;
  ok = ok && ak_test_write_u32le(file, 0);
  ok = ok && ak_test_write_u32le(file, 1);
  ok = ok && fwrite(&red0, 1, 1, file) == 1;
  ok = ok && ak_test_write_u32le(file, 1);
  ok = ok && ak_test_write_u32le(file, 2);
  ok = ok && fwrite(&red1, 1, 1, file) == 1;

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_ply_binary_face_props(const char *path) {
  FILE    *file;
  uint8_t  fc;
  bool     ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = fputs("ply\n"
             "format binary_little_endian 1.0\n"
             "element vertex 4\n"
             "property float x\n"
             "property float y\n"
             "property float z\n"
             "element face 2\n"
             "property list int int other_stuff\n"
             "property list uchar ushort vertex_indices\n"
             "end_header\n",
             file) >= 0;

  ok = ok && ak_test_write_f32le(file, 1.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 1.0f);
  ok = ok && ak_test_write_f32le(file, 1.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, -1.0f);
  ok = ok && ak_test_write_f32le(file, -1.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, -1.0f);
  ok = ok && ak_test_write_f32le(file, -1.0f);
  ok = ok && ak_test_write_f32le(file, 0.0f);
  ok = ok && ak_test_write_f32le(file, 1.0f);

  fc = 3;
  ok = ok && ak_test_write_u32le(file, 2);
  ok = ok && ak_test_write_u32le(file, 100);
  ok = ok && ak_test_write_u32le(file, 200);
  ok = ok && fwrite(&fc, 1, 1, file) == 1;
  ok = ok && ak_test_write_u16le(file, 0);
  ok = ok && ak_test_write_u16le(file, 1);
  ok = ok && ak_test_write_u16le(file, 2);

  ok = ok && ak_test_write_u32le(file, 2);
  ok = ok && ak_test_write_u32le(file, 0xffff3cb0u);
  ok = ok && ak_test_write_u32le(file, 60000);
  ok = ok && fwrite(&fc, 1, 1, file) == 1;
  ok = ok && ak_test_write_u16le(file, 0);
  ok = ok && ak_test_write_u16le(file, 2);
  ok = ok && ak_test_write_u16le(file, 3);

  return fclose(file) == 0 && ok;
}

static
AkMeshPrimitive*
ak_test_first_primitive(AkDoc *doc) {
  AkGeometry *geom;

  for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
    AkMesh *mesh;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (mesh && mesh->primitive)
      return mesh->primitive;
  }

  return NULL;
}

static
AkNode*
ak_test_first_scene_node(AkDoc *doc) {
  AkVisualScene *scene;

  if (!doc || !doc->lib.visualScenes || !doc->lib.visualScenes->chld)
    return NULL;

  scene = (AkVisualScene *)doc->lib.visualScenes->chld;
  return scene->node;
}

static
AkInput*
ak_test_input(AkMeshPrimitive *prim, AkInputSemantic semantic) {
  AkInput *input;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == semantic)
      return input;
  }

  return NULL;
}

static
uint32_t
ak_test_primitive_type_count(AkDoc *doc, AkMeshPrimitiveType type) {
  AkGeometry *geom;
  uint32_t    count;

  count = 0;
  for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (!mesh)
      continue;

    for (prim = mesh->primitive; prim; prim = prim->next) {
      if (prim->type == type)
        count++;
    }
  }

  return count;
}

static
AkMeshPrimitive*
ak_test_primitive_of_type(AkDoc *doc, AkMeshPrimitiveType type) {
  AkGeometry *geom;

  for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (!mesh)
      continue;

    for (prim = mesh->primitive; prim; prim = prim->next) {
      if (prim->type == type)
        return prim;
    }
  }

  return NULL;
}

static
float
ak_test_accessor_f32(AkAccessor *acc, uint32_t index, uint32_t component) {
  const char *data;
  float       value;

  data = (const char *)acc->buffer->data
       + acc->byteOffset
       + (size_t)index * acc->byteStride
       + (size_t)component * acc->bytesPerComponent;
  memcpy(&value, data, sizeof(value));

  return value;
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

TEST_IMPL(format_edge_cases) {
  AkDoc           *doc;
  AkMeshPrimitive *prim;
  AkAccessor      *acc;
  char             dirTemplate[PATH_MAX];
  char            *tmpdir;
  char             stlZeroPath[PATH_MAX];
  char             stlBinaryPath[PATH_MAX];
  char             stlDedupPath[PATH_MAX];
  char             stlColorPath[PATH_MAX];
  char             stlViscamPath[PATH_MAX];
  char             stlAsciiPath[PATH_MAX];
  char             stlBadPath[PATH_MAX];
  char             objPath[PATH_MAX];
  char             objAttrsPath[PATH_MAX];
  char             objLinesPath[PATH_MAX];
  char             objSparseLinesPath[PATH_MAX];
  char             objMaterialPath[PATH_MAX];
  char             objSmoothPath[PATH_MAX];
  char             objCarryPath[PATH_MAX];
  char             objCarryMtlPath[PATH_MAX];
  char             objEmptyUsemtlPath[PATH_MAX];
  char             objInvalidFacesPath[PATH_MAX];
  char             objInvalidSyntaxPath[PATH_MAX];
  char             objDuplicateFacesPath[PATH_MAX];
  char             objVerticesOnlyPath[PATH_MAX];
  char             plyPath[PATH_MAX];
  char             plyPointsPath[PATH_MAX];
  char             plyTristripsPath[PATH_MAX];
  char             plyBinaryTristripsPath[PATH_MAX];
  char             plyEdgesPath[PATH_MAX];
  char             plyFaceEdgesPath[PATH_MAX];
  char             plyBinaryEdgesPath[PATH_MAX];
  char             plyBinaryFacePropsPath[PATH_MAX];
  const char      *tmpBase;
  AkResult         loadResult;

  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-format-edges-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(stlBinaryPath,
           sizeof(stlBinaryPath),
           "%s/solid_binary.stl",
           tmpdir);
  snprintf(stlZeroPath,
           sizeof(stlZeroPath),
           "%s/zero_header.stl",
           tmpdir);
  snprintf(stlDedupPath,
           sizeof(stlDedupPath),
           "%s/dedup_square.stl",
           tmpdir);
  snprintf(stlColorPath,
           sizeof(stlColorPath),
           "%s/color_trailing.stl",
           tmpdir);
  snprintf(stlViscamPath,
           sizeof(stlViscamPath),
           "%s/viscam_color.stl",
           tmpdir);
  snprintf(stlAsciiPath,
           sizeof(stlAsciiPath),
           "%s/ascii.stl",
           tmpdir);
  snprintf(stlBadPath,
           sizeof(stlBadPath),
           "%s/truncated.stl",
           tmpdir);
  snprintf(objPath, sizeof(objPath), "%s/mixed.obj", tmpdir);
  snprintf(objAttrsPath, sizeof(objAttrsPath), "%s/attrs.obj", tmpdir);
  snprintf(objLinesPath, sizeof(objLinesPath), "%s/lines_points.obj", tmpdir);
  snprintf(objSparseLinesPath,
           sizeof(objSparseLinesPath),
           "%s/sparse_lines_points.obj",
           tmpdir);
  snprintf(objMaterialPath,
           sizeof(objMaterialPath),
           "%s/repeated_materials.obj",
           tmpdir);
  snprintf(objSmoothPath,
           sizeof(objSmoothPath),
           "%s/smoothing_groups.obj",
           tmpdir);
  snprintf(objCarryPath,
           sizeof(objCarryPath),
           "%s/material_carry.obj",
           tmpdir);
  snprintf(objCarryMtlPath,
           sizeof(objCarryMtlPath),
           "%s/material_carry.mtl",
           tmpdir);
  snprintf(objEmptyUsemtlPath,
           sizeof(objEmptyUsemtlPath),
           "%s/empty_usemtl.obj",
           tmpdir);
  snprintf(objInvalidFacesPath,
           sizeof(objInvalidFacesPath),
           "%s/invalid_faces.obj",
           tmpdir);
  snprintf(objInvalidSyntaxPath,
           sizeof(objInvalidSyntaxPath),
           "%s/invalid_syntax.obj",
           tmpdir);
  snprintf(objDuplicateFacesPath,
           sizeof(objDuplicateFacesPath),
           "%s/duplicate_faces.obj",
           tmpdir);
  snprintf(objVerticesOnlyPath,
           sizeof(objVerticesOnlyPath),
           "%s/vertices_only.obj",
           tmpdir);
  snprintf(plyPath, sizeof(plyPath), "%s/shuffled.ply", tmpdir);
  snprintf(plyPointsPath,
           sizeof(plyPointsPath),
           "%s/points.ply",
           tmpdir);
  snprintf(plyTristripsPath,
           sizeof(plyTristripsPath),
           "%s/tristrips.ply",
           tmpdir);
  snprintf(plyBinaryTristripsPath,
           sizeof(plyBinaryTristripsPath),
           "%s/binary_tristrips_large.ply",
           tmpdir);
  snprintf(plyEdgesPath,
           sizeof(plyEdgesPath),
           "%s/edges.ply",
           tmpdir);
  snprintf(plyFaceEdgesPath,
           sizeof(plyFaceEdgesPath),
           "%s/face_edges.ply",
           tmpdir);
  snprintf(plyBinaryEdgesPath,
           sizeof(plyBinaryEdgesPath),
           "%s/binary_edges.ply",
           tmpdir);
  snprintf(plyBinaryFacePropsPath,
           sizeof(plyBinaryFacePropsPath),
           "%s/binary_face_props.ply",
           tmpdir);

  ASSERT(ak_test_write_stl_binary_zero_header(stlZeroPath));
  ASSERT(ak_test_write_stl_binary_solid(stlBinaryPath));
  ASSERT(ak_test_write_stl_binary_square(stlDedupPath));
  ASSERT(ak_test_write_stl_binary_color_trailing(stlColorPath));
  ASSERT(ak_test_write_stl_binary_viscam_color(stlViscamPath));
  ASSERT(ak_test_write_stl_ascii_one(stlAsciiPath));
  ASSERT(ak_test_write_stl_binary_truncated(stlBadPath));
  ASSERT(ak_test_write_obj_mixed_face(objPath));
  ASSERT(ak_test_write_obj_extra_attrs(objAttrsPath));
  ASSERT(ak_test_write_obj_lines_points(objLinesPath));
  ASSERT(ak_test_write_obj_sparse_lines_points(objSparseLinesPath));
  ASSERT(ak_test_write_obj_repeated_materials(objMaterialPath));
  ASSERT(ak_test_write_obj_smoothing_groups(objSmoothPath));
  ASSERT(ak_test_write_obj_material_carry(objCarryPath, objCarryMtlPath));
  ASSERT(ak_test_write_obj_empty_usemtl(objEmptyUsemtlPath));
  ASSERT(ak_test_write_obj_invalid_faces(objInvalidFacesPath));
  ASSERT(ak_test_write_obj_invalid_syntax(objInvalidSyntaxPath));
  ASSERT(ak_test_write_obj_duplicate_faces(objDuplicateFacesPath));
  ASSERT(ak_test_write_obj_vertices_only(objVerticesOnlyPath));
  ASSERT(ak_test_write_ply_shuffled(plyPath));
  ASSERT(ak_test_write_ply_points(plyPointsPath));
  ASSERT(ak_test_write_ply_tristrips(plyTristripsPath));
  ASSERT(ak_test_write_ply_binary_tristrips_large(plyBinaryTristripsPath));
  ASSERT(ak_test_write_ply_edges(plyEdgesPath));
  ASSERT(ak_test_write_ply_face_edges(plyFaceEdgesPath));
  ASSERT(ak_test_write_ply_binary_edges(plyBinaryEdgesPath));
  ASSERT(ak_test_write_ply_binary_face_props(plyBinaryFacePropsPath));

  doc = NULL;
  ASSERT(ak_load(&doc, stlZeroPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_first_scene_node(doc) && ak_test_first_scene_node(doc)->visible);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 3);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, stlBinaryPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 3);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, stlDedupPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 6);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(prim->pos && prim->pos->accessor);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, stlColorPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(ak_test_input(prim, AK_INPUT_COLOR) != NULL);
  acc = ak_test_input(prim, AK_INPUT_COLOR)->accessor;
  ASSERT(acc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(acc->count == 3);
  ASSERT(ak_test_accessor_f32(acc, 0, 0) == 1.0f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, stlViscamPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(ak_test_input(prim, AK_INPUT_COLOR) != NULL);
  acc = ak_test_input(prim, AK_INPUT_COLOR)->accessor;
  ASSERT(acc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(acc->count == 3);
  ASSERT(ak_test_accessor_f32(acc, 0, 0) == 1.0f);
  ASSERT(ak_test_accessor_f32(acc, 0, 1) == 0.0f);
  ASSERT(ak_test_accessor_f32(acc, 0, 2) == 0.0f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, stlAsciiPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 3);
  ak_free(doc);

  doc        = NULL;
  loadResult = ak_load(&doc, stlBadPath, AK_FILE_TYPE_AUTO);
  ASSERT(loadResult != AK_OK || !doc);
  if (doc)
    ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_first_scene_node(doc) && ak_test_first_scene_node(doc)->visible);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 6);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(ak_meshPrimitiveIndexMax(prim) == 4);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objAttrsPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(prim->pos->accessor->count == 3);
  ASSERT(ak_test_accessor_f32(prim->pos->accessor, 0, 3) == 1.0f);
  ASSERT(ak_test_input(prim, AK_INPUT_TEXCOORD) != NULL);
  acc = ak_test_input(prim, AK_INPUT_TEXCOORD)->accessor;
  ASSERT(acc->componentSize == AK_COMPONENT_SIZE_VEC3);
  ASSERT(ak_test_accessor_f32(acc, 0, 2) == 0.3f);
  ASSERT(ak_test_input(prim, AK_INPUT_COLOR) != NULL);
  acc = ak_test_input(prim, AK_INPUT_COLOR)->accessor;
  ASSERT(acc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(ak_test_accessor_f32(acc, 0, 0) == 1.0f);
  ASSERT(ak_test_accessor_f32(acc, 1, 0) == 0.25f);
  ASSERT(ak_test_accessor_f32(acc, 2, 3) == 0.40f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objLinesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_primitive_type_count(doc, AK_PRIMITIVE_LINES) == 1);
  ASSERT(ak_test_primitive_type_count(doc, AK_PRIMITIVE_POINTS) == 1);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objSparseLinesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_primitive_of_type(doc, AK_PRIMITIVE_LINES);
  ASSERT(prim != NULL);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 4);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(ak_meshPrimitiveIndexMax(prim) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 0) == 0);
  ASSERT(ak_indexArrayGet(prim->indices, 1) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 2) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 3) == 0);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 2);
  ASSERT(ak_test_accessor_f32(prim->pos->accessor, 0, 0) == 1.0f);
  ASSERT(ak_test_accessor_f32(prim->pos->accessor, 1, 0) == 3.0f);
  ASSERT(ak_test_input(prim, AK_INPUT_COLOR) != NULL);
  acc = ak_test_input(prim, AK_INPUT_COLOR)->accessor;
  ASSERT(acc && acc->count == 2);
  ASSERT(ak_test_accessor_f32(acc, 0, 0) == 0.25f);
  ASSERT(ak_test_accessor_f32(acc, 1, 1) == 1.0f);

  prim = ak_test_primitive_of_type(doc, AK_PRIMITIVE_POINTS);
  ASSERT(prim != NULL);
  ASSERT(prim->nPolygons == 7);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 7);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(ak_meshPrimitiveIndexMax(prim) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 0) == 0);
  ASSERT(ak_indexArrayGet(prim->indices, 1) == 1);
  ASSERT(ak_indexArrayGet(prim->indices, 6) == 0);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 2);
  ASSERT(ak_test_accessor_f32(prim->pos->accessor, 0, 0) == 5.0f);
  ASSERT(ak_test_accessor_f32(prim->pos->accessor, 1, 0) == 1.0f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objMaterialPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_primitive_type_count(doc, AK_PRIMITIVE_TRIANGLES) == 2);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objSmoothPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  {
    AkGeometry *geom;
    uint32_t smoothCount;
    uint32_t flatCount;

    smoothCount = 0;
    flatCount   = 0;
    for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
      AkMesh          *mesh;
      AkMeshPrimitive *it;

      if (!geom->gdata)
        continue;

      mesh = ak_objGet(geom->gdata);
      if (!mesh)
        continue;

      for (it = mesh->primitive; it; it = it->next) {
        if (it->type != AK_PRIMITIVE_TRIANGLES)
          continue;
        if (it->reserved1 & 1u)
          smoothCount++;
        else
          flatCount++;
      }
    }
    ASSERT(smoothCount == 1);
    ASSERT(flatCount == 1);
  }
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objCarryPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  {
    AkGeometry *geom;
    uint32_t triangleCount;
    uint32_t materialCount;

    triangleCount = 0;
    materialCount = 0;
    for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
      AkMesh          *mesh;
      AkMeshPrimitive *it;

      if (!geom->gdata)
        continue;

      mesh = ak_objGet(geom->gdata);
      if (!mesh)
        continue;

      for (it = mesh->primitive; it; it = it->next) {
        if (it->type != AK_PRIMITIVE_TRIANGLES)
          continue;
        triangleCount++;
        if (it->material)
          materialCount++;
      }
    }
    ASSERT(triangleCount == 2);
    ASSERT(materialCount == 2);
  }
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objEmptyUsemtlPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indices && prim->indices->count == 6);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objInvalidFacesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->indices && prim->indices->count == 3);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objInvalidSyntaxPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->indices && prim->indices->count == 3);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objDuplicateFacesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 3);
  ASSERT(prim->indices && prim->indices->count == 9);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, objVerticesOnlyPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_POINTS);
  ASSERT(prim->nPolygons == 3);
  ASSERT(prim->indices == NULL);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_first_scene_node(doc) && ak_test_first_scene_node(doc)->visible);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->pos && prim->pos->accessor);
  acc = prim->pos->accessor;
  ASSERT(acc->count == 3);
  ASSERT(ak_test_accessor_f32(acc, 1, 0) == 1.0f);
  ASSERT(ak_test_accessor_f32(acc, 1, 1) == 0.0f);
  ASSERT(ak_test_accessor_f32(acc, 2, 0) == 0.0f);
  ASSERT(ak_test_accessor_f32(acc, 2, 1) == 1.0f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyPointsPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_POINTS);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indices == NULL);
  ASSERT(prim->pos && prim->pos->accessor);
  ASSERT(prim->pos->accessor->count == 2);
  ASSERT(prim->inputCount == 2);
  acc = ak_test_input(prim, AK_INPUT_COLOR)->accessor;
  ASSERT(acc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(acc->originalComponentType == AKT_UBYTE);
  ASSERT(ak_test_accessor_f32(acc, 0, 3) == 128.0f);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyTristripsPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 6);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(prim->pos && prim->pos->accessor);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyBinaryTristripsPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 398);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 1194);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_USHORT);
  ASSERT(prim->pos && prim->pos->accessor);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyEdgesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_LINES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indexStride == 1);
  ASSERT(prim->indices && prim->indices->count == 4);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(prim->pos && prim->pos->accessor);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyFaceEdgesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_primitive_type_count(doc, AK_PRIMITIVE_TRIANGLES) == 1);
  ASSERT(ak_test_primitive_type_count(doc, AK_PRIMITIVE_LINES) == 1);
  prim = ak_test_primitive_of_type(doc, AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim != NULL);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->indices && prim->indices->count == 3);
  prim = ak_test_primitive_of_type(doc, AK_PRIMITIVE_LINES);
  ASSERT(prim != NULL);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->indices && prim->indices->count == 2);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyBinaryEdgesPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_LINES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indices && prim->indices->count == 4);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ASSERT(prim->pos && prim->pos->accessor);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, plyBinaryFacePropsPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  prim = ak_test_first_primitive(doc);
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 2);
  ASSERT(prim->indices && prim->indices->count == 6);
  ASSERT(ak_meshPrimitiveIndexComponentType(prim) == AKT_UBYTE);
  ak_free(doc);

  unlink(stlZeroPath);
  unlink(stlBinaryPath);
  unlink(stlDedupPath);
  unlink(stlColorPath);
  unlink(stlViscamPath);
  unlink(stlAsciiPath);
  unlink(stlBadPath);
  unlink(objPath);
  unlink(objAttrsPath);
  unlink(objLinesPath);
  unlink(objSparseLinesPath);
  unlink(objMaterialPath);
  unlink(objSmoothPath);
  unlink(objCarryPath);
  unlink(objCarryMtlPath);
  unlink(objEmptyUsemtlPath);
  unlink(objInvalidFacesPath);
  unlink(objInvalidSyntaxPath);
  unlink(objDuplicateFacesPath);
  unlink(objVerticesOnlyPath);
  unlink(plyPath);
  unlink(plyPointsPath);
  unlink(plyTristripsPath);
  unlink(plyBinaryTristripsPath);
  unlink(plyEdgesPath);
  unlink(plyFaceEdgesPath);
  unlink(plyBinaryEdgesPath);
  unlink(plyBinaryFacePropsPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
