/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_states.h"
#include "aio_collada_fx_states_detail.h"
#include "../aio_collada_common.h"
#include "../aio_collada_value.h"
#include "aio_collada_fx_enums.h"
#include "aio_collada_fx_sampler.h"

static aio_enumpair stateMap[] = {
  {_s_dae_alpha_func, AIO_RENDER_STATE_ALPHA_FUNC},
  {_s_dae_blend_func, AIO_RENDER_STATE_BLEND_FUNC},
  {_s_dae_blend_func_separate, AIO_RENDER_STATE_BLEND_FUNC_SEPARATE},
  {_s_dae_blend_equation, AIO_RENDER_STATE_BLEND_EQUATION},
  {_s_dae_blend_equation_separate, AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE},
  {_s_dae_color_material, AIO_RENDER_STATE_COLOR_MATERIAL},
  {_s_dae_cull_face, AIO_RENDER_STATE_CULL_FACE},
  {_s_dae_depth_func, AIO_RENDER_STATE_DEPTH_FUNC},
  {_s_dae_fog_mode, AIO_RENDER_STATE_FOG_MODE},
  {_s_dae_fog_coord_src, AIO_RENDER_STATE_FOG_COORD_SRC},
  {_s_dae_front_face, AIO_RENDER_STATE_FRONT_FACE},
  {_s_dae_light_model_color_control,
    AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL},
  {_s_dae_logic_op, AIO_RENDER_STATE_LOGIC_OP},
  {_s_dae_polygon_mode, AIO_RENDER_STATE_POLYGON_MODE},
  {_s_dae_shade_model, AIO_RENDER_STATE_SHADE_MODEL},
  {_s_dae_stencil_func, AIO_RENDER_STATE_STENCIL_FUNC},
  {_s_dae_stencil_op, AIO_RENDER_STATE_STENCIL_OP},
  {_s_dae_stencil_func_separate, AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE},
  {_s_dae_stencil_op_separate, AIO_RENDER_STATE_STENCIL_OP_SEPARATE},
  {_s_dae_stencil_mask_separate, AIO_RENDER_STATE_STENCIL_MASK_SEPARATE},
  {_s_dae_light_enable, AIO_RENDER_STATE_LIGHT_ENABLE},
  {_s_dae_light_ambient, AIO_RENDER_STATE_LIGHT_AMBIENT},
  {_s_dae_light_diffuse, AIO_RENDER_STATE_LIGHT_DIFFUSE},
  {_s_dae_light_specular, AIO_RENDER_STATE_LIGHT_SPECULAR},
  {_s_dae_light_position, AIO_RENDER_STATE_LIGHT_POSITION},
  {_s_dae_light_constant_attenuation,
    AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION},
  {_s_dae_light_linear_attenuation,
    AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION},
  {_s_dae_light_quadratic_attenuation,
    AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION},
  {_s_dae_light_spot_cutoff, AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF},
  {_s_dae_light_spot_direction, AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION},
  {_s_dae_light_spot_exponent, AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT},
  {_s_dae_texture1D, AIO_RENDER_STATE_TEXTURE1D},
  {_s_dae_texture2D, AIO_RENDER_STATE_TEXTURE2D},
  {_s_dae_texture3D, AIO_RENDER_STATE_TEXTURE3D},
  {_s_dae_textureCUBE, AIO_RENDER_STATE_TEXTURECUBE},
  {_s_dae_textureRECT, AIO_RENDER_STATE_TEXTURERECT},
  {_s_dae_textureDEPTH, AIO_RENDER_STATE_TEXTUREDEPTH},
  {_s_dae_texture1D_enable, AIO_RENDER_STATE_TEXTURE1D_ENABLE},
  {_s_dae_texture2D_enable, AIO_RENDER_STATE_TEXTURE2D_ENABLE},
  {_s_dae_texture3D_enable, AIO_RENDER_STATE_TEXTURE3D_ENABLE},
  {_s_dae_textureCUBE_enable, AIO_RENDER_STATE_TEXTURECUBE_ENABLE},
  {_s_dae_textureRECT_enable, AIO_RENDER_STATE_TEXTURERECT_ENABLE},
  {_s_dae_textureDEPTH_enable, AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE},
  {_s_dae_texture_env_color, AIO_RENDER_STATE_TEXTURE_ENV_COLOR},
  {_s_dae_texture_env_mode, AIO_RENDER_STATE_TEXTURE_ENV_MODE},
  {_s_dae_clip_plane, AIO_RENDER_STATE_CLIP_PLANE},
  {_s_dae_clip_plane_enable, AIO_RENDER_STATE_CLIP_PLANE_ENABLE},
  {_s_dae_blend_color, AIO_RENDER_STATE_BLEND_COLOR},
  {_s_dae_color_mask, AIO_RENDER_STATE_COLOR_MASK},
  {_s_dae_depth_bounds, AIO_RENDER_STATE_DEPTH_BOUNDS},
  {_s_dae_depth_mask, AIO_RENDER_STATE_DEPTH_MASK},
  {_s_dae_depth_range, AIO_RENDER_STATE_DEPTH_RANGE},
  {_s_dae_fog_density, AIO_RENDER_STATE_FOG_DENSITY},
  {_s_dae_fog_start, AIO_RENDER_STATE_FOG_START},
  {_s_dae_fog_end, AIO_RENDER_STATE_FOG_END},
  {_s_dae_fog_color, AIO_RENDER_STATE_FOG_COLOR},
  {_s_dae_light_model_ambient, AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT},
  {_s_dae_lighting_enable, AIO_RENDER_STATE_LIGHTING_ENABLE},
  {_s_dae_line_stipple, AIO_RENDER_STATE_LINE_STIPPLE},
  {_s_dae_line_width, AIO_RENDER_STATE_LINE_WIDTH},
  {_s_dae_material_ambient, AIO_RENDER_STATE_MATERIAL_AMBIENT},
  {_s_dae_material_diffuse, AIO_RENDER_STATE_MATERIAL_DIFFUSE},
  {_s_dae_material_emission, AIO_RENDER_STATE_MATERIAL_EMISSION},
  {_s_dae_material_shininess, AIO_RENDER_STATE_MATERIAL_SHININESS},
  {_s_dae_material_specular, AIO_RENDER_STATE_MATERIAL_SPECULAR},
  {_s_dae_model_view_matrix, AIO_RENDER_STATE_MODEL_VIEW_MATRIX},
  {_s_dae_point_distance_attenuation,
    AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION},
  {_s_dae_point_fade_threshold_size,
    AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE},
  {_s_dae_point_size, AIO_RENDER_STATE_POINT_SIZE},
  {_s_dae_point_size_min, AIO_RENDER_STATE_POINT_SIZE_MIN},
  {_s_dae_point_size_max, AIO_RENDER_STATE_POINT_SIZE_MAX},
  {_s_dae_polygon_offset, AIO_RENDER_STATE_POLYGON_OFFSET},
  {_s_dae_projection_matrix, AIO_RENDER_STATE_PROJECTION_MATRIX},
  {_s_dae_scissor, AIO_RENDER_STATE_SCISSOR},
  {_s_dae_stencil_mask, AIO_RENDER_STATE_STENCIL_MASK},
  {_s_dae_alpha_test_enable, AIO_RENDER_STATE_ALPHA_TEST_ENABLE},
  {_s_dae_blend_enable, AIO_RENDER_STATE_BLEND_ENABLE},
  {_s_dae_color_logic_op_enable, AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE},
  {_s_dae_color_material_enable, AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE},
  {_s_dae_cull_face_enable, AIO_RENDER_STATE_CULL_FACE_ENABLE},
  {_s_dae_depth_bounds_enable, AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE},
  {_s_dae_depth_clamp_enable, AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE},
  {_s_dae_depth_test_enable, AIO_RENDER_STATE_DEPTH_TEST_ENABLE},
  {_s_dae_dither_enable, AIO_RENDER_STATE_DITHER_ENABLE},
  {_s_dae_fog_enable, AIO_RENDER_STATE_FOG_ENABLE},
  {_s_dae_light_model_local_viewer_enable,
    AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE},
  {_s_dae_light_model_two_side_enable,
    AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE},
  {_s_dae_line_smooth_enable,AIO_RENDER_STATE_LINE_SMOOTH_ENABLE},
  {_s_dae_line_stipple_enable, AIO_RENDER_STATE_LINE_STIPPLE_ENABLE},
  {_s_dae_logic_op_enable, AIO_RENDER_STATE_LOGIC_OP_ENABLE},
  {_s_dae_multisample_enable, AIO_RENDER_STATE_MULTISAMPLE_ENABLE},
  {_s_dae_normalize_enable, AIO_RENDER_STATE_NORMALIZE_ENABLE},
  {_s_dae_point_smooth_enable, AIO_RENDER_STATE_POINT_SMOOTH_ENABLE},
  {_s_dae_polygon_offset_fill_enable,
    AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE},
  {_s_dae_polygon_offset_line_enable,
    AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE},
  {_s_dae_polygon_offset_point_enable,
    AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE},
  {_s_dae_polygon_smooth_enable, AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE},
  {_s_dae_polygon_stipple_enable, AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE},
  {_s_dae_rescale_normal_enable, AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE},
  {_s_dae_sample_alpha_to_coverage_enable,
    AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE},
  {_s_dae_sample_alpha_to_one_enable,
    AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE},
  {_s_dae_sample_coverage_enable, AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE},
  {_s_dae_scissor_test_enable, AIO_RENDER_STATE_SCISSOR_TEST_ENABLE},
  {_s_dae_stencil_test_enable, AIO_RENDER_STATE_STENCIL_TEST_ENABLE}
};

static size_t stateMapLen = 0;

int _assetio_hide
aio_dae_fxState(void * __restrict memParent,
                xmlTextReaderPtr reader,
                aio_states ** __restrict dest) {
  aio_states       *states;
  aio_render_state *last_state;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  states = aio_calloc(memParent, sizeof(*states), 1);

  if (stateMapLen == 0) {
    stateMapLen = AIO_ARRAY_LEN(stateMap);
    qsort(stateMap,
          stateMapLen,
          sizeof(stateMap[0]),
          aio_enumpair_cmp);
  }

  last_state = NULL;

  do {
    const aio_enumpair *found;

    _xml_beginElement(_s_dae_states);

    found = bsearch(nodeName,
                    stateMap,
                    stateMapLen,
                    sizeof(stateMap[0]),
                    aio_enumpair_cmp2);

    switch (found->val) {
      case AIO_RENDER_STATE_ALPHA_FUNC:
        aio_dae_fxStateAlphaFunc(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_FUNC:
        aio_dae_fxStateBlend(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_FUNC_SEPARATE:
        aio_dae_fxStateBlendSep(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_BLEND_EQUATION:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_BLEND_EQUATION,
                             AIO_GL_BLEND_EQUATION_FUNC_ADD,
                             aio_dae_fxEnumBlendEq);
        break;
      case AIO_RENDER_STATE_BLEND_EQUATION_SEPARATE:
        aio_dae_fxStateBlendEqSep(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_COLOR_MATERIAL:
        aio_dae_fxStateColorMaterial(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_CULL_FACE:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_CULL_FACE,
                             AIO_GL_FACE_BACK,
                             aio_dae_fxEnumGLFace);
        break;
      case AIO_RENDER_STATE_DEPTH_FUNC:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_DEPTH_FUNC,
                             AIO_GL_FUNC_ALWAYS,
                             aio_dae_fxEnumGlFunc);
        break;
      case AIO_RENDER_STATE_FOG_MODE:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FOG_MODE,
                             AIO_GL_FOG_EXP,
                             aio_dae_fxEnumFog);
        break;
      case AIO_RENDER_STATE_FOG_COORD_SRC:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FOG_COORD_SRC,
                             AIO_GL_FOG_COORD_SRC_FOG_COORDINATE,
                             aio_dae_fxEnumFogCoordSrc);
        break;
      case AIO_RENDER_STATE_FRONT_FACE:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_FRONT_FACE,
                             AIO_GL_FRONT_FACE_CCW,
                             aio_dae_fxEnumFrontFace);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL,
                             AIO_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR,
                             aio_dae_fxEnumLightModelColorCtl);
        break;
      case AIO_RENDER_STATE_LOGIC_OP:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LOGIC_OP,
                             AIO_GL_LOGIC_OP_COPY,
                             aio_dae_fxEnumLogicOp);
        break;
      case AIO_RENDER_STATE_POLYGON_MODE:
        aio_dae_fxStatePolyMode(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_SHADE_MODEL:
        aio_dae_fxState_enum(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_SHADE_MODEL,
                             AIO_GL_SHADE_MODEL_SMOOTH,
                             aio_dae_fxEnumShadeModel);
        break;
      case AIO_RENDER_STATE_STENCIL_FUNC:
        aio_dae_fxStateStencilFunc(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_OP:
        aio_dae_fxStateStencilOp(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_FUNC_SEPARATE:
        aio_dae_fxStateStencilFuncSep(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_OP_SEPARATE:
        aio_dae_fxStateStencilOpSep(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_STENCIL_MASK_SEPARATE:
        aio_dae_fxStateStencilMaskSep(reader, &last_state, &states);
        break;
      case AIO_RENDER_STATE_LIGHT_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_AMBIENT:
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_AMBIENT,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_DIFFUSE: 
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_DIFFUSE,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_SPECULAR:
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_SPECULAR,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_POSITION:
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_POSITION,
                                 (aio_float4){0.0f, 0.0f, 1.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION:
        aio_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION,
                                1);
        break;
      case AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION:
        aio_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_LINEAR_ATTENUATION,
                                0);
        break;
      case AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION:
        aio_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION,
                                0);
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF:
        aio_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_SPOT_CUTOFF,
                                180);
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION:
        aio_dae_fxState_float3_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_LIGHT_SPOT_DIRECTION,
                                 (aio_float3){0.0f, 0.0f, -1.0f},
                                 sizeof(aio_float3));
        break;
      case AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT:
        aio_dae_fxState_float_i(reader,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_LIGHT_SPOT_EXPONENT,
                                0);
        break;
      case AIO_RENDER_STATE_TEXTURE1D:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE1D);
        break;
      case AIO_RENDER_STATE_TEXTURE2D:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE2D);
        break;
      case AIO_RENDER_STATE_TEXTURE3D:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURE3D);
        break;
      case AIO_RENDER_STATE_TEXTURECUBE:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURECUBE);
        break;
      case AIO_RENDER_STATE_TEXTURERECT:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTURERECT);
        break;
      case AIO_RENDER_STATE_TEXTUREDEPTH:
        aio_dae_fxState_sampler(reader,
                                (const char *)nodeName,
                                &last_state,
                                &states,
                                AIO_RENDER_STATE_TEXTUREDEPTH);
        break;
      case AIO_RENDER_STATE_TEXTURE1D_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE1D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE2D_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE2D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE3D_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURE3D_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURECUBE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURECUBE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURERECT_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTURERECT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_TEXTUREDEPTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_TEXTURE_ENV_COLOR:
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_TEXTURE_ENV_COLOR,
                                 NULL,
                                 0);
        break;
      case AIO_RENDER_STATE_TEXTURE_ENV_MODE:
        aio_dae_fxState_str(reader,
                            &last_state,
                            &states,
                            AIO_RENDER_STATE_TEXTURE_ENV_MODE);
        break;
      case AIO_RENDER_STATE_CLIP_PLANE:
        aio_dae_fxState_float4_i(reader,
                                 &last_state,
                                 &states,
                                 AIO_RENDER_STATE_CLIP_PLANE,
                                 (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                                 sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_CLIP_PLANE_ENABLE:
        aio_dae_fxState_ul_i(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_CLIP_PLANE_ENABLE,
                             0);
        break;
      case AIO_RENDER_STATE_BLEND_COLOR:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_BLEND_COLOR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_COLOR_MASK:
        aio_dae_fxState_bool4(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_COLOR_MASK,
                              (aio_bool4){1, 1, 1, 1},
                              sizeof(aio_bool4));
        break;
      case AIO_RENDER_STATE_DEPTH_BOUNDS:
        aio_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_DEPTH_BOUNDS,
                               NULL,
                               0);
        break;
      case AIO_RENDER_STATE_DEPTH_MASK:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_MASK,
                           1);
        break;
      case AIO_RENDER_STATE_DEPTH_RANGE:
        aio_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_DEPTH_RANGE,
                               (aio_float2){0, 1},
                               sizeof(aio_float2));
        break;
      case AIO_RENDER_STATE_FOG_DENSITY:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_DENSITY,
                              1.0f);
        break;
      case AIO_RENDER_STATE_FOG_START:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_START,
                              0.0f);
        break;
      case AIO_RENDER_STATE_FOG_END:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_FOG_END,
                              1.0f);
        break;
      case AIO_RENDER_STATE_FOG_COLOR:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_FOG_COLOR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 0.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_LIGHT_MODEL_AMBIENT,
                               (aio_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_LIGHTING_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHTING_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_STIPPLE:
        aio_dae_fxState_int2(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_LINE_STIPPLE,
                             (aio_int2){1, 65536},
                             sizeof(aio_int2));
        break;
      case AIO_RENDER_STATE_LINE_WIDTH:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_LINE_WIDTH,
                              1.0f);
        break;
      case AIO_RENDER_STATE_MATERIAL_AMBIENT:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_AMBIENT,
                               (aio_float4){0.2f, 0.2f, 0.2f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_DIFFUSE:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_DIFFUSE,
                               (aio_float4){0.8f, 0.8f, 0.8f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_EMISSION:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_EMISSION,
                               (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MATERIAL_SHININESS:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_MATERIAL_SHININESS,
                              0.0f);
        break;
      case AIO_RENDER_STATE_MATERIAL_SPECULAR:
        aio_dae_fxState_float4(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_MATERIAL_SPECULAR,
                               (aio_float4){0.0f, 0.0f, 0.0f, 1.0f},
                               sizeof(aio_float4));
        break;
      case AIO_RENDER_STATE_MODEL_VIEW_MATRIX:
        aio_dae_fxState_float4x4(reader,
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
        aio_dae_fxState_float3(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_POINT_DISTANCE_ATTENUATION,
                               (aio_float3){1.0f, 0.0f, 0.0f},
                               sizeof(aio_float3));
        break;
      case AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_FADE_THRESOLD_SIZE,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE_MIN:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE_MIN,
                              0.0f);
        break;
      case AIO_RENDER_STATE_POINT_SIZE_MAX:
        aio_dae_fxState_float(reader,
                              &last_state,
                              &states,
                              AIO_RENDER_STATE_POINT_SIZE_MAX,
                              1.0f);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET:
        aio_dae_fxState_float2(reader,
                               &last_state,
                               &states,
                               AIO_RENDER_STATE_POLYGON_OFFSET,
                               (aio_float2){0.0f, 0.0f},
                               sizeof(aio_float2));
        break;
      case AIO_RENDER_STATE_PROJECTION_MATRIX:
        aio_dae_fxState_float4x4(reader,
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
        aio_dae_fxState_int4(reader,
                             &last_state,
                             &states,
                             AIO_RENDER_STATE_SCISSOR,
                             NULL,
                             0);
        break;
      case AIO_RENDER_STATE_STENCIL_MASK:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_STENCIL_MASK,
                           4294967295);
        break;
      case AIO_RENDER_STATE_ALPHA_TEST_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_ALPHA_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_BLEND_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_BLEND_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_COLOR_LOGIC_OP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_COLOR_MATERIAL_ENABLE,
                           1);
        break;
      case AIO_RENDER_STATE_CULL_FACE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_CULL_FACE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_BOUNDS_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_CLAMP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DEPTH_TEST_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DEPTH_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_DITHER_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_DITHER_ENABLE,
                           1);
        break;
      case AIO_RENDER_STATE_FOG_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_FOG_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_SMOOTH_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LINE_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LINE_STIPPLE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LINE_STIPPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_LOGIC_OP_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_LOGIC_OP_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_MULTISAMPLE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_MULTISAMPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_NORMALIZE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_NORMALIZE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POINT_SMOOTH_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POINT_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_SMOOTH_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_POLYGON_STIPPLE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_RESCALE_NORMAL_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SAMPLE_COVERAGE_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_SCISSOR_TEST_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_SCISSOR_TEST_ENABLE,
                           0);
        break;
      case AIO_RENDER_STATE_STENCIL_TEST_ENABLE:
        aio_dae_fxState_ul(reader,
                           &last_state,
                           &states,
                           AIO_RENDER_STATE_STENCIL_TEST_ENABLE,
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

  return 0;
}
