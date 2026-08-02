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

typedef struct AkTest3MFZipEntry {
  const char *name;
  const void *data;
  size_t      size;
} AkTest3MFZipEntry;

typedef struct AkTest3MFZipStored {
  uint32_t crc32;
  uint32_t localOffset;
} AkTest3MFZipStored;

static const char AK_TEST_3MF_GCODE_PART[] = "G1 X0 Y0 E0\n";

static uint16_t
ak_test_3mf_load_u16le(const unsigned char *bytes) {
  return (uint16_t)bytes[0] | ((uint16_t)bytes[1] << 8u);
}

static uint32_t
ak_test_3mf_load_u32le(const unsigned char *bytes) {
  return (uint32_t)bytes[0]
         | ((uint32_t)bytes[1] << 8u)
         | ((uint32_t)bytes[2] << 16u)
         | ((uint32_t)bytes[3] << 24u);
}

static bool
ak_test_3mf_zip_first_method(const char *path, uint16_t *method) {
  unsigned char header[10];
  FILE         *file;
  bool          ok;

  if (!path || !method)
    return false;

  file = fopen(path, "rb");
  if (!file)
    return false;

  ok = fread(header, 1, sizeof(header), file) == sizeof(header)
       && ak_test_3mf_load_u32le(header) == 0x04034b50u;
  fclose(file);

  if (!ok)
    return false;

  *method = ak_test_3mf_load_u16le(header + 8);
  return true;
}

static bool
ak_test_3mf_zip_entry_method(const char *path,
                             const char *entryName,
                             uint16_t   *method) {
  unsigned char *data;
  FILE          *file;
  long           fileSize;
  size_t         entryNameLen;
  size_t         centralOffset;
  size_t         centralSize;
  size_t         cursor;
  size_t         eocdPos;
  size_t         start;
  uint16_t       entryCount;
  uint16_t       i;
  bool           ok;

  if (!path || !entryName || !method)
    return false;

  *method = 0u;
  file = fopen(path, "rb");
  if (!file)
    return false;
  ok = fseek(file, 0, SEEK_END) == 0;
  fileSize = ok ? ftell(file) : -1;
  ok = ok && fileSize >= 22 && fseek(file, 0, SEEK_SET) == 0;
  data = ok ? malloc((size_t)fileSize) : NULL;
  if (!data)
    ok = false;
  if (ok)
    ok = fread(data, 1, (size_t)fileSize, file) == (size_t)fileSize;
  fclose(file);
  if (!ok) {
    free(data);
    return false;
  }

  eocdPos = (size_t)fileSize - 22u;
  start   = (size_t)fileSize > 65557u ? (size_t)fileSize - 65557u : 0u;
  for (;;) {
    if (ak_test_3mf_load_u32le(data + eocdPos) == 0x06054b50u)
      break;
    if (eocdPos == start) {
      free(data);
      return false;
    }
    eocdPos--;
  }

  entryCount    = ak_test_3mf_load_u16le(data + eocdPos + 10u);
  centralSize   = ak_test_3mf_load_u32le(data + eocdPos + 12u);
  centralOffset = ak_test_3mf_load_u32le(data + eocdPos + 16u);
  if (centralOffset > (size_t)fileSize
      || centralSize > (size_t)fileSize - centralOffset) {
    free(data);
    return false;
  }

  entryNameLen = strlen(entryName);
  cursor       = centralOffset;
  for (i = 0u; i < entryCount; i++) {
    uint16_t nameLen;
    uint16_t extraLen;
    uint16_t commentLen;
    uint16_t entryMethod;

    if (cursor > (size_t)fileSize || (size_t)fileSize - cursor < 46u
        || ak_test_3mf_load_u32le(data + cursor) != 0x02014b50u) {
      free(data);
      return false;
    }

    entryMethod = ak_test_3mf_load_u16le(data + cursor + 10u);
    nameLen     = ak_test_3mf_load_u16le(data + cursor + 28u);
    extraLen    = ak_test_3mf_load_u16le(data + cursor + 30u);
    commentLen  = ak_test_3mf_load_u16le(data + cursor + 32u);
    cursor += 46u;
    if (nameLen > (size_t)fileSize - cursor) {
      free(data);
      return false;
    }
    if (nameLen == entryNameLen
        && memcmp(data + cursor, entryName, entryNameLen) == 0) {
      *method = entryMethod;
      free(data);
      return true;
    }
    cursor += nameLen;
    if ((size_t)extraLen + (size_t)commentLen > (size_t)fileSize - cursor) {
      free(data);
      return false;
    }
    cursor += (size_t)extraLen + (size_t)commentLen;
  }

  free(data);
  return false;
}

static uint32_t
ak_test_3mf_crc32(const void *data, size_t size) {
  const unsigned char *bytes;
  uint32_t             crc;
  size_t               i;

  bytes = data;
  crc   = 0xffffffffu;
  for (i = 0; i < size; i++) {
    uint32_t j;

    crc ^= bytes[i];
    for (j = 0; j < 8u; j++)
      crc = (crc >> 1u) ^ (0xedb88320u & (uint32_t)-(int32_t)(crc & 1u));
  }

  return crc ^ 0xffffffffu;
}

static bool
ak_test_3mf_write_u16le(FILE *file, uint16_t value) {
  unsigned char out[2];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  return fwrite(out, 1, sizeof(out), file) == sizeof(out);
}

static bool
ak_test_3mf_write_u32le(FILE *file, uint32_t value) {
  unsigned char out[4];

  out[0] = (unsigned char)(value & 0xffu);
  out[1] = (unsigned char)((value >> 8u) & 0xffu);
  out[2] = (unsigned char)((value >> 16u) & 0xffu);
  out[3] = (unsigned char)((value >> 24u) & 0xffu);
  return fwrite(out, 1, sizeof(out), file) == sizeof(out);
}

static bool
ak_test_3mf_zip_write_local(FILE                       *file,
                            const AkTest3MFZipEntry   *entry,
                            AkTest3MFZipStored        *stored) {
  long   offset;
  size_t nameLen;

  if (!file || !entry || !entry->name || (!entry->data && entry->size > 0u))
    return false;
  if (entry->size > UINT32_MAX)
    return false;

  nameLen = strlen(entry->name);
  if (nameLen == 0u || nameLen > UINT16_MAX)
    return false;

  offset = ftell(file);
  if (offset < 0 || offset > UINT32_MAX)
    return false;

  stored->crc32       = ak_test_3mf_crc32(entry->data, entry->size);
  stored->localOffset = (uint32_t)offset;

  return ak_test_3mf_write_u32le(file, 0x04034b50u)
         && ak_test_3mf_write_u16le(file, 20u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u32le(file, stored->crc32)
         && ak_test_3mf_write_u32le(file, (uint32_t)entry->size)
         && ak_test_3mf_write_u32le(file, (uint32_t)entry->size)
         && ak_test_3mf_write_u16le(file, (uint16_t)nameLen)
         && ak_test_3mf_write_u16le(file, 0u)
         && fwrite(entry->name, 1, nameLen, file) == nameLen
         && fwrite(entry->data, 1, entry->size, file) == entry->size;
}

static bool
ak_test_3mf_zip_write_central(FILE                         *file,
                              const AkTest3MFZipEntry     *entry,
                              const AkTest3MFZipStored    *stored) {
  size_t nameLen;

  nameLen = strlen(entry->name);
  if (nameLen == 0u || nameLen > UINT16_MAX || entry->size > UINT32_MAX)
    return false;

  return ak_test_3mf_write_u32le(file, 0x02014b50u)
         && ak_test_3mf_write_u16le(file, 20u)
         && ak_test_3mf_write_u16le(file, 20u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u32le(file, stored->crc32)
         && ak_test_3mf_write_u32le(file, (uint32_t)entry->size)
         && ak_test_3mf_write_u32le(file, (uint32_t)entry->size)
         && ak_test_3mf_write_u16le(file, (uint16_t)nameLen)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u32le(file, 0u)
         && ak_test_3mf_write_u32le(file, stored->localOffset)
         && fwrite(entry->name, 1, nameLen, file) == nameLen;
}

static bool
ak_test_3mf_zip_write_stored(const char                 *path,
                             const AkTest3MFZipEntry   *entries,
                             size_t                     entryCount) {
  AkTest3MFZipStored *stored;
  FILE               *file;
  long                centralOffset;
  long                centralEnd;
  size_t              i;
  bool                ok;

  if (!path || !entries || entryCount == 0u || entryCount > UINT16_MAX)
    return false;

  stored = calloc(entryCount, sizeof(*stored));
  if (!stored)
    return false;

  file = fopen(path, "wb");
  if (!file) {
    free(stored);
    return false;
  }

  ok = true;
  for (i = 0; i < entryCount; i++) {
    if (!ak_test_3mf_zip_write_local(file, &entries[i], &stored[i])) {
      ok = false;
      break;
    }
  }

  centralOffset = ftell(file);
  if (centralOffset < 0 || centralOffset > UINT32_MAX)
    ok = false;

  if (ok) {
    for (i = 0; i < entryCount; i++) {
      if (!ak_test_3mf_zip_write_central(file, &entries[i], &stored[i])) {
        ok = false;
        break;
      }
    }
  }

  centralEnd = ftell(file);
  if (centralEnd < 0 || centralEnd < centralOffset
      || centralEnd - centralOffset > UINT32_MAX)
    ok = false;

  if (ok) {
    ok = ak_test_3mf_write_u32le(file, 0x06054b50u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, 0u)
         && ak_test_3mf_write_u16le(file, (uint16_t)entryCount)
         && ak_test_3mf_write_u16le(file, (uint16_t)entryCount)
         && ak_test_3mf_write_u32le(file, (uint32_t)(centralEnd - centralOffset))
         && ak_test_3mf_write_u32le(file, (uint32_t)centralOffset)
         && ak_test_3mf_write_u16le(file, 0u);
  }

  if (fclose(file) != 0)
    ok = false;

  free(stored);
  if (!ok)
    remove(path);

  return ok;
}

static bool
ak_test_write_3mf_production_child_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/child.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources/>"
    "<build p:UUID=\"build-uuid\">"
    "<item objectid=\"7\" p:path=\"/3D/child.model\" p:UUID=\"item-uuid\"/>"
    "</build>"
    "</model>";
  static const char childModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources>"
    "<object id=\"7\" type=\"model\" p:UUID=\"object-uuid\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  AkTest3MFZipEntry entries[4];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/child.model";
  entries[3].data = childModel;
  entries[3].size = sizeof(childModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_production_child_model_with_extra(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/child.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/Metadata/blob.bin\" ContentType=\"application/octet-stream\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources/>"
    "<build p:UUID=\"build-uuid\">"
    "<item objectid=\"7\" p:path=\"/3D/child.model\" p:UUID=\"item-uuid\"/>"
    "</build>"
    "</model>";
  static const char childModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources>"
    "<object id=\"7\" type=\"model\" p:UUID=\"object-uuid\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  static const char blob[] = "preserved source package data";
  AkTest3MFZipEntry entries[5];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/child.model";
  entries[3].data = childModel;
  entries[3].size = sizeof(childModel) - 1u;
  entries[4].name = "Metadata/blob.bin";
  entries[4].data = blob;
  entries[4].size = sizeof(blob) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_bambu_component_materials_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/Objects/object_40.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/Objects/object_72.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources>"
    "<object id=\"2\" type=\"model\" name=\"Ribbon\">"
    "<components>"
    "<component objectid=\"1\" p:path=\"/3D/Objects/object_40.model\" "
    "transform=\"1 0 0 0 1 0 0 0 1 10 0 0\"/>"
    "</components>"
    "</object>"
    "<object id=\"5\" type=\"model\" name=\"Hat\">"
    "<components>"
    "<component objectid=\"3\" p:path=\"/3D/Objects/object_72.model\" "
    "transform=\"1 0 0 0 1 0 0 0 1 0 20 0\"/>"
    "<component objectid=\"4\" p:path=\"/3D/Objects/object_72.model\" "
    "transform=\"1 0 0 0 1 0 0 0 1 0 0 30\"/>"
    "</components>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"5\"/><item objectid=\"2\"/></build>"
    "</model>";
  static const char object40Model[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
    "<resources>"
    "<object id=\"1\" type=\"model\" name=\"Ribbon Mesh\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  static const char object72Model[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
    "<resources>"
    "<object id=\"3\" type=\"model\" name=\"Hat Shell\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"2\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"2\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"4\" type=\"model\" name=\"Hat Brim\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"3\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"3\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  static const char projectSettings[] =
    "{\"filament_colour\":[\"#F4A925\",\"#C52C18\"]}";
  static const char modelSettings[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<config>"
    "<object id=\"2\">"
    "<metadata key=\"extruder\" value=\"2\"/>"
    "<part id=\"1\"/>"
    "</object>"
    "<object id=\"5\">"
    "<metadata key=\"extruder\" value=\"1\"/>"
    "<part id=\"3\"/>"
    "<part id=\"4\"><metadata key=\"extruder\" value=\"1\"/></part>"
    "</object>"
    "</config>";
  AkTest3MFZipEntry entries[7];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/Objects/object_40.model";
  entries[3].data = object40Model;
  entries[3].size = sizeof(object40Model) - 1u;
  entries[4].name = "3D/Objects/object_72.model";
  entries[4].data = object72Model;
  entries[4].size = sizeof(object72Model) - 1u;
  entries[5].name = "Metadata/project_settings.config";
  entries[5].data = projectSettings;
  entries[5].size = sizeof(projectSettings) - 1u;
  entries[6].name = "Metadata/model_settings.config";
  entries[6].data = modelSettings;
  entries[6].size = sizeof(modelSettings) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_bambu_paint_materials_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/Objects/object_1.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources>"
    "<object id=\"2\" type=\"model\" name=\"Painted Assembly\">"
    "<components>"
    "<component objectid=\"1\" p:path=\"/3D/Objects/object_1.model\"/>"
    "</components>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"2\"/></build>"
    "</model>";
  static const char objectModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
    "<resources>"
    "<object id=\"1\" type=\"model\" name=\"Painted Mesh\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles>"
    "<triangle v1=\"0\" v2=\"1\" v3=\"2\" paint_color=\"4\"/>"
    "<triangle v1=\"0\" v2=\"2\" v3=\"3\" paint_color=\"2C\"/>"
    "<triangle v1=\"1\" v2=\"3\" v3=\"2\"/>"
    "</triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  static const char projectSettings[] =
    "{\"filament_colour\":[\"#111111\",\"#222222\",\"#333333\",\"#444444\",\"#555555\"]}";
  static const char modelSettings[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<config>"
    "<object id=\"2\">"
    "<metadata key=\"extruder\" value=\"3\"/>"
    "<part id=\"1\"/>"
    "</object>"
    "</config>";
  AkTest3MFZipEntry entries[6];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/Objects/object_1.model";
  entries[3].data = objectModel;
  entries[3].size = sizeof(objectModel) - 1u;
  entries[4].name = "Metadata/project_settings.config";
  entries[4].data = projectSettings;
  entries[4].size = sizeof(projectSettings) - 1u;
  entries[5].name = "Metadata/model_settings.config";
  entries[5].data = modelSettings;
  entries[5].size = sizeof(modelSettings) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_materials_extension_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Default Extension=\"png\" ContentType=\"image/png\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char model[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"m\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:m=\"http://schemas.microsoft.com/3dmanufacturing/material/2015/02\">"
    "<resources>"
    "<basematerials id=\"1\">"
    "<base name=\"red\" displaycolor=\"#FF0000FF\"/>"
    "<base name=\"blue\" displaycolor=\"#0000FFFF\"/>"
    "</basematerials>"
    "<m:colorgroup id=\"2\">"
    "<m:color color=\"#00FF0080\"/>"
    "</m:colorgroup>"
    "<m:compositematerials id=\"3\" matid=\"1\" matindices=\"0 1\">"
    "<m:composite values=\"0.25 0.75\"/>"
    "</m:compositematerials>"
    "<m:multiproperties id=\"4\" pids=\"3 2\" blendmethods=\"mix\">"
    "<m:multi pindices=\"0 0\"/>"
    "</m:multiproperties>"
    "<m:texture2d id=\"5\" path=\"/3D/Textures/diffuse.png\" contenttype=\"image/png\"/>"
    "<m:texture2dgroup id=\"6\" texid=\"5\">"
    "<m:tex2coord u=\"0\" v=\"0\"/>"
    "<m:tex2coord u=\"1\" v=\"0\"/>"
    "<m:tex2coord u=\"0\" v=\"1\"/>"
    "</m:texture2dgroup>"
    "<object id=\"10\" type=\"model\" name=\"multi\" pid=\"4\" pindex=\"0\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"11\" type=\"model\" name=\"textured\" pid=\"6\" pindex=\"0\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"1\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"1\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"1\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\" p1=\"0\" p2=\"1\" p3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"10\"/><item objectid=\"11\"/></build>"
    "</model>";
  static const unsigned char texture[] = {
    0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'
  };
  AkTest3MFZipEntry entries[4];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = model;
  entries[2].size = sizeof(model) - 1u;
  entries[3].name = "3D/Textures/diffuse.png";
  entries[3].data = texture;
  entries[3].size = sizeof(texture);

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_component_transform_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/3D/Objects/object_1.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\">"
    "<resources>"
    "<object id=\"2\" type=\"model\" name=\"Assembly\">"
    "<components>"
    "<component objectid=\"1\" p:path=\"/3D/Objects/object_1.model\" "
    "transform=\"1 2 3 4 5 6 7 8 9 10 11 12\"/>"
    "</components>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"2\"/></build>"
    "</model>";
  static const char objectModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\">"
    "<resources>"
    "<object id=\"1\" type=\"model\" name=\"Part\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build/>"
    "</model>";
  AkTest3MFZipEntry entries[4];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/Objects/object_1.model";
  entries[3].data = objectModel;
  entries[3].size = sizeof(objectModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_production_alternatives_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"p pa\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:p=\"http://schemas.microsoft.com/3dmanufacturing/production/2015/06\" "
    "xmlns:pa=\"http://schemas.microsoft.com/3dmanufacturing/production/alternatives/2021/04\">"
    "<resources>"
    "<object id=\"1\" type=\"model\" p:UUID=\"object-uuid\" pa:modelresolution=\"lowres\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "<pa:alternatives>"
    "<pa:alternative objectid=\"2\" UUID=\"alternative-uuid\" "
    "path=\"/3D/full.model\" modelresolution=\"fullres\"/>"
    "</pa:alternatives>"
    "</object>"
    "</resources>"
    "<build p:UUID=\"build-uuid\">"
    "<item objectid=\"1\" p:UUID=\"item-uuid\"/>"
    "</build>"
    "</model>";
  AkTest3MFZipEntry entries[3];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_slice_child_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "<Override PartName=\"/2D/slices.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char modelRels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel1\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/2D/slices.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"s\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:s=\"http://schemas.microsoft.com/3dmanufacturing/slice/2015/07\">"
    "<resources>"
    "<s:slicestack id=\"10\" zbottom=\"0\">"
    "<s:sliceref slicestackid=\"20\" slicepath=\"/2D/slices.model\" ztop=\"1\"/>"
    "</s:slicestack>"
    "<object id=\"7\" type=\"model\" name=\"slice-root\" "
    "s:meshresolution=\"lowres\" s:slicestackid=\"10\" s:slicepath=\"/2D/slices.model\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"7\"/></build>"
    "</model>";
  static const char sliceModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:s=\"http://schemas.microsoft.com/3dmanufacturing/slice/2015/07\">"
    "<resources>"
    "<s:slicestack id=\"20\" zbottom=\"0\">"
    "<s:slice ztop=\"0.5\">"
    "<s:vertices>"
    "<s:vertex x=\"0\" y=\"0\"/>"
    "<s:vertex x=\"1\" y=\"0\"/>"
    "<s:vertex x=\"0\" y=\"1\"/>"
    "</s:vertices>"
    "<s:polygon startv=\"0\">"
    "<s:segment v2=\"1\" p1=\"0\" p2=\"0\" pid=\"1\"/>"
    "<s:segment v2=\"2\" p1=\"0\" p2=\"0\" pid=\"1\"/>"
    "<s:segment v2=\"0\" p1=\"0\" p2=\"0\" pid=\"1\"/>"
    "</s:polygon>"
    "</s:slice>"
    "</s:slicestack>"
    "</resources>"
    "<build/>"
    "</model>";
  AkTest3MFZipEntry entries[5];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/_rels/3dmodel.model.rels";
  entries[2].data = modelRels;
  entries[2].size = sizeof(modelRels) - 1u;
  entries[3].name = "3D/3dmodel.model";
  entries[3].data = rootModel;
  entries[3].size = sizeof(rootModel) - 1u;
  entries[4].name = "2D/slices.model";
  entries[4].data = sliceModel;
  entries[4].size = sizeof(sliceModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_beam_lattice_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"b b2\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:b=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/2017/02\" "
    "xmlns:b2=\"http://schemas.microsoft.com/3dmanufacturing/beamlattice/balls/2020/07\">"
    "<resources>"
    "<object id=\"1\" type=\"model\" name=\"beam-root\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<b:beamlattice radius=\"0.1\" minlength=\"0.001\" cap=\"sphere\" "
    "b2:ballmode=\"mixed\" b2:ballradius=\"0.2\">"
    "<b:beams>"
    "<b:beam v1=\"0\" v2=\"1\" r1=\"0.15\" r2=\"0.16\" cap1=\"sphere\"/>"
    "<b:beam v1=\"1\" v2=\"2\"/>"
    "</b:beams>"
    "<b2:balls>"
    "<b2:ball vindex=\"0\" r=\"0.25\"/>"
    "</b2:balls>"
    "</b:beamlattice>"
    "</mesh>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"1\"/></build>"
    "</model>";
  AkTest3MFZipEntry entries[3];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_boolean_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"bo\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:bo=\"http://schemas.3mf.io/3dmanufacturing/booleanoperations/2023/07\">"
    "<resources>"
    "<object id=\"2\" type=\"model\" name=\"cutter\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"1\" type=\"model\" name=\"base\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"2\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"2\" z=\"0\"/>"
    "</vertices>"
    "<triangles><triangle v1=\"0\" v2=\"1\" v3=\"2\"/></triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"3\" type=\"model\" name=\"cut-base\">"
    "<bo:booleanshape objectid=\"1\" operation=\"difference\" "
    "transform=\"1 0 0 0 1 0 0 0 1 0 0 0\">"
    "<bo:boolean objectid=\"2\" transform=\"1 0 0 0 1 0 0 0 1 0.25 0 0\"/>"
    "</bo:booleanshape>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"3\"/></build>"
    "</model>";
  AkTest3MFZipEntry entries[3];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

static bool
ak_test_write_3mf_displacement_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Default Extension=\"png\" ContentType=\"image/png\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"d\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:d=\"http://schemas.3mf.io/3dmanufacturing/displacement/2023/10\">"
    "<resources>"
    "<d:displacement2d id=\"5\" path=\"/3D/Textures/height.png\" channel=\"R\" "
    "tilestyleu=\"clamp\" tilestylev=\"mirror\" filter=\"nearest\"/>"
    "<d:normvectorgroup id=\"6\">"
    "<d:normvector x=\"0\" y=\"0\" z=\"1\"/>"
    "<d:normvector x=\"0\" y=\"1\" z=\"0\"/>"
    "<d:normvector x=\"1\" y=\"0\" z=\"0\"/>"
    "<d:normvector x=\"0\" y=\"0\" z=\"-1\"/>"
    "</d:normvectorgroup>"
    "<d:disp2dgroup id=\"7\" dispid=\"5\" nid=\"6\" height=\"0.25\" offset=\"-0.05\">"
    "<d:disp2dcoord u=\"0\" v=\"0\" n=\"0\" f=\"0.5\"/>"
    "<d:disp2dcoord u=\"1\" v=\"0\" n=\"1\"/>"
    "<d:disp2dcoord u=\"0\" v=\"1\" n=\"2\" f=\"0.75\"/>"
    "<d:disp2dcoord u=\"1\" v=\"1\" n=\"3\"/>"
    "</d:disp2dgroup>"
    "<object id=\"1\" type=\"model\" name=\"displaced-tetra\">"
    "<d:displacementmesh>"
    "<d:vertices>"
    "<d:vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<d:vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<d:vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "<d:vertex x=\"0\" y=\"0\" z=\"1\"/>"
    "</d:vertices>"
    "<d:triangles did=\"7\">"
    "<d:triangle v1=\"0\" v2=\"1\" v3=\"2\" d1=\"0\" d2=\"1\" d3=\"2\"/>"
    "<d:triangle v1=\"0\" v2=\"3\" v3=\"1\" d1=\"0\" d2=\"3\" d3=\"1\"/>"
    "<d:triangle v1=\"1\" v2=\"3\" v3=\"2\" d1=\"1\" d2=\"3\" d3=\"2\"/>"
    "<d:triangle v1=\"2\" v2=\"3\" v3=\"0\" did=\"7\" d1=\"2\" d2=\"3\" d3=\"0\"/>"
    "</d:triangles>"
    "</d:displacementmesh>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"1\"/></build>"
    "</model>";
  static const unsigned char pngData[] = {
    0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au
  };
  AkTest3MFZipEntry entries[4];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/Textures/height.png";
  entries[3].data = pngData;
  entries[3].size = sizeof(pngData);

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

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
  const float colors[12] = {
    ak_sRGB_linearf(128.0f / 255.0f), 0.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f, 128.0f / 255.0f
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
  colorInput->accessor = ak_test_make_float_accessor(heap,
                                                     colorInput,
                                                     colors,
                                                     4,
                                                     3);
  if (!colorInput->accessor)
    return NULL;
  colorInput->next = prim->input;
  prim->input      = colorInput;
  prim->inputCount++;

  return doc;
}

static AkDoc *
ak_test_make_3mf_package_triangle_doc(void) {
  static const unsigned char thumbnail[] = {
    0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'
  };
  AkDoc           *doc;
  AkPrintPackagePart *part;

  doc = ak_test_make_3mf_triangle_doc();
  if (!doc)
    return NULL;

  part = ak_printAddPackagePartData(
    doc,
    AK_PRINT_PACKAGE_PART_THUMBNAIL,
    "Metadata/thumbnail.png",
    "image/png",
    "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail",
    thumbnail,
    sizeof(thumbnail));
  if (!part)
    return NULL;
  if (!ak_printSetPackagePartRelationship(doc, part, "thumb-rel", "Internal"))
    return NULL;
  part = ak_printAddPackagePartData(doc,
                                    AK_PRINT_PACKAGE_PART_GCODE,
                                    "Metadata/plate_1.gcode",
                                    "text/x.gcode",
                                    NULL,
                                    AK_TEST_3MF_GCODE_PART,
                                    sizeof(AK_TEST_3MF_GCODE_PART) - 1u);
  if (!part)
    return NULL;

  return doc;
}

static AkDoc *
ak_test_make_3mf_geometry_doc(const float *positions,
                              uint32_t     positionCount,
                              AkGeometry **outGeom) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkGeometry *geom;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!heap || !doc)
    return NULL;
  ak_heap_setdata(heap, doc);

  geom = ak_test_make_geom_with_positions(heap, doc, positions, positionCount);
  if (!geom)
    return NULL;

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;
  if (outGeom)
    *outGeom = geom;

  return doc;
}

static uint32_t
ak_test_3mf_count_node_tree(const AkNode *node) {
  uint32_t count;

  count = 0u;
  for (; node; node = node->next) {
    count++;
    if (node->chld)
      count += ak_test_3mf_count_node_tree(node->chld);
  }

  return count;
}

static uint32_t
ak_test_3mf_count_geometry_nodes(const AkNode *node) {
  uint32_t count;

  count = 0u;
  for (; node; node = node->next) {
    if (node->geometry)
      count++;
    if (node->chld)
      count += ak_test_3mf_count_geometry_nodes(node->chld);
  }

  return count;
}

static AkMaterial*
ak_test_3mf_find_material(AkDoc *doc, const char *name) {
  AkMaterial *material;

  if (!doc || !name)
    return NULL;

  for (material = doc->lib.materials.first; material; material = material->next) {
    if (material->name && strcmp(material->name, name) == 0)
      return material;
  }

  return NULL;
}

static bool
ak_test_3mf_material_color_near(AkMaterial *material,
                                float       r,
                                float       g,
                                float       b) {
  AkMaterialInput *baseColor;

  if (!material || !material->surface)
    return false;

  baseColor = material->surface->baseColor;
  if (!baseColor || baseColor->source != AK_MATERIAL_INPUT_CONSTANT)
    return false;

  return fabs(baseColor->color.rgba.R - ak_sRGB_linearf(r)) < 0.001f
         && fabs(baseColor->color.rgba.G - ak_sRGB_linearf(g)) < 0.001f
         && fabs(baseColor->color.rgba.B - ak_sRGB_linearf(b)) < 0.001f
         && fabs(baseColor->color.rgba.A - 1.0f) < 0.001f;
}

static uint32_t
ak_test_3mf_count_primitives_with_material(AkDoc *doc,
                                           AkMaterial *material) {
  AkGeometry *geom;
  uint32_t    count;

  count = 0u;
  if (!doc || !material)
    return 0u;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    AkMesh *mesh;
    AkMeshPrimitive *prim;

    mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
    for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
      if (prim->material == material)
        count++;
    }
  }

  return count;
}

static AkMaterialPropertySet*
ak_test_3mf_find_property_set(AkDoc *doc, uint32_t id) {
  AkMaterialPropertySet *set;

  if (!doc)
    return NULL;

  for (set = doc->materialProperties.sets; set; set = set->next) {
    if (set->id == id)
      return set;
  }

  return NULL;
}

static uint32_t
ak_test_3mf_count_primitive_inputs(AkDoc *doc, AkInputSemantic semantic) {
  AkGeometry *geom;
  uint32_t    count;

  count = 0u;
  if (!doc)
    return 0u;

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    AkMesh          *mesh;
    AkMeshPrimitive *prim;

    mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
    for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
      AkInput *input;

      for (input = prim->input; input; input = input->next) {
        if (input->semantic == semantic) {
          count++;
          break;
        }
      }
    }
  }

  return count;
}

TEST_IMPL(three_mf_export_triangle_roundtrip) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkPrintDocument *print;
  struct stat      st;
  uintptr_t        savedCompressionLevel;
  uint16_t         zipMethod;
  const char      *outDir  = "./assetkit_export_3mf_triangle_roundtrip";
  const char      *mfPath  = "./assetkit_export_3mf_triangle_roundtrip/model.3mf";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_3mf_triangle_doc();
  ASSERT(doc != NULL);

  savedCompressionLevel = ak_opt_get(AK_OPT_ZIP_EXPORT_COMPRESSION_LEVEL);
  ak_opt_set(AK_OPT_ZIP_EXPORT_COMPRESSION_LEVEL, 0u);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_3MF) == AK_OK);
  ak_opt_set(AK_OPT_ZIP_EXPORT_COMPRESSION_LEVEL, savedCompressionLevel);
  ASSERT(stat(mfPath, &st) == 0);
  ASSERT(st.st_size > 0);
  ASSERT(ak_test_3mf_zip_first_method(mfPath, &zipMethod));
  ASSERT(zipMethod == 0u);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->coordSys == AK_ZUP);
  ASSERT(roundTrip->unit != NULL);
  ASSERT(fabs(roundTrip->unit->dist - 0.001) < 0.000001);
  ASSERT(roundTrip->lib.geometries.count == 1);
  print = ak_printDocument(roundTrip);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_CORE));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_PACKAGE));
  ASSERT(print->packagePartCount == 1);
  ASSERT(print->objectCount == 1);
  ASSERT(print->meshObjectCount == 1);
  ASSERT(print->componentObjectCount == 0);
  ASSERT(print->buildItemCount == 1);

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

TEST_IMPL(three_mf_print_validate_mesh_flags) {
  const float degeneratePositions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    2.0f, 0.0f, 0.0f
  };
  const float nonManifoldPositions[15] = {
    0.0f,  0.0f, 0.0f,
    1.0f,  0.0f, 0.0f,
    0.0f,  1.0f, 0.0f,
    0.0f, -1.0f, 0.0f,
    0.0f,  0.0f, 1.0f
  };
  const uint32_t nonManifoldIndices[9] = {
    0u, 1u, 2u,
    1u, 0u, 3u,
    0u, 1u, 4u
  };
  const float negativeMatrix[16] = {
    -1.0f, 0.0f, 0.0f, 0.0f,
     0.0f, 1.0f, 0.0f, 0.0f,
     0.0f, 0.0f, 1.0f, 0.0f,
     0.0f, 0.0f, 0.0f, 1.0f
  };
  AkDoc                 *openDoc;
  AkDoc                 *degenerateDoc;
  AkDoc                 *nonManifoldDoc;
  AkDoc                 *negativeDoc;
  AkGeometry            *geom;
  AkMesh                *mesh;
  AkMeshPrimitive       *prim;
  AkNode                *negativeNode;
  AkPrintDocument       *print;
  AkPrintValidationFlags flags;
  AkHeap                *heap;
  uint32_t               i;

  openDoc = ak_test_make_3mf_triangle_doc();
  ASSERT(openDoc != NULL);
  flags = ak_printValidate(openDoc, AK_PRINT_VALIDATION_NONE);
  ASSERT((flags & AK_PRINT_VALIDATION_OPEN_BOUNDARY) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES) == 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_NON_MANIFOLD) == 0u);
  print = ak_printDocument(openDoc);
  ASSERT(print != NULL);
  ASSERT((print->validationFlags & AK_PRINT_VALIDATION_OPEN_BOUNDARY) != 0u);

  degenerateDoc = ak_test_make_3mf_geometry_doc(degeneratePositions, 3u, NULL);
  ASSERT(degenerateDoc != NULL);
  flags = ak_printValidate(degenerateDoc, AK_PRINT_VALIDATION_NONE);
  ASSERT((flags & AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_OPEN_BOUNDARY) == 0u);
  print = ak_printDocument(degenerateDoc);
  ASSERT(print != NULL);
  ASSERT((print->validationFlags & AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES)
         != 0u);

  geom = NULL;
  nonManifoldDoc = ak_test_make_3mf_geometry_doc(nonManifoldPositions,
                                                 5u,
                                                 &geom);
  ASSERT(nonManifoldDoc != NULL);
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  heap = ak_heap_getheap(nonManifoldDoc);
  ASSERT(heap != NULL);
  prim->indices = ak_indexArrayAlloc(heap, prim, 9u, AKT_UINT);
  ASSERT(prim->indices != NULL);
  for (i = 0u; i < 9u; i++)
    ASSERT(ak_indexArraySet(heap, prim, &prim->indices, i, nonManifoldIndices[i]));
  flags = ak_printValidate(nonManifoldDoc, AK_PRINT_VALIDATION_NONE);
  ASSERT((flags & AK_PRINT_VALIDATION_NON_MANIFOLD) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_OPEN_BOUNDARY) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES) == 0u);

  negativeDoc = ak_test_make_3mf_triangle_doc();
  ASSERT(negativeDoc != NULL);
  ASSERT(negativeDoc->scene != NULL);
  ASSERT(negativeDoc->scene->node != NULL);
  negativeNode = negativeDoc->scene->node->chld;
  ASSERT(negativeNode != NULL);
  ak_nodeSetTransformMatrix(negativeNode, negativeMatrix);
  flags = ak_printValidate(negativeDoc, AK_PRINT_VALIDATION_NEGATIVE_SCALE);
  ASSERT(flags == AK_PRINT_VALIDATION_NEGATIVE_SCALE);
  print = ak_printDocument(negativeDoc);
  ASSERT(print != NULL);
  ASSERT((print->validationFlags & AK_PRINT_VALIDATION_NEGATIVE_SCALE) != 0u);

  flags = ak_printValidate(negativeDoc, AK_PRINT_VALIDATION_NONE);
  ASSERT((flags & AK_PRINT_VALIDATION_NEGATIVE_SCALE) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_OPEN_BOUNDARY) != 0u);
  ASSERT((flags & AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES) == 0u);

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
  AkPrintDocument *print;
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
  print = ak_printDocument(roundTrip);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_CORE));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_MATERIALS));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_PACKAGE));
  ASSERT(print->materialGroupCount == 1);
  ASSERT(print->materialPropertyCount == 3);

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
  ASSERT(colorAcc->normalized);
  ASSERT(colorAcc->originallyNormalized);
  ASSERT(colorAcc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(colorAcc->count == 3);
  ASSERT(((uint8_t *)colorAcc->buffer->data)[0] == 55u);
  ASSERT(((uint8_t *)colorAcc->buffer->data)[3] == 255u);
  ASSERT(prim->material != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_export_package_parts_roundtrip) {
  static const unsigned char thumbnail[] = {
    0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'
  };
  AkDoc               *doc;
  AkDoc               *roundTrip;
  AkPrintDocument     *print;
  AkPrintPackagePart  *part;
  AkPrintPackagePart  *thumbnailPart;
  AkPrintPackagePart  *gcodePart;
  const char          *outDir = "./assetkit_export_3mf_package_parts_roundtrip";
  const char          *mfPath = "./assetkit_export_3mf_package_parts_roundtrip/model.3mf";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_3mf_package_triangle_doc();
  ASSERT(doc != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_3MF) == AK_OK);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);

  print = ak_printDocument(roundTrip);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_PACKAGE));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_THUMBNAIL));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_SLICE));
  ASSERT(print->packagePartCount == 3);

  thumbnailPart = NULL;
  gcodePart     = NULL;
  for (part = print->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_THUMBNAIL) {
      thumbnailPart = part;
    } else if (part->type == AK_PRINT_PACKAGE_PART_GCODE) {
      gcodePart = part;
    }
  }
  ASSERT(thumbnailPart != NULL);
  ASSERT(thumbnailPart->name != NULL);
  ASSERT(strcmp(thumbnailPart->name, "Metadata/thumbnail.png") == 0);
  ASSERT(thumbnailPart->contentType != NULL);
  ASSERT(strcmp(thumbnailPart->contentType, "image/png") == 0);
  ASSERT(thumbnailPart->relationshipType != NULL);
  ASSERT(strstr(thumbnailPart->relationshipType, "thumbnail") != NULL);
  ASSERT(thumbnailPart->relationshipId != NULL);
  ASSERT(strcmp(thumbnailPart->relationshipId, "thumb-rel") == 0);
  ASSERT(thumbnailPart->relationshipTargetMode != NULL);
  ASSERT(strcmp(thumbnailPart->relationshipTargetMode, "Internal") == 0);
  ASSERT(thumbnailPart->size == sizeof(thumbnail));
  ASSERT(thumbnailPart->data != NULL);
  ASSERT(memcmp(thumbnailPart->data, thumbnail, sizeof(thumbnail)) == 0);
  ASSERT(gcodePart != NULL);
  ASSERT(gcodePart->name != NULL);
  ASSERT(strcmp(gcodePart->name, "Metadata/plate_1.gcode") == 0);
  ASSERT(gcodePart->contentType != NULL);
  ASSERT(strcmp(gcodePart->contentType, "text/x.gcode") == 0);
  ASSERT(gcodePart->size == sizeof(AK_TEST_3MF_GCODE_PART) - 1u);
  ASSERT(gcodePart->data != NULL);
  ASSERT(memcmp(gcodePart->data,
                AK_TEST_3MF_GCODE_PART,
                sizeof(AK_TEST_3MF_GCODE_PART) - 1u) == 0);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_export_preserved_model_part_raw_copy) {
  static const char mutatedBlob[] = "mutated data";
  AkDoc              *doc;
  AkDoc              *roundTrip;
  AkPrintDocument    *print;
  AkPrintPackagePart *part;
  AkPrintPackagePart *blobPart;
  uint16_t            method;
  const char         *outDir = "./assetkit_import_3mf_raw_copy_source";
  const char         *mfPath = "./assetkit_import_3mf_raw_copy_source/model.3mf";
  const char         *roundTripDir = "./assetkit_export_3mf_raw_copy_source";
  const char         *roundTripPath = "./assetkit_export_3mf_raw_copy_source/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_production_child_model_with_extra(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  print = ak_printDocument(doc);
  ASSERT(print != NULL);

  blobPart = NULL;
  for (part = print->parts; part; part = part->next) {
    if (part->name && strcmp(part->name, "Metadata/blob.bin") == 0) {
      blobPart = part;
      break;
    }
  }
  ASSERT(blobPart != NULL);
  ASSERT(ak_printSetPackagePartData(doc,
                                    blobPart,
                                    mutatedBlob,
                                    sizeof(mutatedBlob) - 1u));

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(ak_test_3mf_zip_entry_method(roundTripPath, "3D/child.model", &method));
  ASSERT(method == 0u);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_test_export_cleanup(roundTripDir);
  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_production_child_model_path) {
  AkDoc                 *doc;
  AkDoc                 *roundTrip;
  AkGeometry            *geom;
  AkMesh                *mesh;
  AkMeshPrimitive       *prim;
  AkPrintDocument       *print;
  AkPrintDocument       *roundTripPrint;
  AkPrintProductionItem *prod;
  AkPrintProductionItem *buildProd;
  AkPrintProductionItem *itemProd;
  AkPrintProductionItem *objectProd;
  const char            *outDir = "./assetkit_import_3mf_production_child_model";
  const char            *mfPath = "./assetkit_import_3mf_production_child_model/model.3mf";
  const char            *roundTripDir = "./assetkit_export_3mf_production_child_model";
  const char            *roundTripPath = "./assetkit_export_3mf_production_child_model/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_production_child_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  geom = doc->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->type == AK_PRIMITIVE_TRIANGLES);
  ASSERT(prim->nPolygons == 1);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->count == 3);
  ASSERT(prim->indices != NULL);
  ASSERT(prim->indices->count == 3);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_PRODUCTION));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_PRODUCTION) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_PRODUCTION) == 0u);
  ASSERT(print->objectCount == 1);
  ASSERT(print->meshObjectCount == 1);
  ASSERT(print->buildItemCount == 1);
  ASSERT(print->productionItemCount >= 3);

  buildProd  = NULL;
  itemProd   = NULL;
  objectProd = NULL;
  for (prod = print->productionItems; prod; prod = prod->next) {
    if (prod->type == AK_PRINT_PRODUCTION_BUILD)
      buildProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_ITEM)
      itemProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_OBJECT)
      objectProd = prod;
  }

  ASSERT(buildProd != NULL);
  ASSERT(buildProd->uuid != NULL);
  ASSERT(strcmp(buildProd->uuid, "build-uuid") == 0);

  ASSERT(itemProd != NULL);
  ASSERT(itemProd->uuid != NULL);
  ASSERT(strcmp(itemProd->uuid, "item-uuid") == 0);
  ASSERT(itemProd->path != NULL);
  ASSERT(strcmp(itemProd->path, "3D/child.model") == 0);
  ASSERT(itemProd->objectId == 7);

  ASSERT(objectProd != NULL);
  ASSERT(objectProd->uuid != NULL);
  ASSERT(strcmp(objectProd->uuid, "object-uuid") == 0);
  ASSERT(objectProd->path != NULL);
  ASSERT(strcmp(objectProd->path, "3D/child.model") == 0);
  ASSERT(objectProd->objectId == 7);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_PRODUCTION));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_PRODUCTION) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_PRODUCTION) == 0u);

  buildProd  = NULL;
  itemProd   = NULL;
  objectProd = NULL;
  for (prod = roundTripPrint->productionItems; prod; prod = prod->next) {
    if (prod->type == AK_PRINT_PRODUCTION_BUILD
        && prod->uuid
        && strcmp(prod->uuid, "build-uuid") == 0)
      buildProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_ITEM
             && prod->uuid
             && strcmp(prod->uuid, "item-uuid") == 0)
      itemProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_OBJECT
             && prod->uuid
             && strcmp(prod->uuid, "object-uuid") == 0)
      objectProd = prod;
  }

  ASSERT(buildProd != NULL);
  ASSERT(itemProd != NULL);
  ASSERT(itemProd->path != NULL);
  ASSERT(strcmp(itemProd->path, "3D/child.model") == 0);
  ASSERT(itemProd->objectId == 7);
  ASSERT(objectProd != NULL);
  ASSERT(objectProd->objectId == 7);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_bambu_component_materials) {
  AkDoc           *doc;
  AkMaterial      *hatMaterial;
  AkMaterial      *ribbonMaterial;
  AkPrintDocument *print;
  const char      *outDir = "./assetkit_import_3mf_bambu_components";
  const char      *mfPath = "./assetkit_import_3mf_bambu_components/model.3mf";

  ak_test_export_cleanup(outDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_bambu_component_materials_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 3);
  ASSERT(doc->lib.materials.count == 2);
  ASSERT(doc->lib.nodes.count == 5);
  ASSERT(doc->scene != NULL);
  ASSERT(doc->scene->node != NULL);
  ASSERT(ak_test_3mf_count_node_tree(doc->scene->node->chld) == 5);
  ASSERT(ak_test_3mf_count_geometry_nodes(doc->scene->node->chld) == 3);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(print->objectCount == 5);
  ASSERT(print->meshObjectCount == 3);
  ASSERT(print->componentObjectCount == 2);
  ASSERT(print->buildItemCount == 2);

  hatMaterial = ak_test_3mf_find_material(doc, "Bambu/Orca Filament 1");
  ASSERT(hatMaterial != NULL);
  ASSERT(ak_test_3mf_material_color_near(hatMaterial,
                                         244.0f / 255.0f,
                                         169.0f / 255.0f,
                                         37.0f / 255.0f));
  ASSERT(ak_test_3mf_count_primitives_with_material(doc, hatMaterial) == 2);

  ribbonMaterial = ak_test_3mf_find_material(doc, "Bambu/Orca Filament 2");
  ASSERT(ribbonMaterial != NULL);
  ASSERT(ak_test_3mf_material_color_near(ribbonMaterial,
                                         197.0f / 255.0f,
                                         44.0f / 255.0f,
                                         24.0f / 255.0f));
  ASSERT(ak_test_3mf_count_primitives_with_material(doc, ribbonMaterial) == 1);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_bambu_paint_materials) {
  AkDoc      *doc;
  AkMaterial *defaultMaterial;
  AkMaterial *firstMaterial;
  AkMaterial *fifthMaterial;
  const char *outDir = "./assetkit_import_3mf_bambu_paint";
  const char *mfPath = "./assetkit_import_3mf_bambu_paint/model.3mf";

  ak_test_export_cleanup(outDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_bambu_paint_materials_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);
  ASSERT(doc->lib.materials.count == 3);

  firstMaterial = ak_test_3mf_find_material(doc, "Bambu/Orca Filament 1");
  ASSERT(firstMaterial != NULL);
  ASSERT(ak_test_3mf_material_color_near(firstMaterial,
                                         17.0f / 255.0f,
                                         17.0f / 255.0f,
                                         17.0f / 255.0f));
  ASSERT(ak_test_3mf_count_primitives_with_material(doc, firstMaterial) == 1);

  defaultMaterial = ak_test_3mf_find_material(doc, "Bambu/Orca Filament 3");
  ASSERT(defaultMaterial != NULL);
  ASSERT(ak_test_3mf_material_color_near(defaultMaterial,
                                         51.0f / 255.0f,
                                         51.0f / 255.0f,
                                         51.0f / 255.0f));
  ASSERT(ak_test_3mf_count_primitives_with_material(doc, defaultMaterial) == 1);

  fifthMaterial = ak_test_3mf_find_material(doc, "Bambu/Orca Filament 5");
  ASSERT(fifthMaterial != NULL);
  ASSERT(ak_test_3mf_material_color_near(fifthMaterial,
                                         85.0f / 255.0f,
                                         85.0f / 255.0f,
                                         85.0f / 255.0f));
  ASSERT(ak_test_3mf_count_primitives_with_material(doc, fifthMaterial) == 1);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_materials_extension_groups) {
  AkDoc                 *doc;
  AkPrintDocument       *print;
  AkPrintPackagePart    *part;
  AkMaterialPropertySet *baseSet;
  AkMaterialPropertySet *colorSet;
  AkMaterialPropertySet *compositeSet;
  AkMaterialPropertySet *multiSet;
  AkMaterialPropertySet *textureSet;
  bool                   hasTexturePart;
  const char            *outDir = "./assetkit_import_3mf_materials_extension";
  const char            *mfPath = "./assetkit_import_3mf_materials_extension/model.3mf";

  ak_test_export_cleanup(outDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_materials_extension_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 2);
  ASSERT(doc->materialProperties.count == 5);

  baseSet      = ak_test_3mf_find_property_set(doc, 1u);
  colorSet     = ak_test_3mf_find_property_set(doc, 2u);
  compositeSet = ak_test_3mf_find_property_set(doc, 3u);
  multiSet     = ak_test_3mf_find_property_set(doc, 4u);
  textureSet   = ak_test_3mf_find_property_set(doc, 6u);
  ASSERT(baseSet != NULL);
  ASSERT(colorSet != NULL);
  ASSERT(compositeSet != NULL);
  ASSERT(multiSet != NULL);
  ASSERT(textureSet != NULL);
  ASSERT(baseSet->type == AK_MATERIAL_PROPERTY_BASE);
  ASSERT(colorSet->type == AK_MATERIAL_PROPERTY_COLOR);
  ASSERT(compositeSet->type == AK_MATERIAL_PROPERTY_COMPOSITE);
  ASSERT(multiSet->type == AK_MATERIAL_PROPERTY_MULTI);
  ASSERT(textureSet->type == AK_MATERIAL_PROPERTY_TEXTURE2D);
  ASSERT(baseSet->count == 2);
  ASSERT(colorSet->count == 1);
  ASSERT(compositeSet->count == 1);
  ASSERT(multiSet->count == 1);
  ASSERT(textureSet->count == 3);
  ASSERT(baseSet->properties[0].baseColor != NULL);
  ASSERT(colorSet->properties[0].baseColor != NULL);
  ASSERT(compositeSet->properties[0].baseColor != NULL);
  ASSERT(multiSet->properties[0].baseColor != NULL);
  ASSERT(textureSet->properties[0].baseColor == NULL);
  ASSERT(fabs(compositeSet->properties[0].displayColor.rgba.R
              - ak_sRGB_linearf(64.0f / 255.0f)) < 0.01f);
  ASSERT(fabs(compositeSet->properties[0].displayColor.rgba.B
              - ak_sRGB_linearf(191.0f / 255.0f)) < 0.01f);
  ASSERT(fabs(multiSet->properties[0].displayColor.rgba.G
              - ak_sRGB_linearf(128.0f / 255.0f)) < 0.01f);
  ASSERT(ak_test_3mf_count_primitive_inputs(doc, AK_INPUT_COLOR) == 1);
  ASSERT(ak_test_3mf_count_primitive_inputs(doc, AK_INPUT_TEXCOORD) == 1);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_MATERIALS));
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_TEXTURES));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_MATERIALS) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_MATERIALS) == 0u);

  hasTexturePart = false;
  for (part = print->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_TEXTURE
        && part->name
        && strcmp(part->name, "3D/Textures/diffuse.png") == 0) {
      hasTexturePart = true;
      break;
    }
  }
  ASSERT(hasTexturePart);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_component_transform_matrix_order) {
  AkDoc      *doc;
  AkNode     *assembly;
  AkNode     *part;
  AkMatrix    matrix;
  const char *outDir = "./assetkit_import_3mf_component_transform";
  const char *mfPath = "./assetkit_import_3mf_component_transform/model.3mf";

  ak_test_export_cleanup(outDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_component_transform_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->scene != NULL);
  ASSERT(doc->scene->node != NULL);
  ASSERT(doc->scene->node->chld != NULL);

  assembly = doc->scene->node->chld;
  ASSERT(assembly->chld != NULL);
  part = assembly->chld;
  ASSERT(part->geometry != NULL);
  ASSERT(part->transform != NULL);

  ak_transformCombine(part->transform, matrix.val[0]);
  ASSERT(fabs(matrix.val[0][0] - 1.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[0][1] - 2.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[0][2] - 3.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[1][0] - 4.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[1][1] - 5.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[1][2] - 6.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[2][0] - 7.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[2][1] - 8.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[2][2] - 9.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[3][0] - 10.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[3][1] - 11.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[3][2] - 12.0f) < 0.000001f);
  ASSERT(fabs(matrix.val[3][3] - 1.0f) < 0.000001f);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_production_alternatives_roundtrip) {
  AkDoc                 *doc;
  AkDoc                 *roundTrip;
  AkPrintDocument       *print;
  AkPrintDocument       *roundTripPrint;
  AkPrintProductionItem *prod;
  AkPrintProductionItem *objectProd;
  AkPrintProductionItem *alternativeProd;
  const char            *outDir = "./assetkit_import_3mf_production_alternatives";
  const char            *mfPath = "./assetkit_import_3mf_production_alternatives/model.3mf";
  const char            *roundTripDir = "./assetkit_export_3mf_production_alternatives";
  const char            *roundTripPath = "./assetkit_export_3mf_production_alternatives/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_production_alternatives_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_PRODUCTION));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_PRODUCTION) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_PRODUCTION) == 0u);

  objectProd      = NULL;
  alternativeProd = NULL;
  for (prod = print->productionItems; prod; prod = prod->next) {
    if (prod->type == AK_PRINT_PRODUCTION_OBJECT)
      objectProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_ALTERNATIVE)
      alternativeProd = prod;
  }

  ASSERT(objectProd != NULL);
  ASSERT(objectProd->uuid != NULL);
  ASSERT(strcmp(objectProd->uuid, "object-uuid") == 0);
  ASSERT(objectProd->modelResolution != NULL);
  ASSERT(strcmp(objectProd->modelResolution, "lowres") == 0);
  ASSERT(alternativeProd != NULL);
  ASSERT(alternativeProd->uuid != NULL);
  ASSERT(strcmp(alternativeProd->uuid, "alternative-uuid") == 0);
  ASSERT(alternativeProd->path != NULL);
  ASSERT(strcmp(alternativeProd->path, "3D/full.model") == 0);
  ASSERT(alternativeProd->objectId == 2);
  ASSERT(alternativeProd->parentObjectId == 1);
  ASSERT(alternativeProd->modelResolution != NULL);
  ASSERT(strcmp(alternativeProd->modelResolution, "fullres") == 0);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_PRODUCTION));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_PRODUCTION) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_PRODUCTION) == 0u);

  objectProd      = NULL;
  alternativeProd = NULL;
  for (prod = roundTripPrint->productionItems; prod; prod = prod->next) {
    if (prod->type == AK_PRINT_PRODUCTION_OBJECT)
      objectProd = prod;
    else if (prod->type == AK_PRINT_PRODUCTION_ALTERNATIVE)
      alternativeProd = prod;
  }

  ASSERT(objectProd != NULL);
  ASSERT(objectProd->uuid != NULL);
  ASSERT(strcmp(objectProd->uuid, "object-uuid") == 0);
  ASSERT(objectProd->modelResolution != NULL);
  ASSERT(strcmp(objectProd->modelResolution, "lowres") == 0);
  ASSERT(alternativeProd != NULL);
  ASSERT(alternativeProd->uuid != NULL);
  ASSERT(strcmp(alternativeProd->uuid, "alternative-uuid") == 0);
  ASSERT(alternativeProd->path != NULL);
  ASSERT(strcmp(alternativeProd->path, "3D/full.model") == 0);
  ASSERT(alternativeProd->objectId == 2);
  ASSERT(alternativeProd->parentObjectId == 1);
  ASSERT(alternativeProd->modelResolution != NULL);
  ASSERT(strcmp(alternativeProd->modelResolution, "fullres") == 0);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_slice_child_model_path) {
  AkDoc                *doc;
  AkDoc                *roundTrip;
  AkPrintDocument      *print;
  AkPrintDocument      *roundTripPrint;
  AkPrintPackagePart   *part;
  AkPrintSliceStack    *stack;
  AkPrintSlice         *slice;
  AkPrintSliceObject   *sliceObject;
  AkPrintPackagePart   *slicePart;
  AkPrintPackagePart   *modelRelsPart;
  AkPrintPackagePart   *roundTripModelRelsPart;
  AkPrintSliceStack    *rootStack;
  AkPrintSliceStack    *childStack;
  AkPrintSlice         *childSlice;
  AkPrintSliceObject   *rootSliceObject;
  const char           *outDir = "./assetkit_import_3mf_slice_child_model";
  const char           *mfPath = "./assetkit_import_3mf_slice_child_model/model.3mf";
  const char           *roundTripDir = "./assetkit_export_3mf_slice_child_model";
  const char           *roundTripPath = "./assetkit_export_3mf_slice_child_model/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_slice_child_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_SLICE));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_SLICE) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_SLICE) == 0u);
  ASSERT(print->sliceStackCount == 2);
  ASSERT(print->sliceRefCount == 1);
  ASSERT(print->sliceCount == 1);
  ASSERT(print->sliceObjectCount == 1);

  slicePart              = NULL;
  modelRelsPart          = NULL;
  roundTripModelRelsPart = NULL;
  rootStack              = NULL;
  childStack             = NULL;
  childSlice             = NULL;
  rootSliceObject        = NULL;

  for (part = print->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_SLICE
        && part->name
        && strcmp(part->name, "2D/slices.model") == 0) {
      slicePart = part;
    } else if (part->type == AK_PRINT_PACKAGE_PART_RELATIONSHIPS
               && part->name
               && strcmp(part->name, "3D/_rels/3dmodel.model.rels") == 0) {
      modelRelsPart = part;
    }
  }
  ASSERT(slicePart != NULL);
  ASSERT(slicePart->data != NULL);
  ASSERT(slicePart->size > 0);
  ASSERT(modelRelsPart != NULL);
  ASSERT(modelRelsPart->contentType != NULL);
  ASSERT(strcmp(modelRelsPart->contentType,
                "application/vnd.openxmlformats-package.relationships+xml")
         == 0);
  ASSERT(modelRelsPart->data != NULL);
  ASSERT(modelRelsPart->size > 0);

  for (stack = print->sliceStacks; stack; stack = stack->next) {
    if (stack->id == 10u)
      rootStack = stack;
    else if (stack->id == 20u)
      childStack = stack;
  }
  ASSERT(rootStack != NULL);
  ASSERT(rootStack->path != NULL);
  ASSERT(strcmp(rootStack->path, "3D/3dmodel.model") == 0);
  ASSERT(rootStack->sliceRefCount == 1);
  ASSERT(childStack != NULL);
  ASSERT(childStack->path != NULL);
  ASSERT(strcmp(childStack->path, "2D/slices.model") == 0);
  ASSERT(childStack->sliceCount == 1);

  for (slice = print->slices; slice; slice = slice->next) {
    if (slice->stackId == 20u) {
      childSlice = slice;
      break;
    }
  }
  ASSERT(childSlice != NULL);
  ASSERT(childSlice->path != NULL);
  ASSERT(strcmp(childSlice->path, "2D/slices.model") == 0);
  ASSERT(fabs(childSlice->zTop - 0.5f) < 0.000001f);
  ASSERT(childSlice->vertexCount == 3);
  ASSERT(childSlice->polygonCount == 1);
  ASSERT(childSlice->segmentCount == 3);

  for (sliceObject = print->sliceObjects;
       sliceObject;
       sliceObject = sliceObject->next) {
    if (sliceObject->objectId == 7u) {
      rootSliceObject = sliceObject;
      break;
    }
  }
  ASSERT(rootSliceObject != NULL);
  ASSERT(rootSliceObject->path != NULL);
  ASSERT(strcmp(rootSliceObject->path, "3D/3dmodel.model") == 0);
  ASSERT(rootSliceObject->slicePath != NULL);
  ASSERT(strcmp(rootSliceObject->slicePath, "2D/slices.model") == 0);
  ASSERT(rootSliceObject->meshResolution != NULL);
  ASSERT(strcmp(rootSliceObject->meshResolution, "lowres") == 0);
  ASSERT(rootSliceObject->sliceStackId == 10u);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 1);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_SLICE));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_SLICE) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_SLICE) == 0u);
  ASSERT(roundTripPrint->sliceStackCount == 2);
  ASSERT(roundTripPrint->sliceRefCount == 1);
  ASSERT(roundTripPrint->sliceCount == 1);
  ASSERT(roundTripPrint->sliceObjectCount == 1);

  for (part = roundTripPrint->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_RELATIONSHIPS
        && part->name
        && strcmp(part->name, "3D/_rels/3dmodel.model.rels") == 0) {
      roundTripModelRelsPart = part;
      break;
    }
  }
  ASSERT(roundTripModelRelsPart != NULL);
  ASSERT(roundTripModelRelsPart->contentType != NULL);
  ASSERT(strcmp(roundTripModelRelsPart->contentType,
                "application/vnd.openxmlformats-package.relationships+xml")
         == 0);
  ASSERT(roundTripModelRelsPart->data != NULL);
  ASSERT(roundTripModelRelsPart->size == modelRelsPart->size);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_beam_lattice_roundtrip) {
  AkDoc                *doc;
  AkDoc                *roundTrip;
  AkGeometry           *geom;
  AkMesh               *mesh;
  AkMeshPrimitive      *prim;
  AkPrintDocument      *print;
  AkPrintDocument      *roundTripPrint;
  AkPrintBeamLattice   *lattice;
  AkPrintBeam          *beam;
  AkPrintBeamBall      *ball;
  const char           *outDir = "./assetkit_import_3mf_beam_lattice";
  const char           *mfPath = "./assetkit_import_3mf_beam_lattice/model.3mf";
  const char           *roundTripDir = "./assetkit_export_3mf_beam_lattice";
  const char           *roundTripPath = "./assetkit_export_3mf_beam_lattice/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_beam_lattice_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  geom = doc->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->count == 3);
  ASSERT(prim->indices != NULL);
  ASSERT(prim->indices->count == 0);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_BEAM_LATTICE));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_BEAM_LATTICE) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_BEAM_LATTICE) == 0u);
  ASSERT(print->beamLatticeCount == 1);
  ASSERT(print->beamCount == 2);
  ASSERT(print->beamBallCount == 1);

  lattice = print->beamLattices;
  ASSERT(lattice != NULL);
  ASSERT(lattice->objectId == 1);
  ASSERT(fabs(lattice->radius - 0.1f) < 0.000001f);
  ASSERT(fabs(lattice->minLength - 0.001f) < 0.000001f);
  ASSERT(lattice->cap != NULL);
  ASSERT(strcmp(lattice->cap, "sphere") == 0);
  ASSERT(lattice->ballMode != NULL);
  ASSERT(strcmp(lattice->ballMode, "mixed") == 0);
  ASSERT((lattice->flags & AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS) != 0u);
  ASSERT(fabs(lattice->ballRadius - 0.2f) < 0.000001f);

  beam = print->beams;
  ASSERT(beam != NULL);
  ASSERT(beam->v1 == 0);
  ASSERT(beam->v2 == 1);
  ASSERT((beam->flags & AK_PRINT_BEAM_HAS_R1) != 0u);
  ASSERT((beam->flags & AK_PRINT_BEAM_HAS_R2) != 0u);
  ASSERT(fabs(beam->r1 - 0.15f) < 0.000001f);
  ASSERT(fabs(beam->r2 - 0.16f) < 0.000001f);
  ASSERT(beam->cap1 != NULL);
  ASSERT(strcmp(beam->cap1, "sphere") == 0);

  ball = print->beamBalls;
  ASSERT(ball != NULL);
  ASSERT(ball->vindex == 0);
  ASSERT((ball->flags & AK_PRINT_BEAM_BALL_HAS_RADIUS) != 0u);
  ASSERT(fabs(ball->radius - 0.25f) < 0.000001f);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_BEAM_LATTICE));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_BEAM_LATTICE) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_BEAM_LATTICE) == 0u);
  ASSERT(roundTripPrint->beamLatticeCount == 1);
  ASSERT(roundTripPrint->beamCount == 2);
  ASSERT(roundTripPrint->beamBallCount == 1);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_boolean_roundtrip) {
  AkDoc                  *doc;
  AkDoc                  *roundTrip;
  AkPrintDocument        *print;
  AkPrintDocument        *roundTripPrint;
  AkPrintBooleanShape    *shape;
  AkPrintBooleanOperand  *operand;
  const char             *outDir = "./assetkit_import_3mf_boolean";
  const char             *mfPath = "./assetkit_import_3mf_boolean/model.3mf";
  const char             *roundTripDir = "./assetkit_export_3mf_boolean";
  const char             *roundTripPath = "./assetkit_export_3mf_boolean/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_boolean_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 2);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_BOOLEAN));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_BOOLEAN) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_BOOLEAN) == 0u);
  ASSERT(print->booleanShapeCount == 1);
  ASSERT(print->booleanOperandCount == 1);

  shape = print->booleanShapes;
  ASSERT(shape != NULL);
  ASSERT(shape->objectId == 3);
  ASSERT(shape->baseObjectId == 1);
  ASSERT(shape->operation == AK_PRINT_BOOLEAN_OPERATION_DIFFERENCE);
  ASSERT((shape->flags & AK_PRINT_BOOLEAN_SHAPE_HAS_TRANSFORM) != 0u);
  ASSERT(fabs(shape->matrix[0] - 1.0f) < 0.000001f);

  operand = print->booleanOperands;
  ASSERT(operand != NULL);
  ASSERT(operand->objectId == 2);
  ASSERT((operand->flags & AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM) != 0u);
  ASSERT(fabs(operand->matrix[12] - 0.25f) < 0.000001f);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 2);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_BOOLEAN));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_BOOLEAN) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_BOOLEAN) == 0u);
  ASSERT(roundTripPrint->booleanShapeCount == 1);
  ASSERT(roundTripPrint->booleanOperandCount == 1);
  ASSERT(roundTripPrint->booleanShapes != NULL);
  ASSERT(roundTripPrint->booleanShapes->objectId == 3);
  ASSERT(roundTripPrint->booleanShapes->baseObjectId == 1);
  ASSERT(roundTripPrint->booleanShapes->operation == AK_PRINT_BOOLEAN_OPERATION_DIFFERENCE);
  ASSERT(roundTripPrint->booleanOperands != NULL);
  ASSERT(roundTripPrint->booleanOperands->objectId == 2);
  ASSERT((roundTripPrint->booleanOperands->flags & AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM) != 0u);
  ASSERT(fabs(roundTripPrint->booleanOperands->matrix[12] - 0.25f) < 0.000001f);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_displacement_roundtrip) {
  AkDoc                         *doc;
  AkDoc                         *roundTrip;
  AkGeometry                    *geom;
  AkMesh                        *mesh;
  AkMeshPrimitive               *prim;
  AkPrintDocument               *print;
  AkPrintDocument               *roundTripPrint;
  AkPrintDisplacement2D         *displacement;
  AkPrintNormVectorGroup        *normGroup;
  AkPrintDisp2DGroup            *dispGroup;
  AkPrintDisplacementMesh       *dispMesh;
  AkPrintDisplacementTriangle   *dispTriangle;
  const char                    *outDir = "./assetkit_import_3mf_displacement";
  const char                    *mfPath = "./assetkit_import_3mf_displacement/model.3mf";
  const char                    *roundTripDir = "./assetkit_export_3mf_displacement";
  const char                    *roundTripPath = "./assetkit_export_3mf_displacement/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_displacement_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  geom = doc->lib.geometries.first;
  ASSERT(geom != NULL);
  mesh = geom->gdata ? ak_objGet(geom->gdata) : NULL;
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->count == 4);
  ASSERT(prim->nPolygons == 4);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_DISPLACEMENT));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_DISPLACEMENT) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_DISPLACEMENT) == 0u);
  ASSERT(print->displacement2DCount == 1);
  ASSERT(print->normVectorGroupCount == 1);
  ASSERT(print->normVectorCount == 4);
  ASSERT(print->disp2DGroupCount == 1);
  ASSERT(print->disp2DCoordCount == 4);
  ASSERT(print->displacementMeshCount == 1);
  ASSERT(print->displacementTriangleCount == 4);

  displacement = print->displacement2Ds;
  ASSERT(displacement != NULL);
  ASSERT(displacement->id == 5);
  ASSERT(displacement->imagePath != NULL);
  ASSERT(strcmp(displacement->imagePath, "3D/Textures/height.png") == 0);
  ASSERT(displacement->channel != NULL);
  ASSERT(strcmp(displacement->channel, "R") == 0);
  ASSERT(displacement->tileStyleU != NULL);
  ASSERT(strcmp(displacement->tileStyleU, "clamp") == 0);
  ASSERT(displacement->tileStyleV != NULL);
  ASSERT(strcmp(displacement->tileStyleV, "mirror") == 0);
  ASSERT(displacement->filter != NULL);
  ASSERT(strcmp(displacement->filter, "nearest") == 0);

  normGroup = print->normVectorGroups;
  ASSERT(normGroup != NULL);
  ASSERT(normGroup->id == 6);
  ASSERT(normGroup->vectorCount == 4);

  dispGroup = print->disp2DGroups;
  ASSERT(dispGroup != NULL);
  ASSERT(dispGroup->id == 7);
  ASSERT(dispGroup->displacementId == 5);
  ASSERT(dispGroup->normVectorGroupId == 6);
  ASSERT(fabs(dispGroup->height - 0.25f) < 0.000001f);
  ASSERT((dispGroup->flags & AK_PRINT_DISP2D_GROUP_HAS_OFFSET) != 0u);
  ASSERT(fabs(dispGroup->offset - -0.05f) < 0.000001f);
  ASSERT(dispGroup->coordCount == 4);

  dispMesh = print->displacementMeshes;
  ASSERT(dispMesh != NULL);
  ASSERT(dispMesh->objectId == 1);
  ASSERT((dispMesh->flags & AK_PRINT_DISPLACEMENT_MESH_HAS_DEFAULT_GROUP) != 0u);
  ASSERT(dispMesh->defaultGroupId == 7);
  ASSERT(dispMesh->triangleCount == 4);

  dispTriangle = print->displacementTriangles;
  ASSERT(dispTriangle != NULL);
  ASSERT((dispTriangle->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D1) != 0u);
  ASSERT((dispTriangle->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D2) != 0u);
  ASSERT((dispTriangle->flags & AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D3) != 0u);
  ASSERT(dispTriangle->d1 == 0);
  ASSERT(dispTriangle->d2 == 1);
  ASSERT(dispTriangle->d3 == 2);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 1);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_DISPLACEMENT));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_DISPLACEMENT) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_DISPLACEMENT) == 0u);
  ASSERT(roundTripPrint->displacement2DCount == 1);
  ASSERT(roundTripPrint->normVectorGroupCount == 1);
  ASSERT(roundTripPrint->normVectorCount == 4);
  ASSERT(roundTripPrint->disp2DGroupCount == 1);
  ASSERT(roundTripPrint->disp2DCoordCount == 4);
  ASSERT(roundTripPrint->displacementMeshCount == 1);
  ASSERT(roundTripPrint->displacementTriangleCount == 4);
  ASSERT(roundTripPrint->displacementMeshes != NULL);
  ASSERT(roundTripPrint->displacementMeshes->objectId == 1);
  ASSERT(roundTripPrint->displacementMeshes->defaultGroupId == 7);
  ASSERT(roundTripPrint->displacementTriangles != NULL);
  ASSERT(roundTripPrint->displacementTriangles->d1 == 0);
  ASSERT(roundTripPrint->displacementTriangles->d2 == 1);
  ASSERT(roundTripPrint->displacementTriangles->d3 == 2);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

static bool
ak_test_write_3mf_volumetric_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Default Extension=\"png\" ContentType=\"image/png\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"v\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:v=\"http://schemas.3mf.io/3dmanufacturing/volumetric/2022/01\">"
    "<resources>"
    "<v:image3d id=\"2\" name=\"density\">"
    "<v:imagestack rowcount=\"2\" columncount=\"2\" sheetcount=\"2\">"
    "<v:imagesheet path=\"/3D/Volumetric/density_0000.png\"/>"
    "<v:imagesheet path=\"/3D/Volumetric/density_0001.png\"/>"
    "</v:imagestack>"
    "</v:image3d>"
    "<v:functionfromimage3d id=\"3\" displayname=\"density-fn\" image3did=\"2\" "
    "filter=\"nearest\" tilestyleu=\"wrap\" tilestylev=\"clamp\" tilestylew=\"mirror\" "
    "valueoffset=\"-0.5\" valuescale=\"2\"/>"
    "<v:volumedata id=\"4\">"
    "<v:composite basematerialid=\"10\">"
    "<v:materialmapping functionid=\"3\" channel=\"green\" fallbackvalue=\"0.25\"/>"
    "</v:composite>"
    "<v:color functionid=\"3\" channel=\"red\" "
    "transform=\"1 0 0 0 1 0 0 0 1 0.1 0.2 0.3\" "
    "minfeaturesize=\"0.01\" fallbackvalue=\"0.5\"/>"
    "<v:property functionid=\"3\" channel=\"blue\" name=\"ex:opacity\" "
    "required=\"true\" fallbackvalue=\"0.75\"/>"
    "</v:volumedata>"
    "<object id=\"1\" type=\"model\" name=\"volume-bounds\">"
    "<mesh volumeid=\"4\">"
    "<vertices>"
    "<vertex x=\"0\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"1\" y=\"0\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"1\" z=\"0\"/>"
    "<vertex x=\"0\" y=\"0\" z=\"1\"/>"
    "</vertices>"
    "<triangles>"
    "<triangle v1=\"0\" v2=\"1\" v3=\"2\"/>"
    "<triangle v1=\"0\" v2=\"3\" v3=\"1\"/>"
    "<triangle v1=\"1\" v2=\"3\" v3=\"2\"/>"
    "<triangle v1=\"2\" v2=\"3\" v3=\"0\"/>"
    "</triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"5\" type=\"model\" name=\"density-levelset\">"
    "<v:levelset functionid=\"3\" channel=\"red\" meshid=\"1\" volumeid=\"4\" "
    "transform=\"1 0 0 0 1 0 0 0 1 0 0 0\" "
    "minfeaturesize=\"0.02\" meshbboxonly=\"true\" fallbackvalue=\"0.1\"/>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"5\"/></build>"
    "</model>";
  static const unsigned char pngData[] = {
    0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au
  };
  AkTest3MFZipEntry entries[5];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;
  entries[3].name = "3D/Volumetric/density_0000.png";
  entries[3].data = pngData;
  entries[3].size = sizeof(pngData);
  entries[4].name = "3D/Volumetric/density_0001.png";
  entries[4].data = pngData;
  entries[4].size = sizeof(pngData);

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

TEST_IMPL(three_mf_import_volumetric_roundtrip) {
  AkDoc                         *doc;
  AkDoc                         *roundTrip;
  AkPrintDocument               *print;
  AkPrintDocument               *roundTripPrint;
  AkPrintImage3D                *image;
  AkPrintImageSheet             *sheet;
  AkPrintFunctionFromImage3D    *function;
  AkPrintVolumeData             *volume;
  AkPrintVolumetricElement      *element;
  AkPrintVolumetricMesh         *volumeMesh;
  AkPrintLevelSet               *levelSet;
  const char                    *outDir = "./assetkit_import_3mf_volumetric";
  const char                    *mfPath = "./assetkit_import_3mf_volumetric/model.3mf";
  const char                    *roundTripDir = "./assetkit_export_3mf_volumetric";
  const char                    *roundTripPath = "./assetkit_export_3mf_volumetric/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_volumetric_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_VOLUMETRIC));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_VOLUMETRIC) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_VOLUMETRIC) == 0u);
  ASSERT(print->image3DCount == 1);
  ASSERT(print->imageSheetCount == 2);
  ASSERT(print->functionFromImage3DCount == 1);
  ASSERT(print->volumeDataCount == 1);
  ASSERT(print->volumetricElementCount == 3);
  ASSERT(print->volumetricMeshCount == 1);
  ASSERT(print->levelSetCount == 1);

  image = print->image3Ds;
  ASSERT(image != NULL);
  ASSERT(image->id == 2);
  ASSERT(strcmp(image->name, "density") == 0);
  ASSERT(image->rowCount == 2);
  ASSERT(image->columnCount == 2);
  ASSERT(image->sheetCount == 2);
  ASSERT(image->imageSheetCount == 2);

  sheet = print->imageSheets;
  ASSERT(sheet != NULL);
  ASSERT(strcmp(sheet->path, "3D/Volumetric/density_0000.png") == 0);
  ASSERT(sheet->next != NULL);
  ASSERT(strcmp(sheet->next->path, "3D/Volumetric/density_0001.png") == 0);

  function = print->functionFromImage3Ds;
  ASSERT(function != NULL);
  ASSERT(function->id == 3);
  ASSERT(function->image3DId == 2);
  ASSERT(strcmp(function->displayName, "density-fn") == 0);
  ASSERT((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_OFFSET) != 0u);
  ASSERT((function->flags & AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_SCALE) != 0u);
  ASSERT(fabs(function->valueOffset - -0.5f) < 0.000001f);
  ASSERT(fabs(function->valueScale - 2.0f) < 0.000001f);
  ASSERT(strcmp(function->filter, "nearest") == 0);
  ASSERT(strcmp(function->tileStyleU, "wrap") == 0);
  ASSERT(strcmp(function->tileStyleV, "clamp") == 0);
  ASSERT(strcmp(function->tileStyleW, "mirror") == 0);

  volume = print->volumeData;
  ASSERT(volume != NULL);
  ASSERT(volume->id == 4);
  ASSERT((volume->flags & AK_PRINT_VOLUME_DATA_HAS_BASE_MATERIAL_ID) != 0u);
  ASSERT(volume->baseMaterialId == 10);
  ASSERT(volume->materialMappingCount == 1);
  ASSERT(volume->colorCount == 1);
  ASSERT(volume->propertyCount == 1);

  element = print->volumetricElements;
  ASSERT(element != NULL);
  ASSERT(element->type == AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING);
  ASSERT(strcmp(element->channel, "green") == 0);
  ASSERT((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_FALLBACK_VALUE) != 0u);
  ASSERT(fabs(element->fallbackValue - 0.25f) < 0.000001f);
  ASSERT(element->next != NULL);
  element = element->next;
  ASSERT(element->type == AK_PRINT_VOLUMETRIC_ELEMENT_COLOR);
  ASSERT(strcmp(element->channel, "red") == 0);
  ASSERT((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_TRANSFORM) != 0u);
  ASSERT((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_HAS_MIN_FEATURE_SIZE) != 0u);
  ASSERT(fabs(element->matrix[12] - 0.1f) < 0.000001f);
  ASSERT(fabs(element->minFeatureSize - 0.01f) < 0.000001f);
  ASSERT(element->next != NULL);
  element = element->next;
  ASSERT(element->type == AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY);
  ASSERT(strcmp(element->channel, "blue") == 0);
  ASSERT(strcmp(element->name, "ex:opacity") == 0);
  ASSERT((element->flags & AK_PRINT_VOLUMETRIC_ELEMENT_REQUIRED) != 0u);
  ASSERT(fabs(element->fallbackValue - 0.75f) < 0.000001f);

  volumeMesh = print->volumetricMeshes;
  ASSERT(volumeMesh != NULL);
  ASSERT(volumeMesh->objectId == 1);
  ASSERT(volumeMesh->volumeId == 4);
  ASSERT((volumeMesh->flags & AK_PRINT_VOLUMETRIC_MESH_HAS_VOLUME_ID) != 0u);

  levelSet = print->levelSets;
  ASSERT(levelSet != NULL);
  ASSERT(levelSet->objectId == 5);
  ASSERT(levelSet->functionId == 3);
  ASSERT(strcmp(levelSet->channel, "red") == 0);
  ASSERT(levelSet->meshId == 1);
  ASSERT(levelSet->volumeId == 4);
  ASSERT((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_VOLUME_ID) != 0u);
  ASSERT((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u);
  ASSERT((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MIN_FEATURE_SIZE) != 0u);
  ASSERT(fabs(levelSet->minFeatureSize - 0.02f) < 0.000001f);
  ASSERT(fabs(levelSet->fallbackValue - 0.1f) < 0.000001f);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 1);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_VOLUMETRIC));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_VOLUMETRIC) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_VOLUMETRIC) == 0u);
  ASSERT(roundTripPrint->image3DCount == 1);
  ASSERT(roundTripPrint->imageSheetCount == 2);
  ASSERT(roundTripPrint->functionFromImage3DCount == 1);
  ASSERT(roundTripPrint->volumeDataCount == 1);
  ASSERT(roundTripPrint->volumetricElementCount == 3);
  ASSERT(roundTripPrint->volumetricMeshCount == 1);
  ASSERT(roundTripPrint->levelSetCount == 1);
  ASSERT(roundTripPrint->volumetricMeshes != NULL);
  ASSERT(roundTripPrint->volumetricMeshes->objectId == 1);
  ASSERT(roundTripPrint->volumetricMeshes->volumeId == 4);
  ASSERT(roundTripPrint->levelSets != NULL);
  ASSERT(roundTripPrint->levelSets->objectId == 5);
  ASSERT(roundTripPrint->levelSets->meshId == 1);
  ASSERT(roundTripPrint->levelSets->volumeId == 4);
  ASSERT((roundTripPrint->levelSets->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}

static bool
ak_test_write_3mf_implicit_model(const char *path) {
  static const char contentTypes[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">"
    "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>"
    "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>"
    "</Types>";
  static const char rels[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">"
    "<Relationship Id=\"rel0\" "
    "Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\" "
    "Target=\"/3D/3dmodel.model\"/>"
    "</Relationships>";
  static const char rootModel[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<model unit=\"millimeter\" requiredextensions=\"v i\" "
    "xmlns=\"http://schemas.microsoft.com/3dmanufacturing/core/2015/02\" "
    "xmlns:v=\"http://schemas.3mf.io/3dmanufacturing/volumetric/2022/01\" "
    "xmlns:i=\"http://schemas.3mf.io/3dmanufacturing/implicit/2023/12\">"
    "<resources>"
    "<i:implicitfunction id=\"6\" displayname=\"sphere\">"
    "<i:in>"
    "<i:vector identifier=\"pos\" displayname=\"pos\"/>"
    "<i:scalar identifier=\"radius\" displayname=\"radius\"/>"
    "</i:in>"
    "<i:length identifier=\"Length_3\" displayname=\"Length_3\">"
    "<i:in>"
    "<i:vectorref identifier=\"A\" ref=\"inputs.pos\"/>"
    "</i:in>"
    "<i:out>"
    "<i:scalar identifier=\"result\" displayname=\"result\"/>"
    "</i:out>"
    "</i:length>"
    "<i:subtraction identifier=\"Subtraction_4\" displayname=\"Subtraction_4\">"
    "<i:in>"
    "<i:scalarref identifier=\"A\" ref=\"Length_3.result\"/>"
    "<i:scalarref identifier=\"B\" ref=\"inputs.radius\"/>"
    "</i:in>"
    "<i:out>"
    "<i:scalar identifier=\"result\" displayname=\"result\"/>"
    "</i:out>"
    "</i:subtraction>"
    "<i:out>"
    "<i:scalarref identifier=\"shape\" displayname=\"shape\" ref=\"Subtraction_4.result\"/>"
    "</i:out>"
    "</i:implicitfunction>"
    "<object id=\"1\" type=\"model\" name=\"sphere-domain\">"
    "<mesh>"
    "<vertices>"
    "<vertex x=\"-1\" y=\"-1\" z=\"-1\"/>"
    "<vertex x=\"1\" y=\"-1\" z=\"-1\"/>"
    "<vertex x=\"-1\" y=\"1\" z=\"-1\"/>"
    "<vertex x=\"-1\" y=\"-1\" z=\"1\"/>"
    "</vertices>"
    "<triangles>"
    "<triangle v1=\"0\" v2=\"1\" v3=\"2\"/>"
    "<triangle v1=\"0\" v2=\"3\" v3=\"1\"/>"
    "<triangle v1=\"1\" v2=\"3\" v3=\"2\"/>"
    "<triangle v1=\"2\" v2=\"3\" v3=\"0\"/>"
    "</triangles>"
    "</mesh>"
    "</object>"
    "<object id=\"7\" type=\"model\" name=\"sphere-levelset\">"
    "<v:levelset functionid=\"6\" channel=\"shape\" meshid=\"1\" meshbboxonly=\"true\"/>"
    "</object>"
    "</resources>"
    "<build><item objectid=\"7\"/></build>"
    "</model>";
  AkTest3MFZipEntry entries[3];

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes;
  entries[0].size = sizeof(contentTypes) - 1u;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels;
  entries[1].size = sizeof(rels) - 1u;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = rootModel;
  entries[2].size = sizeof(rootModel) - 1u;

  return ak_test_3mf_zip_write_stored(path, entries, AK_ARRAY_LEN(entries));
}

TEST_IMPL(three_mf_import_implicit_roundtrip) {
  AkDoc                  *doc;
  AkDoc                  *roundTrip;
  AkPrintDocument        *print;
  AkPrintDocument        *roundTripPrint;
  AkPrintImplicitFunction *function;
  AkPrintLevelSet        *levelSet;
  const char             *outDir = "./assetkit_import_3mf_implicit";
  const char             *mfPath = "./assetkit_import_3mf_implicit/model.3mf";
  const char             *roundTripDir = "./assetkit_export_3mf_implicit";
  const char             *roundTripPath = "./assetkit_export_3mf_implicit/model.3mf";

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  ASSERT(mkdir(outDir, 0777) == 0);
  ASSERT(ak_test_write_3mf_implicit_model(mfPath));

  doc = NULL;
  ASSERT(ak_load(&doc, mfPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(doc != NULL);
  ASSERT(doc->lib.geometries.count == 1);

  print = ak_printDocument(doc);
  ASSERT(print != NULL);
  ASSERT(ak_printHasFeature(print, AK_PRINT_FEATURE_VOLUMETRIC));
  ASSERT((print->requiredFeatures & AK_PRINT_FEATURE_VOLUMETRIC) != 0u);
  ASSERT((print->unsupportedFeatures & AK_PRINT_FEATURE_VOLUMETRIC) == 0u);
  ASSERT(print->implicitFunctionCount == 1);
  ASSERT(print->levelSetCount == 1);

  function = print->implicitFunctions;
  ASSERT(function != NULL);
  ASSERT(function->id == 6);
  ASSERT(strcmp(function->displayName, "sphere") == 0);
  ASSERT((function->flags & AK_PRINT_IMPLICIT_FUNCTION_HAS_XML) != 0u);
  ASSERT(function->xml != NULL);
  ASSERT(strstr(function->xml, "<i:length") != NULL);
  ASSERT(strstr(function->xml, "<i:subtraction") != NULL);

  levelSet = print->levelSets;
  ASSERT(levelSet != NULL);
  ASSERT(levelSet->objectId == 7);
  ASSERT(levelSet->functionId == 6);
  ASSERT(strcmp(levelSet->channel, "shape") == 0);
  ASSERT(levelSet->meshId == 1);
  ASSERT((levelSet->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u);

  ASSERT(ak_export(doc, roundTripDir, AK_FILE_TYPE_3MF) == AK_OK);
  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, roundTripPath, AK_FILE_TYPE_3MF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.geometries.count == 1);

  roundTripPrint = ak_printDocument(roundTrip);
  ASSERT(roundTripPrint != NULL);
  ASSERT(ak_printHasFeature(roundTripPrint, AK_PRINT_FEATURE_VOLUMETRIC));
  ASSERT((roundTripPrint->requiredFeatures & AK_PRINT_FEATURE_VOLUMETRIC) != 0u);
  ASSERT((roundTripPrint->unsupportedFeatures & AK_PRINT_FEATURE_VOLUMETRIC) == 0u);
  ASSERT(roundTripPrint->implicitFunctionCount == 1);
  ASSERT(roundTripPrint->levelSetCount == 1);
  ASSERT(roundTripPrint->implicitFunctions != NULL);
  ASSERT(roundTripPrint->implicitFunctions->id == 6);
  ASSERT(roundTripPrint->implicitFunctions->xml != NULL);
  ASSERT(strstr(roundTripPrint->implicitFunctions->xml, "<i:length") != NULL);
  ASSERT(strstr(roundTripPrint->implicitFunctions->xml, "<i:subtraction") != NULL);
  ASSERT(roundTripPrint->levelSets != NULL);
  ASSERT(roundTripPrint->levelSets->objectId == 7);
  ASSERT(roundTripPrint->levelSets->functionId == 6);
  ASSERT(roundTripPrint->levelSets->meshId == 1);
  ASSERT((roundTripPrint->levelSets->flags & AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY) != 0u);

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundTripDir);
  TEST_SUCCESS
}
