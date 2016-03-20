/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_states.h"
#include "ak_collada_fx_states_detail.h"
#include "../ak_collada_value.h"
#include "ak_collada_fx_enums.h"
#include "ak_collada_fx_sampler.h"

static ak_enumpair stateMap[] = {
  {_s_dae_alpha_func, AK_RENDER_STATE_ALPHA_FUNC},
  {_s_dae_blend_func, AK_RENDER_STATE_BLEND_FUNC},
  {_s_dae_blend_func_separate, AK_RENDER_STATE_BLEND_FUNC_SEPARATE},
  {_s_dae_blend_equation, AK_RENDER_STATE_BLEND_EQUATION},
  {_s_dae_blend_equation_separate, AK_RENDER_STATE_BLEND_EQUATION_SEPARATE},
  {_s_dae_color_material, AK_RENDER_STATE_COLOR_MATERIAL},
  {_s_dae_cull_face, AK_RENDER_STATE_CULL_FACE},
  {_s_dae_depth_func, AK_RENDER_STATE_DEPTH_FUNC},
  {_s_dae_fog_mode, AK_RENDER_STATE_FOG_MODE},
  {_s_dae_fog_coord_src, AK_RENDER_STATE_FOG_COORD_SRC},
  {_s_dae_front_face, AK_RENDER_STATE_FRONT_FACE},
  {_s_dae_light_model_color_control,
    AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL},
  {_s_dae_logic_op, AK_RENDER_STATE_LOGIC_OP},
  {_s_dae_polygon_mode, AK_RENDER_STATE_POLYGON_MODE},
  {_s_dae_shade_model, AK_RENDER_STATE_SHADE_MODEL},
  {_s_dae_stencil_func, AK_RENDER_STATE_STENCIL_FUNC},
  {_s_dae_stencil_op, AK_RENDER_STATE_STENCIL_OP},
  {_s_dae_stencil_func_separate, AK_RENDER_STATE_STENCIL_FUNC_SEPARATE},
  {_s_dae_stencil_op_separate, AK_RENDER_STATE_STENCIL_OP_SEPARATE},
  {_s_dae_stencil_mask_separate, AK_RENDER_STATE_STENCIL_MASK_SEPARATE},
  {_s_dae_light_enable, AK_RENDER_STATE_LIGHT_ENABLE},
  {_s_dae_light_ambient, AK_RENDER_STATE_LIGHT_AMBIENT},
  {_s_dae_light_diffuse, AK_RENDER_STATE_LIGHT_DIFFUSE},
  {_s_dae_light_specular, AK_RENDER_STATE_LIGHT_SPECULAR},
  {_s_dae_light_position, AK_RENDER_STATE_LIGHT_POSITION},
  {_s_dae_light_constant_attenuation,
    AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION},
  {_s_dae_light_linear_attenuation,
    AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION},
  {_s_dae_light_quadratic_attenuation,
    AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION},
  {_s_dae_light_spot_cutoff, AK_RENDER_STATE_LIGHT_SPOT_CUTOFF},
  {_s_dae_light_spot_direction, AK_RENDER_STATE_LIGHT_SPOT_DIRECTION},
  {_s_dae_light_spot_exponent, AK_RENDER_STATE_LIGHT_SPOT_EXPONENT},
  {_s_dae_texture1D, AK_RENDER_STATE_TEXTURE1D},
  {_s_dae_texture2D, AK_RENDER_STATE_TEXTURE2D},
  {_s_dae_texture3D, AK_RENDER_STATE_TEXTURE3D},
  {_s_dae_textureCUBE, AK_RENDER_STATE_TEXTURECUBE},
  {_s_dae_textureRECT, AK_RENDER_STATE_TEXTURERECT},
  {_s_dae_textureDEPTH, AK_RENDER_STATE_TEXTUREDEPTH},
  {_s_dae_texture1D_enable, AK_RENDER_STATE_TEXTURE1D_ENABLE},
  {_s_dae_texture2D_enable, AK_RENDER_STATE_TEXTURE2D_ENABLE},
  {_s_dae_texture3D_enable, AK_RENDER_STATE_TEXTURE3D_ENABLE},
  {_s_dae_textureCUBE_enable, AK_RENDER_STATE_TEXTURECUBE_ENABLE},
  {_s_dae_textureRECT_enable, AK_RENDER_STATE_TEXTURERECT_ENABLE},
  {_s_dae_textureDEPTH_enable, AK_RENDER_STATE_TEXTUREDEPTH_ENABLE},
  {_s_dae_texture_env_color, AK_RENDER_STATE_TEXTURE_ENV_COLOR},
  {_s_dae_texture_env_mode, AK_RENDER_STATE_TEXTURE_ENV_MODE},
  {_s_dae_clip_plane, AK_RENDER_STATE_CLIP_PLANE},
  {_s_dae_clip_plane_enable, AK_RENDER_STATE_CLIP_PLANE_ENABLE},
  {_s_dae_blend_color, AK_RENDER_STATE_BLEND_COLOR},
  {_s_dae_color_mask, AK_RENDER_STATE_COLOR_MASK},
  {_s_dae_depth_bounds, AK_RENDER_STATE_DEPTH_BOUNDS},
  {_s_dae_depth_mask, AK_RENDER_STATE_DEPTH_MASK},
  {_s_dae_depth_range, AK_RENDER_STATE_DEPTH_RANGE},
  {_s_dae_fog_density, AK_RENDER_STATE_FOG_DENSITY},
  {_s_dae_fog_start, AK_RENDER_STATE_FOG_START},
  {_s_dae_fog_end, AK_RENDER_STATE_FOG_END},
  {_s_dae_fog_color, AK_RENDER_STATE_FOG_COLOR},
  {_s_dae_light_model_ambient, AK_RENDER_STATE_LIGHT_MODEL_AMBIENT},
  {_s_dae_lighting_enable, AK_RENDER_STATE_LIGHTING_ENABLE},
  {_s_dae_line_stipple, AK_RENDER_STATE_LINE_STIPPLE},
  {_s_dae_line_width, AK_RENDER_STATE_LINE_WIDTH},
  {_s_dae_material_ambient, AK_RENDER_STATE_MATERIAL_AMBIENT},
  {_s_dae_material_diffuse, AK_RENDER_STATE_MATERIAL_DIFFUSE},
  {_s_dae_material_emission, AK_RENDER_STATE_MATERIAL_EMISSION},
  {_s_dae_material_shininess, AK_RENDER_STATE_MATERIAL_SHININESS},
  {_s_dae_material_specular, AK_RENDER_STATE_MATERIAL_SPECULAR},
  {_s_dae_model_view_matrix, AK_RENDER_STATE_MODEL_VIEW_MATRIX},
  {_s_dae_point_distance_attenuation,
    AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION},
  {_s_dae_point_fade_threshold_size,
    AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE},
  {_s_dae_point_size, AK_RENDER_STATE_POINT_SIZE},
  {_s_dae_point_size_min, AK_RENDER_STATE_POINT_SIZE_MIN},
  {_s_dae_point_size_max, AK_RENDER_STATE_POINT_SIZE_MAX},
  {_s_dae_polygon_offset, AK_RENDER_STATE_POLYGON_OFFSET},
  {_s_dae_projection_matrix, AK_RENDER_STATE_PROJECTION_MATRIX},
  {_s_dae_scissor, AK_RENDER_STATE_SCISSOR},
  {_s_dae_stencil_mask, AK_RENDER_STATE_STENCIL_MASK},
  {_s_dae_alpha_test_enable, AK_RENDER_STATE_ALPHA_TEST_ENABLE},
  {_s_dae_blend_enable, AK_RENDER_STATE_BLEND_ENABLE},
  {_s_dae_color_logic_op_enable, AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE},
  {_s_dae_color_material_enable, AK_RENDER_STATE_COLOR_MATERIAL_ENABLE},
  {_s_dae_cull_face_enable, AK_RENDER_STATE_CULL_FACE_ENABLE},
  {_s_dae_depth_bounds_enable, AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE},
  {_s_dae_depth_clamp_enable, AK_RENDER_STATE_DEPTH_CLAMP_ENABLE},
  {_s_dae_depth_test_enable, AK_RENDER_STATE_DEPTH_TEST_ENABLE},
  {_s_dae_dither_enable, AK_RENDER_STATE_DITHER_ENABLE},
  {_s_dae_fog_enable, AK_RENDER_STATE_FOG_ENABLE},
  {_s_dae_light_model_local_viewer_enable,
    AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE},
  {_s_dae_light_model_two_side_enable,
    AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE},
  {_s_dae_line_smooth_enable,AK_RENDER_STATE_LINE_SMOOTH_ENABLE},
  {_s_dae_line_stipple_enable, AK_RENDER_STATE_LINE_STIPPLE_ENABLE},
  {_s_dae_logic_op_enable, AK_RENDER_STATE_LOGIC_OP_ENABLE},
  {_s_dae_multisample_enable, AK_RENDER_STATE_MULTISAMPLE_ENABLE},
  {_s_dae_normalize_enable, AK_RENDER_STATE_NORMALIZE_ENABLE},
  {_s_dae_point_smooth_enable, AK_RENDER_STATE_POINT_SMOOTH_ENABLE},
  {_s_dae_polygon_offset_fill_enable,
    AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE},
  {_s_dae_polygon_offset_line_enable,
    AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE},
  {_s_dae_polygon_offset_point_enable,
    AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE},
  {_s_dae_polygon_smooth_enable, AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE},
  {_s_dae_polygon_stipple_enable, AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE},
  {_s_dae_rescale_normal_enable, AK_RENDER_STATE_RESCALE_NORMAL_ENABLE},
  {_s_dae_sample_alpha_to_coverage_enable,
    AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE},
  {_s_dae_sample_alpha_to_one_enable,
    AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE},
  {_s_dae_sample_coverage_enable, AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE},
  {_s_dae_scissor_test_enable, AK_RENDER_STATE_SCISSOR_TEST_ENABLE},
  {_s_dae_stencil_test_enable, AK_RENDER_STATE_STENCIL_TEST_ENABLE}
};

static size_t stateMapLen = 0;

AkResult _assetkit_hide
ak_dae_fxState(void * __restrict memParent,
                xmlTextReaderPtr reader,
                ak_states ** __restrict dest) {
  ak_states       *states;
  AkRenderState *last_state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  states = ak_calloc(memParent, sizeof(*states), 1);

  if (stateMapLen == 0) {
    stateMapLen = ak_ARRAY_LEN(stateMap);
    qsort(stateMap,
          stateMapLen,
          sizeof(stateMap[0]),
          ak_enumpair_cmp);
  }

  last_state = NULL;

  do {
    const ak_enumpair *found;

    _xml_beginElement(_s_dae_states);

    found = bsearch(nodeName,
                    stateMap,
                    stateMapLen,
                    sizeof(stateMap[0]),
                    ak_enumpair_cmp2);

    switch (found->val) {
      case AK_RENDER_STATE_ALPHA_FUNC:
        ak_dae_fxStateAlphaFunc(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_FUNC:
        ak_dae_fxStateBlend(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_FUNC_SEPARATE:
        ak_dae_fxStateBlendSep(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_EQUATION:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_BLEND_EQUATION,
                             AK_GL_BLEND_EQUATION_FUNC_ADD,
                             ak_dae_fxEnumBlendEq);
        break;
      case AK_RENDER_STATE_BLEND_EQUATION_SEPARATE:
        ak_dae_fxStateBlendEqSep(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_COLOR_MATERIAL:
        ak_dae_fxStateColorMaterial(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_CULL_FACE:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_CULL_FACE,
                             AK_GL_FACE_BACK,
                             ak_dae_fxEnumGLFace);
        break;
      case AK_RENDER_STATE_DEPTH_FUNC:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_DEPTH_FUNC,
                             AK_GL_FUNC_ALWAYS,
                             ak_dae_fxEnumGlFunc);
        break;
      case AK_RENDER_STATE_FOG_MODE:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_FOG_MODE,
                             AK_GL_FOG_EXP,
                             ak_dae_fxEnumFog);
        break;
      case AK_RENDER_STATE_FOG_COORD_SRC:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_FOG_COORD_SRC,
                             AK_GL_FOG_COORD_SRC_FOG_COORDINATE,
                             ak_dae_fxEnumFogCoordSrc);
        break;
      case AK_RENDER_STATE_FRONT_FACE:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_FRONT_FACE,
                             AK_GL_FRONT_FACE_CCW,
                             ak_dae_fxEnumFrontFace);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL,
                             AK_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR,
                             ak_dae_fxEnumLightModelColorCtl);
        break;
      case AK_RENDER_STATE_LOGIC_OP:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LOGIC_OP,
                             AK_GL_LOGIC_OP_COPY,
                             ak_dae_fxEnumLogicOp);
        break;
      case AK_RENDER_STATE_POLYGON_MODE:
        ak_dae_fxStatePolyMode(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_SHADE_MODEL:
        ak_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_SHADE_MODEL,
                             AK_GL_SHADE_MODEL_SMOOTH,
                             ak_dae_fxEnumShadeModel);
        break;
      case AK_RENDER_STATE_STENCIL_FUNC:
        ak_dae_fxStateStencilFunc(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_OP:
        ak_dae_fxStateStencilOp(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_FUNC_SEPARATE:
        ak_dae_fxStateStencilFuncSep(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_OP_SEPARATE:
        ak_dae_fxStateStencilOpSep(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_MASK_SEPARATE:
        ak_dae_fxStateStencilMaskSep(reader, &last_state, &states);
        break;
      case AK_RENDER_STATE_LIGHT_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LIGHT_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LIGHT_AMBIENT:
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_LIGHT_AMBIENT,
                                 (ak_float4){0.0f, 0.0f, 0.0f, 1.0f},
                                 sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHT_DIFFUSE: 
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_LIGHT_DIFFUSE,
                                 (ak_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHT_SPECULAR:
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_LIGHT_SPECULAR,
                                 (ak_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHT_POSITION:
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_LIGHT_POSITION,
                                 (ak_float4){0.0f, 0.0f, 1.0f, 0.0f},
                                 sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION:
        ak_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION,
                                1);
        break;
      case AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION:
        ak_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION,
                                0);
        break;
      case AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION:
        ak_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION,
                                0);
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_CUTOFF:
        ak_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_LIGHT_SPOT_CUTOFF,
                                180);
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_DIRECTION:
        ak_dae_fxState_float3_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_LIGHT_SPOT_DIRECTION,
                                 (ak_float3){0.0f, 0.0f, -1.0f},
                                 sizeof(ak_float3));
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_EXPONENT:
        ak_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_LIGHT_SPOT_EXPONENT,
                                0);
        break;
      case AK_RENDER_STATE_TEXTURE1D:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTURE1D);
        break;
      case AK_RENDER_STATE_TEXTURE2D:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTURE2D);
        break;
      case AK_RENDER_STATE_TEXTURE3D:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTURE3D);
        break;
      case AK_RENDER_STATE_TEXTURECUBE:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTURECUBE);
        break;
      case AK_RENDER_STATE_TEXTURERECT:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTURERECT);
        break;
      case AK_RENDER_STATE_TEXTUREDEPTH:
        ak_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AK_RENDER_STATE_TEXTUREDEPTH);
        break;
      case AK_RENDER_STATE_TEXTURE1D_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTURE1D_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTURE2D_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTURE2D_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTURE3D_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTURE3D_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTURECUBE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTURECUBE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTURERECT_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTURERECT_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTUREDEPTH_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_TEXTUREDEPTH_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_TEXTURE_ENV_COLOR:
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_TEXTURE_ENV_COLOR,
                                 NULL,
                                 0);
        break;
      case AK_RENDER_STATE_TEXTURE_ENV_MODE:
        ak_dae_fxState_str(reader,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURE_ENV_MODE);
        break;
      case AK_RENDER_STATE_CLIP_PLANE:
        ak_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_CLIP_PLANE,
                                 (ak_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_CLIP_PLANE_ENABLE:
        ak_dae_fxState_ul_i(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_CLIP_PLANE_ENABLE,
                             0);
        break;
      case AK_RENDER_STATE_BLEND_COLOR:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_BLEND_COLOR,
                               (ak_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_COLOR_MASK:
        ak_dae_fxState_bool4(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_COLOR_MASK,
                              (ak_bool4){1, 1, 1, 1},
                              sizeof(ak_bool4));
        break;
      case AK_RENDER_STATE_DEPTH_BOUNDS:
        ak_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_DEPTH_BOUNDS,
                               NULL,
                               0);
        break;
      case AK_RENDER_STATE_DEPTH_MASK:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_MASK,
                           1);
        break;
      case AK_RENDER_STATE_DEPTH_RANGE:
        ak_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_DEPTH_RANGE,
                               (ak_float2){0, 1},
                               sizeof(ak_float2));
        break;
      case AK_RENDER_STATE_FOG_DENSITY:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_FOG_DENSITY,
                              1.0f);
        break;
      case AK_RENDER_STATE_FOG_START:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_FOG_START,
                              0.0f);
        break;
      case AK_RENDER_STATE_FOG_END:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_FOG_END,
                              1.0f);
        break;
      case AK_RENDER_STATE_FOG_COLOR:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_FOG_COLOR,
                               (ak_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_AMBIENT:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_LIGHT_MODEL_AMBIENT,
                               (ak_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_LIGHTING_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LIGHTING_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LINE_STIPPLE:
        ak_dae_fxState_int2(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LINE_STIPPLE,
                             (ak_int2){1, 65536},
                             sizeof(ak_int2));
        break;
      case AK_RENDER_STATE_LINE_WIDTH:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_LINE_WIDTH,
                              1.0f);
        break;
      case AK_RENDER_STATE_MATERIAL_AMBIENT:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_MATERIAL_AMBIENT,
                               (ak_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_MATERIAL_DIFFUSE:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_MATERIAL_DIFFUSE,
                               (ak_float4){0.8f, 0.8f, 0.8f, 1.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_MATERIAL_EMISSION:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_MATERIAL_EMISSION,
                               (ak_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_MATERIAL_SHININESS:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_MATERIAL_SHININESS,
                              0.0f);
        break;
      case AK_RENDER_STATE_MATERIAL_SPECULAR:
        ak_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_MATERIAL_SPECULAR,
                               (ak_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(ak_float4));
        break;
      case AK_RENDER_STATE_MODEL_VIEW_MATRIX:
        ak_dae_fxState_float4x4(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_MODEL_VIEW_MATRIX,
                                 *(ak_float4x4){
                                   {1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}
                                 },
                                 sizeof(ak_float4x4));
        break;
      case AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION:
        ak_dae_fxState_float3(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION,
                               (ak_float3){1.0f, 0.0f, 0.0f},
                               sizeof(ak_float3));
        break;
      case AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE,
                              1.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_POINT_SIZE,
                              1.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE_MIN:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_POINT_SIZE_MIN,
                              0.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE_MAX:
        ak_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AK_RENDER_STATE_POINT_SIZE_MAX,
                              1.0f);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET:
        ak_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AK_RENDER_STATE_POLYGON_OFFSET,
                               (ak_float2){0.0f, 0.0f},
                               sizeof(ak_float2));
        break;
      case AK_RENDER_STATE_PROJECTION_MATRIX:
        ak_dae_fxState_float4x4(reader,
                                 &last_state,
                                 &states,
                                 AK_RENDER_STATE_PROJECTION_MATRIX,
                                 *(ak_float4x4){
                                   {1.0f, 0.0f, 0.0f, 0.0f},
                                   {0.0f, 1.0f, 0.0f, 0.0f},
                                   {0.0f, 0.0f, 1.0f, 0.0f},
                                   {0.0f, 0.0f, 0.0f, 1.0f}
                                 },
                                 sizeof(ak_float4x4));
        break;
      case AK_RENDER_STATE_SCISSOR:
        ak_dae_fxState_int4(reader,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_SCISSOR,
                             NULL,
                             0);
        break;
      case AK_RENDER_STATE_STENCIL_MASK:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_STENCIL_MASK,
                           4294967295);
        break;
      case AK_RENDER_STATE_ALPHA_TEST_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_ALPHA_TEST_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_BLEND_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_BLEND_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_COLOR_MATERIAL_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_COLOR_MATERIAL_ENABLE,
                           1);
        break;
      case AK_RENDER_STATE_CULL_FACE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_CULL_FACE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_DEPTH_CLAMP_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_CLAMP_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_DEPTH_TEST_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_TEST_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_DITHER_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DITHER_ENABLE,
                           1);
        break;
      case AK_RENDER_STATE_FOG_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_FOG_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LINE_SMOOTH_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LINE_SMOOTH_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LINE_STIPPLE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LINE_STIPPLE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_LOGIC_OP_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LOGIC_OP_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_MULTISAMPLE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_MULTISAMPLE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_NORMALIZE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_NORMALIZE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POINT_SMOOTH_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POINT_SMOOTH_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_RESCALE_NORMAL_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_RESCALE_NORMAL_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_SCISSOR_TEST_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_SCISSOR_TEST_ENABLE,
                           0);
        break;
      case AK_RENDER_STATE_STENCIL_TEST_ENABLE:
        ak_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_STENCIL_TEST_ENABLE,
                           0);
        break;

      default:
        _xml_skipElement;
        break;
      }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = states;

  return AK_OK;
}
