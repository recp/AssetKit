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

#include "scene.h"
#include "node.h"
#include "../core/asset.h"
#include "../core/asset.h"
#include "../fx/mat.h"
#include "../../../array.h"
#include "../../../../include/ak/light.h"

static
AkEvaluateScene*
dae_evalScene(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp);

static
void
dae_vsceneAttachRoots(DAEState * __restrict dst,
                      AkScene  * __restrict vscn,
                      AkNode   * __restrict root);

AK_HIDE
void*
dae_vscene(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           void     * __restrict memp) {
  AkHeap  *heap;
  AkScene *vscn;
  AkNode  *rootNodes;

  heap = dst->heap;
  vscn = ak_heap_calloc(heap, memp, sizeof(*vscn));

  ak_setypeid(vscn, AKT_SCENE);
  xmla_setid(xml, heap, vscn);

  vscn->name    = DAE_XMLA_STRDUP8(xml, heap, name, vscn);
  vscn->cameras = ak_heap_calloc(heap, vscn, sizeof(*vscn->cameras));
  vscn->lights  = ak_heap_calloc(heap, vscn, sizeof(*vscn->lights));

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, vscn, NULL);
    } else if (DAE_XML_TAG_EQ4(xml, node)) {
      AkNode *node;

      if ((node = dae_node(dst, xml, vscn, vscn))) {
        node->next = vscn->node;
        if (node->next)
          node->next->prev = node;
        vscn->node = node;
      }
    } else if (DAE_XML_TAG_EQ(xml, evaluate_scene)) {
      (void)dae_evalScene(dst, xml, vscn);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      vscn->extra = tree_fromxml(heap, vscn, xml);
    }
    xml = xml->next;
  }

  rootNodes = vscn->node;
  vscn->node = ak_heap_calloc(heap, vscn, sizeof(*vscn->node));
  ak_setypeid(vscn->node, AKT_NODE);
  vscn->node->visible = true;

  dae_vsceneAttachRoots(dst, vscn, rootNodes);

  if (vscn->lights->count < 1
      && rootNodes
      && ak_opt_get(AK_OPT_ADD_DEFAULT_LIGHT)) {
    AkLight *light;
    AkNode  *rootNode;
    rootNode = vscn->node;
    if (rootNode) {
      AkHeap         *heap;
      AkDoc          *doc;
      AkInstanceBase *lightInst;

      heap  = ak_heap_getheap(rootNode);
      doc   = ak_heap_data(heap);
      light = ak_defaultLight(rootNode);

      lightInst = ak_nodeAttachLight(rootNode, light);
      ak_instanceListEmpty(vscn->lights);
      ak_instanceListAdd(vscn->lights, lightInst);

      ak_libAddLight(doc, light);
    }
  }

  return vscn;
}

static
void
dae_vsceneAttachRoots(DAEState * __restrict dst,
                      AkScene  * __restrict vscn,
                      AkNode   * __restrict root) {
  AkNode *prev, *tail;

  if (!root)
    return;

  for (tail = root; tail->next; tail = tail->next) { }

  while (tail) {
    prev = tail->prev;

    tail->next = NULL;
    tail->prev = NULL;

    ak_heap_setpm(tail, dst->doc);
    AK_LIB_PREPEND(dst->doc->lib.nodes, tail, docNext);
    ak_addSubNode(vscn->node, tail, false);

    tail = prev;
  }
}

static
AkEvaluateScene*
dae_evalScene(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memParent) {
  AkEvaluateScene *evalScene;
  AkHeap          *heap;

  heap = dst->heap;
  xml  = xml->val;

  evalScene = ak_heap_calloc(heap, memParent, sizeof(*evalScene));
  xmla_setid(xml, heap, evalScene);
  sid_set(xml, heap, evalScene);
  
  evalScene->name   = DAE_XMLA_STRDUP8(xml, heap, name, evalScene);
  evalScene->enable = xmla_bool(DAE_XMLA8(xml, enable), 0);
  
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
       (void)dae_asset(dst, xml, evalScene, NULL);
    } else if (DAE_XML_TAG_EQ(xml, render)) {
      AkRender *ren;
      xml_t    *xren;
      
      ren = ak_heap_calloc(heap, evalScene, sizeof(*ren));
      sid_set(xml, heap, ren);
      
      ren->name       = DAE_XMLA_STRDUP8(xml, heap, name, ren);
      ren->cameraNode = DAE_XMLA_STRDUP(xml, heap, camera_node, ren);
      
      xren = xml->val;
      while (xren) {
        if (DAE_XML_TAG_EQ8(xren, layer) && xren->val) {
          AkStringArrayL *layer;
          char           *contents;
          AkResult        ret;
          
          contents                = xren->val;
          contents[xren->valsize] = '\0';
          
          /* TODO: */
          ret = ak_strtostr_arrayL(heap, ren, contents, ' ', &layer);
          if (ret == AK_OK) {
            layer->next = ren->layer;
            ren->layer  = layer;
          }
        } else if (DAE_XML_TAG_EQ(xren, instance_material)) {
          AkInstanceMaterial *instmat;
        
          if ((instmat = dae_instMaterial(dst, xml, ren))) {
            if (ren->instanceMaterial) {
              ren->instanceMaterial->base.prev = &instmat->base;
              instmat->base.next               = &ren->instanceMaterial->base;
            }
            
            ren->instanceMaterial = instmat;
          }
        } else if (DAE_XML_TAG_EQ8(xren, extra)) {
           ren->extra = tree_fromxml(heap, ren, xml);
        }

        xren = xren->next;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      evalScene->extra = tree_fromxml(heap, evalScene, xml);
    }
    xml = xml->next;
  }

  return evalScene;
}

AK_HIDE
void
dae_scene(DAEState * __restrict dst,
          xml_t    * __restrict xml) {
  AkDoc     *doc;
  AkHeap    *heap;

  heap = dst->heap;
  doc  = dst->doc;
  xml  = xml->val;

  while (xml) {
    if (DAE_XML_TAG_EQ(xml, instance_visual_scene)) {
      DAE_URL_SET(dst, xml, url, doc, &dst->activeScene);
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      doc->extra = tree_fromxml(heap, doc, xml);
    }
    xml = xml->next;
  }
}
