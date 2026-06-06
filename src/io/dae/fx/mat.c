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

#include "mat.h"
#include "effect.h"
#include "../core/asset.h"
#include "../core/param.h"
#include "../core/techn.h"
#include "../../../mat/internal.h"

AK_HIDE
void*
dae_material(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkHeap     *heap;
  AkMaterial *mat;

  heap = dst->heap;
  mat  = ak_heap_calloc(heap, memp, sizeof(*mat));
  ak_setypeid(mat, AKT_MATERIAL);
  
  xmla_setid(xml, heap, mat);
  
  mat->name = DAE_XMLA_STRDUP8(xml, heap, name, mat);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, mat, NULL);
    } else if (DAE_XML_TAG_EQ(xml, inst_effect)) {
      AkInstanceEffect *instEffect;

      if ((instEffect = dae_instEffect(dst, xml, mat))) {
        dae_material_effect_add(dst, mat, instEffect);
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      mat->extra = tree_fromxml(heap, mat, xml);
    }
    xml = xml->next;
  }

  return mat;
}

AK_HIDE
AkBindMaterial*
dae_bindMaterial(DAEState * __restrict dst,
                 xml_t    * __restrict xml,
                 void     * __restrict memp) {
  AkHeap         *heap;
  AkBindMaterial *bindmat;

  heap    = dst->heap;
  bindmat = ak_heap_calloc(heap, memp, sizeof(*bindmat));
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, param)) {
      AkParam *param;
      if ((param = dae_param(dst, xml, bindmat))) {
        if (bindmat->param) {
          bindmat->param->prev = param;
          param->next          = bindmat->param;
        }
        bindmat->param = param;
      }
    } else if (DAE_XML_TAG_EQ(xml, techniquec)) {
      AkInstanceMaterial *imat;
      xml_t              *ximat;
      
      ximat = xml->val;
      while (ximat) {
        if (DAE_XML_TAG_EQ(ximat, instance_material)) {
          if ((imat = dae_instMaterial(dst, ximat, bindmat))) {
            if (bindmat->tcommon) {
              bindmat->tcommon->base.prev = &imat->base;
              imat->base.next             = &bindmat->tcommon->base;
            }

            bindmat->tcommon = imat;
          }
        }
        ximat = ximat->next;
      }
    } else if (DAE_XML_TAG_EQ(xml, technique)) {
      AkTechnique *tq;
      if ((tq = dae_techn(xml, heap, bindmat))) {
        tq->next           = bindmat->technique;
        bindmat->technique = tq;
      }
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      bindmat->extra = tree_fromxml(heap, bindmat, xml);
    }
    xml = xml->next;
  }

  return bindmat;
}

AK_HIDE
AkInstanceMaterial*
dae_instMaterial(DAEState * __restrict dst,
                 xml_t    * __restrict xml,
                 void     * __restrict memp) {
  AkHeap             *heap;
  AkInstanceMaterial *mat;
  xml_attr_t         *att;

  heap = dst->heap;
  mat  = ak_heap_calloc(heap, memp, sizeof(*mat));

  sid_set(xml, heap, mat);

  mat->base.name = DAE_XMLA_STRDUP8(xml, heap, name, mat);
  mat->symbol    = DAE_XMLA_STRDUP8(xml, heap, symbol, mat);

  DAE_URL_SET(dst, xml, target, mat, &mat->base.url);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, bind)) {
      AkBind *bind;
      bind = ak_heap_calloc(heap, mat, sizeof(*bind));
      
      bind->semantic = DAE_XMLA_STRDUP8(xml, heap, semantic, mat);
      bind->target   = DAE_XMLA_STRDUP8(xml, heap, target, mat);
      
      bind->next = mat->bind;
      mat->bind  = bind;
    } else if (DAE_XML_TAG_EQ(xml, bind_vertex_input)) {
      AkBindVertexInput *bvi;
      bvi = ak_heap_calloc(heap, mat, sizeof(*bvi));
      
      bvi->semantic      = DAE_XMLA_STRDUP8(xml, heap, semantic, mat);
      bvi->inputSemantic = DAE_XMLA_STRDUP(xml, heap, input_semantic, mat);

      if ((att = DAE_XMLA(xml, input_set)))
        bvi->inputSet = xmla_u32(att, 0);
      
      bvi->next            = mat->bindVertexInput;
      mat->bindVertexInput = bvi;
    } else if (DAE_XML_TAG_EQ(xml, technique_override)) {
      AkTechniqueOverride *technOv;

      technOv       = ak_heap_calloc(heap, mat, sizeof(*technOv));
      technOv->pass = DAE_XMLA_STRDUP8(xml, heap, pass, technOv);
      technOv->ref  = DAE_XMLA_STRDUP8(xml, heap, ref, technOv);
      
      mat->techniqueOverride = technOv;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      mat->base.extra = tree_fromxml(heap, mat, xml);
    }
    xml = xml->next;
  }

  return mat;
}
