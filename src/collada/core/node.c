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

#define k_s_dae_asset               1
#define k_s_dae_lookat              2
#define k_s_dae_matrix              3
#define k_s_dae_rotate              4
#define k_s_dae_scale               5
#define k_s_dae_skew                6
#define k_s_dae_translate           7
#define k_s_dae_instance_camera     8
#define k_s_dae_instance_controller 9
#define k_s_dae_instance_geometry   10
#define k_s_dae_instance_light      11
#define k_s_dae_instance_node       12
#define k_s_dae_node                13
#define k_s_dae_extra               14

static ak_enumpair nodeMap[] = {
  {_s_dae_asset,               k_s_dae_asset},
  {_s_dae_lookat,              k_s_dae_lookat},
  {_s_dae_matrix,              k_s_dae_matrix},
  {_s_dae_rotate,              k_s_dae_rotate},
  {_s_dae_scale,               k_s_dae_scale},
  {_s_dae_skew,                k_s_dae_skew},
  {_s_dae_translate,           k_s_dae_translate},
  {_s_dae_instance_camera,     k_s_dae_instance_camera},
  {_s_dae_instance_controller, k_s_dae_instance_controller},
  {_s_dae_instance_geometry,   k_s_dae_instance_geometry},
  {_s_dae_instance_light,      k_s_dae_instance_light},
  {_s_dae_instance_node,       k_s_dae_instance_node},
  {_s_dae_node,                k_s_dae_node},
  {_s_dae_extra,               k_s_dae_extra},
};

static size_t nodeMapLen = 0;

AkNode* _assetkit_hide
dae_node(DAEState      * __restrict dst,
         xml_t         * __restrict xml,
         void          * __restrict memp,
         AkVisualScene * __restrict scene) {
  AkNode               *node, *last_chld;
  AkObject             *last_transform;
  AkInstanceBase       *last_camera;
  AkInstanceController *last_controller;
  AkInstanceGeometry   *last_geometry;
  AkInstanceBase       *last_light;
  AkInstanceNode       *last_node;
  char                 *attrVal;
  AkXmlElmState         xest;

  node = ak_heap_calloc(xst->heap, memParent, sizeof(*node));
  ak_setypeid(node, AKT_NODE);

  ak_xml_readid(xst, node);
  ak_xml_readsid(xst, node);

  node->name = ak_xml_attr(xst, node, _s_dae_name);

  node->nodeType = ak_xml_attrenum_def(xst,
                                       _s_dae_type,
                                       dae_enumNodeType,
                                       AK_NODE_TYPE_NODE);

  attrVal = (char *)xmlTextReaderGetAttribute(xst->reader,
                                 (const xmlChar *)_s_dae_layer);
  if (attrVal) {
    AkStringArray *stringArray;
    AkResult ret;

    ret = ak_strtostr_array(xst->heap,
                            memParent,
                            attrVal,
                            ' ',
                            &stringArray);

    if (ret == AK_OK)
      node->layer = stringArray;

    xmlFree(attrVal);
  }

  if (nodeMapLen == 0) {
    nodeMapLen = AK_ARRAY_LEN(nodeMap);
    qsort(nodeMap,
          nodeMapLen,
          sizeof(nodeMap[0]),
          ak_enumpair_cmp);
  }

  last_transform  = NULL;
  last_camera     = NULL;
  last_controller = NULL;
  last_geometry   = NULL;
  last_light      = NULL;
  last_node       = NULL;
  last_chld       = NULL;

  ak_xest_init(xest, _s_dae_node)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    nodeMap,
                    nodeMapLen,
                    sizeof(nodeMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case k_s_dae_asset: {
        (void)dae_assetInf(xst, node, NULL);
        break;
      }
      case k_s_dae_lookat: {
        AkObject *obj;
        AkLookAt *looakAt;
        char     *content;

        obj     = ak_objAlloc(xst->heap,
                              node,
                              sizeof(*looakAt),
                              AKT_LOOKAT,
                              true);
        looakAt = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);

        if (content) {
          ak_strtof(&content, (float *)looakAt->val, 9);
          xmlFree(content);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_matrix: {
        mat4      transform;
        AkObject *obj;
        AkMatrix *matrix;
        char     *content;

        obj   = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*matrix),
                            AKT_MATRIX,
                            true);
        matrix = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);
        if (content) {
          ak_strtof(&content, transform[0], 16);
          glm_mat4_transpose_to(transform, matrix->val);
          xmlFree(content);
        } else {
          glm_mat4_identity(matrix->val);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_rotate: {
        AkObject *obj;
        AkRotate *rotate;
        char     *content;

        obj    = ak_objAlloc(xst->heap,
                             node,
                             sizeof(*rotate),
                             AKT_ROTATE,
                             true);
        rotate = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);
        if (content) {
          ak_strtof(&content, (AkFloat *)rotate->val, 4);
          glm_make_rad(&rotate->val[3]);
          xmlFree(content);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_scale: {
        AkObject *obj;
        AkScale  *scale;
        char     *content;

        obj   = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*scale),
                            AKT_SCALE,
                            true);

        scale = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);
        if (content) {
          ak_strtof(&content, (AkFloat *)scale->val, 3);
          xmlFree(content);
        } else {
          glm_vec3_one(scale->val);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_skew: {
        AkObject *obj;
        AkSkew   *skew;
        char     *content;
        AkFloat   tmp[7];

        obj  = ak_objAlloc(xst->heap,
                           node,
                           sizeof(*skew),
                           AKT_SKEW,
                           true);
        skew = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);
        if (content) {
          ak_strtof(&content, (AkFloat *)tmp, 4);

          /* COLLADA uses degree here, convert it to radians */
          skew->angle = glm_rad(tmp[0]);
          glm_vec3_copy(&tmp[1], skew->rotateAxis);
          glm_vec3_copy(&tmp[4], skew->aroundAxis);

          xmlFree(content);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_translate: {
        AkObject    *obj;
        AkTranslate *translate;
        char        *content;

        obj       = ak_objAlloc(xst->heap,
                                node,
                                sizeof(*translate),
                                AKT_TRANSLATE,
                                true);
        translate = ak_objGet(obj);

        ak_xml_readsid(xst, obj);

        content = ak_xml_rawval(xst);
        if (content) {
          ak_strtof(&content, (AkFloat *)translate->val, 4);
          xmlFree(content);
        }

        if (last_transform)
          last_transform->next = obj;
        else {
          node->transform = ak_heap_calloc(xst->heap,
                                           node,
                                           sizeof(*node->transform));
          node->transform->item = obj;
        }

        last_transform = obj;
        break;
      }
      case k_s_dae_instance_camera: {
        AkInstanceBase *instanceCamera;
        AkXmlElmState   xest2;
        instanceCamera = ak_heap_calloc(xst->heap,
                                        node,
                                        sizeof(*instanceCamera));

        instanceCamera->type = AK_INSTANCE_CAMERA;
        instanceCamera->name = ak_xml_attr(xst,
                                           instanceCamera,
                                           _s_dae_name);
        ak_xml_attr_url(xst,
                        _s_dae_url,
                        instanceCamera,
                        &instanceCamera->url);

        ak_xest_init(xest2, _s_dae_instance_camera)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_extra)) {
            dae_extra(xst, instanceCamera, &instanceCamera->extra);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        instanceCamera->node = node;

        if (last_camera)
          last_camera->next = instanceCamera;
        else
          node->camera = instanceCamera;

        instanceCamera->prev = last_camera;
        last_camera          = instanceCamera;

        if (scene) {
          if (!scene->firstCamNode)
            scene->firstCamNode = node;

          if (instanceCamera)
            ak_instanceListAdd(scene->cameras,
                               instanceCamera);
        }
        break;
      }
      case k_s_dae_instance_controller: {
        AkInstanceController *instanceController;
        AkXmlElmState         xest2;

        instanceController = ak_heap_calloc(xst->heap,
                                            node,
                                            sizeof(*instanceController));

        instanceController->base.type = AK_INSTANCE_CONTROLLER;
        instanceController->base.name = ak_xml_attr(xst,
                                                    instanceController,
                                                    _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        instanceController,
                        &instanceController->base.url);

        ak_xest_init(xest2, _s_dae_instance_controller)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_skeleton)) {
            if (!xmlTextReaderIsEmptyElement(xst->reader)) {
              char *skel;
              if ((skel = ak_xml_val(xst, instanceController)))
                flist_sp_insert(&instanceController->reserved, skel);
            }
          } else if (ak_xml_eqelm(xst, _s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = dae_fxBindMaterial(xst,
                                     instanceController,
                                     &bindMaterial);
            if (ret == AK_OK)
              instanceController->bindMaterial = bindMaterial;

          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            dae_extra(xst, instanceController, &instanceController->base.extra);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        if (last_controller)
          last_controller->base.next = &instanceController->base;
        else
          node->controller = instanceController;

        if (last_controller)
          instanceController->base.prev = &last_controller->base;
        last_controller = instanceController;

        flist_sp_insert(&xst->instCtlrs, instanceController);
        break;
      }
      case k_s_dae_instance_geometry: {
        AkInstanceGeometry *instanceGeom;
        AkXmlElmState       xest2;

        instanceGeom = ak_heap_calloc(xst->heap,
                                      node,
                                      sizeof(*instanceGeom));

        instanceGeom->base.type = AK_INSTANCE_GEOMETRY;
        instanceGeom->base.name = ak_xml_attr(xst, instanceGeom, _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        instanceGeom,
                        &instanceGeom->base.url);

        ak_xest_init(xest2, _s_dae_instance_geometry)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = dae_fxBindMaterial(xst, instanceGeom, &bindMaterial);
            if (ret == AK_OK)
              instanceGeom->bindMaterial = bindMaterial;

          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            dae_extra(xst, instanceGeom, &instanceGeom->base.extra);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        instanceGeom->base.node = node;

        if (last_geometry)
          last_geometry->base.next = &instanceGeom->base;
        else
          node->geometry = instanceGeom;

        if (last_geometry)
          instanceGeom->base.prev = &last_geometry->base;
        last_geometry = instanceGeom;
        break;
      }
      case k_s_dae_instance_light: {
        AkInstanceBase *instanceLight;
        AkXmlElmState   xest2;

        instanceLight = ak_heap_calloc(xst->heap,
                                       node,
                                       sizeof(*instanceLight));

        instanceLight->type = AK_INSTANCE_LIGHT;
        instanceLight->name = ak_xml_attr(xst, instanceLight, _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        instanceLight,
                        &instanceLight->url);

        ak_xest_init(xest2, _s_dae_instance_light)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_extra)) {
            dae_extra(xst, instanceLight, &instanceLight->extra);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        instanceLight->node = node;

        if (last_light)
          last_light->next = instanceLight;
        else
          node->light = instanceLight;

        instanceLight->prev = last_light;
        last_light          = instanceLight;

        if (scene && instanceLight) {
          AkLight *lightObject;
          lightObject = ak_instanceObject(instanceLight);
          if (lightObject)
            ak_instanceListAdd(scene->lights,
                               instanceLight);
        }

        break;
      }
      case k_s_dae_instance_node: {
        AkInstanceNode *instanceNode;
        AkXmlElmState   xest2;

        instanceNode = ak_heap_calloc(xst->heap,
                                      node,
                                      sizeof(*instanceNode));

        instanceNode->base.type = AK_INSTANCE_NODE;
        instanceNode->base.name = ak_xml_attr(xst, instanceNode, _s_dae_name);
        instanceNode->proxy     = ak_xml_attr(xst, instanceNode, _s_dae_proxy);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        instanceNode,
                        &instanceNode->base.url);

        ak_xest_init(xest2, _s_dae_instance_node)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_extra)) {
            dae_extra(xst, instanceNode, &instanceNode->base.extra);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        instanceNode->base.node = node;

        if (last_node)
          last_node->base.next = &instanceNode->base;
        else
          node->node = instanceNode;

        if (last_node)
          instanceNode->base.prev = &last_node->base;
        last_node = instanceNode;
        break;
      }
      case k_s_dae_node: {
        AkNode *subNode;
        AkResult ret;

        subNode = NULL;
        ret = dae_node(xst, node, scene, &subNode);
        if (ret == AK_OK) {
          if (last_chld)
            last_chld->next = subNode;
          else
            node->chld = subNode;

          last_chld = subNode;
        }

        break;
      }
      case k_s_dae_extra: {
        dae_extra(xst, node, &node->extra);
        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  if (ak_isKindOf(memParent, node))
    node->parent = memParent;

  dae_nodeFixup(xst->heap, node);

  *dest = node;

  return AK_OK;
}
