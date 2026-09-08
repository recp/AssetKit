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

#include "../../test_export_common.h"
#include <libdeflate.h>

enum { LGSC_TEST_COUNT = 3, LGSC_TEST_HEADER = 487 };

typedef struct LGSCTestData {
  uint8_t bytes[4096];
  float   values[LGSC_TEST_COUNT][59];
  size_t  size;
} LGSCTestData;

static
void
lgsc_test_u32(uint8_t *p, uint32_t value) {
  p[0] = (uint8_t)value;
  p[1] = (uint8_t)(value >> 8);
  p[2] = (uint8_t)(value >> 16);
  p[3] = (uint8_t)(value >> 24);
}

static
uint32_t
lgsc_test_read_u32(const uint8_t *p) {
  return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

static
void
lgsc_test_f32(uint8_t *p, float value) {
  uint32_t bits;

  memcpy(&bits, &value, sizeof(bits));
  lgsc_test_u32(p, bits);
}

static
bool
lgsc_test_write(const char *path, const void *bytes, size_t size) {
  FILE *file;
  bool  ok;

  if (!(file = fopen(path, "wb")))
    return false;

  ok = fwrite(bytes, 1, size, file) == size;
  return fclose(file) == 0 && ok;
}

static
bool
lgsc_test_data(LGSCTestData *data, uint8_t flags, uint32_t layout) {
  static const uint8_t sizes[] = {3, 3, 45, 1, 3, 4};
  static const uint8_t params[] = {0, 4, 5, 3, 1, 2};
  struct libdeflate_compressor *compressor;
  uint8_t *header, *p;
  size_t   offset, groupBytes, prefix, dst, compressed;
  double   norm;
  float    low, high, q[4], y, u, v, sh[45];
  uint32_t widths[59], symbols[LGSC_TEST_COUNT][59];
  uint32_t group, dim, row, field, bit, maximum, symbol, i, source, largest;
  uint8_t  bits[8], raw[LGSC_TEST_COUNT * 59 * 3];
  bool     rows;

  memset(data, 0, sizeof(*data));
  lgsc_test_u32(data->bytes, LGSC_TEST_HEADER);
  header    = data->bytes + 4;
  header[4] = 10;
  lgsc_test_u32(header, LGSC_TEST_COUNT);

  for (i = 0; i < 8; i++)
    bits[i] = (uint8_t)(layout < 3 ? 8 + layout * 8 : 5 + i * 2);

  memcpy(header + 5, bits, 8);
  header[13] = flags;
  field      = 0;

  for (group = 0; group < 6; group++) {
    for (dim = 0; dim < sizes[group]; dim++, field++) {
      bit           = bits[group == 2 ? (dim < 9 ? 5 : dim < 24 ? 6 : 7) : params[group]];
      maximum       = (1u << bit) - 1;
      widths[field] = (bit + 7) / 8;
      low           = group == 4 ? -3 : -1;
      high          = group == 4 ? -1 : 1;
      lgsc_test_f32(header + 15 + field * 8, low);
      lgsc_test_f32(header + 19 + field * 8, high);

      for (row = 0; row < LGSC_TEST_COUNT; row++) {
        symbol = (uint32_t)((uint64_t)maximum * (17 + (field * 7 + row * 13) % 71) / 100);
        if (field == 58 && (flags & 0x40))
          symbol = row;

        symbols[row][field]      = symbol;
        data->values[row][field] = field == 58 && (flags & 0x40) ? (float)symbol
                                  : (float)(low + (double)symbol * (high - low) / (maximum + (group == 2)));
      }
    }
  }

  if (!(compressor = libdeflate_alloc_compressor(1)))
    return false;

  data->size = 4 + LGSC_TEST_HEADER;
  offset     = 0;
  field      = 0;

  for (group = 0; group < 6; group++) {
    rows       = (flags & (group ? 0x04 : 0x08)) != 0;
    groupBytes = 0;
    prefix     = 0;

    for (dim = 0; dim < sizes[group]; dim++)
      groupBytes += widths[field + dim];

    for (dim = 0; dim < sizes[group]; dim++, field++) {
      for (row = 0; row < LGSC_TEST_COUNT; row++) {
        dst = offset + (rows ? row * groupBytes + prefix : prefix * LGSC_TEST_COUNT + row * widths[field]);
        p   = raw + dst;

        for (i = 0; i < widths[field]; i++)
          p[i] = (uint8_t)(symbols[row][field] >> (i * 8));
      }

      prefix += widths[field];
    }

    offset += groupBytes * LGSC_TEST_COUNT;

    if (!group || group == 5) {
      compressed = libdeflate_gzip_compress(compressor, raw, offset,
                                             data->bytes + data->size + 4,
                                             sizeof(data->bytes) - data->size - 4);
      if (!compressed) {
        libdeflate_free_compressor(compressor);
        return false;
      }

      lgsc_test_u32(data->bytes + data->size, (uint32_t)compressed);
      data->size += 4 + compressed;
      offset      = 0;
    }
  }

  libdeflate_free_compressor(compressor);

  /* Expected canonical attributes, derived independently from quantized fields. */
  for (row = 0; row < LGSC_TEST_COUNT; row++) {
    memcpy(sh, data->values[row] + 6, sizeof(sh));

    if (flags & 0x02) {
      for (dim = 0; dim < 15; dim++) {
        y            = sh[dim];
        u            = sh[dim + 15];
        v            = sh[dim + 30];
        sh[dim]      = (float)(y + 1.5748 * v);
        sh[dim + 15] = (float)(y - 0.1873 * u - 0.4681 * v);
        sh[dim + 30] = (float)(y + 1.8556 * u);
      }
    }

    for (dim = 0; dim < 45; dim++)
      data->values[row][6 + dim] = sh[(flags & 0x10) ? dim : (dim % 3) * 15 + dim / 3];

    memcpy(q, data->values[row] + 55, sizeof(q));

    if (flags & 0x40) {
      largest = symbols[row][58];
      source  = 0;
      high    = sqrtf(fmaxf(0, 1 - q[0] * q[0] - q[1] * q[1] - q[2] * q[2]));

      for (dim = 0; dim < 4; dim++)
        data->values[row][55 + dim] = dim == largest ? high : q[source++];

      memcpy(q, data->values[row] + 55, sizeof(q));
    }

    norm = 0;

    for (dim = 0; dim < 4; dim++)
      norm += (double)q[dim] * q[dim];

    for (dim = 0; dim < 4; dim++)
      data->values[row][55 + dim] = (float)(q[(dim + 1) % 4] / sqrt(norm));

    for (dim = 0; dim < 3; dim++)
      data->values[row][52 + dim] = expf(data->values[row][52 + dim]);

    low = data->values[row][51];
    data->values[row][51] = flags & 0x20 ? fminf(1 - 1e-6f, fmaxf(1e-6f, low))
                                        : 1 / (1 + expf(-low));
  }

  return true;
}

static
bool
lgsc_test_values(AkDoc *doc, const LGSCTestData *data, uint32_t degree, bool rdf) {
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *input;
  AkAccessor      *acc;
  float            values[LGSC_TEST_COUNT * 4], expected;
  uint32_t         row, c, field;

  if (!(mesh = ak_meshFromGeometry(doc->lib.geometries.first))
      || !(prim = mesh->primitive) || !prim->gsplat
      || prim->gsplat->shDegree != degree || prim->gsplat->decodedCount != LGSC_TEST_COUNT
      || !ak_sceneRoots(ak_activeSceneOrFirst(doc)))
    return false;

  for (input = prim->input; input; input = input->next) {
    acc = input->accessor;
    if (input->semantic == AK_INPUT_COLOR)
      continue;

    if (acc->count != LGSC_TEST_COUNT || acc->componentCount > 4
        || ak_accessorAsFloat(acc, values, LGSC_TEST_COUNT * 4) != acc->count * acc->componentCount)
      return false;

    switch (input->semantic) {
      case AK_INPUT_POSITION: field = 0; break;
      case AK_INPUT_ROTATION: field = 55; break;
      case AK_INPUT_SCALE: field = 52; break;
      case AK_INPUT_OPACITY: field = 51; break;
      case AK_INPUT_SH: field = 3 + input->set * 3; break;
      default: return false;
    }

    for (row = 0; row < LGSC_TEST_COUNT; row++) {
      for (c = 0; c < acc->componentCount; c++) {
        expected = data->values[row][field + c];

        if (rdf && (((input->semantic == AK_INPUT_POSITION || input->semantic == AK_INPUT_ROTATION) && c < 2)
                    || (input->semantic == AK_INPUT_SH && (input->set & 1))))
          expected = -expected;

        if (fabsf(values[row * acc->componentCount + c] - expected) > 0.000004f * (1 + fabsf(expected)))
          return false;
      }
    }
  }

  return true;
}

static
void
lgsc_test_name(char name[80], uint32_t slot) {
  uint32_t degree;

  switch (slot) {
    case 0: strcpy(name, "POSITION"); return;
    case 1: strcpy(name, "KHR_gaussian_splatting:ROTATION"); return;
    case 2: strcpy(name, "KHR_gaussian_splatting:SCALE"); return;
    case 3: strcpy(name, "KHR_gaussian_splatting:OPACITY"); return;
    default: break;
  }

  slot -= 4;
  for (degree = 0; (degree + 1) * (degree + 1) <= slot; degree++) {}

  snprintf(name, 80, "KHR_gaussian_splatting:SH_DEGREE_%u_COEF_%u", degree, slot - degree * degree);
}

static
bool
lgsc_test_gltf(const char *jsonPath, const char *glbPath, const LGSCTestData *data,
                uint32_t degree, uint32_t invalid) {
  FILE    *file, *glb;
  uint8_t *json;
  size_t   length, padded, binarySize, colorOffset;
  uint32_t i, primitive, count, primitiveCount, primitiveDegree;
  uint8_t  header[20], color[LGSC_TEST_COUNT * 16];
  char     name[80];

  if (!(file = fopen(jsonPath, "w+b")))
    return false;

  count       = 4 + (degree + 1) * (degree + 1);
  colorOffset = (data->size + 3) & ~(size_t)3;
  memset(color, 0, sizeof(color));

  fprintf(file, "{\"asset\":{\"version\":\"2.0\"},"
                "\"extensionsUsed\":[\"KHR_gaussian_splatting\",\"KHR_gaussian_splatting_compression_lgsc\"],"
                "\"extensionsRequired\":[\"KHR_gaussian_splatting\",\"KHR_gaussian_splatting_compression_lgsc\"],"
                "\"buffers\":[{%s\"byteLength\":%zu}],"
                "\"bufferViews\":[{\"buffer\":0,\"byteLength\":%zu},"
                "{\"buffer\":0,\"byteOffset\":%zu,\"byteLength\":%zu}],\"accessors\":[",
          glbPath ? "" : "\"uri\":\"ak_test_lgsc.bin\",",
          colorOffset + sizeof(color), data->size - (invalid == 8), colorOffset, sizeof(color));

  for (i = 0; i < count; i++) {
    if (i) fputc(',', file);
    fprintf(file, "{\"bufferView\":0,\"byteOffset\":4294967295,\"count\":%d,\"componentType\":%d,\"type\":\"%s\"}",
            LGSC_TEST_COUNT, invalid == 2 && i == 1 ? 5123 : 5126,
            i == 1 ? "VEC4" : i == 3 ? "SCALAR" : "VEC3");
  }

  fprintf(file, ",{\"bufferView\":1,\"count\":%d,\"componentType\":5126,\"type\":\"VEC4\"}],"
                "\"meshes\":[{\"primitives\":[", LGSC_TEST_COUNT);

  for (primitive = 0; primitive < 2; primitive++) {
    /* Case 10 shares the payload across degree-three and degree-zero users.
       The JSON parser visits the degree-zero primitive first. */
    primitiveDegree = invalid == 10 && primitive == 1 ? 0 : degree;
    primitiveCount  = 4 + (primitiveDegree + 1) * (primitiveDegree + 1);

    if (primitive) fputc(',', file);
    fprintf(file, "{\"mode\":%d,\"attributes\":{", invalid == 9 ? 4 : 0);

    for (i = 0; i < primitiveCount; i++) {
      lgsc_test_name(name, i);
      fprintf(file, "%s\"%s\":%u", i ? "," : "", name, i);
    }

    fprintf(file, ",\"COLOR_0\":%u},\"extensions\":{\"KHR_gaussian_splatting\":{"
                  "\"colorSpace\":\"srgb_rec709_display\",\"extensions\":{"
                  "\"KHR_gaussian_splatting_compression_lgsc\":{\"bufferView\":%d,\"numPoints\":%d,"
                  "\"shDegree\":%u,\"flags\":%d,\"attributes\":{",
            count, invalid == 7 ? 99 : 0, LGSC_TEST_COUNT + (invalid == 1),
            invalid == 5 ? 4 : primitiveDegree, invalid == 6 ? 1 : 0);

    for (i = 0; i < primitiveCount; i++) {
      if (invalid == 3 && i == 1)
        continue;

      lgsc_test_name(name, i);
      fprintf(file, "%s\"%s\":%u", i ? "," : "", name, invalid == 4 && i == 1 ? 999 : i);
    }

    fputs("}}}}}}", file);
  }

  fputs("]}],\"nodes\":[{\"mesh\":0}],\"scenes\":[{\"nodes\":[0]}],\"scene\":0}", file);
  fflush(file);

  if (!glbPath) {
    fclose(file);

    if (!(file = fopen("ak_test_lgsc.bin", "wb")))
      return false;

    fwrite(data->bytes, 1, data->size, file);

    for (i = (uint32_t)data->size; i < colorOffset; i++)
      fputc(0, file);

    fwrite(color, 1, sizeof(color), file);
    return fclose(file) == 0;
  }

  length     = (size_t)ftell(file);
  padded     = (length + 3) & ~(size_t)3;
  binarySize = colorOffset + sizeof(color);
  json       = malloc(padded);
  rewind(file);

  if (!json || fread(json, 1, length, file) != length) {
    free(json);
    fclose(file);
    return false;
  }

  fclose(file);
  memset(json + length, ' ', padded - length);

  if (!(glb = fopen(glbPath, "wb"))) {
    free(json);
    return false;
  }

  lgsc_test_u32(header, 0x46546c67);
  lgsc_test_u32(header + 4, 2);
  lgsc_test_u32(header + 8, (uint32_t)(28 + padded + binarySize));
  lgsc_test_u32(header + 12, (uint32_t)padded);
  lgsc_test_u32(header + 16, 0x4e4f534a);
  fwrite(header, 1, 20, glb);
  fwrite(json, 1, padded, glb);
  free(json);
  lgsc_test_u32(header, (uint32_t)binarySize);
  lgsc_test_u32(header + 4, 0x004e4942);
  fwrite(header, 1, 8, glb);
  fwrite(data->bytes, 1, data->size, glb);

  for (i = (uint32_t)data->size; i < colorOffset; i++)
    fputc(0, glb);

  fwrite(color, 1, sizeof(color), glb);
  return fclose(glb) == 0;
}

static
bool
lgsc_test_coord(const char *path) {
  AkDoc           *source, *baked;
  AkMeshPrimitive *expected, *actual;
  AkInput         *input, *other;
  uintptr_t        savedCoord, savedType;
  size_t           count, i;
  float            a[LGSC_TEST_COUNT * 4], b[LGSC_TEST_COUNT * 4];
  bool             ok;

  source     = NULL;
  baked      = NULL;
  savedCoord = ak_opt_get(AK_OPT_COORD);
  savedType  = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ok         = false;
  ak_opt_set(AK_OPT_COORD, (uintptr_t)AK_YUP);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);

  if (ak_load(&source, path, AK_FILE_TYPE_AUTO) != AK_OK)
    goto cleanup;

  ak_changeCoordSys(source, AK_ZUP);
  ak_opt_set(AK_OPT_COORD, (uintptr_t)AK_ZUP);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_ALL);

  if (ak_load(&baked, path, AK_FILE_TYPE_AUTO) != AK_OK || baked->coordSys != AK_ZUP)
    goto cleanup;

  expected = ak_meshFromGeometry(source->lib.geometries.first)->primitive;
  actual   = ak_meshFromGeometry(baked->lib.geometries.first)->primitive;

  for (; expected && actual; expected = expected->next, actual = actual->next) {
    for (input = expected->input; input; input = input->next) {
      for (other = actual->input; other; other = other->next)
        if (other->semantic == input->semantic && other->set == input->set)
          break;

      count = (size_t)input->accessor->count * input->accessor->componentCount;

      if (!other || count > LGSC_TEST_COUNT * 4
          || ak_accessorAsFloat(input->accessor, a, LGSC_TEST_COUNT * 4) != count
          || ak_accessorAsFloat(other->accessor, b, LGSC_TEST_COUNT * 4) != count)
        goto cleanup;

      for (i = 0; i < count; i++)
        if (fabsf(a[i] - b[i]) > 0.00001f)
          goto cleanup;
    }
  }

  ok = !expected && !actual;

cleanup:
  if (source) ak_free(source);
  if (baked) ak_free(baked);
  ak_opt_set(AK_OPT_COORD, savedCoord);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, savedType);
  return ok;
}

TEST_IMPL(lgsc_import) {
  AkDoc       *doc;
  uintptr_t    coord, autoload;
  LGSCTestData data;
  uint32_t     flags, layout;
  AkFileType   type;

  coord    = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  autoload = ak_opt_get(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  ak_opt_set(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD, 0);

  ASSERT(ak_probeFileType("capture.LGSC", &type) == AK_OK && type == AK_FILE_TYPE_LGSC);

  for (layout = 0; layout < 4; layout++) {
    for (flags = 0; flags < 64; flags++) {
      ASSERT(lgsc_test_data(&data, (uint8_t)(flags << 1), layout));
      ASSERT(lgsc_test_write("ak_test_lgsc.LGSC", data.bytes, data.size));
      ASSERT(ak_load(&doc, "ak_test_lgsc.LGSC", flags & 1 ? AK_FILE_TYPE_AUTO : AK_FILE_TYPE_LGSC) == AK_OK);
      ASSERT(doc->inf->ftype == AK_FILE_TYPE_LGSC);
      ASSERT(lgsc_test_values(doc, &data, 3, true));
      ak_free(doc);
    }
  }

  ASSERT(lgsc_test_coord("ak_test_lgsc.LGSC"));
  unlink("ak_test_lgsc.LGSC");
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, coord);
  ak_opt_set(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD, autoload);
  TEST_SUCCESS
}

TEST_IMPL(lgsc_gltf_import) {
  AkDoc           *doc;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *color;
  uintptr_t        coord;
  LGSCTestData     data;
  uint32_t         degree, binary, borrow, invalid;
  float            values[LGSC_TEST_COUNT * 4];

  coord = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ASSERT(lgsc_test_data(&data, 0x46, 3));

  for (borrow = 0; borrow < 2; borrow++) {
    ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, borrow ? AK_COORD_CVT_DISABLED : AK_COORD_CVT_ALL);

    for (degree = 0; degree < 4; degree++) {
      for (binary = 0; binary < 2; binary++) {
        ASSERT(lgsc_test_gltf("ak_test_lgsc.gltf", binary ? "ak_test_lgsc.glb" : NULL, &data, degree, 0));
        ASSERT(ak_load(&doc, binary ? "ak_test_lgsc.glb" : "ak_test_lgsc.gltf", AK_FILE_TYPE_AUTO) == AK_OK);
        ASSERT(lgsc_test_values(doc, &data, degree, false));
        mesh = ak_meshFromGeometry(doc->lib.geometries.first);
        prim = mesh->primitive;
        ASSERT(prim->next && prim->pos->accessor == prim->next->pos->accessor);
        ASSERT(prim->pos->accessor->buffer == prim->next->pos->accessor->buffer);
        for (color = prim->input; color && color->semantic != AK_INPUT_COLOR; color = color->next) {}
        ASSERT(color && ak_accessorAsFloat(color->accessor, values, LGSC_TEST_COUNT * 4) == LGSC_TEST_COUNT * 4);
        ASSERT(values[0] == 0 && values[LGSC_TEST_COUNT * 4 - 1] == 0);
        ak_free(doc);
      }
    }
  }

  for (invalid = 1; invalid <= 9; invalid++) {
    ASSERT(lgsc_test_gltf("ak_test_lgsc.gltf", NULL, &data, 3, invalid));
    doc = (void *)(uintptr_t)1;
    ASSERT(ak_load(&doc, "ak_test_lgsc.gltf", AK_FILE_TYPE_AUTO) != AK_OK && !doc);
  }

  ASSERT(lgsc_test_gltf("ak_test_lgsc.gltf", "ak_test_lgsc.glb", &data, 3, 10));
  ASSERT(ak_load(&doc, "ak_test_lgsc.glb", AK_FILE_TYPE_AUTO) == AK_OK);
  ASSERT(lgsc_test_values(doc, &data, 3, false));
  mesh = ak_meshFromGeometry(doc->lib.geometries.first);
  prim = mesh->primitive;
  ASSERT(prim->next && prim->next->gsplat->shDegree == 0);
  ASSERT(prim->pos->accessor == prim->next->pos->accessor);
  ASSERT(prim->pos->accessor->buffer->length == LGSC_TEST_COUNT * 59 * sizeof(float));
  ak_free(doc);

  ASSERT(lgsc_test_gltf("ak_test_lgsc.gltf", NULL, &data, 3, 10));
  ASSERT(lgsc_test_coord("ak_test_lgsc.gltf"));
  ASSERT(lgsc_test_coord("ak_test_lgsc.glb"));

  unlink("ak_test_lgsc.gltf");
  unlink("ak_test_lgsc.glb");
  unlink("ak_test_lgsc.bin");
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, coord);
  TEST_SUCCESS
}

TEST_IMPL(lgsc_reject_invalid) {
  AkDoc       *doc;
  LGSCTestData data, invalid;
  size_t       size;
  uint32_t     variant, geometryOffset;

  ASSERT(lgsc_test_data(&data, 0x46, 0));

  for (size = 0; size < data.size; size++) {
    ASSERT(lgsc_test_write("ak_test_lgsc_bad.lgsc", data.bytes, size));
    doc = (void *)(uintptr_t)1;
    ASSERT(ak_load(&doc, "ak_test_lgsc_bad.lgsc", AK_FILE_TYPE_AUTO) != AK_OK && !doc);
  }

  geometryOffset = 4 + LGSC_TEST_HEADER + 4;

  for (variant = 0; variant < 10; variant++) {
    invalid = data;

    switch (variant) {
      case 0: lgsc_test_u32(invalid.bytes + 4, UINT32_MAX); break;
      case 1: invalid.bytes[9] = 0; break;
      case 2: invalid.bytes[9] = 25; break;
      case 3: invalid.bytes[17] |= 0x80; break;
      case 4: invalid.bytes[8] = 9; break;
      case 5: lgsc_test_f32(invalid.bytes + 19, INFINITY); break;
      case 6: lgsc_test_f32(invalid.bytes + 19, 2); break;
      case 7: invalid.bytes[geometryOffset] = 0; break;
      case 8: invalid.bytes[geometryOffset + lgsc_test_read_u32(invalid.bytes + geometryOffset - 4) - 8] ^= 1; break;
      case 9: invalid.bytes[invalid.size++] = 0; break;
    }

    ASSERT(lgsc_test_write("ak_test_lgsc_bad.lgsc", invalid.bytes, invalid.size));
    ASSERT(ak_load(&doc, "ak_test_lgsc_bad.lgsc", AK_FILE_TYPE_AUTO) != AK_OK && !doc);
  }

  unlink("ak_test_lgsc_bad.lgsc");
  TEST_SUCCESS
}
