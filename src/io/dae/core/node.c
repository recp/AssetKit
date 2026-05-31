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

#include "node.h"
#include "enum.h"

#include "../../../array.h"
#include "../core/asset.h"
#include "../fx/mat.h"
#include "../fixup/node.h"

#include <cglm/cglm.h>

AK_HIDE
void*
dae_node2(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  return dae_node(dst, xml, memp, NULL);
}

AK_HIDE
AkNode*
dae_node(DAEState      * __restrict dst,
         xml_t         * __restrict xml,
         void          *            memp,
         AkVisualScene *            scene) {
  AkHeap      *heap;
  AkNode      *node;
  const xml_t *sval;
  xml_attr_t  *att;
  AkObject    *last_trans;

  heap = dst->heap;
  node = ak_heap_calloc(heap, memp, sizeof(*node));
  ak_setypeid(node, AKT_NODE);
  node->visible = true;

  xmla_setid(xml, heap, node);
  sid_set(xml, heap, node);
  
  node->name     = DAE_XMLA_STRDUP8(xml, heap, name, node);
  node->nodeType = dae_nodeType(DAE_XMLA4(xml, type));
  if (node->nodeType < 1)
    node->nodeType = AK_NODE_TYPE_NODE;

  if ((att = DAE_XMLA8(xml, layer))) {
    AkStringArray *strArray;
    AkResult       ret;
    char          *layer;

    layer = xmla_strdup(att, heap, node);
    ret   = layer
            ? ak_strtostr_array(heap, node, layer, ' ', &strArray)
            : AK_ERR;
    if (ret == AK_OK)
      node->layer = strArray;
  }

  last_trans = NULL;
  xml        = xml->val;

  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, node, NULL);
    } else if (DAE_XML_TAG_EQ8(xml, lookat) && (sval = xmls(xml))) {
      AkObject *obj;
      AkLookAt *looakAt;
      
      obj     = ak_objAlloc(heap, node, sizeof(*looakAt), AKT_LOOKAT, true);
      looakAt = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, (float *)looakAt->val, 9);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ8(xml, matrix) && (sval = xmls(xml))) {
      mat4      transform;
      AkObject *obj;
      AkMatrix *matrix;

      obj    = ak_objAlloc(heap, node, sizeof(*matrix), AKT_MATRIX, true);
      matrix = ak_objGet(obj);

      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, transform[0], 16);
      glm_mat4_transpose_to(transform, matrix->val);
  
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));

      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ8(xml, rotate) && (sval = xmls(xml))) {
      AkObject *obj;
      AkRotate *rotate;
      
      obj    = ak_objAlloc(heap, node, sizeof(*rotate), AKT_ROTATE, true);
      rotate = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, (AkFloat *)rotate->val, 4);
      glm_make_rad(&rotate->val[3]);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ8(xml, scale) && (sval = xmls(xml))) {
      AkObject *obj;
      AkScale  *scale;
      
      obj   = ak_objAlloc(heap, node, sizeof(*scale), AKT_SCALE, true);
      scale = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, (AkFloat *)scale->val, 3);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ8(xml, skew) && (sval = xmls(xml))) {
      AkObject *obj;
      AkSkew   *skew;
      AkFloat   tmp[7];
      
      obj  = ak_objAlloc(heap, node, sizeof(*skew), AKT_SKEW, true);
      skew = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, (AkFloat *)tmp, 4);
      
      /* COLLADA uses degree here, convert it to radians */
      skew->angle = glm_rad(tmp[0]);
      glm_vec3_copy(&tmp[1], skew->rotateAxis);
      glm_vec3_copy(&tmp[4], skew->aroundAxis);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ(xml, translate) && (sval = xmls(xml))) {
      AkObject    *obj;
      AkTranslate *transl;
      
      obj    = ak_objAlloc(heap, node, sizeof(*transl), AKT_TRANSLATE, true);
      transl = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      xml_strtof_fast(sval, (AkFloat *)transl->val, 4);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      AK_APPEND_FLINK(node->transform->item, last_trans, obj);
    } else if (DAE_XML_TAG_EQ(xml, instance_camera)) {
      AkInstanceBase *instcam;

      instcam       = ak_heap_calloc(heap, node, sizeof(*instcam));
      instcam->type = AK_INSTANCE_CAMERA;
      instcam->name = DAE_XMLA_STRDUP8(xml, heap, name, instcam);
      DAE_URL_SET(dst, xml, url, instcam, &instcam->url);
      
      instcam->node = node;
      
      instcam->next = node->camera;
      node->camera  = instcam;

      instcam->prev = node->camera;
      if (node->camera)
        node->camera->prev = instcam;
      
      if (scene) {
        if (!scene->firstCamNode)
          scene->firstCamNode = node;
        
        if (instcam)
          ak_instanceListAdd(scene->cameras, instcam);
      }
    } else if (DAE_XML_TAG_EQ(xml, instance_controller)) {
      AkInstanceController *instctl;
      xml_t                *xinstctl;

      instctl            = ak_heap_calloc(heap, node, sizeof(*instctl));
      instctl->base.type = AK_INSTANCE_CONTROLLER;
      instctl->base.name = DAE_XMLA_STRDUP8(xml, heap, name, instctl);
      DAE_URL_SET(dst, xml, url, instctl, &instctl->base.url);
      
      xinstctl           = xml->val;
      while (xinstctl) {
        if (DAE_XML_TAG_EQ8(xinstctl, skeleton)) {
          char *skel;
          if ((skel = xml_strdup(xinstctl, heap, instctl)))
            flist_sp_insert(&instctl->reserved, skel);
        } else if (DAE_XML_TAG_EQ(xinstctl, bind_material)) {
          instctl->bindMaterial = dae_bindMaterial(dst, xinstctl, instctl);
        } else if (DAE_XML_TAG_EQ8(xinstctl, extra)) {
          instctl->base.extra = tree_fromxml(heap, instctl, xinstctl);
        }
        xinstctl = xinstctl->next;
      }

      instctl->base.node = node;
      flist_sp_insert(&dst->instCtlrs, instctl);;
    } else if (DAE_XML_TAG_EQ(xml, instance_geometry)) {
      AkInstanceGeometry *instgeo;
      xml_t              *xinstgeo;

      instgeo            = ak_heap_calloc(heap, node, sizeof(*instgeo));
      instgeo->base.type = AK_INSTANCE_GEOMETRY;
      instgeo->base.name = DAE_XMLA_STRDUP8(xml, heap, name, instgeo);
      DAE_URL_SET(dst, xml, url, instgeo, &instgeo->base.url);

      xinstgeo           = xml->val;
      while (xinstgeo) {
        if (DAE_XML_TAG_EQ(xinstgeo, bind_material)) {
          ak__instanceGeometrySetBindMaterial(instgeo, dae_bindMaterial(dst, xinstgeo, instgeo));
        } else if (DAE_XML_TAG_EQ8(xinstgeo, extra)) {
          instgeo->base.extra = tree_fromxml(heap, instgeo, xinstgeo);
        }
        xinstgeo = xinstgeo->next;
      }

      instgeo->base.node = node;

      instgeo->base.next = (void *)node->geometry;
      node->geometry     = instgeo;
    } else if (DAE_XML_TAG_EQ(xml, instance_light)) {
      AkInstanceBase *instlight;
      
      instlight       = ak_heap_calloc(heap, node, sizeof(*instlight));
      instlight->type = AK_INSTANCE_LIGHT;
      instlight->name = DAE_XMLA_STRDUP8(xml, heap, name, instlight);
      DAE_URL_SET(dst, xml, url, instlight, &instlight->url);
      
      instlight->node = node;
      
      instlight->next = node->light;
      node->light     = instlight;
      
      instlight->prev = node->light;
      if (node->light)
        node->light->prev = instlight;
      
      if (scene && instlight) {
        AkLight *lightObject;
        lightObject = ak_instanceObject(instlight);
        if (lightObject)
          ak_instanceListAdd(scene->lights, instlight);
      }
    } else if (DAE_XML_TAG_EQ(xml, instance_node)) {
      AkInstanceNode *instnode;
      
      instnode            = ak_heap_calloc(heap, node, sizeof(*instnode));
      instnode->base.type = AK_INSTANCE_NODE;
      instnode->base.name = DAE_XMLA_STRDUP8(xml, heap, name, instnode);
      instnode->proxy     = DAE_XMLA_STRDUP8(xml, heap, proxy, instnode);
      DAE_URL_SET(dst, xml, url, instnode, &instnode->base.url);
      
      if (node->node)
        instnode->base.next = &node->node->base;

      instnode->base.node = node;
      node->node          = instnode;
      
      if (node->node) {
        instnode->base.prev   = &node->node->base;
        node->node->base.prev = &instnode->base;
      }
    } else if (DAE_XML_TAG_EQ4(xml, node)) {
      AkNode *subNode;
      
      if ((subNode = dae_node(dst, xml, node, scene))) {
        if (node->chld) {
          node->chld->prev = subNode;
        }

        subNode->next = node->chld;
        node->chld    = subNode;
      }

    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      node->extra = tree_fromxml(heap, node, xml);
    }
    xml = xml->next;
  }
  
  if (ak_isKindOf(memp, node))
    node->parent = memp;

  dae_nodeFixup(heap, node);

  return node;
}
