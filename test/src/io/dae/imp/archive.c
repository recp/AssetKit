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

#include "../../../test_common.h"
#include "image/export.h"

#include <limits.h>
#include <unistd.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

static
int
ak_test_base64_value(unsigned char c) {
  if (c >= 'A' && c <= 'Z')
    return (int)(c - 'A');
  if (c >= 'a' && c <= 'z')
    return (int)(c - 'a') + 26;
  if (c >= '0' && c <= '9')
    return (int)(c - '0') + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

static
bool
ak_test_write_base64(const char *path, const char *encoded) {
  FILE    *file;
  uint32_t bits;
  unsigned bitCount;

  file = fopen(path, "wb");
  if (!file)
    return false;

  bits     = 0u;
  bitCount = 0u;
  while (*encoded) {
    int value;

    if (*encoded == '=')
      break;
    value = ak_test_base64_value((unsigned char)*encoded++);
    if (value < 0)
      continue;

    bits = (bits << 6u) | (uint32_t)value;
    bitCount += 6u;
    if (bitCount >= 8u) {
      unsigned char byte;

      bitCount -= 8u;
      byte = (unsigned char)((bits >> bitCount) & 0xffu);
      if (fwrite(&byte, 1u, 1u, file) != 1u) {
        fclose(file);
        return false;
      }
    }
  }

  return fclose(file) == 0;
}

TEST_IMPL(dae_archive_import_embeds_relative_texture) {
  static const char archiveBase64[] =
    "UEsDBAoAAAAAAIYb+1yssAjtPAAAADwAAAAMABwAbWFuaWZlc3QueG1sVVQJAAMc"
    "pmZqHKZmanV4CwABBPUBAAAEAAAAADw/eG1sIHZlcnNpb249IjEuMCI/PjxkYWVf"
    "cm9vdD4uL21vZGVscy9tb2RlbC5kYWU8L2RhZV9yb290PlBLAwQKAAAAAACGG/tc"
    "S7JIBkwBAABMAQAAEAAcAG1vZGVscy9tb2RlbC5kYWVVVAkAAxymZmocpmZqdXgL"
    "AAEE9QEAAAQAAAAAPD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0idXRmLTgi"
    "Pz48Q09MTEFEQSB4bWxucz0iaHR0cDovL3d3dy5jb2xsYWRhLm9yZy8yMDA4LzAz"
    "L0NPTExBREFTY2hlbWEiIHZlcnNpb249IjEuNS4wIj48YXNzZXQ+PHVuaXQgbmFt"
    "ZT0ibWV0ZXIiIG1ldGVyPSIxIi8+PHVwX2F4aXM+WV9VUDwvdXBfYXhpcz48L2Fz"
    "c2V0PjxsaWJyYXJ5X2ltYWdlcz48aW1hZ2UgaWQ9IndhbGwtaW1hZ2UiIG5hbWU9"
    "IndhbGwiPjxpbml0X2Zyb20+PHJlZj4uLi90ZXh0dXJlcy93YWxsJTIwY29sb3Iu"
    "ZGRzPC9yZWY+PC9pbml0X2Zyb20+PC9pbWFnZT48L2xpYnJhcnlfaW1hZ2VzPjwv"
    "Q09MTEFEQT5QSwMECgAAAAAAhhv7XG0IgxIIAAAACAAAABcAHAB0ZXh0dXJlcy93"
    "YWxsIGNvbG9yLmRkc1VUCQADHKZmahymZmp1eAsAAQT1AQAABAAAAABERFMgfAAA"
    "AFBLAQIeAwoAAAAAAIYb+1yssAjtPAAAADwAAAAMABgAAAAAAAAAAACkgQAAAABt"
    "YW5pZmVzdC54bWxVVAUAAxymZmp1eAsAAQT1AQAABAAAAABQSwECHgMKAAAAAACG"
    "G/tcS7JIBkwBAABMAQAAEAAYAAAAAAAAAAAApIGCAAAAbW9kZWxzL21vZGVsLmRh"
    "ZVVUBQADHKZmanV4CwABBPUBAAAEAAAAAFBLAQIeAwoAAAAAAIYb+1xtCIMSCAAA"
    "AAgAAAAXABgAAAAAAAAAAACkgRgCAAB0ZXh0dXJlcy93YWxsIGNvbG9yLmRkc1VU"
    "BQADHKZmanV4CwABBPUBAAAEAAAAAFBLBQYAAAAAAwADAAUBAABxAgAAAAA=";
  static const unsigned char texture[] = {
    'D', 'D', 'S', ' ', 0x7c, 0x00, 0x00, 0x00
  };
  char           tmpdir[] = "/tmp/assetkit-dae-archive-XXXXXX";
  char           path[PATH_MAX];
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;

  ASSERT(mkdtemp(tmpdir) != NULL);
  snprintf(path, sizeof(path), "%s/model.ZAE", tmpdir);
  ASSERT(ak_test_write_base64(path, archiveBase64));

  doc = NULL;
  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  image  = doc->lib.images.first;
  source = ak_imageSource(image);
  ASSERT(image != NULL);
  ASSERT(source != NULL);
  ASSERT(source->type == AK_IMAGE_SOURCE_BUFFER);
  ASSERT(source->uri != NULL);
  ASSERT(strcmp(source->uri, "../textures/wall%20color.dds") == 0);
  ASSERT(source->buffer != NULL);
  ASSERT(source->buffer->length == sizeof(texture));
  ASSERT(memcmp(source->buffer->data, texture, sizeof(texture)) == 0);
  ASSERT(source->mimeType != NULL);
  ASSERT(strcmp(source->mimeType, "image/vnd-ms.dds") == 0);
  ak_free(doc);

  doc = NULL;
  ASSERT(ak_load(&doc, path, AK_FILE_TYPE_COLLADA) == AK_OK && doc);
  ak_free(doc);

  unlink(path);
  rmdir(tmpdir);

  TEST_SUCCESS
}
