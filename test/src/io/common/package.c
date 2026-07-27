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

#include "../../test_common.h"
#include "image/export.h"
#include "io/common/zip.h"

#include <limits.h>
#include <unistd.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static const char ak_test_package_ply[] =
  "ply\n"
  "format ascii 1.0\n"
  "element vertex 3\n"
  "property float x\n"
  "property float y\n"
  "property float z\n"
  "element face 1\n"
  "property list uchar int vertex_indices\n"
  "end_header\n"
  "0 0 0\n"
  "1 0 0\n"
  "0 1 0\n"
  "3 0 1 2\n";

static const char ak_test_package_gltf[] =
  "{"
  "\"asset\":{\"version\":\"2.0\"},"
  "\"buffers\":[{\"uri\":\"mesh.bin\",\"byteLength\":36}],"
  "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36}],"
  "\"accessors\":[{"
    "\"bufferView\":0,\"componentType\":5126,\"count\":3,\"type\":\"VEC3\","
    "\"min\":[0,0,0],\"max\":[1,1,0]"
  "}],"
  "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0}}]}],"
  "\"nodes\":[{\"mesh\":0}],"
  "\"scenes\":[{\"nodes\":[0]}],"
  "\"scene\":0,"
  "\"images\":[{\"uri\":\"textures/pixel.png\"}]"
  "}";

static const float ak_test_package_positions[] = {
  0.0f, 0.0f, 0.0f,
  1.0f, 0.0f, 0.0f,
  0.0f, 1.0f, 0.0f
};

static const unsigned char ak_test_package_png[] = {
  0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a
};

static
uint32_t
ak_test_package_crc32(const void * __restrict data, size_t size) {
  const unsigned char *bytes;
  uint32_t             crc;
  size_t               i;

  bytes = data;
  crc   = UINT32_MAX;
  for (i = 0u; i < size; i++) {
    unsigned bit;

    crc ^= bytes[i];
    for (bit = 0u; bit < 8u; bit++)
      crc = (crc >> 1u) ^ (0xedb88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

static
bool
ak_test_package_u16(FILE *file, uint16_t value) {
  unsigned char data[2];

  data[0] = (unsigned char)value;
  data[1] = (unsigned char)(value >> 8u);
  return fwrite(data, 1u, sizeof(data), file) == sizeof(data);
}

static
bool
ak_test_package_u32(FILE *file, uint32_t value) {
  unsigned char data[4];

  data[0] = (unsigned char)value;
  data[1] = (unsigned char)(value >> 8u);
  data[2] = (unsigned char)(value >> 16u);
  data[3] = (unsigned char)(value >> 24u);
  return fwrite(data, 1u, sizeof(data), file) == sizeof(data);
}

static
bool
ak_test_package_write_zip(const char            * __restrict path,
                          const AkZipWriteEntry * __restrict entries,
                          size_t                             count) {
  uint32_t *offsets;
  uint32_t *crcs;
  FILE     *file;
  long      centralOffset;
  long      centralEnd;
  size_t    i;
  bool      ok;

  if (count == 0u || count > UINT16_MAX)
    return false;

  offsets = calloc(count, sizeof(*offsets));
  crcs    = calloc(count, sizeof(*crcs));
  file    = fopen(path, "wb");
  if (!offsets || !crcs || !file) {
    free(offsets);
    free(crcs);
    if (file)
      fclose(file);
    return false;
  }

  ok = true;
  for (i = 0u; i < count; i++) {
    long     offset;
    size_t   nameLen;
    uint32_t size;

    offset  = ftell(file);
    nameLen = strlen(entries[i].name);
    if (offset < 0
        || offset > UINT32_MAX
        || nameLen > UINT16_MAX
        || entries[i].size > UINT32_MAX) {
      ok = false;
      break;
    }

    offsets[i] = (uint32_t)offset;
    crcs[i]    = ak_test_package_crc32(entries[i].data, entries[i].size);
    size       = (uint32_t)entries[i].size;
    ok = ak_test_package_u32(file, 0x04034b50u)
         && ak_test_package_u16(file, 20u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u32(file, crcs[i])
         && ak_test_package_u32(file, size)
         && ak_test_package_u32(file, size)
         && ak_test_package_u16(file, (uint16_t)nameLen)
         && ak_test_package_u16(file, 0u)
         && fwrite(entries[i].name, 1u, nameLen, file) == nameLen
         && fwrite(entries[i].data, 1u, entries[i].size, file)
            == entries[i].size;
    if (!ok)
      break;
  }

  centralOffset = ftell(file);
  for (i = 0u; ok && i < count; i++) {
    size_t   nameLen;
    uint32_t size;

    nameLen = strlen(entries[i].name);
    size    = (uint32_t)entries[i].size;
    ok = ak_test_package_u32(file, 0x02014b50u)
         && ak_test_package_u16(file, 20u)
         && ak_test_package_u16(file, 20u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u32(file, crcs[i])
         && ak_test_package_u32(file, size)
         && ak_test_package_u32(file, size)
         && ak_test_package_u16(file, (uint16_t)nameLen)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u32(file, 0u)
         && ak_test_package_u32(file, offsets[i])
         && fwrite(entries[i].name, 1u, nameLen, file) == nameLen;
  }

  centralEnd = ftell(file);
  if (centralOffset < 0
      || centralEnd < centralOffset
      || centralOffset > UINT32_MAX
      || centralEnd - centralOffset > UINT32_MAX) {
    ok = false;
  }
  if (ok) {
    ok = ak_test_package_u32(file, 0x06054b50u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, 0u)
         && ak_test_package_u16(file, (uint16_t)count)
         && ak_test_package_u16(file, (uint16_t)count)
         && ak_test_package_u32(file,
                                (uint32_t)(centralEnd - centralOffset))
         && ak_test_package_u32(file, (uint32_t)centralOffset)
         && ak_test_package_u16(file, 0u);
  }

  if (fclose(file) != 0)
    ok = false;
  free(offsets);
  free(crcs);
  if (!ok)
    unlink(path);
  return ok;
}

static
bool
ak_test_package_path(char       * __restrict path,
                     size_t                  pathCap,
                     char       * __restrict tempDir,
                     const char * __restrict fileName) {
  if (!mkdtemp(tempDir))
    return false;

  return snprintf(path, pathCap, "%s/%s", tempDir, fileName) > 0;
}

TEST_IMPL(zip_package_selects_shallowest_root_and_embeds_resources) {
  AkZipWriteEntry entries[] = {
    {
      .name = "nested/deeper/ignored.ply",
      .data = ak_test_package_ply,
      .size = sizeof(ak_test_package_ply) - 1u
    },
    {
      .name = "scene/model.gltf",
      .data = ak_test_package_gltf,
      .size = sizeof(ak_test_package_gltf) - 1u
    },
    {
      .name = "scene/mesh.bin",
      .data = ak_test_package_positions,
      .size = sizeof(ak_test_package_positions)
    },
    {
      .name = "scene/textures/pixel.png",
      .data = ak_test_package_png,
      .size = sizeof(ak_test_package_png)
    }
  };
  char           tempDir[] = "/tmp/assetkit-package-root-XXXXXX";
  char           path[PATH_MAX];
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;

  ASSERT(ak_test_package_path(path,
                              sizeof(path),
                              tempDir,
                              "assets.zip"));
  ASSERT(ak_test_package_write_zip(path,
                                   entries,
                                   AK_ARRAY_LEN(entries)));

  doc = NULL;
  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->inf != NULL);
  ASSERT(doc->inf->ftype == AK_FILE_TYPE_GLTF);
  ASSERT(doc->inf->name != NULL);
  ASSERT(strcmp(doc->inf->name, path) == 0);
  ASSERT(doc->lib.buffers.first != NULL);
  ASSERT(doc->lib.buffers.first->length == sizeof(ak_test_package_positions));
  ASSERT(memcmp(doc->lib.buffers.first->data,
                ak_test_package_positions,
                sizeof(ak_test_package_positions)) == 0);

  image  = doc->lib.images.first;
  source = ak_imageSource(image);
  ASSERT(image != NULL);
  ASSERT(source != NULL);
  ASSERT(source->type == AK_IMAGE_SOURCE_BUFFER);
  ASSERT(source->buffer != NULL);
  ASSERT(source->buffer->length == sizeof(ak_test_package_png));
  ASSERT(memcmp(source->buffer->data,
                ak_test_package_png,
                sizeof(ak_test_package_png)) == 0);
  ASSERT(source->mimeType != NULL);
  ASSERT(strcmp(source->mimeType, "image/png") == 0);
  ak_free(doc);

  unlink(path);
  rmdir(tempDir);
  TEST_SUCCESS
}

TEST_IMPL(zip_package_equal_depth_uses_archive_order) {
  AkZipWriteEntry entries[] = {
    {
      .name = "first.ply",
      .data = ak_test_package_ply,
      .size = sizeof(ak_test_package_ply) - 1u
    },
    {
      .name = "second.gltf",
      .data = ak_test_package_gltf,
      .size = sizeof(ak_test_package_gltf) - 1u
    }
  };
  char   tempDir[] = "/tmp/assetkit-package-order-XXXXXX";
  char   path[PATH_MAX];
  AkDoc *doc;

  ASSERT(ak_test_package_path(path,
                              sizeof(path),
                              tempDir,
                              "assets.zip"));
  ASSERT(ak_test_package_write_zip(path,
                                   entries,
                                   AK_ARRAY_LEN(entries)));

  doc = NULL;
  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->inf != NULL);
  ASSERT(doc->inf->ftype == AK_FILE_TYPE_PLY);
  ak_free(doc);

  unlink(path);
  rmdir(tempDir);
  TEST_SUCCESS
}

TEST_IMPL(zip_package_rejects_resource_escape) {
  static const char escapingGltf[] =
    "{"
    "\"asset\":{\"version\":\"2.0\"},"
    "\"buffers\":[{"
      "\"uri\":\"../../assetkit-package-outside.bin\","
      "\"byteLength\":4"
    "}]"
    "}";
  static const unsigned char outsideData[] = {1u, 2u, 3u, 4u};
  AkZipWriteEntry entries[] = {
    {
      .name = "nested/model.gltf",
      .data = escapingGltf,
      .size = sizeof(escapingGltf) - 1u
    }
  };
  char   tempDir[] = "/tmp/assetkit-package-scope-XXXXXX";
  char   path[PATH_MAX];
  AkDoc *doc;
  FILE  *outside;

  outside = fopen("/tmp/assetkit-package-outside.bin", "wb");
  ASSERT(outside != NULL);
  ASSERT(fwrite(outsideData, 1u, sizeof(outsideData), outside)
         == sizeof(outsideData));
  ASSERT(fclose(outside) == 0);

  ASSERT(ak_test_package_path(path,
                              sizeof(path),
                              tempDir,
                              "assets.zip"));
  ASSERT(ak_test_package_write_zip(path,
                                   entries,
                                   AK_ARRAY_LEN(entries)));

  doc = NULL;
  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.buffers.first != NULL);
  ASSERT(doc->lib.buffers.first->data == NULL);
  ASSERT(doc->lib.buffers.first->length == 0u);
  ak_free(doc);

  unlink(path);
  rmdir(tempDir);
  unlink("/tmp/assetkit-package-outside.bin");
  TEST_SUCCESS
}
