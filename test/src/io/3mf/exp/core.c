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

  return doc;
}

TEST_IMPL(three_mf_export_triangle_roundtrip) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkPrintDocument *print;
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
  ASSERT(colorAcc->componentSize == AK_COMPONENT_SIZE_VEC4);
  ASSERT(colorAcc->count == 3);
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
  ASSERT(print->packagePartCount == 2);

  thumbnailPart = NULL;
  for (part = print->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_THUMBNAIL) {
      thumbnailPart = part;
      break;
    }
  }
  ASSERT(thumbnailPart != NULL);
  ASSERT(thumbnailPart->name != NULL);
  ASSERT(strcmp(thumbnailPart->name, "Metadata/thumbnail.png") == 0);
  ASSERT(thumbnailPart->contentType != NULL);
  ASSERT(strcmp(thumbnailPart->contentType, "image/png") == 0);
  ASSERT(thumbnailPart->relationshipType != NULL);
  ASSERT(strstr(thumbnailPart->relationshipType, "thumbnail") != NULL);
  ASSERT(thumbnailPart->size == sizeof(thumbnail));
  ASSERT(thumbnailPart->data != NULL);
  ASSERT(memcmp(thumbnailPart->data, thumbnail, sizeof(thumbnail)) == 0);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_production_child_model_path) {
  AkDoc                 *doc;
  AkGeometry            *geom;
  AkMesh                *mesh;
  AkMeshPrimitive       *prim;
  AkPrintDocument       *print;
  AkPrintProductionItem *prod;
  AkPrintProductionItem *buildProd;
  AkPrintProductionItem *itemProd;
  AkPrintProductionItem *objectProd;
  const char            *outDir = "./assetkit_import_3mf_production_child_model";
  const char            *mfPath = "./assetkit_import_3mf_production_child_model/model.3mf";

  ak_test_export_cleanup(outDir);
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

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(three_mf_import_slice_child_model_path) {
  AkDoc                *doc;
  AkPrintDocument      *print;
  AkPrintPackagePart   *part;
  AkPrintSliceStack    *stack;
  AkPrintSlice         *slice;
  AkPrintSliceObject   *sliceObject;
  AkPrintPackagePart   *slicePart;
  AkPrintSliceStack    *rootStack;
  AkPrintSliceStack    *childStack;
  AkPrintSlice         *childSlice;
  AkPrintSliceObject   *rootSliceObject;
  const char           *outDir = "./assetkit_import_3mf_slice_child_model";
  const char           *mfPath = "./assetkit_import_3mf_slice_child_model/model.3mf";

  ak_test_export_cleanup(outDir);
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

  slicePart       = NULL;
  rootStack       = NULL;
  childStack      = NULL;
  childSlice      = NULL;
  rootSliceObject = NULL;

  for (part = print->parts; part; part = part->next) {
    if (part->type == AK_PRINT_PACKAGE_PART_SLICE
        && part->name
        && strcmp(part->name, "2D/slices.model") == 0) {
      slicePart = part;
      break;
    }
  }
  ASSERT(slicePart != NULL);
  ASSERT(slicePart->data != NULL);
  ASSERT(slicePart->size > 0);

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

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}
