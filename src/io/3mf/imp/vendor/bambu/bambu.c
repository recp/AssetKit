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

#include "bambu.h"
#include "../../../../common/zip.h"
#include "../../../../common/text_number.h"
#include "../../../../../json.h"
#include "../../../../../strpool.h"
#include "../../../../../id.h"
#include "../../../../../mat/internal.h"

#include <stdlib.h>
#include <string.h>

static const char AK_3MF_BAMBU_PROJECT_SETTINGS_PART[] = "Metadata/project_settings.config";
static const char AK_3MF_BAMBU_MODEL_SETTINGS_PART[]   = "Metadata/model_settings.config";
static const char AK_3MF_BAMBU_MATERIAL_NAME_PREFIX[]  = "Bambu/Orca Filament ";

static
bool
ak_3mf_bambu_orca_tag_sz(const xml_t * __restrict xml,
                         const char  * __restrict tag,
                         size_t                   tagSize) {
  const char *colon;
  const char *xmlTag;
  size_t      xmlTagSize;

  if (!xml || !xml->tag || !tag)
    return false;

  xmlTag     = xml->tag;
  xmlTagSize = xml->tagsize;
  colon      = memchr(xmlTag, ':', xmlTagSize);
  if (colon) {
    xmlTagSize -= (size_t)(colon - xmlTag) + 1u;
    xmlTag      = colon + 1u;
  }

  return xmlTagSize == tagSize && memcmp(xmlTag, tag, tagSize) == 0;
}

static
xml_attr_t*
ak_3mf_bambu_orca_xmla_local_sz(const xml_t * __restrict xml,
                                const char  * __restrict name,
                                size_t                   nameLen) {
  xml_attr_t *attr;

  if (!xml || !name)
    return NULL;

  if ((attr = xmla_sz(xml, name, nameLen)))
    return attr;

  for (attr = xml->attr; attr; attr = attr->next) {
    const char *attrName;
    const char *colon;
    size_t      attrNameLen;

    if (!attr->name || attr->namesize < nameLen)
      continue;

    attrName    = attr->name;
    attrNameLen = attr->namesize;
    colon       = memchr(attrName, ':', attrNameLen);
    if (colon) {
      attrNameLen -= (size_t)(colon - attrName) + 1u;
      attrName     = colon + 1u;
    }

    if (attrNameLen == nameLen && memcmp(attrName, name, nameLen) == 0)
      return attr;
  }

  return NULL;
}

static
bool
ak_3mf_bambu_orca_reserve_parts(AK3MFImportState * __restrict st,
                                size_t                         extra) {
  AK3MFBambuPartMaterial *parts;
  size_t                  needed;
  size_t                  newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->bambuPartCount > SIZE_MAX - extra)
    return false;

  needed = st->bambuPartCount + extra;
  if (needed <= st->bambuPartCapacity)
    return true;

  newCapacity = st->bambuPartCapacity ? st->bambuPartCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  parts = realloc(st->bambuParts, sizeof(*parts) * newCapacity);
  if (!parts)
    return false;

  memset(parts + st->bambuPartCapacity,
         0,
         sizeof(*parts) * (newCapacity - st->bambuPartCapacity));
  st->bambuParts        = parts;
  st->bambuPartCapacity = newCapacity;
  return true;
}

static
bool
ak_3mf_bambu_orca_add_part_material(AK3MFImportState * __restrict st,
                                    uint32_t                      objectId,
                                    uint32_t                      extruder) {
  size_t i;

  if (!st || objectId == 0u || extruder == 0u)
    return true;

  for (i = 0; i < st->bambuPartCount; i++) {
    if (st->bambuParts[i].objectId == objectId) {
      st->bambuParts[i].extruder = extruder;
      return true;
    }
  }

  if (!ak_3mf_bambu_orca_reserve_parts(st, 1u))
    return false;

  st->bambuParts[st->bambuPartCount].objectId = objectId;
  st->bambuParts[st->bambuPartCount].extruder = extruder;
  st->bambuPartCount++;
  return true;
}

static
bool
ak_3mf_bambu_orca_metadata_key_eq(xml_t       * __restrict metadata,
                                  const char  * __restrict key,
                                  size_t                   keyLen) {
  xml_attr_t *attr;

  attr = ak_3mf_bambu_orca_xmla_local_sz(metadata, _s_ak_key, _s_ak_key_len);
  return attr
         && attr->val
         && attr->valsize == keyLen
         && memcmp(attr->val, key, keyLen) == 0;
}

static
uint32_t
ak_3mf_bambu_orca_metadata_u32(xml_t       * __restrict parent,
                               const char  * __restrict key,
                               size_t                   keyLen,
                               uint32_t                 fallback) {
  xml_t *metadata;

  for (metadata = parent ? parent->val : NULL; metadata; metadata = metadata->next) {
    if (!ak_3mf_bambu_orca_tag_sz(metadata, _s_ak_metadata, _s_ak_metadata_len)
        || !ak_3mf_bambu_orca_metadata_key_eq(metadata, key, keyLen))
      continue;

    return xmla_u32(ak_3mf_bambu_orca_xmla_local_sz(metadata,
                                                    _s_ak_value,
                                                    _s_ak_value_len),
                    fallback);
  }

  return fallback;
}

static
void
ak_3mf_bambu_orca_parse_project_settings(AK3MFImportState * __restrict st) {
  void         *data;
  size_t        size;
  json_doc_t   *jdoc;
  json_t       *colorsJson;
  json_array_t *colors;
  json_t       *item;
  size_t        count;
  size_t        i;

  if (!st || !st->packagePath)
    return;

  data = NULL;
  size = 0u;
  if (ak_zip_extract_file(st->packagePath,
                          AK_3MF_BAMBU_PROJECT_SETTINGS_PART,
                          &data,
                          &size) != AK_OK)
    return;

  jdoc = json_parse_len(data, size, false);
  if (!jdoc || !jdoc->root) {
    if (jdoc)
      json_free(jdoc);
    free(data);
    return;
  }

  colorsJson = json_get(jdoc->root, _s_ak_filament_colour);
  colors     = json_array(colorsJson);
  count      = colors && colors->count > 0 ? (size_t)colors->count : 0u;
  if (count == 0u)
    goto cleanup;

  st->bambuColors = calloc(count, sizeof(*st->bambuColors));
  st->bambuMaterials = calloc(count, sizeof(*st->bambuMaterials));
  if (!st->bambuColors || !st->bambuMaterials) {
    free(st->bambuColors);
    free(st->bambuMaterials);
    st->bambuColors    = NULL;
    st->bambuMaterials = NULL;
    goto cleanup;
  }

  st->bambuColorCount = count;
  i = 0u;
  for (item = colors->base.value; item && i < count; item = item->next, i++) {
    const char *color;

    color = json_string(item);
    (void)ak_3mf_parse_color_slice(color,
                                   item->valsize > 0 ? (size_t)item->valsize : 0u,
                                   st->bambuColors[i]);
  }

cleanup:
  json_free(jdoc);
  free(data);
}

static
void
ak_3mf_bambu_orca_parse_model_settings(AK3MFImportState * __restrict st) {
  void      *data;
  size_t     size;
  xml_doc_t *xdoc;
  xml_t     *objectXml;

  if (!st || !st->packagePath)
    return;

  data = NULL;
  size = 0u;
  if (ak_zip_extract_file(st->packagePath,
                          AK_3MF_BAMBU_MODEL_SETTINGS_PART,
                          &data,
                          &size) != AK_OK)
    return;

  xdoc = xml_parse(data, XML_PREFIXES | XML_READONLY);
  if (!xdoc
      || !xdoc->root
      || !ak_3mf_bambu_orca_tag_sz(xdoc->root, _s_ak_config, _s_ak_config_len)) {
    if (xdoc)
      xml_free(xdoc);
    free(data);
    return;
  }

  for (objectXml = xdoc->root->val; objectXml; objectXml = objectXml->next) {
    xml_t    *partXml;
    uint32_t  objectId;
    uint32_t  objectExtruder;

    if (!ak_3mf_bambu_orca_tag_sz(objectXml, _s_ak_object, _s_ak_object_len))
      continue;

    objectId       = xmla_u32(ak_3mf_bambu_orca_xmla_local_sz(objectXml,
                                                              _s_ak_id,
                                                              _s_ak_id_len),
                              0u);
    objectExtruder = ak_3mf_bambu_orca_metadata_u32(objectXml,
                                                    _s_ak_extruder,
                                                    _s_ak_extruder_len,
                                                    0u);
    (void)ak_3mf_bambu_orca_add_part_material(st, objectId, objectExtruder);

    for (partXml = objectXml->val; partXml; partXml = partXml->next) {
      uint32_t partId;
      uint32_t partExtruder;

      if (!ak_3mf_bambu_orca_tag_sz(partXml, _s_ak_part, _s_ak_part_len))
        continue;

      partId       = xmla_u32(ak_3mf_bambu_orca_xmla_local_sz(partXml,
                                                              _s_ak_id,
                                                              _s_ak_id_len),
                              0u);
      partExtruder = ak_3mf_bambu_orca_metadata_u32(partXml,
                                                    _s_ak_extruder,
                                                    _s_ak_extruder_len,
                                                    objectExtruder);
      (void)ak_3mf_bambu_orca_add_part_material(st, partId, partExtruder);
    }
  }

  xml_free(xdoc);
  free(data);
}

AK_HIDE
void
ak_3mf_bambu_orca_parse_metadata(AK3MFImportState * __restrict st) {
  ak_3mf_bambu_orca_parse_project_settings(st);
  if (st && st->bambuColorCount > 0u)
    ak_3mf_bambu_orca_parse_model_settings(st);
}

AK_HIDE
AkMaterial*
ak_3mf_bambu_orca_material_for_extruder(AK3MFImportState * __restrict st,
                                        uint32_t                      extruder) {
  AkHeap            *heap;
  AkMaterial        *material;
  AkMaterialSurface *surface;
  char               name[64];
  char              *p;
  uint32_t           colorIndex;

  if (!st || !st->doc || extruder == 0u)
    return NULL;

  colorIndex = extruder - 1u;
  if (colorIndex >= st->bambuColorCount
      || !st->bambuColors
      || !st->bambuMaterials)
    return NULL;

  if (st->bambuMaterials[colorIndex])
    return st->bambuMaterials[colorIndex];

  heap = ak_heap_getheap(st->doc);
  if (!heap)
    return NULL;

  material = ak_heap_calloc(heap, st->doc, sizeof(*material));
  if (!material)
    return NULL;
  ak_setypeid(material, AKT_MATERIAL);

  surface = ak_heap_calloc(heap, material, sizeof(*surface));
  if (!surface)
    return NULL;

  surface->baseColor = ak_3mf_material_color_input(heap,
                                                   surface,
                                                   st->bambuColors[colorIndex]);
  surface->metallic  = ak_3mf_material_scalar_input(heap,
                                                    surface,
                                                    _s_ak_metallic,
                                                    0.0f);
  surface->roughness = ak_3mf_material_scalar_input(heap,
                                                    surface,
                                                    _s_ak_roughness,
                                                    1.0f);
  if (!surface->baseColor || !surface->metallic || !surface->roughness)
    return NULL;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->alphaCutoff      = 0.5f;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  material->surface         = surface;

  memcpy(name,
         AK_3MF_BAMBU_MATERIAL_NAME_PREFIX,
         sizeof(AK_3MF_BAMBU_MATERIAL_NAME_PREFIX) - 1u);
  p  = name + sizeof(AK_3MF_BAMBU_MATERIAL_NAME_PREFIX) - 1u;
  p  = ak_io_text_format_uint64(p, extruder);
  *p = '\0';
  material->name = ak_heap_strdup(heap, material, name);

  AK_LIB_PREPEND(st->doc->lib.materials, material, next);
  st->bambuMaterials[colorIndex] = material;
  return material;
}

AK_HIDE
uint32_t
ak_3mf_bambu_orca_extruder_for_object(AK3MFImportState * __restrict st,
                                      uint32_t                      objectId) {
  size_t i;

  if (!st || objectId == 0u)
    return 0u;

  for (i = 0; i < st->bambuPartCount; i++) {
    if (st->bambuParts[i].objectId == objectId)
      return st->bambuParts[i].extruder;
  }

  return 0u;
}

AK_HIDE
AkMaterial*
ak_3mf_bambu_orca_material_for_object(AK3MFImportState * __restrict st,
                                      uint32_t                      objectId) {
  return ak_3mf_bambu_orca_material_for_extruder(
    st,
    ak_3mf_bambu_orca_extruder_for_object(st, objectId));
}
