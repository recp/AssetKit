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

#include "../internal.h"
#include "../vendor/bambu/bambu.h"
#include "../../../common/util.h"
#include "../../../../string_fast.h"
#include "../../../../strpool.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

typedef struct AK3MFFastSlice {
  const char *begin;
  const char *end;
} AK3MFFastSlice;

typedef struct AK3MFFastMeshSlices {
  AK3MFFastSlice objectTag;
  AK3MFFastSlice vertices;
  AK3MFFastSlice triangles;
  uint32_t        objectId;
  size_t          vertexCount;
  size_t          triangleCount;
  bool            hasPaint;
} AK3MFFastMeshSlices;

typedef struct AK3MFFastTag {
  AK3MFFastSlice full;
  AK3MFFastSlice localName;
  bool           closing;
  bool           selfClosing;
} AK3MFFastTag;

typedef enum AK3MFPaintKind {
  AK_3MF_PAINT_NONE = 0,
  AK_3MF_PAINT_ORCA,
  AK_3MF_PAINT_SEGMENTATION
} AK3MFPaintKind;

enum {
  AK_3MF_PAINT_BUCKET_COUNT = 64u,
  AK_3MF_PAINT_STACK_NODES  = 128u,
  AK_3MF_PAINT_STACK_LOCAL  = 64u,
  AK_3MF_PAINT_STATE_SEGMENTED = 255u
};

typedef struct AK3MFPaintNode {
  uint32_t children[4];
  uint8_t  splitSides;
  uint8_t  specialSide;
  uint8_t  state;
} AK3MFPaintNode;

typedef struct AK3MFPaintTree {
  AK3MFPaintNode  stackNodes[AK_3MF_PAINT_STACK_NODES];
  AK3MFPaintNode *nodes;
  size_t          nodeCount;
  size_t          nodeCapacity;
  size_t          readOffset;
  bool            heapNodes;
} AK3MFPaintTree;

typedef struct AK3MFPaintMidpoint {
  uint32_t a;
  uint32_t b;
  uint32_t index;
} AK3MFPaintMidpoint;

typedef struct AK3MFPaintPlan {
  size_t   counts[AK_3MF_PAINT_BUCKET_COUNT];
  size_t   outputTriangleCount;
  size_t   extraVertexCount;
  uint32_t defaultExtruder;
  struct AK3MFPaintTriangle *triangles;
} AK3MFPaintPlan;

typedef struct AK3MFPaintTriangle {
  uint32_t v[3];
  uint32_t paintOffset;
  uint32_t paintSize;
  uint8_t  state;
} AK3MFPaintTriangle;

typedef struct AK3MFPaintBucketFill {
  void    *items[AK_3MF_PAINT_BUCKET_COUNT];
  size_t   offsets[AK_3MF_PAINT_BUCKET_COUNT];
  AkTypeId componentType;
} AK3MFPaintBucketFill;

typedef struct AK3MFPaintSubdivider {
  AK3MFPaintMidpoint  stackMidpoints[AK_3MF_PAINT_STACK_LOCAL];
  uint32_t            stackLocalToGlobal[AK_3MF_PAINT_STACK_LOCAL];
  AK3MFPaintMidpoint *midpoints;
  uint32_t           *localToGlobal;
  size_t              midpointCount;
  size_t              midpointCapacity;
  size_t              localCount;
  size_t              localCapacity;
  float              *positions;
  size_t             *vertexCursor;
  size_t              outputVertexCount;
  AK3MFPaintPlan     *plan;
  AK3MFPaintBucketFill *fill;
  bool                heapMidpoints;
  bool                heapLocalToGlobal;
  bool                ok;
} AK3MFPaintSubdivider;

static
bool
ak_3mf_fast_parse_u32_slice(const AK3MFFastSlice * __restrict slice,
                            uint32_t             * __restrict out) {
  uint64_t    value;
  const char *p;
  bool        found;

  if (!slice || !out || !slice->begin || slice->end <= slice->begin)
    return false;

  p     = slice->begin;
  value = 0u;
  found = false;
  while (p < slice->end && *p >= '0' && *p <= '9') {
    value = value * 10u + (uint64_t)(*p++ - '0');
    if (value > UINT32_MAX)
      return false;
    found = true;
  }

  if (!found)
    return false;

  *out = (uint32_t)value;
  return true;
}

static
const char*
ak_3mf_fast_tag_end(const char * __restrict p,
                    const char * __restrict end) {
  return p && end ? memchr(p, '>', (size_t)(end - p)) : NULL;
}

static
const char*
ak_3mf_fast_skip_space(const char *p, const char *end) {
  while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
    p++;
  return p;
}

static
const char*
ak_3mf_fast_skip_tag_name(const char *p, const char *end) {
  while (p < end && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r'
         && *p != '/' && *p != '>')
    p++;
  return p;
}

static
bool
ak_3mf_fast_slice_eq_packed(const AK3MFFastSlice * __restrict slice,
                            uint64_t                          packed,
                            size_t                            len) {
  size_t sliceLen;

  if (!slice || !slice->begin || slice->end < slice->begin)
    return false;

  sliceLen = (size_t)(slice->end - slice->begin);
  if (len == _s_ak_triangles_len)
    return sliceLen == _s_ak_triangles_len
           && ak_str_load8_fast(slice->begin) == packed
           && slice->begin[8] == _s_ak_triangles_last;

  return ak_str_eq_packed_fast(slice->begin, sliceLen, packed, len);
}

static
bool
ak_3mf_fast_slice_eq_paint_color(const AK3MFFastSlice * __restrict slice) {
  size_t sliceLen;

  if (!slice || !slice->begin || slice->end < slice->begin)
    return false;

  sliceLen = (size_t)(slice->end - slice->begin);
  return sliceLen == _s_ak_paint_color_len
         && ak_str_load8_fast(slice->begin) == _s_ak_paint_color_u64_prefix
         && slice->begin[8] == 'l'
         && slice->begin[9] == 'o'
         && slice->begin[10] == 'r';
}

static
bool
ak_3mf_fast_slice_eq_mmu_segmentation(const AK3MFFastSlice * __restrict slice) {
  size_t sliceLen;

  if (!slice || !slice->begin || slice->end < slice->begin)
    return false;

  sliceLen = (size_t)(slice->end - slice->begin);
  return sliceLen == _s_ak_mmu_segmentation_len
         && ak_str_load8_fast(slice->begin) == _s_ak_mmu_segmentation_u64_prefix
         && slice->begin[8] == 'e'
         && slice->begin[9] == 'n'
         && slice->begin[10] == 't'
         && slice->begin[11] == 'a'
         && slice->begin[12] == 't'
         && slice->begin[13] == 'i'
         && slice->begin[14] == 'o'
         && slice->begin[15] == 'n';
}

static
bool
ak_3mf_fast_next_tag(const char       ** __restrict cursor,
                     const char        * __restrict end,
                     AK3MFFastTag       * __restrict tag) {
  const char *p;
  const char *tagEnd;
  const char *nameBegin;
  const char *nameEnd;
  const char *colon;

  if (!cursor || !*cursor || !end || !tag)
    return false;

  p = *cursor;
  for (;;) {
    p = memchr(p, '<', (size_t)(end - p));
    if (!p)
      return false;
    if (p + 1u >= end)
      return false;
    if (p[1] != '!' && p[1] != '?')
      break;
    tagEnd = ak_3mf_fast_tag_end(p, end);
    if (!tagEnd)
      return false;
    p = tagEnd + 1u;
  }

  tagEnd = ak_3mf_fast_tag_end(p, end);
  if (!tagEnd)
    return false;

  nameBegin = p + 1u;
  tag->closing = false;
  if (nameBegin < tagEnd && *nameBegin == '/') {
    tag->closing = true;
    nameBegin++;
  }

  nameEnd = nameBegin;
  while (nameEnd < tagEnd
         && *nameEnd != ' ' && *nameEnd != '\t'
         && *nameEnd != '\n' && *nameEnd != '\r'
         && *nameEnd != '/' && *nameEnd != '>')
    nameEnd++;

  if (nameEnd <= nameBegin) {
    *cursor = tagEnd + 1u;
    return ak_3mf_fast_next_tag(cursor, end, tag);
  }

  colon = memchr(nameBegin, ':', (size_t)(nameEnd - nameBegin));
  if (colon)
    nameBegin = colon + 1u;

  tag->full.begin      = p;
  tag->full.end        = tagEnd;
  tag->localName.begin = nameBegin;
  tag->localName.end   = nameEnd;
  tag->selfClosing     = tagEnd > p && tagEnd[-1] == '/';
  *cursor              = tagEnd + 1u;
  return true;
}

static
bool
ak_3mf_fast_next_attr(const char      ** __restrict cursor,
                      const char       * __restrict tagEnd,
                      AK3MFFastSlice    * __restrict name,
                      AK3MFFastSlice    * __restrict value) {
  const char *p;
  const char *attrName;
  const char *attrNameEnd;
  const char *colon;
  char        quote;
  size_t      nameLen;

  if (!cursor || !*cursor || !tagEnd || !name || !value)
    return false;

  p = ak_3mf_fast_skip_space(*cursor, tagEnd);
  if (p >= tagEnd || *p == '/' || *p == '>')
    return false;

  attrName = p;
  while (p < tagEnd && *p != '=' && *p != ' ' && *p != '\t'
         && *p != '\n' && *p != '\r')
    p++;
  attrNameEnd = p;

  p = ak_3mf_fast_skip_space(p, tagEnd);
  if (p >= tagEnd || *p != '=')
    return false;
  p++;
  p = ak_3mf_fast_skip_space(p, tagEnd);
  if (p >= tagEnd)
    return false;

  quote = 0;
  if (*p == '"' || *p == '\'' || *p == '`')
    quote = *p++;

  value->begin = p;
  if (quote) {
    while (p < tagEnd && *p != quote)
      p++;
    if (p >= tagEnd)
      return false;
    value->end = p++;
  } else {
    while (p < tagEnd && *p != ' ' && *p != '\t' && *p != '\n'
           && *p != '\r' && *p != '/' && *p != '>')
      p++;
    value->end = p;
  }

  name->begin = attrName;
  name->end   = attrNameEnd;
  nameLen     = (size_t)(attrNameEnd - attrName);
  colon       = memchr(attrName, ':', nameLen);
  if (colon) {
    name->begin = colon + 1u;
    name->end   = attrNameEnd;
  }

  *cursor = p;
  return true;
}

static
bool
ak_3mf_fast_attr_slice_packed(const char       * __restrict tagBegin,
                              const char       * __restrict tagEnd,
                              uint64_t                       namePacked,
                              size_t                         nameLen,
                              AK3MFFastSlice    * __restrict out) {
  const char *p;

  if (!tagBegin || !tagEnd || !out)
    return false;

  p = ak_3mf_fast_skip_tag_name(tagBegin, tagEnd);

  while (p < tagEnd) {
    AK3MFFastSlice attrName;
    AK3MFFastSlice attrValue;

    if (!ak_3mf_fast_next_attr(&p, tagEnd, &attrName, &attrValue))
      return false;
    if (ak_3mf_fast_slice_eq_packed(&attrName, namePacked, nameLen)) {
      *out = attrValue;
      return true;
    }
  }

  return false;
}

static
bool
ak_3mf_fast_attr_u32_local(const char * __restrict tagBegin,
                           const char * __restrict tagEnd,
                           uint64_t                 namePacked,
                           size_t                  nameLen,
                           uint32_t   * __restrict out) {
  AK3MFFastSlice slice;

  if (!out
      || !ak_3mf_fast_attr_slice_packed(tagBegin,
                                        tagEnd,
                                        namePacked,
                                        nameLen,
                                        &slice))
    return false;

  return ak_3mf_fast_parse_u32_slice(&slice, out);
}

static
bool
ak_3mf_fast_find_child_content(const char       * __restrict p,
                               const char       * __restrict end,
                               uint64_t                       tagPacked,
                               size_t                         tagLen,
                               AK3MFFastSlice    * __restrict out) {
  const char *cursor;
  AK3MFFastTag tag;

  cursor = p;
  while (ak_3mf_fast_next_tag(&cursor, end, &tag)) {
    const char *innerCursor;
    uint32_t    depth;

    if (tag.closing
        || !ak_3mf_fast_slice_eq_packed(&tag.localName, tagPacked, tagLen))
      continue;

    if (tag.selfClosing)
      return false;

    out->begin  = tag.full.end + 1u;
    innerCursor = out->begin;
    depth       = 1u;
    while (ak_3mf_fast_next_tag(&innerCursor, end, &tag)) {
      if (!ak_3mf_fast_slice_eq_packed(&tag.localName, tagPacked, tagLen))
        continue;

      if (tag.closing) {
        depth--;
        if (depth == 0u) {
          out->end = tag.full.begin;
          return true;
        }
      } else if (!tag.selfClosing) {
        depth++;
      }
    }

    return false;
  }

  return false;
}

static
bool
ak_3mf_fast_attr_is_triangle_material(const AK3MFFastSlice * __restrict attrName) {
  size_t len;

  if (!attrName || !attrName->begin || attrName->end < attrName->begin)
    return false;

  len = (size_t)(attrName->end - attrName->begin);
  if (len == 2u)
    return attrName->begin[0] == 'p'
           && (attrName->begin[1] == '1'
               || attrName->begin[1] == '2'
               || attrName->begin[1] == '3');

  return len == 3u
         && attrName->begin[0] == 'p'
         && attrName->begin[1] == 'i'
         && attrName->begin[2] == 'd';
}

static
bool
ak_3mf_fast_attr_paint_kind(const AK3MFFastSlice * __restrict attrName,
                            AK3MFPaintKind       * __restrict kind) {
  if (ak_3mf_fast_slice_eq_paint_color(attrName)) {
    if (kind)
      *kind = AK_3MF_PAINT_ORCA;
    return true;
  }

  if (ak_3mf_fast_slice_eq_mmu_segmentation(attrName)) {
    if (kind)
      *kind = AK_3MF_PAINT_SEGMENTATION;
    return true;
  }

  return false;
}

static
bool
ak_3mf_fast_triangle_inspect_attrs(const AK3MFFastTag * __restrict tag,
                                   bool               * __restrict hasPaint) {
  const char *p;

  if (!memchr(tag->full.begin,
              'p',
              (size_t)(tag->full.end - tag->full.begin))
      && !memchr(tag->full.begin,
                 'm',
                 (size_t)(tag->full.end - tag->full.begin)))
    return false;

  p = ak_3mf_fast_skip_tag_name(tag->full.begin, tag->full.end);
  while (p < tag->full.end) {
    AK3MFFastSlice attrName;
    AK3MFFastSlice attrValue;

    if (!ak_3mf_fast_next_attr(&p, tag->full.end, &attrName, &attrValue))
      return false;
    if (ak_3mf_fast_attr_is_triangle_material(&attrName))
      return true;
    if (ak_3mf_fast_attr_paint_kind(&attrName, NULL) && hasPaint)
      *hasPaint = true;
  }

  return false;
}

static
bool
ak_3mf_fast_count_tags_packed(const char * __restrict p,
                              const char * __restrict end,
                              uint64_t                 tagPacked,
                              size_t                   tagLen,
                              bool                     rejectTriangleMaterialAttrs,
                              bool      * __restrict   hasPaintAttrs,
                              size_t    * __restrict count) {
  const char  *cursor;
  AK3MFFastTag tag;
  size_t       n;

  if (!count)
    return false;

  cursor = p;
  n      = 0u;
  while (ak_3mf_fast_next_tag(&cursor, end, &tag)) {
    if (!tag.closing
        && ak_3mf_fast_slice_eq_packed(&tag.localName, tagPacked, tagLen)) {
      if (rejectTriangleMaterialAttrs
          && ak_3mf_fast_triangle_inspect_attrs(&tag, hasPaintAttrs))
        return false;
      n++;
    }
  }

  *count = n;
  return true;
}

static
bool
ak_3mf_fast_next_object(const char       ** __restrict cursor,
                        const char        * __restrict end,
                        AK3MFFastSlice     * __restrict objectTag,
                        AK3MFFastSlice     * __restrict objectBody) {
  const char *objectBegin;
  const char *tagEnd;
  const char *objectClose;

  AK3MFFastTag tag;

  while (ak_3mf_fast_next_tag(cursor, end, &tag)) {
    const char *innerCursor;

    if (tag.closing
        || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                        _s_ak_object_u64_exact,
                                        _s_ak_object_len))
      continue;
    if (tag.selfClosing)
      return false;

    objectBegin = tag.full.begin;
    tagEnd      = tag.full.end;
    innerCursor = tagEnd + 1u;
    while (ak_3mf_fast_next_tag(&innerCursor, end, &tag)) {
      if (!tag.closing
          || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                          _s_ak_object_u64_exact,
                                          _s_ak_object_len))
        continue;

      objectClose       = tag.full.begin;
      objectTag->begin  = objectBegin;
      objectTag->end    = tagEnd;
      objectBody->begin = tagEnd + 1u;
      objectBody->end   = objectClose;
      *cursor           = tag.full.end + 1u;
      return true;
    }

    return false;
  }

  return false;
}

static
bool
ak_3mf_fast_read_object_slices(const AK3MFFastSlice * __restrict objectTag,
                               const AK3MFFastSlice * __restrict objectBody,
                               AK3MFFastMeshSlices  * __restrict slices) {
  AK3MFFastSlice mesh;

  memset(slices, 0, sizeof(*slices));
  slices->objectTag = *objectTag;

  if (!ak_3mf_fast_attr_u32_local(objectTag->begin,
                                  objectTag->end,
                                  _s_ak_id_u64_exact,
                                  _s_ak_id_len,
                                  &slices->objectId)
      || slices->objectId == 0u)
    return false;

  if (!ak_3mf_fast_find_child_content(objectBody->begin,
                                      objectBody->end,
                                      _s_ak_mesh_u64_exact,
                                      _s_ak_mesh_len,
                                      &mesh))
    return false;

  if (!ak_3mf_fast_find_child_content(mesh.begin,
                                      mesh.end,
                                      _s_ak_vertices_u64_exact,
                                      _s_ak_vertices_len,
                                      &slices->vertices)
      || !ak_3mf_fast_find_child_content(mesh.begin,
                                         mesh.end,
                                         _s_ak_triangles_u64_prefix,
                                         _s_ak_triangles_len,
                                         &slices->triangles))
    return false;

  if (!ak_3mf_fast_count_tags_packed(slices->vertices.begin,
                                     slices->vertices.end,
                                     _s_ak_vertex_u64_exact,
                                     _s_ak_vertex_len,
                                     false,
                                     NULL,
                                     &slices->vertexCount)
      || !ak_3mf_fast_count_tags_packed(slices->triangles.begin,
                                        slices->triangles.end,
                                        _s_ak_triangle_u64_exact,
                                        _s_ak_triangle_len,
                                        true,
                                        &slices->hasPaint,
                                        &slices->triangleCount))
    return false;

  return slices->vertexCount > 0u
         && slices->triangleCount > 0u
         && slices->vertexCount <= UINT32_MAX
         && slices->triangleCount <= UINT32_MAX / 3u;
}

static
bool
ak_3mf_fast_reserve_objects(AK3MFImportState * __restrict st,
                            size_t                         extra) {
  AK3MFObject *objects;
  size_t       needed;
  size_t       newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->objectCount > SIZE_MAX - extra)
    return false;

  needed = st->objectCount + extra;
  if (needed <= st->objectCapacity)
    return true;

  newCapacity = st->objectCapacity ? st->objectCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  objects = realloc(st->objects, sizeof(*objects) * newCapacity);
  if (!objects)
    return false;

  memset(objects + st->objectCapacity,
         0,
         sizeof(*objects) * (newCapacity - st->objectCapacity));
  st->objects        = objects;
  st->objectCapacity = newCapacity;
  return true;
}

static
const char*
ak_3mf_fast_dup_model_path(AK3MFImportState * __restrict st,
                           const char       * __restrict modelPath) {
  AkHeap     *heap;
  const char *src;
  size_t      len;

  if (!st || !st->doc || !modelPath)
    return NULL;

  heap = ak_heap_getheap(st->doc);
  if (!heap)
    return NULL;

  src = modelPath;
  len = strlen(modelPath);
  while (len > 0u && (*src == '/' || *src == '\\')) {
    src++;
    len--;
  }

  return ak_heap_strndup(heap, st->doc, src, len);
}

static
char*
ak_3mf_fast_dup_slice_cstr(const AK3MFFastSlice * __restrict slice) {
  char  *out;
  size_t len;

  if (!slice || !slice->begin || slice->end <= slice->begin)
    return NULL;

  len = (size_t)(slice->end - slice->begin);
  out = malloc(len + 1u);
  if (!out)
    return NULL;

  memcpy(out, slice->begin, len);
  out[len] = '\0';
  return out;
}

static
bool
ak_3mf_fast_add_production_object(AK3MFImportState       * __restrict st,
                                  const AK3MFFastSlice   * __restrict objectTag,
                                  uint32_t                             objectId) {
  AK3MFFastSlice uuidSlice;
  char          *uuid;

  if (!st || !st->print || !objectTag)
    return true;
  if (!ak_3mf_fast_attr_slice_packed(objectTag->begin,
                                     objectTag->end,
                                     _s_ak_UUID_u64_exact,
                                     _s_ak_UUID_len,
                                     &uuidSlice))
    return true;

  uuid = ak_3mf_fast_dup_slice_cstr(&uuidSlice);
  if (!uuid)
    return false;

  (void)ak_printAddProductionItem(st->doc,
                                  AK_PRINT_PRODUCTION_OBJECT,
                                  uuid,
                                  st->currentModelPath,
                                  NULL,
                                  NULL,
                                  objectId,
                                  0u);
  free(uuid);
  return true;
}

static
bool
ak_3mf_fast_parse_vertex_tag(const char *p,
                             const char * __restrict tagEnd,
                             float      * __restrict out) {
  AK3MFFastSlice attrName;
  AK3MFFastSlice attrValue;
  bool           hasX;
  bool           hasY;
  bool           hasZ;

  p    = ak_3mf_fast_skip_tag_name(p, tagEnd);
  hasX = false;
  hasY = false;
  hasZ = false;

  while (ak_3mf_fast_next_attr(&p, tagEnd, &attrName, &attrValue)) {
    size_t len;

    len = (size_t)(attrName.end - attrName.begin);
    if (len != 1u)
      continue;

    switch (*attrName.begin) {
      case 'x':
        out[0] = (float)xml__parse_double(attrValue.begin, attrValue.end, 0.0);
        hasX = true;
        break;
      case 'y':
        out[1] = (float)xml__parse_double(attrValue.begin, attrValue.end, 0.0);
        hasY = true;
        break;
      case 'z':
        out[2] = (float)xml__parse_double(attrValue.begin, attrValue.end, 0.0);
        hasZ = true;
        break;
      default:
        break;
    }
  }

  return hasX && hasY && hasZ;
}

static
bool
ak_3mf_fast_fill_positions(const AK3MFFastSlice * __restrict vertices,
                           float                * __restrict positions,
                           size_t                            vertexCount) {
  const char  *cursor;
  AK3MFFastTag tag;
  size_t       i;

  cursor = vertices->begin;
  i      = 0u;
  while (ak_3mf_fast_next_tag(&cursor, vertices->end, &tag)) {
    if (tag.closing
        || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                        _s_ak_vertex_u64_exact,
                                        _s_ak_vertex_len))
      continue;
    if (i >= vertexCount)
      return false;
    if (!ak_3mf_fast_parse_vertex_tag(tag.full.begin,
                                      tag.full.end,
                                      positions + i * 3u))
      return false;
    i++;
  }

  return i == vertexCount;
}

static
bool
ak_3mf_fast_parse_triangle(const char *p,
                           const char * __restrict tagEnd,
                           uint32_t                 vertexCount,
                           uint32_t                 v[3]) {
  AK3MFFastSlice attrName;
  AK3MFFastSlice attrValue;
  bool           hasV1;
  bool           hasV2;
  bool           hasV3;

  p     = ak_3mf_fast_skip_tag_name(p, tagEnd);
  hasV1 = false;
  hasV2 = false;
  hasV3 = false;

  while (ak_3mf_fast_next_attr(&p, tagEnd, &attrName, &attrValue)) {
    size_t len;

    len = (size_t)(attrName.end - attrName.begin);
    if (len != 2u || attrName.begin[0] != 'v')
      continue;

    switch (attrName.begin[1]) {
      case '1':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[0]))
          return false;
        hasV1 = true;
        break;
      case '2':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[1]))
          return false;
        hasV2 = true;
        break;
      case '3':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[2]))
          return false;
        hasV3 = true;
        break;
      default:
        break;
    }
  }

  return hasV1
         && hasV2
         && hasV3
         && v[0] < vertexCount
         && v[1] < vertexCount
         && v[2] < vertexCount;
}

static
bool
ak_3mf_fast_parse_triangle_paint(const char       *p,
                                 const char       * __restrict tagEnd,
                                 uint32_t                       vertexCount,
                                 uint32_t                       v[3],
                                 AK3MFFastSlice   * __restrict paint,
                                 AK3MFPaintKind   * __restrict paintKind) {
  AK3MFFastSlice attrName;
  AK3MFFastSlice attrValue;
  bool           hasV1;
  bool           hasV2;
  bool           hasV3;

  p = ak_3mf_fast_skip_tag_name(p, tagEnd);
  hasV1 = false;
  hasV2 = false;
  hasV3 = false;
  paint->begin = NULL;
  paint->end = NULL;
  *paintKind = AK_3MF_PAINT_NONE;

  while (ak_3mf_fast_next_attr(&p, tagEnd, &attrName, &attrValue)) {
    AK3MFPaintKind attrPaintKind;
    size_t         len;

    if (ak_3mf_fast_attr_is_triangle_material(&attrName))
      return false;

    attrPaintKind = AK_3MF_PAINT_NONE;
    if (ak_3mf_fast_attr_paint_kind(&attrName, &attrPaintKind)) {
      if (*paintKind == AK_3MF_PAINT_NONE
          || attrPaintKind == AK_3MF_PAINT_ORCA) {
        *paint     = attrValue;
        *paintKind = attrPaintKind;
      }
      continue;
    }

    len = (size_t)(attrName.end - attrName.begin);
    if (len != 2u || attrName.begin[0] != 'v')
      continue;

    switch (attrName.begin[1]) {
      case '1':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[0]))
          return false;
        hasV1 = true;
        break;
      case '2':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[1]))
          return false;
        hasV2 = true;
        break;
      case '3':
        if (!ak_3mf_fast_parse_u32_slice(&attrValue, &v[2]))
          return false;
        hasV3 = true;
        break;
      default:
        break;
    }
  }

  return hasV1
         && hasV2
         && hasV3
         && v[0] < vertexCount
         && v[1] < vertexCount
         && v[2] < vertexCount;
}

static
bool
ak_3mf_fast_hex_nibble(char c, uint8_t * __restrict out) {
  if (c >= '0' && c <= '9') {
    *out = (uint8_t)(c - '0');
    return true;
  }

  c = (char)(c | 0x20);
  if (c >= 'a' && c <= 'f') {
    *out = (uint8_t)(c - 'a' + 10);
    return true;
  }

  return false;
}

static
bool
ak_3mf_fast_orca_paint_state(const AK3MFFastSlice * __restrict paint,
                             uint32_t             * __restrict state) {
  size_t  len;
  uint8_t first;
  size_t  i;

  if (!paint || !paint->begin || paint->end < paint->begin || !state)
    return false;

  len = (size_t)(paint->end - paint->begin);
  if (len == 0u) {
    *state = 0u;
    return true;
  }

  if (len == 1u) {
    if (paint->begin[0] == '4') {
      *state = 1u;
      return true;
    }
    if (paint->begin[0] == '8') {
      *state = 2u;
      return true;
    }
    return false;
  }

  if (len < 2u || len > 5u)
    return false;
  if ((paint->begin[len - 1u] | 0x20) != 'c')
    return false;
  if (!ak_3mf_fast_hex_nibble(paint->begin[0], &first) || first > 14u)
    return false;

  for (i = 1u; i + 1u < len; i++) {
    if ((paint->begin[i] | 0x20) != 'f')
      return false;
  }

  *state = 3u + (uint32_t)first + (uint32_t)(len - 2u) * 15u;
  return *state < AK_3MF_PAINT_BUCKET_COUNT;
}

static
uint32_t
ak_3mf_fast_paint_bucket(const AK3MFPaintPlan * __restrict plan,
                         uint32_t                           state) {
  uint32_t extruder;

  extruder = state == 0u ? plan->defaultExtruder : state;
  return extruder < AK_3MF_PAINT_BUCKET_COUNT ? extruder : 0u;
}

static
bool
ak_3mf_fast_paint_emit_global(AK3MFPaintBucketFill * __restrict fill,
                              uint32_t                           bucket,
                              uint32_t                           v0,
                              uint32_t                           v1,
                              uint32_t                           v2) {
  size_t offset;

  if (!fill || bucket >= AK_3MF_PAINT_BUCKET_COUNT || !fill->items[bucket])
    return false;

  offset = fill->offsets[bucket];
  switch (fill->componentType) {
    case AKT_UBYTE: {
      uint8_t *dst = (uint8_t *)fill->items[bucket];
      dst[offset++] = (uint8_t)v0;
      dst[offset++] = (uint8_t)v1;
      dst[offset++] = (uint8_t)v2;
      break;
    }
    case AKT_USHORT: {
      uint16_t *dst = (uint16_t *)fill->items[bucket];
      dst[offset++] = (uint16_t)v0;
      dst[offset++] = (uint16_t)v1;
      dst[offset++] = (uint16_t)v2;
      break;
    }
    case AKT_UINT: {
      uint32_t *dst = (uint32_t *)fill->items[bucket];
      dst[offset++] = v0;
      dst[offset++] = v1;
      dst[offset++] = v2;
      break;
    }
    default:
      return false;
  }

  fill->offsets[bucket] = offset;
  return true;
}

static
bool
ak_3mf_fast_paint_emit_state(AK3MFPaintPlan       * __restrict plan,
                             AK3MFPaintBucketFill * __restrict fill,
                             uint32_t                           state,
                             const uint32_t                     v[3]) {
  uint32_t bucket;

  bucket = ak_3mf_fast_paint_bucket(plan, state);
  if (fill)
    return ak_3mf_fast_paint_emit_global(fill, bucket, v[0], v[1], v[2]);

  plan->counts[bucket]++;
  plan->outputTriangleCount++;
  return true;
}

static
bool
ak_3mf_paint_tree_read_nibble(AK3MFPaintTree       * __restrict tree,
                              const AK3MFFastSlice * __restrict paint,
                              uint8_t              * __restrict nibble) {
  size_t len;

  len = (size_t)(paint->end - paint->begin);
  if (tree->readOffset >= len)
    return false;

  return ak_3mf_fast_hex_nibble(paint->end[-1 - (ptrdiff_t)tree->readOffset++],
                                nibble);
}

static
bool
ak_3mf_paint_tree_decode_node(AK3MFPaintTree       * __restrict tree,
                              const AK3MFFastSlice * __restrict paint,
                              uint32_t             * __restrict nodeIndex) {
  AK3MFPaintNode node;
  uint32_t       index;
  uint8_t        code;
  uint32_t       childCount;
  uint32_t       i;

  if (tree->nodeCount >= tree->nodeCapacity)
    return false;
  if (!ak_3mf_paint_tree_read_nibble(tree, paint, &code))
    return false;

  memset(&node, 0, sizeof(node));
  node.splitSides  = (uint8_t)(code & 3u);
  node.specialSide = (uint8_t)((code >> 2u) & 3u);
  index            = (uint32_t)tree->nodeCount++;

  if (node.splitSides == 0u) {
    if (node.specialSide == 3u) {
      uint8_t extraState;

      if (!ak_3mf_paint_tree_read_nibble(tree, paint, &extraState))
        return false;
      node.state = (uint8_t)(extraState + 3u);
      if (node.state > 15u)
        node.state = 15u;
    } else {
      node.state = node.specialSide;
    }

    tree->nodes[index] = node;
    *nodeIndex = index;
    return true;
  }

  childCount = (uint32_t)node.splitSides + 1u;
  for (i = 0u; i < childCount; i++) {
    if (!ak_3mf_paint_tree_decode_node(tree, paint, &node.children[i]))
      return false;
  }

  tree->nodes[index] = node;
  *nodeIndex = index;
  return true;
}

static
bool
ak_3mf_paint_tree_decode(AK3MFPaintTree       * __restrict tree,
                         const AK3MFFastSlice * __restrict paint,
                         uint32_t             * __restrict rootIndex) {
  size_t len;

  if (!tree || !paint || !paint->begin || paint->end <= paint->begin)
    return false;

  len = (size_t)(paint->end - paint->begin);
  memset(tree, 0, sizeof(*tree));
  if (len <= AK_ARRAY_LEN(tree->stackNodes)) {
    tree->nodes        = tree->stackNodes;
    tree->nodeCapacity = AK_ARRAY_LEN(tree->stackNodes);
  } else {
    tree->nodes = malloc(sizeof(*tree->nodes) * len);
    if (!tree->nodes)
      return false;
    tree->nodeCapacity = len;
    tree->heapNodes    = true;
  }

  return ak_3mf_paint_tree_decode_node(tree, paint, rootIndex);
}

static
void
ak_3mf_paint_tree_dispose(AK3MFPaintTree * __restrict tree) {
  if (tree && tree->heapNodes)
    free(tree->nodes);
}

static
bool
ak_3mf_paint_subdivider_ensure_midpoints(AK3MFPaintSubdivider * __restrict sub,
                                         size_t                              needed) {
  AK3MFPaintMidpoint *midpoints;
  size_t              newCapacity;

  if (needed <= sub->midpointCapacity)
    return true;

  newCapacity = sub->midpointCapacity * 2u;
  while (newCapacity < needed)
    newCapacity *= 2u;

  if (sub->heapMidpoints) {
    midpoints = realloc(sub->midpoints, sizeof(*midpoints) * newCapacity);
  } else {
    midpoints = malloc(sizeof(*midpoints) * newCapacity);
    if (midpoints)
      memcpy(midpoints,
             sub->stackMidpoints,
             sizeof(*midpoints) * sub->midpointCount);
  }
  if (!midpoints)
    return false;

  sub->midpoints        = midpoints;
  sub->midpointCapacity = newCapacity;
  sub->heapMidpoints    = true;
  return true;
}

static
bool
ak_3mf_paint_subdivider_ensure_local(AK3MFPaintSubdivider * __restrict sub,
                                     size_t                              needed) {
  uint32_t *localToGlobal;
  size_t    newCapacity;

  if (!sub->fill || needed <= sub->localCapacity)
    return true;

  newCapacity = sub->localCapacity * 2u;
  while (newCapacity < needed)
    newCapacity *= 2u;

  if (sub->heapLocalToGlobal) {
    localToGlobal = realloc(sub->localToGlobal, sizeof(*localToGlobal) * newCapacity);
  } else {
    localToGlobal = malloc(sizeof(*localToGlobal) * newCapacity);
    if (localToGlobal)
      memcpy(localToGlobal,
             sub->stackLocalToGlobal,
             sizeof(*localToGlobal) * sub->localCount);
  }
  if (!localToGlobal)
    return false;

  sub->localToGlobal        = localToGlobal;
  sub->localCapacity        = newCapacity;
  sub->heapLocalToGlobal    = true;
  return true;
}

static
void
ak_3mf_paint_subdivider_init(AK3MFPaintSubdivider  * __restrict sub,
                             AK3MFPaintPlan        * __restrict plan,
                             AK3MFPaintBucketFill  * __restrict fill,
                             float                 * __restrict positions,
                             size_t                * __restrict vertexCursor,
                             size_t                              outputVertexCount,
                             const uint32_t                      v[3]) {
  memset(sub, 0, sizeof(*sub));
  sub->midpoints        = sub->stackMidpoints;
  sub->midpointCapacity = AK_ARRAY_LEN(sub->stackMidpoints);
  sub->localToGlobal    = sub->stackLocalToGlobal;
  sub->localCapacity    = AK_ARRAY_LEN(sub->stackLocalToGlobal);
  sub->localCount       = 3u;
  sub->positions        = positions;
  sub->vertexCursor     = vertexCursor;
  sub->outputVertexCount = outputVertexCount;
  sub->plan             = plan;
  sub->fill             = fill;
  sub->ok               = true;

  if (fill) {
    sub->localToGlobal[0] = v[0];
    sub->localToGlobal[1] = v[1];
    sub->localToGlobal[2] = v[2];
  }
}

static
void
ak_3mf_paint_subdivider_dispose(AK3MFPaintSubdivider * __restrict sub) {
  if (!sub)
    return;
  if (sub->heapMidpoints)
    free(sub->midpoints);
  if (sub->heapLocalToGlobal)
    free(sub->localToGlobal);
}

static
uint32_t
ak_3mf_paint_subdivider_midpoint(AK3MFPaintSubdivider * __restrict sub,
                                 uint32_t                           a,
                                 uint32_t                           b) {
  uint32_t lo;
  uint32_t hi;
  size_t   i;
  uint32_t localIndex;

  if (!sub->ok)
    return 0u;

  lo = a < b ? a : b;
  hi = a < b ? b : a;
  for (i = 0u; i < sub->midpointCount; i++) {
    if (sub->midpoints[i].a == lo && sub->midpoints[i].b == hi)
      return sub->midpoints[i].index;
  }

  localIndex = (uint32_t)sub->localCount;
  if (!ak_3mf_paint_subdivider_ensure_midpoints(sub, sub->midpointCount + 1u)
      || !ak_3mf_paint_subdivider_ensure_local(sub, sub->localCount + 1u)) {
    sub->ok = false;
    return 0u;
  }

  sub->midpoints[sub->midpointCount].a     = lo;
  sub->midpoints[sub->midpointCount].b     = hi;
  sub->midpoints[sub->midpointCount].index = localIndex;
  sub->midpointCount++;
  sub->localCount++;

  if (sub->fill) {
    uint32_t globalIndex;
    uint32_t ga;
    uint32_t gb;

    if (!sub->vertexCursor || *sub->vertexCursor >= sub->outputVertexCount) {
      sub->ok = false;
      return 0u;
    }

    globalIndex = (uint32_t)(*sub->vertexCursor)++;
    sub->localToGlobal[localIndex] = globalIndex;
    ga = sub->localToGlobal[a];
    gb = sub->localToGlobal[b];
    sub->positions[(size_t)globalIndex * 3u + 0u] =
      (sub->positions[(size_t)ga * 3u + 0u]
       + sub->positions[(size_t)gb * 3u + 0u]) * 0.5f;
    sub->positions[(size_t)globalIndex * 3u + 1u] =
      (sub->positions[(size_t)ga * 3u + 1u]
       + sub->positions[(size_t)gb * 3u + 1u]) * 0.5f;
    sub->positions[(size_t)globalIndex * 3u + 2u] =
      (sub->positions[(size_t)ga * 3u + 2u]
       + sub->positions[(size_t)gb * 3u + 2u]) * 0.5f;
  }

  return localIndex;
}

static
void
ak_3mf_paint_subdivider_emit(AK3MFPaintSubdivider * __restrict sub,
                             uint32_t                           state,
                             uint32_t                           i0,
                             uint32_t                           i1,
                             uint32_t                           i2) {
  uint32_t v[3];

  if (!sub->ok)
    return;

  if (sub->fill) {
    v[0] = sub->localToGlobal[i0];
    v[1] = sub->localToGlobal[i1];
    v[2] = sub->localToGlobal[i2];
    sub->ok = ak_3mf_fast_paint_emit_state(sub->plan, sub->fill, state, v);
    return;
  }

  sub->plan->counts[ak_3mf_fast_paint_bucket(sub->plan, state)]++;
  sub->plan->outputTriangleCount++;
}

static
void
ak_3mf_paint_subdivide_node(AK3MFPaintSubdivider * __restrict sub,
                            const AK3MFPaintNode * __restrict nodes,
                            uint32_t                           nodeIndex,
                            uint32_t                           i0,
                            uint32_t                           i1,
                            uint32_t                           i2) {
  const AK3MFPaintNode *node;
  uint32_t              verts[3];
  uint32_t              r0;
  uint32_t              r1;
  uint32_t              r2;
  uint32_t              childCount;
  uint32_t              children[4];
  uint32_t              i;

  if (!sub->ok)
    return;

  node = &nodes[nodeIndex];
  if (node->splitSides == 0u) {
    ak_3mf_paint_subdivider_emit(sub, node->state, i0, i1, i2);
    return;
  }

  childCount = (uint32_t)node->splitSides + 1u;
  for (i = 0u; i < childCount; i++)
    children[i] = node->children[childCount - 1u - i];

  verts[0] = i0;
  verts[1] = i1;
  verts[2] = i2;
  r0 = verts[node->specialSide % 3u];
  r1 = verts[(node->specialSide + 1u) % 3u];
  r2 = verts[(node->specialSide + 2u) % 3u];

  if (node->splitSides == 1u) {
    uint32_t m;

    m = ak_3mf_paint_subdivider_midpoint(sub, r1, r2);
    ak_3mf_paint_subdivide_node(sub, nodes, children[0], r0, r1, m);
    ak_3mf_paint_subdivide_node(sub, nodes, children[1], m, r2, r0);
  } else if (node->splitSides == 2u) {
    uint32_t m01;
    uint32_t m20;

    m01 = ak_3mf_paint_subdivider_midpoint(sub, r0, r1);
    m20 = ak_3mf_paint_subdivider_midpoint(sub, r2, r0);
    ak_3mf_paint_subdivide_node(sub, nodes, children[0], r0, m01, m20);
    ak_3mf_paint_subdivide_node(sub, nodes, children[1], m01, r1, m20);
    ak_3mf_paint_subdivide_node(sub, nodes, children[2], r1, r2, m20);
  } else {
    uint32_t m01;
    uint32_t m12;
    uint32_t m20;

    m01 = ak_3mf_paint_subdivider_midpoint(sub, r0, r1);
    m12 = ak_3mf_paint_subdivider_midpoint(sub, r1, r2);
    m20 = ak_3mf_paint_subdivider_midpoint(sub, r2, r0);
    ak_3mf_paint_subdivide_node(sub, nodes, children[0], r0, m01, m20);
    ak_3mf_paint_subdivide_node(sub, nodes, children[1], m01, r1, m12);
    ak_3mf_paint_subdivide_node(sub, nodes, children[2], m12, r2, m20);
    ak_3mf_paint_subdivide_node(sub, nodes, children[3], m01, m12, m20);
  }
}

static
bool
ak_3mf_fast_paint_process_segmentation(AK3MFPaintPlan       * __restrict plan,
                                       AK3MFPaintBucketFill * __restrict fill,
                                       const AK3MFFastSlice * __restrict paint,
                                       const uint32_t                     v[3],
                                       float                * __restrict positions,
                                       size_t               * __restrict vertexCursor,
                                       size_t                             outputVertexCount) {
  AK3MFPaintTree       tree;
  AK3MFPaintSubdivider sub;
  uint32_t             rootIndex;
  bool                 ok;

  if (!ak_3mf_paint_tree_decode(&tree, paint, &rootIndex))
    return ak_3mf_fast_paint_emit_state(plan, fill, 0u, v);

  ak_3mf_paint_subdivider_init(&sub,
                               plan,
                               fill,
                               positions,
                               vertexCursor,
                               outputVertexCount,
                               v);
  ak_3mf_paint_subdivide_node(&sub, tree.nodes, rootIndex, 0u, 1u, 2u);
  if (!fill)
    plan->extraVertexCount += sub.localCount - 3u;
  ok = sub.ok;
  ak_3mf_paint_subdivider_dispose(&sub);
  ak_3mf_paint_tree_dispose(&tree);
  return ok;
}

static
bool
ak_3mf_fast_paint_analyze(AK3MFPaintPlan       * __restrict plan,
                          AK3MFPaintKind                     paintKind,
                          const AK3MFFastSlice * __restrict paint,
                          const uint32_t                     v[3],
                          uint8_t              * __restrict stateOut) {
  uint32_t state;

  if (paintKind == AK_3MF_PAINT_NONE) {
    *stateOut = 0u;
    return ak_3mf_fast_paint_emit_state(plan, NULL, 0u, v);
  }

  if (paintKind == AK_3MF_PAINT_ORCA
      && ak_3mf_fast_orca_paint_state(paint, &state)
      && state < AK_3MF_PAINT_STATE_SEGMENTED) {
    *stateOut = (uint8_t)state;
    return ak_3mf_fast_paint_emit_state(plan, NULL, state, v);
  }

  *stateOut = AK_3MF_PAINT_STATE_SEGMENTED;
  return ak_3mf_fast_paint_process_segmentation(plan,
                                                NULL,
                                                paint,
                                                v,
                                                NULL,
                                                NULL,
                                                0u);
}

static
bool
ak_3mf_fast_fill_indices(const AK3MFFastSlice * __restrict triangles,
                         AkIndexArray         * __restrict indices,
                         uint32_t                          vertexCount,
                         size_t                            triangleCount) {
  const char  *cursor;
  AK3MFFastTag tag;
  size_t       i;

  cursor = triangles->begin;
  i      = 0u;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *dst;

      dst = (uint8_t *)indices->items;
      while (ak_3mf_fast_next_tag(&cursor, triangles->end, &tag)) {
        uint32_t v[3];

        if (tag.closing
            || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                            _s_ak_triangle_u64_exact,
                                            _s_ak_triangle_len))
          continue;
        if (i >= triangleCount * 3u)
          return false;
        if (!ak_3mf_fast_parse_triangle(tag.full.begin,
                                        tag.full.end,
                                        vertexCount,
                                        v))
          return false;
        dst[i++] = (uint8_t)v[0];
        dst[i++] = (uint8_t)v[1];
        dst[i++] = (uint8_t)v[2];
      }
      break;
    }
    case AKT_USHORT: {
      uint16_t *dst;

      dst = (uint16_t *)indices->items;
      while (ak_3mf_fast_next_tag(&cursor, triangles->end, &tag)) {
        uint32_t v[3];

        if (tag.closing
            || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                            _s_ak_triangle_u64_exact,
                                            _s_ak_triangle_len))
          continue;
        if (i >= triangleCount * 3u)
          return false;
        if (!ak_3mf_fast_parse_triangle(tag.full.begin,
                                        tag.full.end,
                                        vertexCount,
                                        v))
          return false;
        dst[i++] = (uint16_t)v[0];
        dst[i++] = (uint16_t)v[1];
        dst[i++] = (uint16_t)v[2];
      }
      break;
    }
    case AKT_UINT: {
      uint32_t *dst;

      dst = (uint32_t *)indices->items;
      while (ak_3mf_fast_next_tag(&cursor, triangles->end, &tag)) {
        uint32_t v[3];

        if (tag.closing
            || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                            _s_ak_triangle_u64_exact,
                                            _s_ak_triangle_len))
          continue;
        if (i >= triangleCount * 3u)
          return false;
        if (!ak_3mf_fast_parse_triangle(tag.full.begin,
                                        tag.full.end,
                                        vertexCount,
                                        v))
          return false;
        dst[i++] = v[0];
        dst[i++] = v[1];
        dst[i++] = v[2];
      }
      break;
    }
    default:
      return false;
  }

  indices->max = vertexCount > 0u ? vertexCount - 1u : 0u;
  return i == triangleCount * 3u;
}

static
bool
ak_3mf_fast_analyze_paint(AK3MFImportState          * __restrict st,
                          const AK3MFFastMeshSlices * __restrict slices,
                          AK3MFPaintPlan            * __restrict plan) {
  const char  *cursor;
  AK3MFFastTag tag;
  size_t       triangleIndex;

  memset(plan, 0, sizeof(*plan));
  plan->defaultExtruder = ak_3mf_bambu_orca_extruder_for_object(st, slices->objectId);
  plan->triangles = malloc(sizeof(*plan->triangles) * slices->triangleCount);
  if (!plan->triangles)
    return false;

  cursor = slices->triangles.begin;
  triangleIndex = 0u;
  while (ak_3mf_fast_next_tag(&cursor, slices->triangles.end, &tag)) {
    AK3MFFastSlice paint;
    AK3MFPaintKind paintKind;
    AK3MFPaintTriangle *cached;
    uint32_t       v[3];
    uint8_t        state;

    if (tag.closing
        || !ak_3mf_fast_slice_eq_packed(&tag.localName,
                                        _s_ak_triangle_u64_exact,
                                        _s_ak_triangle_len))
      continue;
    if (triangleIndex >= slices->triangleCount)
      return false;

    if (!ak_3mf_fast_parse_triangle_paint(tag.full.begin,
                                          tag.full.end,
                                          (uint32_t)slices->vertexCount,
                                          v,
                                          &paint,
                                          &paintKind)
        || !ak_3mf_fast_paint_analyze(plan,
                                      paintKind,
                                      &paint,
                                      v,
                                      &state))
      return false;

    cached = &plan->triangles[triangleIndex++];
    cached->v[0] = v[0];
    cached->v[1] = v[1];
    cached->v[2] = v[2];
    cached->state = state;
    cached->paintOffset = 0u;
    cached->paintSize = 0u;
    if (state == AK_3MF_PAINT_STATE_SEGMENTED) {
      size_t paintOffset;
      size_t paintSize;

      paintOffset = (size_t)(paint.begin - slices->triangles.begin);
      paintSize   = (size_t)(paint.end - paint.begin);
      if (paintOffset > UINT32_MAX || paintSize > UINT32_MAX)
        return false;
      cached->paintOffset = (uint32_t)paintOffset;
      cached->paintSize   = (uint32_t)paintSize;
    }
  }

  return triangleIndex == slices->triangleCount
         && plan->outputTriangleCount > 0u
         && plan->outputTriangleCount <= UINT32_MAX;
}

static
bool
ak_3mf_fast_append_painted_primitive(AK3MFImportState        * __restrict st,
                                     AkMesh                  * __restrict mesh,
                                     AkAccessor              * __restrict posAcc,
                                     AK3MFPaintBucketFill    * __restrict fill,
                                     AkMeshPrimitive        ** __restrict lastPrim,
                                     uint32_t                              objectId,
                                     uint32_t                              bucket,
                                     size_t                                triangleCount,
                                     AkUInt                                maxIndex,
                                     AkTypeId                              indexComponentType) {
  AkHeap          *heap;
  AkTriangles     *tri;
  AkMeshPrimitive *prim;
  AkIndexArray    *indices;

  if (triangleCount == 0u)
    return true;
  if (triangleCount > UINT32_MAX || triangleCount > SIZE_MAX / 3u)
    return false;

  heap = ak_heap_getheap(st->doc);
  tri  = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*tri));
  if (!tri)
    return false;

  tri->mode         = AK_TRIANGLES;
  tri->base.type    = AK_PRIMITIVE_TRIANGLES;
  prim              = (AkMeshPrimitive *)tri;
  prim->mesh        = mesh;
  prim->nPolygons   = (uint32_t)triangleCount;
  prim->indexStride = 1u;
  prim->material    = bucket == 0u
                      ? ak_3mf_bambu_orca_material_for_object(st, objectId)
                      : ak_3mf_bambu_orca_material_for_extruder(st, bucket);
  prim->pos         = io_input(heap,
                               prim,
                               posAcc,
                               AK_INPUT_POSITION,
                               _s_POSITION,
                               0u);
  if (!prim->pos)
    return false;

  indices = ak_indexArrayAlloc(heap,
                               prim,
                               triangleCount * 3u,
                               indexComponentType);
  if (!indices)
    return false;

  indices->max        = maxIndex;
  prim->indices       = indices;
  fill->items[bucket] = indices->items;

  prim->next = NULL;
  if (*lastPrim)
    (*lastPrim)->next = prim;
  else
    mesh->primitive = prim;
  *lastPrim = prim;
  mesh->primitiveCount++;
  return true;
}

static
bool
ak_3mf_fast_fill_paint_indices(const AK3MFFastMeshSlices * __restrict slices,
                               AK3MFPaintPlan            * __restrict plan,
                               AK3MFPaintBucketFill      * __restrict fill,
                               float                      * __restrict positions,
                               size_t                                  outputVertexCount) {
  size_t   vertexCursor;
  size_t   triangleIndex;
  uint32_t bucket;

  vertexCursor = slices->vertexCount;
  for (triangleIndex = 0u;
       triangleIndex < slices->triangleCount;
       triangleIndex++) {
    AK3MFPaintTriangle *cached;

    cached = &plan->triangles[triangleIndex];
    if (cached->state != AK_3MF_PAINT_STATE_SEGMENTED) {
      if (!ak_3mf_fast_paint_emit_state(plan, fill, cached->state, cached->v))
        return false;
      continue;
    }

    {
      AK3MFFastSlice paint;

      paint.begin = slices->triangles.begin + cached->paintOffset;
      paint.end   = paint.begin + cached->paintSize;
      if (!ak_3mf_fast_paint_process_segmentation(plan,
                                                  fill,
                                                  &paint,
                                                  cached->v,
                                                  positions,
                                                  &vertexCursor,
                                                  outputVertexCount))
        return false;
    }
  }

  if (vertexCursor != outputVertexCount)
    return false;

  for (bucket = 0u; bucket < AK_3MF_PAINT_BUCKET_COUNT; bucket++) {
    if (fill->offsets[bucket] != plan->counts[bucket] * 3u)
      return false;
  }

  return true;
}

static
AkGeometry*
ak_3mf_fast_create_painted_mesh(AK3MFImportState             * __restrict st,
                                const AK3MFFastMeshSlices    * __restrict slices) {
  AkDoc                 *doc;
  AkHeap                *heap;
  AkGeometry            *geom;
  AkMesh                *mesh;
  AkBuffer              *posBuff;
  AkAccessor            *posAcc;
  AkMeshPrimitive       *lastPrim;
  AK3MFPaintPlan         plan;
  AK3MFPaintBucketFill   fill;
  float                 *positions;
  size_t                 outputVertexCount;
  uint32_t               bucket;
  AkUInt                 maxIndex;
  AkTypeId               indexComponentType;
  bool                   ok;

  memset(&plan, 0, sizeof(plan));
  if (!ak_3mf_fast_analyze_paint(st, slices, &plan)) {
    free(plan.triangles);
    return NULL;
  }

  ok = false;
  if (slices->vertexCount > SIZE_MAX - plan.extraVertexCount)
    goto cleanup;
  outputVertexCount = slices->vertexCount + plan.extraVertexCount;
  if (outputVertexCount == 0u
      || outputVertexCount > UINT32_MAX
      || outputVertexCount > SIZE_MAX / (sizeof(float) * 3u))
    goto cleanup;

  doc  = st->doc;
  heap = ak_heap_getheap(doc);
  if (!heap)
    goto cleanup;

  mesh = ak_allocMeshEx(heap, doc, &geom, true);
  if (!mesh || !geom)
    goto cleanup;

  posBuff = ak_heap_calloc(heap, doc, sizeof(*posBuff));
  if (!posBuff)
    goto cleanup;
  posBuff->length = outputVertexCount * sizeof(float) * 3u;
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  if (!posBuff->data)
    goto cleanup;
  positions = posBuff->data;

  if (!ak_3mf_fast_fill_positions(&slices->vertices,
                                  positions,
                                  slices->vertexCount))
    goto cleanup;

  AK_LIB_PREPEND(doc->lib.buffers, posBuff, next);

  posAcc = io_acc(heap,
                  doc,
                  AK_COMPONENT_SIZE_VEC3,
                  AKT_FLOAT,
                  (uint32_t)outputVertexCount,
                  posBuff);
  if (!posAcc)
    goto cleanup;
  AK_LIB_PREPEND(doc->lib.accessors, posAcc, next);

  memset(&fill, 0, sizeof(fill));
  maxIndex            = (AkUInt)(outputVertexCount - 1u);
  indexComponentType  = ak_indexComponentTypeForMax(maxIndex);
  fill.componentType  = indexComponentType;
  lastPrim            = NULL;

  for (bucket = 0u; bucket < AK_3MF_PAINT_BUCKET_COUNT; bucket++) {
    if (!ak_3mf_fast_append_painted_primitive(st,
                                              mesh,
                                              posAcc,
                                              &fill,
                                              &lastPrim,
                                              slices->objectId,
                                              bucket,
                                              plan.counts[bucket],
                                              maxIndex,
                                              indexComponentType))
      goto cleanup;
  }

  if (mesh->primitiveCount == 0u)
    goto cleanup;

  if (!ak_3mf_fast_fill_paint_indices(slices,
                                      &plan,
                                      &fill,
                                      positions,
                                      outputVertexCount))
    goto cleanup;

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);
  ok = true;

cleanup:
  free(plan.triangles);
  if (!ok)
    return NULL;
  return geom;
}

static
AkGeometry*
ak_3mf_fast_create_mesh(AK3MFImportState             * __restrict st,
                        const AK3MFFastMeshSlices    * __restrict slices) {
  AkDoc           *doc;
  AkHeap          *heap;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  AkMeshPrimitive *prim;
  AkBuffer        *posBuff;
  AkAccessor      *posAcc;
  AkIndexArray    *indices;
  float           *positions;
  size_t           indexCount;

  if (slices->hasPaint)
    return ak_3mf_fast_create_painted_mesh(st, slices);

  doc  = st->doc;
  heap = ak_heap_getheap(doc);
  if (!heap)
    return NULL;

  mesh = ak_allocMeshEx(heap, doc, &geom, true);
  if (!mesh || !geom)
    return NULL;

  posBuff = ak_heap_calloc(heap, doc, sizeof(*posBuff));
  if (!posBuff)
    return NULL;
  if (slices->vertexCount > SIZE_MAX / (sizeof(float) * 3u))
    return NULL;
  posBuff->length = slices->vertexCount * sizeof(float) * 3u;
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  if (!posBuff->data)
    return NULL;
  positions = posBuff->data;

  if (!ak_3mf_fast_fill_positions(&slices->vertices,
                                  positions,
                                  slices->vertexCount))
    return NULL;

  AK_LIB_PREPEND(doc->lib.buffers, posBuff, next);

  posAcc = io_acc(heap,
                  doc,
                  AK_COMPONENT_SIZE_VEC3,
                  AKT_FLOAT,
                  (uint32_t)slices->vertexCount,
                  posBuff);
  if (!posAcc)
    return NULL;
  AK_LIB_PREPEND(doc->lib.accessors, posAcc, next);

  tri              = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*tri));
  if (!tri)
    return NULL;
  tri->mode        = AK_TRIANGLES;
  tri->base.type   = AK_PRIMITIVE_TRIANGLES;
  prim             = (AkMeshPrimitive *)tri;
  prim->mesh       = mesh;
  prim->nPolygons  = (uint32_t)slices->triangleCount;
  prim->indexStride = 1u;
  mesh->primitive  = prim;
  mesh->primitiveCount = 1u;

  prim->pos = io_input(heap,
                       prim,
                       posAcc,
                       AK_INPUT_POSITION,
                       _s_POSITION,
                       0u);
  if (!prim->pos)
    return NULL;

  indexCount = slices->triangleCount * 3u;
  indices    = ak_indexArrayAlloc(heap,
                                  prim,
                                  indexCount,
                                  ak_indexComponentTypeForMax((AkUInt)slices->vertexCount - 1u));
  if (!indices)
    return NULL;
  if (!ak_3mf_fast_fill_indices(&slices->triangles,
                                indices,
                                (uint32_t)slices->vertexCount,
                                slices->triangleCount))
    return NULL;

  prim->indices  = indices;
  prim->material = ak_3mf_bambu_orca_material_for_object(st, slices->objectId);

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);
  return geom;
}

AK_HIDE
AK3MFFastLoadResult
ak_3mf_fast_load_mesh_model_part(AK3MFImportState * __restrict st,
                                 const char       * __restrict modelPath,
                                 const char       * __restrict modelData,
                                 size_t                         modelSize) {
  AK3MFFastMeshSlices stackSlices[8];
  AK3MFFastMeshSlices *slices;
  const char          *p;
  const char          *end;
  const char          *savedModelPath;
  const char          *storedModelPath;
  size_t               sliceCount;
  size_t               sliceCapacity;
  size_t               i;

  if (!st || !st->doc || !modelPath || !modelData || modelSize == 0u)
    return AK_3MF_FAST_LOAD_UNSUPPORTED;

  p             = modelData;
  end           = modelData + modelSize;
  slices        = stackSlices;
  sliceCount    = 0u;
  sliceCapacity = AK_ARRAY_LEN(stackSlices);

  for (;;) {
    AK3MFFastSlice objectTag;
    AK3MFFastSlice objectBody;
    AK3MFFastMeshSlices nextSlice;

    if (!ak_3mf_fast_next_object(&p, end, &objectTag, &objectBody))
      break;

    if (!ak_3mf_fast_read_object_slices(&objectTag, &objectBody, &nextSlice)) {
      if (slices != stackSlices)
        free(slices);
      return AK_3MF_FAST_LOAD_UNSUPPORTED;
    }

    if (sliceCount == sliceCapacity) {
      AK3MFFastMeshSlices *newSlices;
      size_t               newCapacity;

      if (sliceCapacity > SIZE_MAX / 2u) {
        if (slices != stackSlices)
          free(slices);
        return AK_3MF_FAST_LOAD_ERROR;
      }

      newCapacity = sliceCapacity * 2u;
      if (slices == stackSlices) {
        newSlices = malloc(sizeof(*newSlices) * newCapacity);
        if (!newSlices)
          return AK_3MF_FAST_LOAD_ERROR;
        memcpy(newSlices, stackSlices, sizeof(stackSlices));
      } else {
        newSlices = realloc(slices, sizeof(*newSlices) * newCapacity);
        if (!newSlices) {
          free(slices);
          return AK_3MF_FAST_LOAD_ERROR;
        }
      }

      slices        = newSlices;
      sliceCapacity = newCapacity;
    }

    slices[sliceCount++] = nextSlice;
  }

  if (sliceCount == 0u)
    return AK_3MF_FAST_LOAD_UNSUPPORTED;

  if (!ak_3mf_fast_reserve_objects(st, sliceCount)) {
    if (slices != stackSlices)
      free(slices);
    return AK_3MF_FAST_LOAD_ERROR;
  }

  storedModelPath = ak_3mf_fast_dup_model_path(st, modelPath);
  if (!storedModelPath) {
    if (slices != stackSlices)
      free(slices);
    return AK_3MF_FAST_LOAD_ERROR;
  }

  savedModelPath       = st->currentModelPath;
  st->currentModelPath = storedModelPath;

  for (i = 0u; i < sliceCount; i++) {
    AK3MFFastMeshSlices *slice;
    AK3MFObject         *object;

    slice        = &slices[i];
    object       = &st->objects[st->objectCount];
    object->path = storedModelPath;
    object->id   = slice->objectId;
    object->kind = AK_3MF_OBJECT_MESH;
    object->geom = ak_3mf_fast_create_mesh(st, slice);
    if (!object->geom
        || !ak_3mf_fast_add_production_object(st,
                                              &slice->objectTag,
                                              object->id)) {
      st->currentModelPath = savedModelPath;
      if (slices != stackSlices)
        free(slices);
      return AK_3MF_FAST_LOAD_ERROR;
    }

    if (st->print)
      st->print->meshObjectCount++;
    st->objectCount++;
  }

  if (st->print)
    st->print->objectCount = (uint32_t)st->objectCount;

  st->currentModelPath = savedModelPath;
  if (slices != stackSlices)
    free(slices);
  return AK_3MF_FAST_LOAD_LOADED;
}
