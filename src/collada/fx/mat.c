/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "mat.h"
#include "effect.h"
#include "../core/asset.h"
#include "../core/param.h"
#include "../core/techn.h"

_assetkit_hide
void*
dae_material(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkHeap     *heap;
  AkMaterial *mat;

  heap = dst->heap;
  mat  = ak_heap_calloc(heap, memp, sizeof(*mat));
  
  xmla_setid(xml, heap, mat);
  
  mat->name = xmla_strdup_by(xml, heap, _s_dae_name, mat);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_asset)) {
      (void)dae_asset(dst, xml, mat, NULL);
    } else if (xml_tag_eq(xml, _s_dae_inst_effect)) {
      AkInstanceEffect *instEffect;

      if ((instEffect = dae_instEffect(dst, xml, mat))) {
        if (mat->effect) {
          mat->effect->base.prev = &instEffect->base;
          instEffect->base.next  = &mat->effect->base;
        }
      
        mat->effect = instEffect;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      mat->extra = tree_fromxml(heap, mat, xml);
    }
    xml = xml->next;
  }

  return mat;
}

_assetkit_hide
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
    if (xml_tag_eq(xml, _s_dae_param)) {
      AkParam *param;
      if ((param = dae_param(dst, xml, bindmat))) {
        if (bindmat->param) {
          bindmat->param->prev = param;
          param->next          = bindmat->param;
        }
        bindmat->param = param;
      }
    } else if (xml_tag_eq(xml, _s_dae_techniquec)) {
      AkInstanceMaterial *imat;
      xml_t              *ximat;
      
      ximat = xml->val;
      while (ximat) {
        if (xml_tag_eq(ximat, _s_dae_instance_material)) {
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
    } else if (xml_tag_eq(xml, _s_dae_technique)) {
      AkTechnique *tq;
      if ((tq = dae_techn(xml, heap, bindmat))) {
        tq->next           = bindmat->technique;
        bindmat->technique = tq;
      }
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      bindmat->extra = tree_fromxml(heap, bindmat, xml);
    }
    xml = xml->next;
  }

  return bindmat;
}

_assetkit_hide
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

  mat->base.name = xmla_strdup_by(xml, heap, _s_dae_name,   mat);
  mat->symbol    = xmla_strdup_by(xml, heap, _s_dae_symbol, mat);

  url_set(dst, xml, _s_dae_target, mat, &mat->base.url);

  xml = xml->val;
  while (xml) {
    if (xml_tag_eq(xml, _s_dae_bind)) {
      AkBind *bind;
      bind = ak_heap_calloc(heap, mat, sizeof(*bind));
      
      bind->semantic = xmla_strdup_by(xml, heap, _s_dae_semantic, mat);
      bind->target   = xmla_strdup_by(xml, heap, _s_dae_target,   mat);
      
      bind->next = mat->bind;
      mat->bind  = bind;
    } else if (xml_tag_eq(xml, _s_dae_bind_vertex_input)) {
      AkBindVertexInput *bvi;
      bvi = ak_heap_calloc(heap, mat, sizeof(*bvi));
      
      bvi->semantic      = xmla_strdup_by(xml, heap, _s_dae_semantic, mat);
      bvi->inputSemantic = xmla_strdup_by(xml, heap, _s_dae_input_semantic,
                                          mat);

      if ((att = xmla(xml, _s_dae_input_set)))
        bvi->inputSet = xmla_uint32(att, 0);
      
      bvi->next            = mat->bindVertexInput;
      mat->bindVertexInput = bvi;
    } else if (xml_tag_eq(xml, _s_dae_technique_override)) {
      AkTechniqueOverride *technOv;

      technOv       = ak_heap_calloc(heap, mat, sizeof(*technOv));
      technOv->pass = xmla_strdup_by(xml, heap, _s_dae_pass, technOv);
      technOv->ref  = xmla_strdup_by(xml, heap, _s_dae_ref,  technOv);
      
      mat->techniqueOverride = technOv;
    } else if (xml_tag_eq(xml, _s_dae_extra)) {
      mat->base.extra = tree_fromxml(heap, mat, xml);
    }
    xml = xml->next;
  }

  return mat;
}
