/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_states_detail.h"
#include "../aio_collada_common.h"
#include "../aio_collada_value.h"
#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_sampler.h"

#define _AIO_APPEND_STATE(last_state, state)                                  \
  do {                                                                        \
    if (*last_state) {                                                        \
      (*last_state)->next = (aio_render_state *)state;                        \
    } else {                                                                  \
      *last_state = (aio_render_state *)state;                                \
      (*states)->next = *last_state;                                          \
    }                                                                         \
    (*last_state) = (aio_render_state *)state;                                \
  } while (0)

int _assetio_hide
aio_dae_fxState_enum(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     long defaultEnumVal,
                     aio_dae_fxEnumFn_t enumFn) {
  aio_state_t_ul *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    long enumVal;
    enumVal = enumFn(attrValStr);
    if (enumVal == -1)
      enumVal = defaultEnumVal;

    state->val = enumVal;
    xmlFree(attrValStr);
  } else {
    state->val = defaultEnumVal;
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_bool4(xmlTextReaderPtr __restrict reader,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_bool * defaultVal,
                      size_t defaultValSize) {
  aio_state_t_bool4 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomb(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_int2(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize) {
  aio_state_t_int2 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomi(&attrValStr, state->val, 1, 2);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_int4(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize) {
  aio_state_t_int4 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomi(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_ul(xmlTextReaderPtr __restrict reader,
                   aio_render_state ** __restrict last_state,
                   aio_states ** __restrict states,
                   long state_type,
                   unsigned long defaultVal) {
  aio_state_t_ul *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
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

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_ul_i(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     unsigned long defaultVal) {
  aio_state_t_ul_i * state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
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

  _xml_readAttr(state, state->param, _s_dae_param);
  _xml_readAttrUsingFn(state->index,
                       _s_dae_index,
                       strtol, NULL, 10);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float(xmlTextReaderPtr __restrict reader,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_float defaultVal) {
  aio_state_t_float * state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    state->val = strtof(attrValStr, NULL);
    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float_i(xmlTextReaderPtr __restrict reader,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type,
                        aio_float defaultVal) {
  aio_state_t_float_i * state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    state->val = strtof(attrValStr, NULL);
    xmlFree(attrValStr);
  } else {
    state->val = defaultVal;
  }

  _xml_readAttr(state, state->param, _s_dae_param);
  _xml_readAttrUsingFn(state->index,
                       _s_dae_index,
                       strtol, NULL, 10);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float2(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float2 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, state->val, 1, 2);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float3(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float3 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, state->val, 1, 3);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float3_i(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float3_i *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, state->val, 1, 3);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);
  _xml_readAttrUsingFn(state->index,
                       _s_dae_index,
                       strtol, NULL, 10);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float4 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4_i(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float4_i *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, state->val, 1, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);
  _xml_readAttrUsingFn(state->index,
                       _s_dae_index,
                       strtol, NULL, 10);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4x4(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float4x4 *state;
  char *attrValStr;

  state = aio_calloc(*states, sizeof(*state), 1);

  state->base.state_type = state_type;
  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);
  if (attrValStr) {
    aio_strtomf(&attrValStr, *state->val, 4, 4);
    xmlFree(attrValStr);
  } else {
    if (defaultVal)
      memcpy(*state->val, defaultVal, defaultValSize);
  }

  _xml_readAttr(state, state->param, _s_dae_param);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_sampler(xmlTextReaderPtr __restrict reader,
                        const char *elm,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type) {
  aio_state_t_sampler * state;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = state_type;

  _xml_readAttrUsingFn(state->index,
                       _s_dae_index,
                       strtol, NULL, 10);

  do {
    _xml_beginElement(elm);

    if (_xml_eqElm(_s_dae_value)) {
      aio_fx_sampler_common * sampler;
      int ret;

      sampler = NULL;
      ret = aio_dae_fxSampler(state,
                              reader,
                              (const char *)nodeName,
                              &sampler);

      if (ret == 0)
        state->val = sampler;
    } else if (_xml_eqElm(_s_dae_param)) {
      _xml_readText(state, state->param);
    } else if (_xml_eqElm(_s_dae_index)) {
      _xml_readTextUsingFn(state->index, strtol, NULL, 10);
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_str(xmlTextReaderPtr __restrict reader,
                    aio_render_state ** __restrict last_state,
                    aio_states ** __restrict states,
                    long state_type) {
  aio_state_t_str * state;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = state_type;

  _xml_readAttr(state, state->val, _s_dae_value);
  _xml_readAttr(state, state->param, _s_dae_param);
  _xml_readAttrUsingFn(state->index, _s_dae_index, strtol, NULL, 10);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateAlphaFunc(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states) {
  aio_alpha_func *state;
  const xmlChar  *nodeName;
  int             nodeType;
  int             nodeRet;
  
  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_ALPHA_FUNC;

  do {
    _xml_beginElement(_s_dae_alpha_func);

    if (_xml_eqElm(_s_dae_func)) {
      char *valStr;

      _xml_readAttr(state, state->func.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->func.val = AIO_GL_FUNC_ALWAYS;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->func.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_value)) {
      _xml_readAttrUsingFn(state->val.val, _s_dae_value, strtof, NULL);
      _xml_readAttr(state, state->val.param, _s_dae_param);
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateBlend(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states) {
  aio_blend_func * state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_BLEND_FUNC;

  do {
    _xml_beginElement(_s_dae_blend_func);

    if (_xml_eqElm(_s_dae_src)) {
      char *valStr;

      _xml_readAttr(state, state->src.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->src.val = AIO_GL_BLEND_ONE;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_dest)) {
      char *valStr;

      _xml_readAttr(state, state->src.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->src.val = AIO_GL_BLEND_ZERO;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateBlendSep(xmlTextReaderPtr __restrict reader,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states) {
  aio_blend_func_separate *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_BLEND_FUNC_SEPARATE;

  do {
    _xml_beginElement(_s_dae_blend_func_separate);

    if (_xml_eqElm(_s_dae_src_rgb)) {
      char *valStr;

      _xml_readAttr(state, state->src_rgb.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->src_rgb.val = AIO_GL_BLEND_ONE;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src_rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_dest_rgb)) {
      char *valStr;

      _xml_readAttr(state, state->dest_rgb.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->dest_rgb.val = AIO_GL_BLEND_ZERO;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->dest_rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_src_alpha)) {
      char *valStr;

      _xml_readAttr(state, state->src_alpha.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->src_alpha.val = AIO_GL_BLEND_ONE;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->src_alpha.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_dest_alpha)) {
      char *valStr;

      _xml_readAttr(state, state->dest_alpha.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->dest_alpha.val = AIO_GL_BLEND_ZERO;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlend(valStr);

        if (enumVal != -1)
          state->dest_alpha.val = enumVal;

        xmlFree(valStr);
      }

    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateBlendEqSep(xmlTextReaderPtr __restrict reader,
                          aio_render_state ** __restrict last_state,
                          aio_states ** __restrict states) {
  aio_blend_equation_separate *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE;

  do {
    _xml_beginElement(_s_dae_blend_equation_separate);

    if (_xml_eqElm(_s_dae_rgb)) {
      char *valStr;

      _xml_readAttr(state, state->rgb.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->rgb.val = AIO_GL_BLEND_EQUATION_FUNC_ADD;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlendEq(valStr);

        if (enumVal != -1)
          state->rgb.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_alpha)) {
      char *valStr;

      _xml_readAttr(state, state->alpha.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->alpha.val = AIO_GL_BLEND_EQUATION_FUNC_ADD;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumBlendEq(valStr);

        if (enumVal != -1)
          state->alpha.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateColorMaterial(xmlTextReaderPtr __restrict reader,
                             aio_render_state ** __restrict last_state,
                             aio_states ** __restrict states) {
  aio_color_material *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_COLOR_MATERIAL;

  do {
    _xml_beginElement(_s_dae_color_material);

    if (_xml_eqElm(_s_dae_face)) {
      char *valStr;

      _xml_readAttr(state, state->face.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AIO_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_mode)) {
      char *valStr;

      _xml_readAttr(state, state->mode.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->mode.val = AIO_GL_MATERIAL_AMBIENT_AND_DIFFUSE;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumMaterial(valStr);

        if (enumVal != -1)
          state->mode.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStatePolyMode(xmlTextReaderPtr __restrict reader,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states) {
  aio_polygon_mode *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_POLYGON_MODE;

  do {
    _xml_beginElement(_s_dae_polygon_mode);

    if (_xml_eqElm(_s_dae_face)) {
      char *valStr;

      _xml_readAttr(state, state->face.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AIO_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_mode)) {
      char *valStr;

      _xml_readAttr(state, state->mode.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->mode.val = AIO_GL_POLYGON_MODE_FILL;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumPolyMode(valStr);

        if (enumVal != -1)
          state->mode.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilFunc(xmlTextReaderPtr __restrict reader,
                           aio_render_state ** __restrict last_state,
                           aio_states ** __restrict states) {
  aio_stencil_func *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_STENCIL_FUNC;

  do {
    _xml_beginElement(_s_dae_stencil_func);

    if (_xml_eqElm(_s_dae_stencil_func)) {
      char *valStr;

      _xml_readAttr(state, state->func.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->func.val = AIO_GL_FUNC_ALWAYS;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->func.val = enumVal;

        xmlFree(valStr);
      }

    } else if (_xml_eqElm(_s_dae_ref)) {
      char *valStr;

      _xml_readAttr(state, state->ref.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      if (valStr) {
        state->mask.val = strtol(valStr, NULL, 10);
        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_mask)) {
      char *valStr;

      _xml_readAttr(state, state->mask.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
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
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilOp(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states) {
  aio_stencil_op *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_STENCIL_OP;

  do {
    _xml_beginElement(_s_dae_stencil_op);

    if (_xml_eqElm(_s_dae_fail)) {
      char *valStr;

      _xml_readAttr(state, state->fail.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->fail.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->fail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_zfail)) {
      char *valStr;

      _xml_readAttr(state, state->zfail.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->zfail.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zfail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_zpass)) {
      char *valStr;

      _xml_readAttr(state, state->zpass.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->zpass.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zpass.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilFuncSep(xmlTextReaderPtr __restrict reader,
                              aio_render_state ** __restrict last_state,
                              aio_states ** __restrict states) {
  aio_stencil_func_separate *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE;

  do {
    _xml_beginElement(_s_dae_stencil_func_separate);

    if (_xml_eqElm(_s_dae_front)) {
      char *valStr;

      _xml_readAttr(state, state->front.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->front.val = AIO_GL_FUNC_ALWAYS;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->front.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_back)) {
      char *valStr;

      _xml_readAttr(state, state->back.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->back.val = AIO_GL_FUNC_ALWAYS;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGlFunc(valStr);

        if (enumVal != -1)
          state->back.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_ref)) {
      char *valStr;

      _xml_readAttr(state, state->ref.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      if (valStr) {
        state->mask.val = strtol(valStr, NULL, 10);
        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_mask)) {
      char *valStr;

      _xml_readAttr(state, state->mask.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
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
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilOpSep(xmlTextReaderPtr __restrict reader,
                            aio_render_state ** __restrict last_state,
                            aio_states ** __restrict states) {
  aio_stencil_op_separate *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_STENCIL_OP_SEPARATE;

  do {
    _xml_beginElement(_s_dae_stencil_op_separate);

    if (_xml_eqElm(_s_dae_face)) {
      char *valStr;

      _xml_readAttr(state, state->face.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AIO_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_fail)) {
      char *valStr;

      _xml_readAttr(state, state->fail.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->fail.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->fail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_zfail)) {
      char *valStr;

      _xml_readAttr(state, state->zfail.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->zfail.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zfail.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_zpass)) {
      char *valStr;

      _xml_readAttr(state, state->zpass.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->zpass.val = AIO_GL_STENCIL_OP_KEEP;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumStencilOp(valStr);

        if (enumVal != -1)
          state->zpass.val = enumVal;

        xmlFree(valStr);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilMaskSep(xmlTextReaderPtr __restrict reader,
                              aio_render_state ** __restrict last_state,
                              aio_states ** __restrict states) {
  aio_stencil_mask_separate *state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  state = aio_calloc(*states, sizeof(*state), 1);
  state->base.state_type = AIO_RENDER_STATE_STENCIL_MASK_SEPARATE;

  do {
    _xml_beginElement(_s_dae_stencil_mask_separate);

    if (_xml_eqElm(_s_dae_face)) {
      char *valStr;

      _xml_readAttr(state, state->face.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_value);

      state->face.val = AIO_GL_FACE_FRONT_AND_BACK;
      if (valStr) {
        long enumVal;
        enumVal = aio_dae_fxEnumGLFace(valStr);

        if (enumVal != -1)
          state->face.val = enumVal;

        xmlFree(valStr);
      }
    } else if (_xml_eqElm(_s_dae_mask)) {
      char *valStr;

      _xml_readAttr(state, state->mask.param, _s_dae_param);
      valStr = (char *)xmlTextReaderGetAttribute(reader,
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
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}
