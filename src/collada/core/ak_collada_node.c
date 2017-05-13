/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_node.h"
#include "ak_collada_enums.h"

#include "../../ak_array.h"
#include "../core/ak_collada_asset.h"
#include "../fx/ak_collada_fx_material.h"
#include "../ak_collada_node_fixup.h"

#include <cglm.h>

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

AkResult _assetkit_hide
ak_dae_node2(AkXmlState * __restrict xst,
             void       * __restrict memParent,
             void      ** __restrict dest) {
  return ak_dae_node(xst,
                     memParent,
                     NULL,
                     (AkNode **)dest);
}

AkResult _assetkit_hide
ak_dae_node(AkXmlState    * __restrict xst,
            void          * __restrict memParent,
            AkVisualScene * __restrict scene,
            AkNode       ** __restrict dest) {
  AkDoc                *doc;
  AkNode               *node, *last_chld;
  AkObject             *last_transform;
  AkInstanceBase       *last_camera;
  AkInstanceController *last_controller;
  AkInstanceGeometry   *last_geometry;
  AkInstanceBase       *last_light;
  AkInstanceNode       *last_node;
  char                 *attrVal;
  AkXmlElmState         xest;

  doc  = ak_heap_data(xst->heap);
  node = ak_heap_calloc(xst->heap, memParent, sizeof(*node));

  ak_xml_readid(xst, node);
  ak_xml_readsid(xst, node);

  node->name = ak_xml_attr(xst, node, _s_dae_name);

  node->nodeType = ak_xml_attrenum_def(xst,
                                       _s_dae_type,
                                       ak_dae_enumNodeType,
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
      goto cont;
    }

    switch (found->val) {
      case k_s_dae_asset: {
        AkAssetInf *assetInf;
        AkResult ret;

        assetInf = NULL;
        ret = ak_dae_assetInf(xst, node, &assetInf);
        if (ret == AK_OK)
          node->inf = assetInf;

        break;
      }
      case k_s_dae_lookat: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject *obj;
          AkLookAt *looakAt;

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*looakAt),
                            AK_NODE_TRANSFORM_TYPE_LOOK_AT,
                            true);

          looakAt = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, (float *)looakAt->val, 9);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_matrix: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject *obj;
          AkMatrix *matrix;
          mat4      transform;

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*matrix),
                            AK_NODE_TRANSFORM_TYPE_MATRIX,
                            true);

          matrix = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, transform[0], 16);

          glm_mat4_transpose_to(transform, matrix->val);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_rotate: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject *obj;
          AkRotate *rotate;

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*rotate),
                            AK_NODE_TRANSFORM_TYPE_ROTATE,
                            true);

          rotate = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, (AkFloat *)rotate->val, 4);
          glm_make_rad(&rotate->val[3]);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_scale: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject *obj;
          AkScale  *scale;

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*scale),
                            AK_NODE_TRANSFORM_TYPE_SCALE,
                            true);

          scale = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, (AkFloat *)scale->val, 4);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_skew: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject *obj;
          AkSkew   *skew;
          AkFloat   tmp[7];

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*skew),
                            AK_NODE_TRANSFORM_TYPE_SKEW,
                            true);

          skew = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, (AkFloat *)tmp, 4);

          /* COLLADA uses degree here, convert it to radians */
          skew->angle = glm_rad(tmp[0]);
          glm_vec_copy(&tmp[1], skew->rotateAxis);
          glm_vec_copy(&tmp[4], skew->aroundAxis);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_translate: {
        char *content;
        content = ak_xml_rawval(xst);

        if (content) {
          AkObject    *obj;
          AkTranslate *translate;

          obj = ak_objAlloc(xst->heap,
                            node,
                            sizeof(*translate),
                            AK_NODE_TRANSFORM_TYPE_TRANSLATE,
                            true);

          translate = ak_objGet(obj);

          ak_xml_readsid(xst, obj);

          ak_strtof(&content, (AkFloat *)translate->val, 4);

          if (last_transform)
            last_transform->next = obj;
          else
            node->transform = obj;

          last_transform = obj;

          xmlFree(content);
        }
        break;
      }
      case k_s_dae_instance_camera: {
        AkInstanceBase *instanceCamera;
        AkXmlElmState   xest2;
        instanceCamera = ak_heap_calloc(xst->heap,
                                        node,
                                        sizeof(*instanceCamera));

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
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                instanceCamera,
                                nodePtr,
                                &tree,
                                NULL);
            instanceCamera->extra = tree;

            ak_xml_skipelm(xst);
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

        last_camera = instanceCamera;

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
        AkInstanceController *controller;
        AkSkeleton           *last_skeleton;
        AkXmlElmState         xest2;

        controller = ak_heap_calloc(xst->heap,
                                    node,
                                    sizeof(*controller));

        controller->name = ak_xml_attr(xst, controller, _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        controller,
                        &controller->url);

        last_skeleton = NULL;

        ak_xest_init(xest2, _s_dae_instance_controller)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_skeleton)) {
            if (!xmlTextReaderIsEmptyElement(xst->reader)) {
              AkSkeleton *skeleton;
              skeleton = ak_heap_calloc(xst->heap,
                                        controller,
                                        sizeof(*skeleton));

              skeleton->val = ak_xml_val(xst, controller);

              if (last_skeleton)
                last_skeleton->next = skeleton;
              else
                controller->skeleton = skeleton;

              last_skeleton = skeleton;
            }
          } else if (ak_xml_eqelm(xst, _s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = ak_dae_fxBindMaterial(xst,
                                        controller,
                                        &bindMaterial);
            if (ret == AK_OK)
              controller->bindMaterial = bindMaterial;

          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                controller,
                                nodePtr,
                                &tree,
                                NULL);
            controller->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        if (last_controller)
          last_controller->next = controller;
        else
          node->controller = controller;

        last_controller = controller;

        break;
      }
      case k_s_dae_instance_geometry: {
        AkInstanceGeometry *geometry;
        AkXmlElmState       xest2;

        geometry = ak_heap_calloc(xst->heap,
                                  node,
                                  sizeof(*geometry));

        geometry->base.name = ak_xml_attr(xst, geometry, _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        geometry,
                        &geometry->base.url);

        ak_xest_init(xest2, _s_dae_instance_geometry)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = ak_dae_fxBindMaterial(xst, geometry, &bindMaterial);
            if (ret == AK_OK)
              geometry->bindMaterial = bindMaterial;

          } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                geometry,
                                nodePtr,
                                &tree,
                                NULL);
            geometry->base.extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        geometry->base.node = node;

        if (last_geometry)
          last_geometry->base.next = &geometry->base;
        else
          node->geometry = geometry;

        last_geometry = geometry;

        break;
      }
      case k_s_dae_instance_light: {
        AkInstanceBase *lightInst;
        AkXmlElmState   xest2;

        lightInst = ak_heap_calloc(xst->heap, node, sizeof(*lightInst));

        lightInst->name = ak_xml_attr(xst, lightInst, _s_dae_name);

        ak_xml_attr_url(xst,
                        _s_dae_url,
                        lightInst,
                        &lightInst->url);

        ak_xest_init(xest2, _s_dae_instance_light)

        do {
          if (ak_xml_begin(&xest2))
            break;

          if (ak_xml_eqelm(xst, _s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                lightInst,
                                nodePtr,
                                &tree,
                                NULL);
            lightInst->extra = tree;

            ak_xml_skipelm(xst);
            break;
          } else {
            ak_xml_skipelm(xst);
          }

          /* end element */
          if (ak_xml_end(&xest2))
            break;
        } while (xst->nodeRet);

        lightInst->node = node;

        if (last_light)
          last_light->next = lightInst;
        else
          node->light = lightInst;

        last_light = lightInst;

        break;
      }
      case k_s_dae_instance_node: {
        AkInstanceNode *instanceNode;
        AkXmlElmState   xest2;

        instanceNode = ak_heap_calloc(xst->heap,
                                      node,
                                      sizeof(*instanceNode));

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
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(xst->reader);
            tree = NULL;

            ak_tree_fromXmlNode(xst->heap,
                                instanceNode,
                                nodePtr,
                                &tree,
                                NULL);
            instanceNode->base.extra = tree;

            ak_xml_skipelm(xst);
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

        last_node = instanceNode;

        break;
      }
      case k_s_dae_node: {
        AkNode *subNode;
        AkResult ret;

        subNode = NULL;
        ret = ak_dae_node(xst, node, scene, &subNode);
        if (ret == AK_OK) {
          if (last_chld)
            last_chld->next = subNode;
          else
            node->chld = subNode;

          last_chld = subNode;

          subNode->parent = node;
        }

        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree    *tree;

        nodePtr = xmlTextReaderExpand(xst->reader);
        tree = NULL;

        ak_tree_fromXmlNode(xst->heap,
                            node,
                            nodePtr,
                            &tree,
                            NULL);
        node->extra = tree;

        ak_xml_skipelm(xst);
        break;
      }
      default:
        ak_xml_skipelm(xst);
        break;
    }

  cont:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  ak_dae_nodeFixup(xst->heap, doc, node);
  *dest = node;

  return AK_OK;
}
