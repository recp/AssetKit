/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_states.h"
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

typedef struct {
  const char * key;
  aio_render_state_type val;
} aio_render_state_pair;

int _assetio_hide
state_pair_cmp(const void *, const void *);

int _assetio_hide
state_pair_cmp2(const void *, const void *);

int _assetio_hide
aio_dae_fxState(xmlNode * __restrict xml_node,
                aio_states ** __restrict dest) {
  xmlNode          * curr_node;
  aio_states       * states;
  aio_render_state * last_state;
  size_t             stateMapLen;

  states = aio_malloc(sizeof(*states));
  memset(states, '\0', sizeof(*states));

  last_state = NULL;

  aio_render_state_pair stateMap[] = {
    {"alpha_func", AIO_RENDER_STATE_ALPHA_FUNC},
    {"blend_func", AIO_RENDER_STATE_BLEND_FUNC},
    {"blend_func_separate", AIO_RENDER_STATE_BLEND_FUNC_SEPARATE},
    {"blend_equation", AIO_RENDER_STATE_BLEND_EQUATION},
    {"blend_equation_separate", AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE},
    {"color_material", AIO_RENDER_STATE_COLOR_MATERIAL},
    {"cull_face", AIO_RENDER_STATE_CULL_FACE},
    {"depth_func", AIO_RENDER_STATE_DEPTH_FUNC},
    {"fog_mode", AIO_RENDER_STATE_FOG_MODE},
    {"fog_coord_src", AIO_RENDER_STATE_FOG_COORD_SRC},
    {"front_face", AIO_RENDER_STATE_FRONT_FACE},
    {"light_model_color_control", AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL},
    {"logic_op", AIO_RENDER_STATE_LOGIC_OP},
    {"polygon_mode", AIO_RENDER_STATE_POLYGON_MODE},
    {"shade_model", AIO_RENDER_STATE_SHADE_MODEL},
    {"stencil_func", AIO_RENDER_STATE_STENCIL_FUNC},
    {"stencil_op", AIO_RENDER_STATE_STENCIL_OP},
    {"stencil_func_separate", AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE},
    {"stencil_op_separate", AIO_RENDER_STATE_STENCIL_OP_SEPARATE},
    {"stencil_mask_separate", AIO_RENDER_STATE_STENCIL_MASK_SEPARATE},
    {"light_enable", AIO_RENDER_STATE_LIGHT_ENABLE},
    {"light_ambient", AIO_RENDER_STATE_LIGHT_AMBIENT},
    {"light_diffuse", AIO_RENDER_STATE_LIGHT_DIFFUSE},
    {"light_specular", AIO_RENDER_STATE_LIGHT_SPECULAR},
    {"light_position", AIO_RENDER_STATE_LIGHT_POSITION},
    {"light_constant_attenuation", AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION},
    {"light_linear_attenuation", AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION},
    {"light_quadratic_attenuation", AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION},
    {"light_spot_cutoff", AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF},
    {"light_spot_direction", AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION},
    {"light_spot_exponent", AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT},
    {"texture1D", AIO_RENDER_STATE_TEXTURE1D},
    {"texture2D", AIO_RENDER_STATE_TEXTURE2D},
    {"texture3D", AIO_RENDER_STATE_TEXTURE3D},
    {"textureCUBE", AIO_RENDER_STATE_TEXTURECUBE},
    {"textureRECT", AIO_RENDER_STATE_TEXTURERECT},
    {"textureDEPTH", AIO_RENDER_STATE_TEXTUREDEPTH},
    {"texture1D_enable", AIO_RENDER_STATE_TEXTURE1D_ENABLE},
    {"texture2D_enable", AIO_RENDER_STATE_TEXTURE2D_ENABLE},
    {"texture3D_enable", AIO_RENDER_STATE_TEXTURE3D_ENABLE},
    {"textureCUBE_enable", AIO_RENDER_STATE_TEXTURECUBE_ENABLE},
    {"textureRECT_enable", AIO_RENDER_STATE_TEXTURERECT_ENABLE},
    {"textureDEPTH_enable", AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE},
    {"texture_env_color", AIO_RENDER_STATE_TEXTURE_ENV_COLOR},
    {"texture_env_mode", AIO_RENDER_STATE_TEXTURE_ENV_MODE},
    {"clip_plane", AIO_RENDER_STATE_CLIP_PLANE},
    {"clip_plane_enable", AIO_RENDER_STATE_CLIP_PLANE_ENABLE},
    {"blend_color", AIO_RENDER_STATE_BLEND_COLOR},
    {"color_mask", AIO_RENDER_STATE_COLOR_MASK},
    {"depth_bounds", AIO_RENDER_STATE_DEPTH_BOUNDS},
    {"depth_mask", AIO_RENDER_STATE_DEPTH_MASK},
    {"depth_range", AIO_RENDER_STATE_DEPTH_RANGE},
    {"fog_density", AIO_RENDER_STATE_FOG_DENSITY},
    {"fog_start", AIO_RENDER_STATE_FOG_START},
    {"fog_end", AIO_RENDER_STATE_FOG_END},
    {"fog_color", AIO_RENDER_STATE_FOG_COLOR},
    {"light_model_ambient", AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT},
    {"lighting_enable", AIO_RENDER_STATE_LIGHTING_ENABLE},
    {"line_stipple", AIO_RENDER_STATE_LINE_STIPPLE},
    {"line_width", AIO_RENDER_STATE_LINE_WIDTH},
    {"material_ambient", AIO_RENDER_STATE_MATERIAL_AMBIENT},
    {"material_diffuse", AIO_RENDER_STATE_MATERIAL_DIFFUSE},
    {"material_emission", AIO_RENDER_STATE_MATERIAL_EMISSION},
    {"material_shininess", AIO_RENDER_STATE_MATERIAL_SHININESS},
    {"material_specular", AIO_RENDER_STATE_MATERIAL_SPECULAR},
    {"model_view_matrix", AIO_RENDER_STATE_MODEL_VIEW_MATRIX},
    {"point_distance_attenuation", AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION},
    {"point_fade_threshold_size", AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE},
    {"point_size", AIO_RENDER_STATE_POINT_SIZE},
    {"point_size_min", AIO_RENDER_STATE_POINT_SIZE_MIN},
    {"point_size_max", AIO_RENDER_STATE_POINT_SIZE_MAX},
    {"polygon_offset", AIO_RENDER_STATE_POLYGON_OFFSET},
    {"projection_matrix", AIO_RENDER_STATE_PROJECTION_MATRIX},
    {"scissor", AIO_RENDER_STATE_SCISSOR},
    {"stencil_mask", AIO_RENDER_STATE_STENCIL_MASK},
    {"alpha_test_enable", AIO_RENDER_STATE_ALPHA_TEST_ENABLE},
    {"blend_enable", AIO_RENDER_STATE_BLEND_ENABLE},
    {"color_logic_op_enable", AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE},
    {"color_material_enable", AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE},
    {"cull_face_enable", AIO_RENDER_STATE_CULL_FACE_ENABLE},
    {"depth_bounds_enable", AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE},
    {"depth_clamp_enable", AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE},
    {"depth_test_enable", AIO_RENDER_STATE_DEPTH_TEST_ENABLE},
    {"dither_enable", AIO_RENDER_STATE_DITHER_ENABLE},
    {"fog_enable", AIO_RENDER_STATE_FOG_ENABLE},
    {"light_model_local_viewer_enable",
      AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE},
    {"light_model_two_side_enable",
      AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE},
    {"line_smooth_enable",AIO_RENDER_STATE_LINE_SMOOTH_ENABLE},
    {"line_stipple_enable", AIO_RENDER_STATE_LINE_STIPPLE_ENABLE},
    {"logic_op_enable", AIO_RENDER_STATE_LOGIC_OP_ENABLE},
    {"multisample_enable", AIO_RENDER_STATE_MULTISAMPLE_ENABLE},
    {"normalize_enable", AIO_RENDER_STATE_NORMALIZE_ENABLE},
    {"point_smooth_enable", AIO_RENDER_STATE_POINT_SMOOTH_ENABLE},
    {"polygon_offset_fill_enable", AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE},
    {"polygon_offset_line_enable", AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE},
    {"polygon_offset_point_enable",
      AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE},
    {"polygon_smooth_enable", AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE},
    {"polygon_stipple_enable", AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE},
    {"rescale_normal_enable", AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE},
    {"sample_alpha_to_coverage_enable",
      AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE},
    {"sample_alpha_to_one_enable", AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE},
    {"sample_coverage_enable", AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE},
    {"scissor_test_enable", AIO_RENDER_STATE_SCISSOR_TEST_ENABLE},
    {"stencil_test_enable", AIO_RENDER_STATE_STENCIL_TEST_ENABLE}
  };

  stateMapLen = AIO_ARRAY_LEN(stateMap);

  qsort(stateMap,
        stateMapLen,
        sizeof(stateMap[0]),
        state_pair_cmp);

  for (curr_node = xml_node->children;
       curr_node && curr_node->type == XML_ELEMENT_NODE;
       curr_node = curr_node->next) {

    const char * node_name;
    aio_render_state_pair * found;

    node_name = (const char *)curr_node->name;

    found = bsearch(node_name,
                    stateMap,
                    stateMapLen,
                    sizeof(stateMap[0]),
                    state_pair_cmp2);

    switch (found->val) {
      case AIO_RENDER_STATE_ALPHA_FUNC:
        aio_dae_fxStateAlphaFunc(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_FUNC:
        aio_dae_fxStateBlend(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_FUNC_SEPARATE:
        aio_dae_fxStateBlendSep(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_EQUATION:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_BLEND_EQUATION,
                             AIO_GL_BLEND_EQUATION_FUNC_ADD,
                             aio_dae_fxEnumBlendEq);
        break;
      case AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE:
        aio_dae_fxStateBlendEqSep(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_COLOR_MATERIAL:
        aio_dae_fxStateColorMaterial(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_CULL_FACE:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_CULL_FACE,
                             AIO_GL_FACE_BACK,
                             aio_dae_fxEnumGLFace);
        break;
      case AIO_RENDER_STATE_DEPTH_FUNC:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_DEPTH_FUNC,
                             AIO_GL_FUNC_ALWAYS,
                             aio_dae_fxEnumGlFunc);
        break;
      case AIO_RENDER_STATE_FOG_MODE:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FOG_MODE,
                             AIO_GL_FOG_EXP,
                             aio_dae_fxEnumFog);
        break;
      case AIO_RENDER_STATE_FOG_COORD_SRC:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FOG_COORD_SRC,
                             AIO_GL_FOG_COORD_SRC_FOG_COORDINATE,
                             aio_dae_fxEnumFogCoordSrc);
        break;
      case AIO_RENDER_STATE_FRONT_FACE:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FRONT_FACE,
                             AIO_GL_FRONT_FACE_CCW,
                             aio_dae_fxEnumFrontFace);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL,
                             AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR,
                             aio_dae_fxEnumLightModelColorCtl);
        break;
      case AIO_RENDER_STATE_LOGIC_OP:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LOGIC_OP,
                             AIO_GL_LOGIC_OP_COPY,
                             aio_dae_fxEnumLogicOp);
        break;
      case AIO_RENDER_STATE_POLYGON_MODE:
        aio_dae_fxStatePolyMode(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_SHADE_MODEL:
        aio_dae_fxState_enum(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_SHADE_MODEL,
                             AIO_GL_SHADE_MODEL_SMOOTH,
                             aio_dae_fxEnumShadeModel);
        break;
      case AIO_RENDER_STATE_STENCIL_FUNC:
        aio_dae_fxStateStencilFunc(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_OP:
        aio_dae_fxStateStencilOp(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE:
        aio_dae_fxStateStencilFuncSep(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_OP_SEPARATE:
        aio_dae_fxStateStencilOpSep(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_MASK_SEPARATE:
        aio_dae_fxStateStencilMaskSep(curr_node, &last_state, &states);
        break;
      case AIO_RENDER_STATE_LIGHT_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_AMBIENT:
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_AMBIENT,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_DIFFUSE: {
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_DIFFUSE,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_SPECULAR:
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_SPECULAR,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_POSITION:
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_POSITION,
                                 (aio_float4){0.0f, 0.0f, 1.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION:
        aio_dae_fxState_float_i(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION,
                                1);
        break;
      case AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION:
        aio_dae_fxState_float_i(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION,
                                0);
        break;
      case AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION:
        aio_dae_fxState_float_i(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION,
                                0);
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF:
        aio_dae_fxState_float_i(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF,
                                180);
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION:
        aio_dae_fxState_float3_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION,
                                 (aio_float3){0.0f, 0.0f, -1.0f},
                                 sizeof(aio_float3));
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT:
        aio_dae_fxState_float_i(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT,
                                0);
        break;
      case AIO_RENDER_STATE_TEXTURE1D:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE1D);
        break;
      case AIO_RENDER_STATE_TEXTURE2D:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE2D);
        break;
      case AIO_RENDER_STATE_TEXTURE3D:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE3D);
        break;
      case AIO_RENDER_STATE_TEXTURECUBE:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURECUBE);
        break;
      case AIO_RENDER_STATE_TEXTURERECT:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURERECT);
        break;
      case AIO_RENDER_STATE_TEXTUREDEPTH:
        aio_dae_fxState_sampler(curr_node,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTUREDEPTH);
        break;
      case AIO_RENDER_STATE_TEXTURE1D_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE1D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE2D_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE2D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE3D_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE3D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURECUBE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURECUBE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURERECT_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURERECT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE_ENV_COLOR:
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_TEXTURE_ENV_COLOR,
                                 NULL,
                                 0);
        break;
      case AIO_RENDER_STATE_TEXTURE_ENV_MODE:
        aio_dae_fxState_str(curr_node,
                            &last_state,
                            &states,
                            AIO_RENDER_STATE_TEXTURE_ENV_MODE);
        break;
      case AIO_RENDER_STATE_CLIP_PLANE:
        aio_dae_fxState_float4_i(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_CLIP_PLANE,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_CLIP_PLANE_ENABLE:
        aio_dae_fxState_ul_i(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_CLIP_PLANE_ENABLE,
                             0);
        break;
      case AIO_RENDER_STATE_BLEND_COLOR:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_BLEND_COLOR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_COLOR_MASK:
        aio_dae_fxState_bool4(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_COLOR_MASK,
                              (aio_bool4){1, 1, 1, 1},
                              sizeof(aio_bool4));
        break;
      case AIO_RENDER_STATE_DEPTH_BOUNDS:
        aio_dae_fxState_float2(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_DEPTH_BOUNDS,
                               NULL,
                               0);
        break;
      case AIO_RENDER_STATE_DEPTH_MASK:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_MASK,
                           1);
        break;
      case AIO_RENDER_STATE_DEPTH_RANGE:
        aio_dae_fxState_float2(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_DEPTH_RANGE,
                               (aio_float2){0, 1},
                               sizeof(aio_float2));
        break;
      case AIO_RENDER_STATE_FOG_DENSITY:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_DENSITY,
                              1.0f);
        break;
      case AIO_RENDER_STATE_FOG_START:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_START,
                              0.0f);
        break;
      case AIO_RENDER_STATE_FOG_END:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_END,
                              1.0f);
        break;
      case AIO_RENDER_STATE_FOG_COLOR:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_FOG_COLOR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT,
                               (aio_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHTING_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHTING_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_STIPPLE:
        aio_dae_fxState_int2(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LINE_STIPPLE,
                             (aio_int2){1, 65536},
                             sizeof(aio_int2));
        break;
      case AIO_RENDER_STATE_LINE_WIDTH:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_LINE_WIDTH,
                              1.0f);
        break;
      case AIO_RENDER_STATE_MATERIAL_AMBIENT:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_AMBIENT,
                               (aio_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_DIFFUSE:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_DIFFUSE,
                               (aio_float4){0.8f, 0.8f, 0.8f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_EMISSION:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_EMISSION,
                               (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_SHININESS:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_MATERIAL_SHININESS,
                              0.0f);
        break;
      case AIO_RENDER_STATE_MATERIAL_SPECULAR:
        aio_dae_fxState_float4(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_SPECULAR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MODEL_VIEW_MATRIX:
        aio_dae_fxState_float4x4(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_MODEL_VIEW_MATRIX,
                                 *(aio_float4x4){
                                   {1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}
                                 },
                                 sizeof(aio_float4x4));
        break;
      case AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION:
        aio_dae_fxState_float3(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION,
                               (aio_float3){1.0f, 0.0f, 0.0f},
                               sizeof(aio_float3));
        break;
      case AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE_MIN:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE_MIN,
                              0.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE_MAX:
        aio_dae_fxState_float(curr_node,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE_MAX,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET:
        aio_dae_fxState_float2(curr_node,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_POLYGON_OFFSET,
                               (aio_float2){0.0f, 0.0f},
                               sizeof(aio_float2));
        break;
      case AIO_RENDER_STATE_PROJECTION_MATRIX:
        aio_dae_fxState_float4x4(curr_node,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_PROJECTION_MATRIX,
                                 *(aio_float4x4){
                                   {1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}
                                 },
                                 sizeof(aio_float4x4));
        break;
      case AIO_RENDER_STATE_SCISSOR:
        aio_dae_fxState_int4(curr_node,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_SCISSOR,
                             NULL,
                             0);
        break;
      case AIO_RENDER_STATE_STENCIL_MASK:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_STENCIL_MASK,
                           4294967295);
        break;
      case AIO_RENDER_STATE_ALPHA_TEST_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_ALPHA_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_BLEND_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_BLEND_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE,
                           1);
        break;
      case AIO_RENDER_STATE_CULL_FACE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_CULL_FACE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_TEST_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DITHER_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DITHER_ENABLE,
                           1);
        break;
      case AIO_RENDER_STATE_FOG_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_FOG_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_SMOOTH_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LINE_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_STIPPLE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LINE_STIPPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LOGIC_OP_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LOGIC_OP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_MULTISAMPLE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_MULTISAMPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_NORMALIZE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_NORMALIZE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POINT_SMOOTH_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POINT_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SCISSOR_TEST_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SCISSOR_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_STENCIL_TEST_ENABLE:
        aio_dae_fxState_ul(curr_node,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_STENCIL_TEST_ENABLE,
                           0);
        break;

      default:
        break;
    }
    }
  }
  
  *dest = states;
  
  return 0;
}

int _assetio_hide
state_pair_cmp(const void * a, const void * b) {
  const aio_render_state_pair * _a = a;
  const aio_render_state_pair * _b = b;

  return strcmp(_a->key, _b->key);
}

int _assetio_hide
state_pair_cmp2(const void * a, const void * b) {
  const char * _a = a;
  const aio_render_state_pair * _b = b;

  return strcmp(_a, _b->key);
}
