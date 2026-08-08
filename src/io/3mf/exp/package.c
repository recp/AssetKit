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

#include "common.h"
#include "../package_source.h"
#include "../../../../include/ak/path.h"
#include "../../../../include/ak/options.h"

#include <stdlib.h>
#include <string.h>

static
const char*
ak_3mf_zip_part_name(const char * __restrict name) {
  if (!name)
    return NULL;
  while (*name == '/' || *name == '\\')
    name++;
  return *name ? name : NULL;
}

static
bool
ak_3mf_reserved_part_name(const char * __restrict name) {
  if (!name)
    return false;

  switch (name[0]) {
    case '_':
      return ak_str_eq_cstr_fast(name,
                                 "_rels/.rels",
                                 sizeof("_rels/.rels") - 1u);
    case '3':
      return ak_str_eq_cstr_fast(name,
                                 "3D/3dmodel.model",
                                 sizeof("3D/3dmodel.model") - 1u);
    case '[':
      return ak_str_eq_cstr_fast(name,
                                 "[Content_Types].xml",
                                 sizeof("[Content_Types].xml") - 1u);
    default:
      return false;
  }
}

static
bool
ak_3mf_extra_part_exportable(const AkPrintPackagePart * __restrict part) {
  const char *name;

  if (!part || !part->name || (!part->data && part->size > 0u))
    return false;

  name = ak_3mf_zip_part_name(part->name);
  if (!name)
    return false;
  if (ak_3mf_reserved_part_name(name))
    return false;

  return true;
}

static
size_t
ak_3mf_count_extra_parts(const AkPrintDocument * __restrict print) {
  const AkPrintPackagePart *part;
  size_t                    count;

  count = 0u;
  for (part = print ? print->parts : NULL; part; part = part->next) {
    if (ak_3mf_extra_part_exportable(part))
      count++;
  }

  return count;
}

static
bool
ak_3mf_build_content_types_xml(const AkPrintDocument * __restrict print,
                               const AK3MFExportState * __restrict st,
                               AK3MFBuffer           * __restrict contentTypes) {
  const AkPrintPackagePart *part;
  size_t                    i;

  io_buffer_init(contentTypes);

  AK_3MF_BUF_LIT(contentTypes,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Types xmlns=\"http://schemas.openxmlformats.org/package/2006/content-types\">\n"
                 "  <Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
                 "  <Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n");
  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->contentType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    AK_3MF_BUF_LIT(contentTypes, "  <Override PartName=\"/");
    ak_3mf_buf_attr(contentTypes, name);
    AK_3MF_BUF_LIT(contentTypes, "\" ContentType=\"");
    ak_3mf_buf_attr(contentTypes, part->contentType);
    AK_3MF_BUF_LIT(contentTypes, "\"/>\n");
  }

  for (i = 0u; st && i < st->imageCount; i++) {
    AK_3MF_BUF_LIT(contentTypes, "  <Override PartName=\"/");
    ak_3mf_buf_attr(contentTypes, st->images[i].partName);
    AK_3MF_BUF_LIT(contentTypes, "\" ContentType=\"");
    ak_3mf_buf_attr(contentTypes, st->images[i].payload.mimeType);
    AK_3MF_BUF_LIT(contentTypes, "\"/>\n");
  }

  AK_3MF_BUF_LIT(contentTypes, "</Types>\n");
  return contentTypes->result == AK_OK;
}

static
void
ak_3mf_release_generated_images(AK3MFExportState * __restrict st) {
  size_t i;

  if (!st)
    return;
  for (i = 0u; i < st->imageCount; i++)
    ak_imageExportPayloadRelease(&st->images[i].payload);
  free(st->images);
  st->images        = NULL;
  st->imageCount    = 0u;
  st->imageCapacity = 0u;
}

static
bool
ak_3mf_build_rels_xml(const AkPrintDocument * __restrict print,
                      AK3MFBuffer           * __restrict rels) {
  const AkPrintPackagePart *part;
  uint32_t                  relId;

  io_buffer_init(rels);
  relId = 1u;

  AK_3MF_BUF_LIT(rels,
                 "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                 "<Relationships xmlns=\"http://schemas.openxmlformats.org/package/2006/relationships\">\n"
                 "  <Relationship Target=\"/3D/3dmodel.model\" Id=\"rel0\" Type=\"http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel\"/>\n");

  for (part = print ? print->parts : NULL; part; part = part->next) {
    const char *name;

    if (!ak_3mf_extra_part_exportable(part) || !part->relationshipType)
      continue;

    name = ak_3mf_zip_part_name(part->name);
    AK_3MF_BUF_LIT(rels, "  <Relationship Target=\"/");
    ak_3mf_buf_attr(rels, name);
    if (part->relationshipId) {
      AK_3MF_BUF_LIT(rels, "\" Id=\"");
      ak_3mf_buf_attr(rels, part->relationshipId);
      relId++;
    } else {
      AK_3MF_BUF_LIT(rels, "\" Id=\"rel");
      ak_3mf_buf_u32(rels, relId++);
    }
    AK_3MF_BUF_LIT(rels, "\" Type=\"");
    ak_3mf_buf_attr(rels, part->relationshipType);
    if (part->relationshipTargetMode) {
      AK_3MF_BUF_LIT(rels, "\" TargetMode=\"");
      ak_3mf_buf_attr(rels, part->relationshipTargetMode);
    }
    AK_3MF_BUF_LIT(rels, "\"/>\n");
  }

  AK_3MF_BUF_LIT(rels, "</Relationships>\n");
  return rels->result == AK_OK;
}

static
bool
ak_3mf_try_clone_source_package(AkDoc      * __restrict doc,
                                const char * __restrict filepath) {
  AkPrintDocument    *print;
  AK3MFPackageSource *source;

  if (!doc || !filepath || !doc->inf || doc->inf->ftype != AK_FILE_TYPE_3MF)
    return false;

  print = ak_printDocument(doc);
  if (!print || ak_3mf_package_source_has_mutated_parts(print))
    return false;

  source = ak_3mf_package_source_get(print);
  if (!source || !source->path)
    return false;

  return ak_copyfile(source->path, filepath);
}

AK_HIDE
AkResult
ak_3mf_export(AkDoc * __restrict doc, const char * __restrict filepath) {
  AK3MFExportState st;
  AK3MFBuffer      model;
  AK3MFBuffer      contentTypes;
  AK3MFBuffer      rels;
  AkZipWriteEntry *entries;
  AK3MFPackageSource *source;
  AkZipArchive     *sourceArchive;
  AkPrintDocument *print;
  AkPrintPackagePart *part;
  size_t           extraPartCount;
  size_t           entryCount;
  size_t           entryIndex;
  uintptr_t        compressionLevelOpt;
  unsigned         compressionLevel;
  AkResult         result;

  if (!doc || !filepath)
    return AK_ERR;

  sourceArchive = NULL;
  if (ak_3mf_try_clone_source_package(doc, filepath))
    return AK_OK;

  memset(&st, 0, sizeof(st));
  io_buffer_init(&st.resources);
  io_buffer_init(&st.build);
  st.doc          = doc;
  st.print        = ak_printDocument(doc);
  st.result       = AK_OK;
  st.nextObjectId = 1u;
  st.usesProductionExtension = st.print && st.print->productionItemCount > 0u;
  st.usesProductionAlternativeExtension =
    ak_3mf_uses_production_alternative_extension(st.print);
  st.usesSliceExtension = st.print
                          && (st.print->sliceStackCount > 0u
                              || st.print->sliceObjectCount > 0u);
  st.usesBeamLatticeExtension = st.print && st.print->beamLatticeCount > 0u;
  st.usesBeamBallExtension    = ak_3mf_uses_beam_ball_extension(st.print);
  st.usesBooleanExtension     = st.print && st.print->booleanShapeCount > 0u;
  st.usesDisplacementExtension = st.print
                                 && (st.print->displacement2DCount > 0u
                                     || st.print->normVectorGroupCount > 0u
                                     || st.print->disp2DGroupCount > 0u
                                     || st.print->displacementMeshCount > 0u);
  st.usesImplicitExtension = st.print && st.print->implicitFunctionCount > 0u;
  st.usesVolumetricExtension = st.print
                               && (st.print->image3DCount > 0u
                                   || st.print->imageSheetCount > 0u
                                   || st.print->functionFromImage3DCount > 0u
                                   || st.print->implicitFunctionCount > 0u
                                   || st.print->volumeDataCount > 0u
                                   || st.print->volumetricElementCount > 0u
                                   || st.print->volumetricMeshCount > 0u
                                   || st.print->levelSetCount > 0u);

  if (!ak_3mf_write_slice_stacks(&st)
      || !ak_3mf_write_displacement_resources(&st)
      || !ak_3mf_write_volumetric_resources(&st)
      || !ak_3mf_write_scene(&st)
      || !ak_3mf_write_library_fallback(&st)
      || !ak_3mf_write_level_sets(&st)
      || !ak_3mf_write_boolean_shapes(&st)
      || st.resources.result != AK_OK
      || st.build.result != AK_OK
      || !ak_3mf_build_model_xml(&st, &model)) {
    ak_3mf_release_generated_images(&st);
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  print          = st.print;
  extraPartCount = ak_3mf_count_extra_parts(print);
  entryCount     = 3u + extraPartCount + st.imageCount;
  entries        = calloc(entryCount, sizeof(*entries));
  if (!entries) {
    ak_3mf_release_generated_images(&st);
    ak_3mf_buf_free(&model);
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  if (!ak_3mf_build_content_types_xml(print,
                                      &st,
                                      &contentTypes)
      || !ak_3mf_build_rels_xml(print, &rels)) {
    free(entries);
    ak_3mf_release_generated_images(&st);
    ak_3mf_buf_free(&contentTypes);
    ak_3mf_buf_free(&rels);
    ak_3mf_buf_free(&model);
    ak_3mf_buf_free(&st.resources);
    ak_3mf_buf_free(&st.build);
    return AK_ERR;
  }

  entries[0].name = "[Content_Types].xml";
  entries[0].data = contentTypes.data;
  entries[0].size = contentTypes.len;
  entries[1].name = "_rels/.rels";
  entries[1].data = rels.data;
  entries[1].size = rels.len;
  entries[2].name = "3D/3dmodel.model";
  entries[2].data = model.data;
  entries[2].size = model.len;

  source = ak_3mf_package_source_get(print);
  if (source && source->path
      && ak_zip_open(source->path, &sourceArchive) != AK_OK)
    sourceArchive = NULL;

  entryIndex = 3u;
  for (part = print ? print->parts : NULL; part; part = part->next) {
    size_t sourceIndex;

    if (!ak_3mf_extra_part_exportable(part))
      continue;
    entries[entryIndex].name = ak_3mf_zip_part_name(part->name);
    entries[entryIndex].data = part->data;
    entries[entryIndex].size = part->size;
    if (sourceArchive
        && (part->flags & AK_PRINT_PACKAGE_PART_DATA_MUTATED) == 0u
        && ak_zip_archive_find_entry_index(sourceArchive,
                                           entries[entryIndex].name,
                                           &sourceIndex)) {
      entries[entryIndex].sourceArchive = sourceArchive;
      entries[entryIndex].sourceIndex   = sourceIndex;
    }
    entryIndex++;
  }
  {
    size_t i;

    for (i = 0u; i < st.imageCount; i++) {
      entries[entryIndex].name = st.images[i].partName;
      entries[entryIndex].data = st.images[i].payload.data;
      entries[entryIndex].size = st.images[i].payload.byteLength;
      entryIndex++;
    }
  }

  compressionLevelOpt = ak_opt_get(AK_OPT_ZIP_EXPORT_COMPRESSION_LEVEL);
  compressionLevel    = compressionLevelOpt > 12u ? 12u : (unsigned)compressionLevelOpt;
  result              = ak_zip_write_deflated_level(filepath,
                                                    entries,
                                                    entryCount,
                                                    compressionLevel);

  ak_zip_close(sourceArchive);
  free(entries);
  ak_3mf_release_generated_images(&st);
  ak_3mf_buf_free(&contentTypes);
  ak_3mf_buf_free(&rels);
  ak_3mf_buf_free(&model);
  ak_3mf_buf_free(&st.resources);
  ak_3mf_buf_free(&st.build);

  return result;
}
