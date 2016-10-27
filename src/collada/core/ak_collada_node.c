/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_node.h"
#include "../../ak_array.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_enums.h"
#include "../fx/ak_collada_fx_material.h"

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
#define k_s_dae_extra               13

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
  {_s_dae_extra,               k_s_dae_extra},
};

static size_t nodeMapLen = 0;

AkResult _assetkit_hide
ak_dae_node(AkHeap * __restrict heap,
            void * __restrict memParent,
            xmlTextReaderPtr reader,
            AkNode ** __restrict dest) {
  AkDoc         *doc;
  AkNode        *node;
  AkObject      *last_transform;
  AkInstanceCamera     *last_camera;
  AkInstanceController *last_controller;
  AkInstanceGeometry   *last_geometry;
  AkInstanceLight      *last_light;
  AkInstanceNode       *last_node;
  const xmlChar *nodeName;
  char          *attrVal;
  int            nodeType;
  int            nodeRet;

  doc  = ak_heap_attachment(heap);
  node = ak_heap_calloc(heap, memParent, sizeof(*node), true);

  _xml_readId(node);
  _xml_readAttr(node, node->sid, _s_dae_sid);
  _xml_readAttr(node, node->name, _s_dae_name);

  _xml_readAttrAsEnumWithDef(node->nodeType,
                             _s_dae_type,
                             ak_dae_enumNodeType,
                             AK_NODE_TYPE_NODE);

  attrVal = (char *)xmlTextReaderGetAttribute(reader,                  
                                 (const xmlChar *)_s_dae_layer);
  if (attrVal) {
    AkStringArray *stringArray;
    AkResult ret;

    ret = ak_strtostr_array(heap,
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

  do {
    const ak_enumpair *found;

    _xml_beginElement(_s_dae_node);

    found = bsearch(nodeName,
                    nodeMap,
                    nodeMapLen,
                    sizeof(nodeMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case k_s_dae_asset: {
        AkAssetInf *assetInf;
        AkResult ret;

        assetInf = NULL;
        ret = ak_dae_assetInf(heap, node, reader, &assetInf);
        if (ret == AK_OK)
          node->inf = assetInf;

        break;
      }
      case k_s_dae_lookat: {
        char *content;
        _xml_readMutText(content);

        if (content) {
          AkObject *obj;
          AkLookAt *looakAt;

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*looakAt),
                            AK_NODE_TRANSFORM_TYPE_LOOK_AT,
                            true,
                            false);

          looakAt = ak_objGet(obj);

          _xml_readAttr(looakAt, looakAt->sid, _s_dae_sid);
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
        _xml_readMutText(content);

        if (content) {
          AkObject *obj;
          AkMatrix *matrix;
          mat4      transform;

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*matrix),
                            AK_NODE_TRANSFORM_TYPE_MATRIX,
                            true,
                            false);

          matrix = ak_objGet(obj);

          _xml_readAttr(matrix, matrix->sid, _s_dae_sid);

          ak_strtof(&content, transform[0], 16);

          glm_mat4_transpose(transform);
          ak_coordCvtTransform(doc->coordSys,
                               transform,
                               ak_defaultCoordSys(),
                               matrix->val);
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
        _xml_readMutText(content);

        if (content) {
          AkObject *obj;
          AkRotate *rotate;

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*rotate),
                            AK_NODE_TRANSFORM_TYPE_ROTATE,
                            true,
                            false);

          rotate = ak_objGet(obj);

          _xml_readAttr(rotate, rotate->sid, _s_dae_sid);
          ak_strtof(&content, (AkFloat *)rotate->val, 4);

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
        _xml_readMutText(content);

        if (content) {
          AkObject *obj;
          AkScale  *scale;

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*scale),
                            AK_NODE_TRANSFORM_TYPE_SCALE,
                            true,
                            false);

          scale = ak_objGet(obj);

          _xml_readAttr(scale, scale->sid, _s_dae_sid);
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
        _xml_readMutText(content);

        if (content) {
          AkObject *obj;
          AkSkew   *skew;
          AkFloat   tmp[7];

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*skew),
                            AK_NODE_TRANSFORM_TYPE_SKEW,
                            true,
                            false);

          skew = ak_objGet(obj);

          _xml_readAttr(skew, skew->sid, _s_dae_sid);
          ak_strtof(&content, (AkFloat *)tmp, 4);

          /* COLLADA uses degree here, convert it to radians */
          skew->angle = glm_rad(tmp[0]);
          glm_vec_dup(&tmp[1], skew->rotateAxis);
          glm_vec_dup(&tmp[4], skew->aroundAxis);

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
        _xml_readMutText(content);

        if (content) {
          AkObject    *obj;
          AkTranslate *translate;

          obj = ak_objAlloc(heap,
                            node,
                            sizeof(*translate),
                            AK_NODE_TRANSFORM_TYPE_TRANSLATE,
                            true,
                            false);

          translate = ak_objGet(obj);

          _xml_readAttr(translate, translate->sid, _s_dae_sid);
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
        AkInstanceCamera *instanceCamera;
        instanceCamera = ak_heap_calloc(heap,
                                        node,
                                        sizeof(*instanceCamera),
                                        false);

        _xml_readAttr(instanceCamera, instanceCamera->name, _s_dae_name);
        _xml_readAttr(instanceCamera, instanceCamera->url, _s_dae_url);

        do {
          _xml_beginElement(_s_dae_instance_camera);

          if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap, instanceCamera, nodePtr, &tree, NULL);
            instanceCamera->extra = tree;
            
            _xml_skipElement;
            break;
          } else {
            _xml_skipElement;
          }
          
          /* end element */
          _xml_endElement;
        } while (nodeRet);

        if (last_camera)
          last_camera->next = instanceCamera;
        else
          node->camera = instanceCamera;

        last_camera = instanceCamera;

        break;
      }
      case k_s_dae_instance_controller: {
        AkInstanceController *controller;
        AkSkeleton           *last_skeleton;

        controller = ak_heap_calloc(heap, node, sizeof(*controller), false);

        _xml_readAttr(controller, controller->name, _s_dae_name);
        _xml_readAttr(controller, controller->url, _s_dae_url);

        last_skeleton = NULL;

        do {
          _xml_beginElement(_s_dae_instance_controller);

          if (_xml_eqElm(_s_dae_skeleton)) {
            if (!xmlTextReaderIsEmptyElement(reader)) {
              AkSkeleton *skeleton;
              skeleton = ak_heap_calloc(heap,
                                        controller,
                                        sizeof(*skeleton),
                                        false);

              _xml_readText(controller, skeleton->val);

              if (last_skeleton)
                last_skeleton->next = skeleton;
              else
                controller->skeleton = skeleton;

              last_skeleton = skeleton;
            }
          } else if (_xml_eqElm(_s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = ak_dae_fxBindMaterial(heap,
                                        controller,
                                        reader,
                                        &bindMaterial);
            if (ret == AK_OK)
              controller->bindMaterial = bindMaterial;

          } else if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap,
                                controller,
                                nodePtr,
                                &tree,
                                NULL);
            controller->extra = tree;

            _xml_skipElement;
            break;
          } else {
            _xml_skipElement;
          }

          /* end element */
          _xml_endElement;
        } while (nodeRet);

        if (last_controller)
          last_controller->next = controller;
        else
          node->controller = controller;
        
        last_controller = controller;

        break;
      }
      case k_s_dae_instance_geometry: {
        AkInstanceGeometry *geometry;

        geometry = ak_heap_calloc(heap, node, sizeof(*geometry), false);

        _xml_readAttr(geometry, geometry->base.name, _s_dae_name);
        _xml_readAttr(geometry, geometry->base.url, _s_dae_url);

        do {
          _xml_beginElement(_s_dae_instance_geometry);

          if (_xml_eqElm(_s_dae_bind_material)) {
            AkBindMaterial *bindMaterial;
            AkResult ret;

            ret = ak_dae_fxBindMaterial(heap, geometry, reader, &bindMaterial);
            if (ret == AK_OK)
              geometry->bindMaterial = bindMaterial;

          } else if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap,
                                geometry,
                                nodePtr,
                                &tree,
                                NULL);
            geometry->base.extra = tree;

            _xml_skipElement;
            break;
          } else {
            _xml_skipElement;
          }

          /* end element */
          _xml_endElement;
        } while (nodeRet);

        if (last_geometry)
          last_geometry->next = geometry;
        else
          node->geometry = geometry;
        
        last_geometry = geometry;
        
        break;
      }
      case k_s_dae_instance_light: {
        AkInstanceLight *light;

        light = ak_heap_calloc(heap, node, sizeof(*light), false);

        _xml_readAttr(light, light->name, _s_dae_name);
        _xml_readAttr(light, light->url, _s_dae_url);

        do {
          _xml_beginElement(_s_dae_instance_light);

          if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap,
                                light,
                                nodePtr,
                                &tree,
                                NULL);
            light->extra = tree;

            _xml_skipElement;
            break;
          } else {
            _xml_skipElement;
          }

          /* end element */
          _xml_endElement;
        } while (nodeRet);

        if (last_light)
          last_light->next = light;
        else
          node->light = light;
        
        last_light = light;

        break;
      }
      case k_s_dae_instance_node: {
        AkInstanceNode *instanceNode;

        instanceNode = ak_heap_calloc(heap,
                                      node,
                                      sizeof(*instanceNode),
                                      false);

        _xml_readAttr(instanceNode, instanceNode->name, _s_dae_name);
        _xml_readAttr(instanceNode, instanceNode->url, _s_dae_url);
        _xml_readAttr(instanceNode, instanceNode->proxy, _s_dae_proxy);

        do {
          _xml_beginElement(_s_dae_instance_node);

          if (_xml_eqElm(_s_dae_extra)) {
            xmlNodePtr nodePtr;
            AkTree    *tree;

            nodePtr = xmlTextReaderExpand(reader);
            tree = NULL;

            ak_tree_fromXmlNode(heap, instanceNode, nodePtr, &tree, NULL);
            instanceNode->extra = tree;

            _xml_skipElement;
            break;
          } else {
            _xml_skipElement;
          }

          /* end element */
          _xml_endElement;
        } while (nodeRet);

        if (last_node)
          last_node->next = instanceNode;
        else
          node->node = instanceNode;
        
        last_node = instanceNode;
        
        break;
      }
      case k_s_dae_extra: {
        xmlNodePtr nodePtr;
        AkTree    *tree;

        nodePtr = xmlTextReaderExpand(reader);
        tree = NULL;

        ak_tree_fromXmlNode(heap, node, nodePtr, &tree, NULL);
        node->extra = tree;

        _xml_skipElement;
        break;
      }
      default:
        _xml_skipElement;
        break;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = node;
  
  return AK_OK;
}
