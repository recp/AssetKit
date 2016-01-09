/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_states_detail.h"

#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include "../aio_collada_common.h"
#include "../aio_collada_value.h"
#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_sampler.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

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
aio_dae_fxState_enum(xmlNode * __restrict xml_node,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     long defaultEnumVal,
                     aio_dae_fxEnumFn_t enumFn) {
  aio_state_t_ul * state;
  xmlAttr        * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    const char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      long val;

      val = enumFn(attr_val);
      if (val == -1)
        val = defaultEnumVal;

      state->val = val;
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_bool4(xmlNode * __restrict xml_node,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_bool * defaultVal,
                      size_t defaultValSize) {
  aio_state_t_bool4 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomb(&attr_val, state->val, 1, 4);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_int2(xmlNode * __restrict xml_node,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize) {
  aio_state_t_int2 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomi(&attr_val, state->val, 1, 2);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_int4(xmlNode * __restrict xml_node,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize) {
  aio_state_t_int4 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomi(&attr_val, state->val, 1, 4);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_ul(xmlNode * __restrict xml_node,
                   aio_render_state ** __restrict last_state,
                   aio_states ** __restrict states,
                   long state_type,
                   unsigned long defaultVal) {
  aio_state_t_ul * state;
  xmlAttr        * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    const char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      long val;
      char * tmp;

      val = strtoul(attr_val, &tmp, 10);
      if (*tmp == '\0')
        val = defaultVal;

      state->val = val;
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_ul_i(xmlNode * __restrict xml_node,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     unsigned long defaultVal) {
  aio_state_t_ul_i * state;
  xmlAttr          * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    const char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      long val;
      char * tmp;

      val = strtoul(attr_val, &tmp, 10);
      if (*tmp == '\0')
        val = defaultVal;

      state->val = val;
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float(xmlNode * __restrict xml_node,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_float defaultVal) {
  aio_state_t_float * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val)
        state->val = defaultVal;
      else
        state->val = strtof(attr_val, NULL);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float_i(xmlNode * __restrict xml_node,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type,
                        aio_float defaultVal) {
  aio_state_t_float_i * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val)
        state->val = defaultVal;
      else
        state->val = strtof(attr_val, NULL);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float2(xmlNode * __restrict xml_node,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float2 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, state->val, 1, 2);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float3(xmlNode * __restrict xml_node,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float3 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, state->val, 1, 3);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float3_i(xmlNode * __restrict xml_node,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float3_i * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, state->val, 1, 3);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4(xmlNode * __restrict xml_node,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize) {
  aio_state_t_float4 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, state->val, 1, 4);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4_i(xmlNode * __restrict xml_node,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float4_i * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, state->val, 1, 4);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_float4x4(xmlNode * __restrict xml_node,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize) {
  aio_state_t_float4x4 * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      if (!attr_val && defaultVal)
        memcpy(*state->val, defaultVal, defaultValSize);
      else
        aio_strtomf(&attr_val, *state->val, 4, 4);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_sampler(xmlNode * __restrict xml_node,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type) {
  aio_state_t_sampler * state;
  xmlAttr * curr_attr;
  xmlNode * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
      break;
    }
  }

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "value")) {
      aio_fx_sampler_common * sampler;
      int ret;

      sampler = NULL;
      ret = aio_dae_fxSampler(curr_node, &sampler);

      if (ret == 0)
        state->val = sampler;
    } else if (AIO_IS_EQ_CASE(node_name, "param")) {
      char * node_content;

      node_content = aio_xml_content(curr_node);
      state->param = aio_strdup(node_content);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxState_str(xmlNode * __restrict xml_node,
                    aio_render_state ** __restrict last_state,
                    aio_states ** __restrict states,
                    long state_type) {
  aio_state_t_str * state;
  xmlAttr * curr_attr;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = state_type;

  for (curr_attr = xml_node->properties;
       curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
       curr_attr = curr_attr->next) {
    const char * attr_name;
    char * attr_val;

    attr_name = (const char *)curr_attr->name;
    attr_val = aio_xml_content((xmlNode *)curr_attr);

    if (AIO_IS_EQ_CASE(attr_name, "value")) {
      state->val = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
      state->param = aio_strdup(attr_val);
    } else if (AIO_IS_EQ_CASE(attr_name, "index")) {
      state->index = strtol(attr_val, NULL, 10);
    }
  }

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateAlphaFunc(xmlNode * __restrict xml_node,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states) {
  aio_alpha_func * state;
  xmlNode        * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_ALPHA_FUNC;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "func")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumGlFunc(attr_val);
          if (val == -1)
            val = AIO_GL_FUNC_ALWAYS;

          state->func.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->func.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "value")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value"))
          state->val.val = strtof(attr_val, NULL);
        else if (AIO_IS_EQ_CASE(attr_name, "param"))
          state->val.param = aio_strdup(attr_val);
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateBlend(xmlNode * __restrict xml_node,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states) {
  aio_blend_func * state;
  xmlNode        * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_BLEND_FUNC;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "src")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ONE;

          state->src.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->src.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "desc")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ZERO;

          state->dest.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->dest.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateBlendSep(xmlNode * __restrict xml_node,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states) {
  aio_blend_func_separate * state;
  xmlNode                 * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_BLEND_FUNC_SEPARATE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "src_rgb")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ONE;

          state->src_rgb.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->src_rgb.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "dest_rgb")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ZERO;

          state->dest_rgb.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->dest_rgb.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "src_alpha")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ONE;

          state->src_alpha.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->src_alpha.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "dest_alpha")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlend(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_ZERO;

          state->dest_alpha.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->dest_alpha.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateBlendEqSep(xmlNode * __restrict xml_node,
                          aio_render_state ** __restrict last_state,
                          aio_states ** __restrict states) {
  aio_blend_equation_separate * state;
  xmlNode * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "rgb")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlendEq(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_EQUATION_FUNC_ADD;

          state->rgb.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->rgb.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "alpha")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumBlendEq(attr_val);
          if (val == -1)
            val = AIO_GL_BLEND_EQUATION_FUNC_ADD;

          state->alpha.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->alpha.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateColorMaterial(xmlNode * __restrict xml_node,
                             aio_render_state ** __restrict last_state,
                             aio_states ** __restrict states) {
  aio_color_material * state;
  xmlNode            * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_COLOR_MATERIAL;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "face")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumFace(attr_val);
          if (val == -1)
            val = AIO_GL_FACE_FRONT_AND_BACK;

          state->face.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->face.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "mode")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumMaterial(attr_val);
          if (val == -1)
            val = AIO_GL_MATERIAL_AMBIENT_AND_DIFFUSE;

          state->mode.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->mode.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStatePolyMode(xmlNode * __restrict xml_node,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states) {
  aio_polygon_mode * state;
  xmlNode          * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_POLYGON_MODE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "face")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumLogicOp(attr_val);
          if (val == -1)
            val = AIO_GL_FACE_FRONT_AND_BACK;

          state->face.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->face.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "value")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumPolyMode(attr_val);
          if (val == -1)
            val = AIO_GL_POLYGON_MODE_FILL;

          state->face.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->face.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilFunc(xmlNode * __restrict xml_node,
                           aio_render_state ** __restrict last_state,
                           aio_states ** __restrict states) {
  aio_stencil_func * state;
  xmlNode          * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_STENCIL_FUNC;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "func")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumGlFunc(attr_val);
          if (val == -1)
            val = AIO_GL_FUNC_ALWAYS;

          state->func.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->func.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "ref")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          state->ref.val = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->ref.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "mask")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          char * tmp;
          long   val;

          val = strtol(attr_val, &tmp, 10);
          if (*tmp == '\0')
            val = 255;

          state->mask.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->mask.param = aio_strdup(attr_val);
        }
      }
    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilOp(xmlNode * __restrict xml_node,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states) {
  aio_stencil_op * state;
  xmlNode        * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_STENCIL_OP;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "fail")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->fail.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->fail.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "zfail")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->zfail.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->zfail.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "zpass")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->zpass.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->zpass.param = aio_strdup(attr_val);
        }
      }

    }
  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilFuncSep(xmlNode * __restrict xml_node,
                              aio_render_state ** __restrict last_state,
                              aio_states ** __restrict states) {
  aio_stencil_func_separate * state;
  xmlNode * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "front")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumGlFunc(attr_val);
          if (val == -1)
            val = AIO_GL_FUNC_ALWAYS;

          state->front.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->front.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "back")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumGlFunc(attr_val);
          if (val == -1)
            val = AIO_GL_FUNC_ALWAYS;

          state->back.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->back.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "ref")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          state->ref.val = strtoul(attr_val, NULL, 10);
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->ref.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "mask")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          char * tmp;
          long   val;

          val = strtol(attr_val, &tmp, 10);
          if (*tmp == '\0')
            val = 255;

          state->mask.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->mask.param = aio_strdup(attr_val);
        }
      }
    }

  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilOpSep(xmlNode * __restrict xml_node,
                            aio_render_state ** __restrict last_state,
                            aio_states ** __restrict states) {
  aio_stencil_op_separate * state;
  xmlNode * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_STENCIL_OP_SEPARATE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "face")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumFace(attr_val);
          if (val == -1)
            val = AIO_GL_FACE_FRONT_AND_BACK;

          state->face.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->face.param = aio_strdup(attr_val);
        }
      }

    } else if (AIO_IS_EQ_CASE(node_name, "fail")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->fail.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->fail.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "zfail")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->zfail.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->zfail.param = aio_strdup(attr_val);
        }
      }
    } else if (AIO_IS_EQ_CASE(node_name, "zpass")) {
      xmlAttr * curr_attr;

      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;

        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);

        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;

          val = aio_dae_fxEnumStencilOp(attr_val);
          if (val == -1)
            val = AIO_GL_STENCIL_OP_KEEP;

          state->zpass.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->zpass.param = aio_strdup(attr_val);
        }
      }
    }

  } /* for */

  _AIO_APPEND_STATE(last_state, state);

  return 0;
}

int _assetio_hide
aio_dae_fxStateStencilMaskSep(xmlNode * __restrict xml_node,
                              aio_render_state ** __restrict last_state,
                              aio_states ** __restrict states) {
  aio_stencil_mask_separate * state;
  xmlNode * curr_node;

  state = aio_malloc(sizeof(*state));
  memset(state, '\0', sizeof(*state));

  state->base.state_type = AIO_RENDER_STATE_STENCIL_MASK_SEPARATE;

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    node_name = (const char *)curr_node->name;

    if (AIO_IS_EQ_CASE(node_name, "face")) {
      xmlAttr * curr_attr;
      
      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;
        
        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);
        
        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          long val;
          
          val = aio_dae_fxEnumGlFunc(attr_val);
          if (val == -1)
            val = AIO_GL_FUNC_ALWAYS;
          
          state->face.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->mask.param = aio_strdup(attr_val);
        }
      }
      
    } else if (AIO_IS_EQ_CASE(node_name, "mask")) {
      xmlAttr * curr_attr;
      
      for (curr_attr = curr_node->properties;
           curr_attr && curr_attr->type == XML_ATTRIBUTE_NODE;
           curr_attr = curr_attr->next) {
        const char * attr_name;
        const char * attr_val;
        
        attr_name = (const char *)curr_attr->name;
        attr_val = aio_xml_content((xmlNode *)curr_attr);
        
        if (AIO_IS_EQ_CASE(attr_name, "value")) {
          char * tmp;
          long   val;
          
          val = strtol(attr_val, &tmp, 10);
          if (*tmp == '\0')
            val = 255;
          
          state->mask.val = val;
        } else if (AIO_IS_EQ_CASE(attr_name, "param")) {
          state->mask.param = aio_strdup(attr_val);
        }
      }
    }
    
  } /* for */
  
  _AIO_APPEND_STATE(last_state, state);
  
  return 0;
}
