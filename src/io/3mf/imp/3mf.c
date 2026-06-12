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

#include "3mf.h"
#include "../../common/util.h"
#include "../../common/zip.h"
#include "../../../mat/internal.h"
#include "../../../strpool.h"
#include "../../../id.h"
#include "../../../../include/ak/path.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define AK_3MF_XMLA(XML, NAME) xmla_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)

typedef enum AK3MFObjectKind {
  AK_3MF_OBJECT_EMPTY      = 0,
  AK_3MF_OBJECT_MESH       = 1,
  AK_3MF_OBJECT_COMPONENTS = 2
} AK3MFObjectKind;

typedef struct AK3MFComponent {
  uint32_t objectId;
  float    matrix[16];
} AK3MFComponent;

typedef struct AK3MFPropertyGroup {
  AkMaterialPropertySet *set;
  uint8_t *colors;
  uint32_t id;
  uint32_t count;
  bool     hasAlpha;
} AK3MFPropertyGroup;

typedef struct AK3MFObject {
  AK3MFComponent *components;
  uint32_t    id;
  AkGeometry *geom;
  const char *name;
  uint32_t    pid;
  uint32_t    pindex;
  uint32_t    componentCount;
  AK3MFObjectKind kind;
} AK3MFObject;

typedef struct AK3MFImportState {
  AkDoc               *doc;
  AK3MFObject         *objects;
  AK3MFPropertyGroup  *properties;
  size_t               objectCount;
  size_t               propertyCount;
} AK3MFImportState;

static
bool
ak_3mf_attr_contains(const xml_attr_t * __restrict attr,
                     const char       * __restrict needle) {
  size_t needleLen;
  size_t i;

  if (!attr || !attr->val || !needle)
    return false;

  needleLen = strlen(needle);
  if (needleLen == 0 || attr->valsize < needleLen)
    return false;

  for (i = 0; i <= (size_t)attr->valsize - needleLen; i++) {
    if (memcmp(attr->val + i, needle, needleLen) == 0)
      return true;
  }

  return false;
}

static
bool
ak_3mf_tag(const xml_t * __restrict xml, const char * __restrict tag) {
  const char *xmlTag;
  size_t     xmlTagSize;
  size_t     tagSize;
  size_t     i;

  if (!xml || !xml->tag || !tag)
    return false;

  xmlTag     = xml->tag;
  xmlTagSize = xml->tagsize;
  for (i = 0; i < xmlTagSize; i++) {
    if (xmlTag[i] == ':') {
      xmlTagSize -= i + 1u;
      xmlTag     += i + 1u;
      break;
    }
  }

  tagSize = strlen(tag);
  return tagSize == xmlTagSize && memcmp(xmlTag, tag, tagSize) == 0;
}

static
double
ak_3mf_unit_scale(const xml_attr_t * __restrict unitAttr,
                  const char      ** __restrict unitName) {
  if (!unitAttr || !unitAttr->val) {
    *unitName = "millimeter";
    return 0.001;
  }

  if (unitAttr->valsize == 6 && memcmp(unitAttr->val, "micron", 6) == 0) {
    *unitName = "micron";
    return 0.000001;
  }
  if (unitAttr->valsize == 10 && memcmp(unitAttr->val, "millimeter", 10) == 0) {
    *unitName = "millimeter";
    return 0.001;
  }
  if (unitAttr->valsize == 10 && memcmp(unitAttr->val, "centimeter", 10) == 0) {
    *unitName = "centimeter";
    return 0.01;
  }
  if (unitAttr->valsize == 4 && memcmp(unitAttr->val, "inch", 4) == 0) {
    *unitName = "inch";
    return 0.0254;
  }
  if (unitAttr->valsize == 4 && memcmp(unitAttr->val, "foot", 4) == 0) {
    *unitName = "foot";
    return 0.3048;
  }
  if (unitAttr->valsize == 5 && memcmp(unitAttr->val, "meter", 5) == 0) {
    *unitName = "meter";
    return 1.0;
  }

  *unitName = "millimeter";
  return 0.001;
}

static
char*
ak_3mf_strdup_attr_path(const xml_attr_t * __restrict attr) {
  const char *src;
  size_t      len;
  char       *dst;

  if (!attr || !attr->val || attr->valsize == 0)
    return NULL;

  src = attr->val;
  len = attr->valsize;
  while (len > 0 && (*src == '/' || *src == '\\')) {
    src++;
    len--;
  }

  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}

static
char*
ak_3mf_model_path_from_rels(const char * __restrict filepath) {
  static const char defaultPath[] = "3D/3dmodel.model";
  void            *relsData;
  size_t           relsSize;
  xml_doc_t       *xdoc;
  xml_t           *root;
  xml_t           *rel;
  char            *target;

  target   = NULL;
  relsData = NULL;
  relsSize = 0;
  if (ak_zip_extract_file(filepath, "_rels/.rels", &relsData, &relsSize) != AK_OK)
    goto fallback;

  xdoc = xml_parse(relsData, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root) {
    if (xdoc)
      xml_free(xdoc);
    free(relsData);
    goto fallback;
  }

  root = xdoc->root;
  for (rel = root->val; rel; rel = rel->next) {
    xml_attr_t *type;

    if (!ak_3mf_tag(rel, "Relationship"))
      continue;

    type = AK_3MF_XMLA(rel, Type);
    if (type && !ak_3mf_attr_contains(type, "3dmodel"))
      continue;

    target = ak_3mf_strdup_attr_path(AK_3MF_XMLA(rel, Target));
    if (target)
      break;
  }

  xml_free(xdoc);
  free(relsData);
  if (target)
    return target;

fallback:
  target = malloc(sizeof(defaultPath));
  if (target)
    memcpy(target, defaultPath, sizeof(defaultPath));
  return target;
}

static
AkDoc*
ak_3mf_doc_new(const char * __restrict filepath,
               xml_t      * __restrict root) {
  AkHeap *heap;
  AkDoc  *doc;
  const char *unitName;
  double      unitScale;

  heap = ak_heap_new(NULL, NULL, NULL);
  if (!heap)
    return NULL;

  doc = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!doc)
    return NULL;

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = ak_heap_strdup(heap, doc->inf, filepath);
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = false;
  doc->inf->ftype     = AK_FILE_TYPE_3MF;
  doc->coordSys       = AK_ZUP;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  unitScale = ak_3mf_unit_scale(root ? AK_3MF_XMLA(root, unit) : NULL, &unitName);

  doc->inf->base.coordSys   = AK_ZUP;
  doc->inf->base.unit       = ak_heap_calloc(heap, doc->inf, sizeof(*doc->inf->base.unit));
  doc->inf->base.unit->dist = unitScale;
  doc->inf->base.unit->name = ak_heap_strdup(heap, doc->inf->base.unit, unitName);
  doc->unit                 = doc->inf->base.unit;

  return doc;
}

static
AkScene*
ak_3mf_scene_new(AkDoc * __restrict doc) {
  AkHeap  *heap;
  AkScene *scene;
  AkNode  *root;

  heap  = ak_heap_getheap(doc);
  scene = ak_heap_calloc(heap, doc, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  if (!scene || !root)
    return NULL;

  ak_setypeid(root, AKT_NODE);
  root->visible = true;

  scene->node = root;
  doc->scene  = scene;
  AK_LIB_PREPEND(doc->lib.scenes, scene, next);

  return scene;
}

static
AkNode*
ak_3mf_node_new(AkDoc       * __restrict doc,
                AkNode      * __restrict parent,
                const char  * __restrict name) {
  AkHeap *heap;
  AkNode *node;

  heap = ak_heap_getheap(doc);
  node = ak_heap_calloc(heap, doc, sizeof(*node));
  if (!node)
    return NULL;

  ak_setypeid(node, AKT_NODE);
  node->visible = true;
  if (name)
    node->name = ak_heap_strdup(heap, node, name);

  AK_LIB_PREPEND(doc->lib.nodes, node, docNext);
  if (parent)
    ak_addSubNode(parent, node, false);

  return node;
}

static
size_t
ak_3mf_count_children(const xml_t * __restrict parent,
                      const char  * __restrict tag) {
  const xml_t *child;
  size_t       count;

  count = 0;
  if (!parent)
    return 0;

  for (child = parent->val; child; child = child->next) {
    if (ak_3mf_tag(child, tag))
      count++;
  }

  return count;
}

static
uint8_t
ak_3mf_hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f')
    return (uint8_t)(10 + c - 'a');
  if (c >= 'A' && c <= 'F')
    return (uint8_t)(10 + c - 'A');
  return 0xffu;
}

static
bool
ak_3mf_parse_color_attr(const xml_attr_t * __restrict attr,
                        uint8_t                       rgba[4]) {
  const char *p;
  uint16_t    len;
  uint32_t    i;

  rgba[0] = 255u;
  rgba[1] = 255u;
  rgba[2] = 255u;
  rgba[3] = 255u;

  if (!attr || !attr->val || attr->valsize == 0)
    return false;

  p   = attr->val;
  len = attr->valsize;
  if (len > 0 && *p == '#') {
    p++;
    len--;
  }

  if (len != 6u && len != 8u)
    return false;

  for (i = 0; i < len / 2u; i++) {
    uint8_t hi;
    uint8_t lo;

    hi = ak_3mf_hex_digit(p[i * 2u + 0u]);
    lo = ak_3mf_hex_digit(p[i * 2u + 1u]);
    if (hi == 0xffu || lo == 0xffu)
      return false;

    rgba[i] = (uint8_t)((hi << 4u) | lo);
  }

  return true;
}

static
AkMaterialInput*
ak_3mf_material_color_input(AkHeap     * __restrict heap,
                            void       * __restrict parent,
                            uint8_t                  rgba[4]) {
  AkMaterialInput *input;

  input = ak_heap_calloc(heap, parent, sizeof(*input));
  if (!input)
    return NULL;

  input->semantic    = _s_ak_baseColor;
  input->source      = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType   = AK_MATERIAL_VALUE_COLOR;
  input->channels    = AK_TEXTURE_CHANNEL_RGBA;
  input->colorSpace  = AK_TEXTURE_COLORSPACE_SRGB;
  input->color.rgba.R = rgba[0] / 255.0f;
  input->color.rgba.G = rgba[1] / 255.0f;
  input->color.rgba.B = rgba[2] / 255.0f;
  input->color.rgba.A = rgba[3] / 255.0f;

  return input;
}

static
AkMaterialInput*
ak_3mf_material_scalar_input(AkHeap      * __restrict heap,
                             void        * __restrict parent,
                             const char  * __restrict semantic,
                             float                    value) {
  AkMaterialInput *input;

  input = ak_heap_calloc(heap, parent, sizeof(*input));
  if (!input)
    return NULL;

  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_FLOAT;
  input->value[0]   = value;
  input->colorSpace = AK_TEXTURE_COLORSPACE_LINEAR;

  return input;
}

static
size_t
ak_3mf_count_property_groups(xml_t * __restrict resourcesXml) {
  xml_t *xml;
  size_t count;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (xml = resourcesXml->val; xml; xml = xml->next) {
    if (ak_3mf_tag(xml, "basematerials")
        || ak_3mf_tag(xml, "colorgroup"))
      count++;
  }

  return count;
}

static
AK3MFPropertyGroup*
ak_3mf_find_property_group(AK3MFImportState * __restrict st, uint32_t id) {
  size_t i;

  if (!st)
    return NULL;

  for (i = 0; i < st->propertyCount; i++) {
    if (st->properties[i].id == id)
      return &st->properties[i];
  }

  return NULL;
}

static
bool
ak_3mf_property_color(AK3MFImportState * __restrict st,
                      uint32_t                      id,
                      uint32_t                      index,
                      uint8_t                       rgba[4],
                      bool               * __restrict hasAlpha) {
  AK3MFPropertyGroup *group;

  group = ak_3mf_find_property_group(st, id);
  if (!group || index >= group->count)
    return false;

  memcpy(rgba, group->colors + (size_t)index * 4u, 4u);
  if (hasAlpha && rgba[3] < 255u)
    *hasAlpha = true;
  return true;
}

static
size_t
ak_3mf_parse_property_groups(AK3MFImportState * __restrict st,
                             xml_t            * __restrict resourcesXml) {
  xml_t  *xml;
  size_t  cursor;

  if (!st || !resourcesXml || !st->properties)
    return 0;

  cursor = 0;
  for (xml = resourcesXml->val; xml; xml = xml->next) {
    AK3MFPropertyGroup *group;
    AkMaterialPropertySet *set;
    AkHeap              *heap;
    const char         *childTag;
    xml_t              *child;
    size_t              colorCount;
    size_t              i;
    bool                baseMaterials;

    baseMaterials = ak_3mf_tag(xml, "basematerials");
    if (baseMaterials) {
      childTag = "base";
    } else if (ak_3mf_tag(xml, "colorgroup")) {
      childTag = "color";
    } else {
      continue;
    }

    colorCount = ak_3mf_count_children(xml, childTag);
    if (colorCount == 0 || colorCount > UINT32_MAX)
      continue;

    group         = &st->properties[cursor];
    group->id     = xmla_u32(AK_3MF_XMLA(xml, id), (uint32_t)(cursor + 1u));
    group->count  = (uint32_t)colorCount;
    group->colors = calloc(colorCount, 4u);
    if (!group->colors)
      continue;

    heap            = ak_heap_getheap(st->doc);
    set             = ak_heap_calloc(heap, st->doc, sizeof(*set));
    set->id         = group->id;
    set->count      = group->count;
    set->type       = baseMaterials ? AK_MATERIAL_PROPERTY_BASE : AK_MATERIAL_PROPERTY_COLOR;
    set->properties = ak_heap_calloc(heap,
                                     set,
                                     sizeof(*set->properties) * colorCount);
    set->next       = st->doc->materialProperties.sets;
    st->doc->materialProperties.sets = set;
    st->doc->materialProperties.count++;
    group->set      = set;

    i = 0;
    for (child = xml->val; child; child = child->next) {
      uint8_t rgba[4];

      if (!ak_3mf_tag(child, childTag))
        continue;

      if (!ak_3mf_parse_color_attr(baseMaterials
                                     ? AK_3MF_XMLA(child, displaycolor)
                                     : AK_3MF_XMLA(child, color),
                                   rgba)) {
        rgba[0] = 255u;
        rgba[1] = 255u;
        rgba[2] = 255u;
        rgba[3] = 255u;
      }

      if (rgba[3] < 255u)
        group->hasAlpha = true;

      memcpy(group->colors + i * 4u, rgba, 4u);
      if (set && set->properties) {
        AkMaterialProperty *prop;

        prop = &set->properties[i];
        prop->displayColor.rgba.R = rgba[0] / 255.0f;
        prop->displayColor.rgba.G = rgba[1] / 255.0f;
        prop->displayColor.rgba.B = rgba[2] / 255.0f;
        prop->displayColor.rgba.A = rgba[3] / 255.0f;
        prop->materialIndex       = (uint32_t)i;
        prop->baseColor           = ak_3mf_material_color_input(heap, set, rgba);
        prop->metallic            = ak_3mf_material_scalar_input(heap,
                                                                 set,
                                                                 _s_ak_metallic,
                                                                 0.0f);
        prop->roughness           = ak_3mf_material_scalar_input(heap,
                                                                 set,
                                                                 _s_ak_roughness,
                                                                 1.0f);
        if (baseMaterials)
          prop->name = xmla_strdup(AK_3MF_XMLA(child, name), heap, set);
      }
      i++;
    }
    cursor++;
  }

  return cursor;
}

static
AkGeometry*
ak_3mf_parse_mesh(AK3MFImportState * __restrict st,
                  xml_t            * __restrict objXml,
                  xml_t            * __restrict meshXml) {
  AkDoc           *doc;
  AkHeap          *heap;
  xml_t           *verticesXml;
  xml_t           *trianglesXml;
  xml_t           *vertexXml;
  xml_t           *triangleXml;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  AkMeshPrimitive *prim;
  AkBuffer        *posBuff;
  AkAccessor      *posAcc;
  AkIndexArray    *indices;
  uint8_t         *colors;
  float           *srcPositions;
  float           *positions;
  size_t           vertexCount;
  size_t           outputVertexCount;
  size_t           triangleCount;
  size_t           i;
  AkUInt           maxIndex;
  uint32_t         defaultPid;
  uint32_t         defaultPIndex;
  bool             hasColor;
  bool             hasAlpha;

  if (!st || !st->doc || !meshXml)
    return NULL;

  doc          = st->doc;
  verticesXml  = xml_elem(meshXml, "vertices");
  trianglesXml = xml_elem(meshXml, "triangles");
  vertexCount  = ak_3mf_count_children(verticesXml, "vertex");
  triangleCount = ak_3mf_count_children(trianglesXml, "triangle");
  if (vertexCount == 0
      || triangleCount == 0
      || vertexCount > UINT32_MAX
      || triangleCount > SIZE_MAX / 3u
      || triangleCount * 3u > UINT32_MAX)
    return NULL;

  srcPositions = malloc(vertexCount * sizeof(float) * 3u);
  if (!srcPositions)
    return NULL;

  i = 0;
  for (vertexXml = verticesXml->val; vertexXml; vertexXml = vertexXml->next) {
    if (!ak_3mf_tag(vertexXml, "vertex"))
      continue;

    srcPositions[i * 3u + 0u] = xmla_float(AK_3MF_XMLA(vertexXml, x), 0.0f);
    srcPositions[i * 3u + 1u] = xmla_float(AK_3MF_XMLA(vertexXml, y), 0.0f);
    srcPositions[i * 3u + 2u] = xmla_float(AK_3MF_XMLA(vertexXml, z), 0.0f);
    i++;
  }

  defaultPid    = xmla_u32(AK_3MF_XMLA(objXml, pid), 0u);
  defaultPIndex = xmla_u32(AK_3MF_XMLA(objXml, pindex), UINT32_MAX);
  hasColor      = false;
  hasAlpha      = false;
  for (triangleXml = trianglesXml->val;
       triangleXml;
       triangleXml = triangleXml->next) {
    uint32_t pid;
    uint32_t p1;

    if (!ak_3mf_tag(triangleXml, "triangle"))
      continue;

    pid = xmla_u32(AK_3MF_XMLA(triangleXml, pid), defaultPid);
    p1  = xmla_u32(AK_3MF_XMLA(triangleXml, p1), defaultPIndex);
    if (pid != 0u
        && p1 != UINT32_MAX
        && ak_3mf_find_property_group(st, pid)) {
      hasColor = true;
      break;
    }
  }
  if (!hasColor
      && defaultPid != 0u
      && defaultPIndex != UINT32_MAX
      && ak_3mf_find_property_group(st, defaultPid))
    hasColor = true;

  heap = ak_heap_getheap(doc);
  mesh = ak_allocMesh(heap, doc, &geom);
  if (!mesh || !geom) {
    free(srcPositions);
    return NULL;
  }

  outputVertexCount = hasColor ? triangleCount * 3u : vertexCount;

  posBuff         = ak_heap_calloc(heap, doc, sizeof(*posBuff));
  if (!posBuff) {
    free(srcPositions);
    return NULL;
  }

  posBuff->length = outputVertexCount * sizeof(float) * 3u;
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  positions       = posBuff->data;
  if (!posBuff->data) {
    free(srcPositions);
    return NULL;
  }

  colors = NULL;
  if (hasColor) {
    colors = ak_heap_alloc(heap, posBuff, outputVertexCount * 4u);
    if (!colors) {
      free(srcPositions);
      return NULL;
    }
  } else {
    memcpy(positions, srcPositions, posBuff->length);
  }

  AK_LIB_PREPEND(doc->lib.buffers, posBuff, next);

  posAcc = io_acc(heap,
                  doc,
                  AK_COMPONENT_SIZE_VEC3,
                  AKT_FLOAT,
                  (uint32_t)outputVertexCount,
                  posBuff);
  if (!posAcc)
    return NULL;
  AK_LIB_PREPEND(doc->lib.accessors, posAcc, next);

  tri            = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*tri));
  tri->mode      = AK_TRIANGLES;
  tri->base.type = AK_PRIMITIVE_TRIANGLES;
  prim           = (AkMeshPrimitive *)tri;
  prim->mesh     = mesh;
  prim->nPolygons = (uint32_t)triangleCount;
  prim->indexStride = 1u;
  mesh->primitive = prim;
  mesh->primitiveCount = 1u;

  prim->pos = io_input(heap,
                       prim,
                       posAcc,
                       AK_INPUT_POSITION,
                       _s_POSITION,
                       0u);

  maxIndex = (AkUInt)(outputVertexCount - 1u);
  indices  = NULL;
  if (!hasColor) {
    indices = ak_indexArrayAlloc(heap,
                                 prim,
                                 triangleCount * 3u,
                                 ak_indexComponentTypeForMax(maxIndex));
    if (!indices) {
      free(srcPositions);
      return NULL;
    }

    indices->max = maxIndex;
  }

  i = 0;
  for (triangleXml = trianglesXml->val;
       triangleXml;
       triangleXml = triangleXml->next) {
    AkUInt v[3];

    if (!ak_3mf_tag(triangleXml, "triangle"))
      continue;

    v[0] = xmla_u32(AK_3MF_XMLA(triangleXml, v1), 0u);
    v[1] = xmla_u32(AK_3MF_XMLA(triangleXml, v2), 0u);
    v[2] = xmla_u32(AK_3MF_XMLA(triangleXml, v3), 0u);
    if (v[0] >= vertexCount || v[1] >= vertexCount || v[2] >= vertexCount) {
      free(srcPositions);
      return NULL;
    }

    if (hasColor) {
      uint32_t pid;
      uint32_t p[3];
      uint32_t j;

      pid  = xmla_u32(AK_3MF_XMLA(triangleXml, pid), defaultPid);
      p[0] = xmla_u32(AK_3MF_XMLA(triangleXml, p1), defaultPIndex);
      p[1] = xmla_u32(AK_3MF_XMLA(triangleXml, p2), p[0]);
      p[2] = xmla_u32(AK_3MF_XMLA(triangleXml, p3), p[0]);

      for (j = 0; j < 3u; j++) {
        uint8_t rgba[4] = {255u, 255u, 255u, 255u};

        memcpy(positions + i * 3u,
               srcPositions + (size_t)v[j] * 3u,
               sizeof(float) * 3u);

        if (pid != 0u && p[j] != UINT32_MAX)
          (void)ak_3mf_property_color(st, pid, p[j], rgba, &hasAlpha);
        memcpy(colors + i * 4u, rgba, 4u);
        i++;
      }
    } else {
      if (!ak_indexArraySet(heap, prim, &indices, i++, v[0])
          || !ak_indexArraySet(heap, prim, &indices, i++, v[1])
          || !ak_indexArraySet(heap, prim, &indices, i++, v[2])) {
        free(srcPositions);
        return NULL;
      }
    }
  }

  free(srcPositions);

  if (hasColor) {
    AkBuffer   *colorBuff;
    AkAccessor *colorAcc;

    colorBuff         = ak_heap_calloc(heap, doc, sizeof(*colorBuff));
    colorBuff->length = outputVertexCount * 4u;
    colorBuff->data   = colors;
    AK_LIB_PREPEND(doc->lib.buffers, colorBuff, next);

    colorAcc = io_acc(heap,
                      doc,
                      AK_COMPONENT_SIZE_VEC4,
                      AKT_UBYTE,
                      (uint32_t)outputVertexCount,
                      colorBuff);
    if (!colorAcc)
      return NULL;
    colorAcc->bytesPerComponent = sizeof(uint8_t);
    colorAcc->byteStride        = 4u;
    colorAcc->fillByteSize      = 4u;
    AK_LIB_PREPEND(doc->lib.accessors, colorAcc, next);

    io_input(heap, prim, colorAcc, AK_INPUT_COLOR, _s_COLOR, 0u);
    prim->material = ak_materialDefaultVertexColorAlpha(doc, hasAlpha);
  } else {
    prim->indices = indices;
  }

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);
  return geom;
}

static
size_t
ak_3mf_count_resource_objects(xml_t * __restrict resourcesXml) {
  xml_t *objXml;
  size_t count;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    if (ak_3mf_tag(objXml, "object")
        && (xml_elem(objXml, "mesh") || xml_elem(objXml, "components")))
      count++;
  }

  return count;
}

static
AK3MFObject*
ak_3mf_find_object(AK3MFObject * __restrict objects,
                   size_t                   objectCount,
                   uint32_t                 id) {
  size_t i;

  for (i = 0; i < objectCount; i++) {
    if (objects[i].id == id)
      return &objects[i];
  }

  return NULL;
}

static
bool
ak_3mf_parse_transform_attr(const xml_attr_t * __restrict attr,
                            float                         matrix[16]);

static
bool
ak_3mf_parse_components(AK3MFImportState * __restrict st,
                        AK3MFObject      * __restrict object,
                        xml_t            * __restrict componentsXml) {
  xml_t   *componentXml;
  size_t   componentCount;
  size_t   i;

  if (!st || !object || !componentsXml)
    return false;

  componentCount = ak_3mf_count_children(componentsXml, "component");
  if (componentCount == 0 || componentCount > UINT32_MAX)
    return false;

  object->components = ak_heap_calloc(ak_heap_getheap(st->doc),
                                      st->doc,
                                      sizeof(*object->components) * componentCount);
  if (!object->components)
    return false;

  object->componentCount = (uint32_t)componentCount;
  object->kind           = AK_3MF_OBJECT_COMPONENTS;

  i = 0;
  for (componentXml = componentsXml->val;
       componentXml;
       componentXml = componentXml->next) {
    AK3MFComponent *component;

    if (!ak_3mf_tag(componentXml, "component"))
      continue;

    component           = &object->components[i++];
    component->objectId = xmla_u32(AK_3MF_XMLA(componentXml, objectid), 0u);
    if (!ak_3mf_parse_transform_attr(AK_3MF_XMLA(componentXml, transform),
                                     component->matrix))
      return false;
  }

  return true;
}

static
size_t
ak_3mf_parse_resources(AK3MFImportState * __restrict st,
                       xml_t            * __restrict resourcesXml) {
  xml_t  *objXml;
  size_t  count;

  count = 0;
  if (!st || !resourcesXml)
    return 0;

  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    AK3MFObject *object;
    xml_t      *meshXml;
    xml_t      *componentsXml;

    if (!ak_3mf_tag(objXml, "object"))
      continue;
    meshXml = xml_elem(objXml, "mesh");
    componentsXml = xml_elem(objXml, "components");
    if (!meshXml && !componentsXml)
      continue;

    if (count >= st->objectCount)
      break;

    object         = &st->objects[count];
    object->id     = xmla_u32(AK_3MF_XMLA(objXml, id), (uint32_t)(count + 1u));
    object->pid    = xmla_u32(AK_3MF_XMLA(objXml, pid), 0u);
    object->pindex = xmla_u32(AK_3MF_XMLA(objXml, pindex), UINT32_MAX);
    object->name   = xmla_strdup(AK_3MF_XMLA(objXml, name),
                                  ak_heap_getheap(st->doc),
                                  st->doc);

    if (meshXml) {
      object->geom = ak_3mf_parse_mesh(st, objXml, meshXml);
      if (!object->geom)
        continue;
      object->kind = AK_3MF_OBJECT_MESH;
      count++;
    } else if (componentsXml && ak_3mf_parse_components(st, object, componentsXml)) {
      count++;
    }
  }

  return count;
}

static
bool
ak_3mf_parse_transform_attr(const xml_attr_t * __restrict attr,
                            float                         matrix[16]) {
  char   *copy;
  char   *it;
  char   *end;
  float   values[12];
  size_t  i;

  matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
  matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
  matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;

  if (!attr || !attr->val || attr->valsize == 0)
    return true;

  copy = malloc((size_t)attr->valsize + 1u);
  if (!copy)
    return false;

  memcpy(copy, attr->val, attr->valsize);
  copy[attr->valsize] = '\0';

  it = copy;
  for (i = 0; i < 12u; i++) {
    errno     = 0;
    values[i] = strtof(it, &end);
    if (end == it || errno == ERANGE) {
      free(copy);
      return false;
    }
    it = end;
  }

  free(copy);

  matrix[0]  = values[0];
  matrix[4]  = values[1];
  matrix[8]  = values[2];
  matrix[1]  = values[3];
  matrix[5]  = values[4];
  matrix[9]  = values[5];
  matrix[2]  = values[6];
  matrix[6]  = values[7];
  matrix[10] = values[8];
  matrix[12] = values[9];
  matrix[13] = values[10];
  matrix[14] = values[11];

  return true;
}

static
void
ak_3mf_identity_matrix(float matrix[16]) {
  matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
  matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
  matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

static
bool
ak_3mf_attach_object_node(AK3MFImportState * __restrict st,
                          AkNode           * __restrict parent,
                          AK3MFObject      * __restrict object,
                          const float                   matrix[16],
                          const char       * __restrict fallbackName,
                          uint32_t                      depth) {
  AkNode   *node;
  char      name[64];
  uint32_t  i;

  if (!st || !parent || !object)
    return false;
  if (depth > 512u)
    return false;

  if (object->name) {
    fallbackName = object->name;
  } else if (!fallbackName) {
    snprintf(name, sizeof(name), "3MF Object %u", object->id);
    fallbackName = name;
  }

  node = ak_3mf_node_new(st->doc, parent, fallbackName);
  if (!node)
    return false;
  ak_nodeSetTransformMatrix(node, matrix);

  if (object->kind == AK_3MF_OBJECT_MESH) {
    return object->geom && ak_nodeAttachGeometry(node, object->geom);
  }

  if (object->kind != AK_3MF_OBJECT_COMPONENTS)
    return true;

  for (i = 0; i < object->componentCount; i++) {
    AK3MFComponent *component;
    AK3MFObject    *child;

    component = &object->components[i];
    child     = ak_3mf_find_object(st->objects,
                                   st->objectCount,
                                   component->objectId);
    if (!child)
      continue;

    if (!ak_3mf_attach_object_node(st,
                                   node,
                                   child,
                                   component->matrix,
                                   NULL,
                                   depth + 1u))
      return false;
  }

  return true;
}

static
bool
ak_3mf_attach_build_items(AK3MFImportState * __restrict st,
                          AkScene          * __restrict scene,
                          xml_t            * __restrict buildXml) {
  xml_t   *itemXml;
  size_t   attached;

  attached = 0;
  if (!st || !buildXml)
    return false;

  for (itemXml = buildXml->val; itemXml; itemXml = itemXml->next) {
    AK3MFObject *object;
    uint32_t     objectId;
    float        matrix[16];
    char         name[48];

    if (!ak_3mf_tag(itemXml, "item"))
      continue;

    objectId = xmla_u32(AK_3MF_XMLA(itemXml, objectid), 0u);
    object   = ak_3mf_find_object(st->objects, st->objectCount, objectId);
    if (!object)
      continue;

    snprintf(name, sizeof(name), "3MF Object %u", objectId);
    if (!ak_3mf_parse_transform_attr(AK_3MF_XMLA(itemXml, transform), matrix))
      return false;
    if (!ak_3mf_attach_object_node(st, scene->node, object, matrix, name, 0u))
      return false;

    attached++;
  }

  return attached > 0;
}

static
bool
ak_3mf_attach_resource_fallback(AK3MFImportState * __restrict st,
                                AkScene          * __restrict scene) {
  size_t i;
  float  matrix[16];

  ak_3mf_identity_matrix(matrix);
  for (i = 0; i < st->objectCount; i++) {
    char    name[48];

    snprintf(name, sizeof(name), "3MF Object %u", st->objects[i].id);
    if (!ak_3mf_attach_object_node(st,
                                   scene->node,
                                   &st->objects[i],
                                   matrix,
                                   name,
                                   0u))
      return false;
  }

  return true;
}

AK_HIDE
AkResult
imp_3mf(AkDoc ** __restrict dest, const char * __restrict filepath) {
  char        *modelPath;
  void        *modelData;
  size_t       modelSize;
  xml_doc_t   *xdoc;
  xml_t       *root;
  xml_t       *resourcesXml;
  xml_t       *buildXml;
  AkDoc       *doc;
  AkScene     *scene;
  AK3MFImportState st;
  size_t       propertyCapacity;
  AkResult     result;
  size_t       i;

  if (!dest || !filepath)
    return AK_ERR;

  *dest     = NULL;
  modelData = NULL;
  modelSize = 0;
  xdoc      = NULL;
  memset(&st, 0, sizeof(st));

  modelPath = ak_3mf_model_path_from_rels(filepath);
  if (!modelPath)
    return AK_ERR;

  result = ak_zip_extract_file(filepath, modelPath, &modelData, &modelSize);
  free(modelPath);
  if (result != AK_OK)
    return result;

  xdoc = xml_parse(modelData, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root) {
    result = AK_EBADF;
    goto cleanup;
  }

  root = xdoc->root;
  if (!ak_3mf_tag(root, "model")) {
    result = AK_EBADF;
    goto cleanup;
  }

  doc = ak_3mf_doc_new(filepath, root);
  if (!doc) {
    result = AK_ERR;
    goto cleanup;
  }
  st.doc = doc;

  scene = ak_3mf_scene_new(doc);
  if (!scene) {
    result = AK_ERR;
    goto cleanup;
  }

  resourcesXml   = xml_elem(root, "resources");
  buildXml       = xml_elem(root, "build");
  propertyCapacity = ak_3mf_count_property_groups(resourcesXml);
  if (propertyCapacity > 0) {
    st.properties = calloc(propertyCapacity, sizeof(*st.properties));
    if (!st.properties) {
      result = AK_ERR;
      goto cleanup;
    }
    st.propertyCount = ak_3mf_parse_property_groups(&st, resourcesXml);
  }

  st.objectCount = ak_3mf_count_resource_objects(resourcesXml);
  if (st.objectCount > 0) {
    st.objects = calloc(st.objectCount, sizeof(*st.objects));
    if (!st.objects) {
      result = AK_ERR;
      goto cleanup;
    }
  }

  st.objectCount = ak_3mf_parse_resources(&st, resourcesXml);
  if (st.objectCount > 0
      && !ak_3mf_attach_build_items(&st, scene, buildXml)) {
    if (!ak_3mf_attach_resource_fallback(&st, scene)) {
      result = AK_ERR;
      goto cleanup;
    }
  }

  *dest  = doc;
  result = AK_OK;

cleanup:
  if (st.properties) {
    for (i = 0; i < st.propertyCount; i++)
      free(st.properties[i].colors);
  }
  free(st.properties);
  free(st.objects);
  if (xdoc)
    xml_free(xdoc);
  free(modelData);
  return result;
}
