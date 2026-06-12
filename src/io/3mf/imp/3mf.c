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
#include "../../../strpool.h"
#include "../../../id.h"
#include "../../../../include/ak/path.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

typedef struct AK3MFObject {
  uint32_t    id;
  AkGeometry *geom;
} AK3MFObject;

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
  return xml_tag_eqsz(xml, tag, strlen(tag));
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

    type = xmla(rel, "Type");
    if (type && !ak_3mf_attr_contains(type, "3dmodel"))
      continue;

    target = ak_3mf_strdup_attr_path(xmla(rel, "Target"));
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
ak_3mf_doc_new(const char * __restrict filepath) {
  AkHeap *heap;
  AkDoc  *doc;

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

  doc->inf->base.coordSys    = AK_ZUP;
  doc->inf->base.unit        = ak_heap_calloc(heap, doc->inf, sizeof(*doc->inf->base.unit));
  doc->inf->base.unit->dist  = 0.001;
  doc->inf->base.unit->name  = ak_heap_strdup(heap, doc->inf->base.unit, "millimeter");
  doc->unit                  = doc->inf->base.unit;

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
AkGeometry*
ak_3mf_parse_mesh(AkDoc * __restrict doc, xml_t * __restrict meshXml) {
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
  float           *positions;
  size_t           vertexCount;
  size_t           triangleCount;
  size_t           i;
  AkUInt           maxIndex;

  if (!doc || !meshXml)
    return NULL;

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

  heap = ak_heap_getheap(doc);
  mesh = ak_allocMesh(heap, doc, &geom);
  if (!mesh || !geom)
    return NULL;

  posBuff         = ak_heap_calloc(heap, doc, sizeof(*posBuff));
  if (!posBuff)
    return NULL;

  posBuff->length = vertexCount * sizeof(float) * 3u;
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  positions       = posBuff->data;
  if (!posBuff->data)
    return NULL;

  i = 0;
  for (vertexXml = verticesXml->val; vertexXml; vertexXml = vertexXml->next) {
    if (!ak_3mf_tag(vertexXml, "vertex"))
      continue;

    positions[i * 3u + 0u] = xmla_float(xmla(vertexXml, "x"), 0.0f);
    positions[i * 3u + 1u] = xmla_float(xmla(vertexXml, "y"), 0.0f);
    positions[i * 3u + 2u] = xmla_float(xmla(vertexXml, "z"), 0.0f);
    i++;
  }

  AK_LIB_PREPEND(doc->lib.buffers, posBuff, next);

  posAcc = io_acc(heap,
                  doc,
                  AK_COMPONENT_SIZE_VEC3,
                  AKT_FLOAT,
                  (uint32_t)vertexCount,
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

  maxIndex = (AkUInt)(vertexCount - 1u);
  indices  = ak_indexArrayAlloc(heap,
                                prim,
                                triangleCount * 3u,
                                ak_indexComponentTypeForMax(maxIndex));
  if (!indices)
    return NULL;

  indices->max = maxIndex;
  i = 0;
  for (triangleXml = trianglesXml->val;
       triangleXml;
       triangleXml = triangleXml->next) {
    AkUInt v[3];

    if (!ak_3mf_tag(triangleXml, "triangle"))
      continue;

    v[0] = xmla_u32(xmla(triangleXml, "v1"), 0u);
    v[1] = xmla_u32(xmla(triangleXml, "v2"), 0u);
    v[2] = xmla_u32(xmla(triangleXml, "v3"), 0u);
    if (v[0] > maxIndex || v[1] > maxIndex || v[2] > maxIndex)
      return NULL;

    if (!ak_indexArraySet(heap, prim, &indices, i++, v[0])
        || !ak_indexArraySet(heap, prim, &indices, i++, v[1])
        || !ak_indexArraySet(heap, prim, &indices, i++, v[2]))
      return NULL;
  }

  prim->indices = indices;

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);
  return geom;
}

static
size_t
ak_3mf_count_mesh_objects(xml_t * __restrict resourcesXml) {
  xml_t *objXml;
  size_t count;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    if (ak_3mf_tag(objXml, "object") && xml_elem(objXml, "mesh"))
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
size_t
ak_3mf_parse_resources(AkDoc        * __restrict doc,
                       xml_t        * __restrict resourcesXml,
                       AK3MFObject  * __restrict objects,
                       size_t                    capacity) {
  xml_t  *objXml;
  size_t  count;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    xml_t      *meshXml;
    AkGeometry *geom;

    if (!ak_3mf_tag(objXml, "object"))
      continue;
    meshXml = xml_elem(objXml, "mesh");
    if (!meshXml)
      continue;

    if (count >= capacity)
      break;

    geom = ak_3mf_parse_mesh(doc, meshXml);
    if (!geom)
      continue;

    objects[count].id   = xmla_u32(xmla(objXml, "id"), (uint32_t)(count + 1u));
    objects[count].geom = geom;
    count++;
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
bool
ak_3mf_attach_build_items(AkDoc       * __restrict doc,
                          AkScene     * __restrict scene,
                          xml_t       * __restrict buildXml,
                          AK3MFObject * __restrict objects,
                          size_t                   objectCount) {
  xml_t   *itemXml;
  size_t   attached;

  attached = 0;
  if (!buildXml)
    return false;

  for (itemXml = buildXml->val; itemXml; itemXml = itemXml->next) {
    AK3MFObject *object;
    AkNode      *node;
    uint32_t     objectId;
    float        matrix[16];
    char         name[48];

    if (!ak_3mf_tag(itemXml, "item"))
      continue;

    objectId = xmla_u32(xmla(itemXml, "objectid"), 0u);
    object   = ak_3mf_find_object(objects, objectCount, objectId);
    if (!object || !object->geom)
      continue;

    snprintf(name, sizeof(name), "3MF Object %u", objectId);
    node = ak_3mf_node_new(doc, scene->node, name);
    if (!node)
      return false;

    if (!ak_3mf_parse_transform_attr(xmla(itemXml, "transform"), matrix))
      return false;
    ak_nodeSetTransformMatrix(node, matrix);

    if (!ak_nodeAttachGeometry(node, object->geom))
      return false;

    attached++;
  }

  return attached > 0;
}

static
bool
ak_3mf_attach_resource_fallback(AkDoc       * __restrict doc,
                                AkScene     * __restrict scene,
                                AK3MFObject * __restrict objects,
                                size_t                   objectCount) {
  size_t i;

  for (i = 0; i < objectCount; i++) {
    AkNode *node;
    char    name[48];

    if (!objects[i].geom)
      continue;

    snprintf(name, sizeof(name), "3MF Object %u", objects[i].id);
    node = ak_3mf_node_new(doc, scene->node, name);
    if (!node)
      return false;
    if (!ak_nodeAttachGeometry(node, objects[i].geom))
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
  AK3MFObject *objects;
  size_t       objectCapacity;
  size_t       objectCount;
  AkResult     result;

  if (!dest || !filepath)
    return AK_ERR;

  *dest     = NULL;
  modelData = NULL;
  modelSize = 0;
  xdoc      = NULL;
  objects   = NULL;

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

  doc = ak_3mf_doc_new(filepath);
  if (!doc) {
    result = AK_ERR;
    goto cleanup;
  }

  scene = ak_3mf_scene_new(doc);
  if (!scene) {
    result = AK_ERR;
    goto cleanup;
  }

  resourcesXml   = xml_elem(root, "resources");
  buildXml       = xml_elem(root, "build");
  objectCapacity = ak_3mf_count_mesh_objects(resourcesXml);
  if (objectCapacity > 0) {
    objects = calloc(objectCapacity, sizeof(*objects));
    if (!objects) {
      result = AK_ERR;
      goto cleanup;
    }
  }

  objectCount = ak_3mf_parse_resources(doc,
                                       resourcesXml,
                                       objects,
                                       objectCapacity);
  if (objectCount > 0
      && !ak_3mf_attach_build_items(doc,
                                    scene,
                                    buildXml,
                                    objects,
                                    objectCount)) {
    if (!ak_3mf_attach_resource_fallback(doc, scene, objects, objectCount)) {
      result = AK_ERR;
      goto cleanup;
    }
  }

  *dest  = doc;
  result = AK_OK;

cleanup:
  free(objects);
  if (xdoc)
    xml_free(xdoc);
  free(modelData);
  return result;
}
