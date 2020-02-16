/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "node.h"
#include "enum.h"

#include "../../array.h"
#include "../core/asset.h"
#include "../fx/mat.h"
#include "../fixup/node.h"

#include <cglm/cglm.h>

_assetkit_hide
void*
dae_node2(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  return dae_node(dst, xml, memp, NULL);
}

_assetkit_hide
AkNode*
dae_node(DAEState      * __restrict dst,
         xml_t         * __restrict xml,
         void          * __restrict memp,
         AkVisualScene * __restrict scene) {
  AkHeap     *heap;
  AkNode     *node;
  char       *sval;
  xml_attr_t *att;

  heap = dst->heap;
  node = ak_heap_calloc(heap, memp, sizeof(*node));
  ak_setypeid(node, AKT_NODE);

  xmla_setid(xml, heap, node);
  sid_set(xml, heap, node);
  
  node->name     = xmla_strdup_by(xml, heap, _s_dae_name, node);
  node->nodeType = dae_nodeType(xmla(xml, _s_dae_type));
  if (node->nodeType < 1)
    node->nodeType = AK_NODE_TYPE_NODE;

  if ((att = xmla(xml, _s_dae_layer))) {
    AkStringArray *strArray;
    AkResult       ret;

    ret = ak_strtostr_array(heap, node, (char *)att->val, ' ', &strArray);
    if (ret == AK_OK)
      node->layer = strArray;
  }

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, node, NULL);
    } else if (xml_tag_eq(xml, _s_dae_lookat) && (sval = xml->val)) {
      AkObject *obj;
      AkLookAt *looakAt;
      
      obj     = ak_objAlloc(heap, node, sizeof(*looakAt), AKT_LOOKAT, true);
      looakAt = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      ak_strtof(&sval, (float *)looakAt->val, 9);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_matrix) && (sval = xml->val)) {
      mat4      transform;
      AkObject *obj;
      AkMatrix *matrix;


      obj    = ak_objAlloc(heap, node, sizeof(*matrix), AKT_MATRIX, true);
      matrix = ak_objGet(obj);

      sid_set(xml, heap, obj);
      ak_strtof(&sval, transform[0], 16);
      glm_mat4_transpose_to(transform, matrix->val);
  
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));

      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_rotate) && (sval = xml->val)) {
      AkObject *obj;
      AkRotate *rotate;
      
      obj    = ak_objAlloc(heap, node, sizeof(*rotate), AKT_ROTATE, true);
      rotate = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      ak_strtof(&sval, (AkFloat *)rotate->val, 4);
      glm_make_rad(&rotate->val[3]);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_scale) && (sval = xml->val)) {
      AkObject *obj;
      AkScale  *scale;
      
      obj   = ak_objAlloc(heap, node, sizeof(*scale), AKT_SCALE, true);
      scale = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      ak_strtof(&sval, (AkFloat *)scale->val, 3);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_skew) && (sval = xml->val)) {
      AkObject *obj;
      AkSkew   *skew;
      AkFloat   tmp[7];
      
      obj  = ak_objAlloc(heap, node, sizeof(*skew), AKT_SKEW, true);
      skew = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      ak_strtof(&sval, (AkFloat *)tmp, 4);
      
      /* COLLADA uses degree here, convert it to radians */
      skew->angle = glm_rad(tmp[0]);
      glm_vec3_copy(&tmp[1], skew->rotateAxis);
      glm_vec3_copy(&tmp[4], skew->aroundAxis);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_translate) && (sval = xml->val)) {
      AkObject    *obj;
      AkTranslate *transl;
      
      obj    = ak_objAlloc(heap, node, sizeof(*transl), AKT_TRANSLATE, true);
      transl = ak_objGet(obj);
      
      sid_set(xml, heap, obj);
      ak_strtof(&sval, (AkFloat *)transl->val, 4);
      
      if (!node->transform)
        node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
      
      obj->next             = node->transform->item;
      node->transform->item = obj;
    } else if (xml_tag_eq(xml, _s_dae_instance_camera)) {
      AkInstanceBase *instcam;

      instcam       = ak_heap_calloc(heap, node, sizeof(*instcam));
      instcam->type = AK_INSTANCE_CAMERA;
      instcam->name = xmla_strdup_by(xml, heap, _s_dae_name, instcam);
      url_set(dst, xml, _s_dae_url, instcam, &instcam->url);
      
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
    } else if (xml_tag_eq(xml, _s_dae_instance_controller)) {
      AkInstanceController *instctl;
      xml_t                *xinstctl;

      instctl            = ak_heap_calloc(heap, node, sizeof(*instctl));
      instctl->base.type = AK_INSTANCE_CONTROLLER;
      instctl->base.name = xmla_strdup_by(xml, heap, _s_dae_name, instctl);
      url_set(dst, xml, _s_dae_url, instctl, &instctl->base.url);
      
      xinstctl           = xml->val;
      while (xinstctl) {
        if (xml_tag_eq(xinstctl, _s_dae_skeleton) && (sval = xinstctl->val)) {
          if ((sval = xml_strdup(xinstctl, heap, instctl)))
            flist_sp_insert(&instctl->reserved, sval);
        } else if (xml_tag_eq(xinstctl, _s_dae_bind_material)) {
          instctl->bindMaterial = dae_bindMaterial(dst, xinstctl, instctl);
        } else if (xml_tag_eq(xinstctl, _s_dae_extra)) {
          instctl->base.extra = tree_fromxml(heap, instctl, xinstctl);
        }
        xinstctl = xinstctl->next;
      }

      instctl->base.node = node;
      
      instctl->base.next = &node->controller->base;
      node->controller   = instctl;

      if (node->controller) {
        instctl->base.prev          = &node->controller->base;
        node->controller->base.prev = &instctl->base;
      }

      flist_sp_insert(&dst->instCtlrs, instctl);
    } else if (xml_tag_eq(xml, _s_dae_instance_geometry)) {
      AkInstanceGeometry *instgeo;
      xml_t              *xinstgeo;

      instgeo            = ak_heap_calloc(heap, node, sizeof(*instgeo));
      instgeo->base.type = AK_INSTANCE_GEOMETRY;
      instgeo->base.name = xmla_strdup_by(xml, heap, _s_dae_name, instgeo);
      url_set(dst, xml, _s_dae_url, instgeo, &instgeo->base.url);

      xinstgeo           = xml->val;
      while (xinstgeo) {
        if (xml_tag_eq(xinstgeo, _s_dae_bind_material)) {
          instgeo->bindMaterial = dae_bindMaterial(dst, xinstgeo, instgeo);
        } else if (xml_tag_eq(xinstgeo, _s_dae_extra)) {
          instgeo->base.extra = tree_fromxml(heap, instgeo, xinstgeo);
        }
        xinstgeo = xinstgeo->next;
      }

      instgeo->base.node = node;

      instgeo->base.next = (void *)node->geometry;
      node->geometry     = instgeo;

      if (node->controller) {
        instgeo->base.prev          = &node->controller->base;
        node->controller->base.prev = &instgeo->base;
      }

      flist_sp_insert(&dst->instCtlrs, instgeo);
    } else if (xml_tag_eq(xml, _s_dae_instance_light)) {
      AkInstanceBase *instlight;
      
      instlight       = ak_heap_calloc(heap, node, sizeof(*instlight));
      instlight->type = AK_INSTANCE_LIGHT;
      instlight->name = xmla_strdup_by(xml, heap, _s_dae_name, instlight);
      url_set(dst, xml, _s_dae_url, instlight, &instlight->url);
      
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
    } else if (xml_tag_eq(xml, _s_dae_instance_node)) {
      AkInstanceNode *instnode;
      
      instnode            = ak_heap_calloc(heap, node, sizeof(*instnode));
      instnode->base.type = AK_INSTANCE_NODE;
      instnode->base.name = xmla_strdup_by(xml, heap, _s_dae_name,  instnode);
      instnode->proxy     = xmla_strdup_by(xml, heap, _s_dae_proxy, instnode);
      url_set(dst, xml, _s_dae_url, instnode, &instnode->base.url);
      
      instnode->base.node = node;
      
      instnode->base.next = &node->node->base;
      node->node         = instnode;
      
      if (node->node) {
        instnode->base.prev   = &node->node->base;
        node->node->base.prev = &instnode->base;
      }
    } else if (xml_tag_eq(xml, _s_dae_node)) {
      AkNode *subNode;
      
      if ((subNode = dae_node(dst, xml, node, scene))) {
        if (node->chld) {
          node->chld->prev = subNode;
        }

        subNode->next = node->chld;
        node->chld    = subNode;
      }

    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      node->extra = tree_fromxml(heap, node, xml);
    }
    xml = xml->next;
  }
  
  if (ak_isKindOf(memp, node))
    node->parent = memp;

  dae_nodeFixup(heap, node);

  return node;
}
