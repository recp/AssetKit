/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_states_detail.h"
#include "../ak_collada_common.h"
#include "../core/ak_collada_value.h"
#include "ak_collada_fx_enums.h"
#include "ak_collada_fx_sampler.h"

#define _ak_APPEND_STATE(last_state, state)                                   \
  do {                                                                        \
    if (*last_state) {                                                        \
      (*last_state)->next = (AkRenderState *)state;                           \
    } else {                                                                  \
      *last_state = (AkRenderState *)state;                                   \
      (*states)->next = *last_state;                                          \
    }                                                                         \
    (*last_state) = (AkRenderState *)state;                                   \
  } while (0)

AkResult _assetkit_hide
ak_dae_fxState_enum(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkEnum defaultEnumVal,
                    ak_dae_fxEnumFn_t enumFn) {
  ak_state_t_ul *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    AkEnum enumVal;
    enumVal = enumFn(attrValStr);
    if (enumVal == -1)
      enumVal = defaultEnumVal;

    state->val = enumVal;
    xmlFree(attrValStr);
  } else {
    state->val = defaultEnumVal;
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_bool4(AkXmlState * __restrict xst,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkBool * defaultVal,
                     size_t defaultValSize) {
  ak_state_t_bool4 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomb(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_int2(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize) {
  ak_state_t_int2 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomi(&attrValStr, state->val, 1, 2);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_int4(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize) {
  ak_state_t_int4 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomi(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_ul(AkXmlState * __restrict xst,
                  AkRenderState ** __restrict last_state,
                  AkStates ** __restrict states,
                  AkRenderStateType state_type,
                  unsigned long defaultVal) {
  ak_state_t_ul *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    char *tmp;
    state->val = strtol(attrValStr, &tmp, 10);
    if (*tmp == '\0')
      state->val = defaultVal;

    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_ul_i(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    unsigned long defaultVal) {
  ak_state_t_ul_i * state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    char *tmp;
    state->val = strtol(attrValStr, &tmp, 10);
    if (*tmp == '\0')
      state->val = defaultVal;

    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);
  state->index = ak_xml_attrui(xst, _s_dae_index);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float(AkXmlState * __restrict xst,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkFloat defaultVal) {
  ak_state_t_float * state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    state->val = strtof(attrValStr, NULL);
    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float_i(AkXmlState * __restrict xst,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type,
                       AkFloat defaultVal) {
  ak_state_t_float_i * state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    state->val = strtof(attrValStr, NULL);
    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);
  state->index = ak_xml_attrui(xst, _s_dae_index);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float2(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize) {
  ak_state_t_float2 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, state->val, 1, 2);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float3(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize) {
  ak_state_t_float3 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, state->val, 1, 3);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float3_i(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize) {
  ak_state_t_float3_i *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, state->val, 1, 3);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);
  state->index = ak_xml_attrui(xst, _s_dae_index);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float4(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize) {
  ak_state_t_float4 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float4_i(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize) {
  ak_state_t_float4_i *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);
  state->index = ak_xml_attrui(xst, _s_dae_index);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_float4x4(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize) {
  ak_state_t_float4x4 *state;
  char *attrValStr;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    ak_strtomf(&attrValStr, *state->val, 4, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(*state->val, defaultVal, defaultValSize);
  }

  state->param = ak_xml_attr(xst, state, _s_dae_param);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_sampler(AkXmlState * __restrict xst,
                       const char *elm,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type) {
  ak_state_t_sampler * state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = state_type;

  state->index = ak_xml_attrui(xst, _s_dae_index);

  do {
    if (ak_xml_beginelm(xst, elm))
      break;

    if (ak_xml_eqelm(xst, _s_dae_value)) {
      AkFxSamplerCommon * sampler;
      AkResult ret;

      sampler = NULL;
      ret = ak_dae_fxSampler(xst,
                             state,
                             (const char *)xst->nodeName,
                             &sampler);

      if (ret == AK_OK)
        state->val = sampler;
    } else if (ak_xml_eqelm(xst, _s_dae_param)) {
      state->param = ak_xml_val(xst, state);
    } else if (ak_xml_eqelm(xst, _s_dae_index)) {
      state->index = ak_xml_valul(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxState_str(AkXmlState * __restrict xst,
                   AkRenderState ** __restrict last_state,
                   AkStates ** __restrict states,
                   AkRenderStateType state_type) {
  ak_state_t_str * state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = state_type;

  state->val   = ak_xml_attr(xst, state, _s_dae_value);
  state->param = ak_xml_attr(xst, state, _s_dae_param);
  state->index = ak_xml_attrui(xst, _s_dae_index);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateAlphaFunc(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states) {
  ak_alpha_func *state;
  
  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_ALPHA_FUNC;

  do {
    if (ak_xml_beginelm(xst, _s_dae_alpha_func))
      break;

    if (ak_xml_eqelm(xst, _s_dae_func)) {
      char *valStr;

      state->func.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->func.val = AK_GL_FUNC_ALWAYS;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->func.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_value)) {
      state->val.val   = ak_xml_attrf(xst, _s_dae_value);
      state->val.param = ak_xml_attr(xst, state, _s_dae_param);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);

  _ak_APPEND_STATE(last_state, state);

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateBlend(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states) {
  ak_blend_func * state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_BLEND_FUNC;

  do {
    if (ak_xml_beginelm(xst, _s_dae_blend_func))
      break;

    if (ak_xml_eqelm(xst, _s_dae_src)) {
      char *valStr;

      state->src.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->src.val = AK_GL_BLEND_ONE;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_dest)) {
      char *valStr;

      state->src.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->src.val = AK_GL_BLEND_ZERO;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateBlendSep(AkXmlState * __restrict xst,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states) {
  ak_blend_func_separate *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_BLEND_FUNC_SEPARATE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_blend_func_separate))
      break;

    if (ak_xml_eqelm(xst, _s_dae_src_rgb)) {
      char *valStr;

      state->src_rgb.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->src_rgb.val = AK_GL_BLEND_ONE;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src_rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_dest_rgb)) {
      char *valStr;

      state->dest_rgb.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->dest_rgb.val = AK_GL_BLEND_ZERO;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->dest_rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_src_alpha)) {
      char *valStr;

      state->src_alpha.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->src_alpha.val = AK_GL_BLEND_ONE;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src_alpha.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_dest_alpha)) {
      char *valStr;

      state->dest_alpha.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->dest_alpha.val = AK_GL_BLEND_ZERO;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->dest_alpha.val = enumVal;

        xmlFree(valStr);
      }

    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateBlendEqSep(AkXmlState * __restrict xst,
                         AkRenderState ** __restrict last_state,
                         AkStates ** __restrict states) {
  ak_blend_equation_separate *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_BLEND_EQUATION_SEPARATE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_blend_equation_separate))
      break;

    if (ak_xml_eqelm(xst, _s_dae_rgb)) {
      char *valStr;

      state->rgb.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->rgb.val = AK_GL_BLEND_EQUATION_FUNC_ADD;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlendEq(valStr);

        if (enumVal != -1)
          state->rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_alpha)) {
      char *valStr;

      state->alpha.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->alpha.val = AK_GL_BLEND_EQUATION_FUNC_ADD;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumBlendEq(valStr);

        if (enumVal != -1)
          state->alpha.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateColorMaterial(AkXmlState * __restrict xst,
                            AkRenderState ** __restrict last_state,
                            AkStates ** __restrict states) {
  ak_color_material *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_COLOR_MATERIAL;

  do {
    if (ak_xml_beginelm(xst, _s_dae_color_material))
      break;

    if (ak_xml_eqelm(xst, _s_dae_face)) {
      char *valStr;

      state->face.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AK_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_mode)) {
      char *valStr;

      state->mode.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->mode.val = AK_GL_MATERIAL_TYPE_AMBIENT_AND_DIFFUSE;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumMaterial(valStr);

        if (enumVal != -1)
          state->mode.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStatePolyMode(AkXmlState * __restrict xst,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states) {
  ak_polygon_mode *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_POLYGON_MODE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_polygon_mode))
      break;

    if (ak_xml_eqelm(xst, _s_dae_face)) {
      char *valStr;

      state->face.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AK_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_mode)) {
      char *valStr;

      state->mode.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->mode.val = AK_GL_POLYGON_MODE_FILL;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumPolyMode(valStr);

        if (enumVal != -1)
          state->mode.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateStencilFunc(AkXmlState * __restrict xst,
                          AkRenderState ** __restrict last_state,
                          AkStates ** __restrict states) {
  ak_stencil_func *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_STENCIL_FUNC;

  do {
    if (ak_xml_beginelm(xst, _s_dae_stencil_func))
      break;

    if (ak_xml_eqelm(xst, _s_dae_stencil_func)) {
      char *valStr;

      state->func.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->func.val = AK_GL_FUNC_ALWAYS;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->func.val = enumVal;

        xmlFree(valStr);
      }

    } else if (ak_xml_eqelm(xst, _s_dae_ref)) {
      char *valStr;

      state->ref.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      if (valStr) {
        state->mask.val = strtol(valStr, NULL, 10);
        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_mask)) {
      char *valStr;

      state->mask.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
      if (valStr) {
        char * tmp;
        long   val;

        val = strtol(valStr, &tmp, 10);
        if (*tmp == '\0')
          val = 255;

        state->mask.val = val;
        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateStencilOp(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states) {
  ak_stencil_op *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_STENCIL_OP;

  do {
    if (ak_xml_beginelm(xst, _s_dae_stencil_op))
      break;

    if (ak_xml_eqelm(xst, _s_dae_fail)) {
      char *valStr;

      state->fail.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->fail.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->fail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_zfail)) {
      char *valStr;

      state->zfail.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->zfail.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zfail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_zpass)) {
      char *valStr;

      state->zpass.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->zpass.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zpass.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateStencilFuncSep(AkXmlState * __restrict xst,
                             AkRenderState ** __restrict last_state,
                             AkStates ** __restrict states) {
  ak_stencil_func_separate *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_STENCIL_FUNC_SEPARATE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_stencil_func_separate))
      break;

    if (ak_xml_eqelm(xst, _s_dae_front)) {
      char *valStr;

      state->front.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->front.val = AK_GL_FUNC_ALWAYS;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->front.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_back)) {
      char *valStr;

      state->back.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->back.val = AK_GL_FUNC_ALWAYS;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->back.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_ref)) {
      char *valStr;

      state->ref.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      if (valStr) {
        state->mask.val = strtol(valStr, NULL, 10);
        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_mask)) {
      char *valStr;

      state->mask.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
      if (valStr) {
        char * tmp;
        long   val;

        val = strtol(valStr, &tmp, 10);
        if (*tmp == '\0')
          val = 255;

        state->mask.val = val;
        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateStencilOpSep(AkXmlState * __restrict xst,
                           AkRenderState ** __restrict last_state,
                           AkStates ** __restrict states) {
  ak_stencil_op_separate *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_STENCIL_OP_SEPARATE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_stencil_op_separate))
      break;

    if (ak_xml_eqelm(xst, _s_dae_face)) {
      char *valStr;

      state->face.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AK_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_fail)) {
      char *valStr;

      state->fail.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->fail.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->fail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_zfail)) {
      char *valStr;

      state->zfail.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->zfail.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zfail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_zpass)) {
      char *valStr;

      state->zpass.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->zpass.val = AK_GL_STENCIL_OP_KEEP;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zpass.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxStateStencilMaskSep(AkXmlState * __restrict xst,
                             AkRenderState ** __restrict last_state,
                             AkStates ** __restrict states) {
  ak_stencil_mask_separate *state;

  state = ak_heap_calloc(xst->heap, *states, sizeof(*state), false);
  state->base.state_type = AK_RENDER_STATE_STENCIL_MASK_SEPARATE;

  do {
    if (ak_xml_beginelm(xst, _s_dae_stencil_mask_separate))
      break;

    if (ak_xml_eqelm(xst, _s_dae_face)) {
      char *valStr;

      state->face.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AK_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        AkEnum enumVal;
        enumVal = ak_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }
    } else if (ak_xml_eqelm(xst, _s_dae_mask)) {
      char *valStr;

      state->mask.param = ak_xml_attr(xst, state, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_value);
      if (valStr) {
        char * tmp;
        long   val;

        val = strtol(valStr, &tmp, 10);
        if (*tmp == '\0')
          val = 255;

        state->mask.val = val;
        xmlFree(valStr);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    ak_xml_endelm(xst);
  } while (xst->nodeRet);
  
  _ak_APPEND_STATE(last_state, state);
  
  return AK_OK;
}
