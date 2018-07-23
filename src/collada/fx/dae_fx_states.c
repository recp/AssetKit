/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "dae_fx_states.h"
#include "dae_fx_states_detail.h"
#include "../core/dae_value.h"
#include "dae_fx_enums.h"
#include "dae_fx_sampler.h"

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
dae_fxState(AkXmlState * __restrict xst,
            void * __restrict memParent,
            AkStates ** __restrict dest) {
  AkStates      *states;
  AkRenderState *last_state;
  AkXmlElmState  xest;

  states = ak_heap_calloc(xst->heap,
                          memParent,
                          sizeof(*states));

  if (stateMapLen == 0) {
    stateMapLen = AK_ARRAY_LEN(stateMap);
    qsort(stateMap,
          stateMapLen,
          sizeof(stateMap[0]),
          ak_enumpair_cmp);
  }

  last_state = NULL;

  ak_xest_init(xest, _s_dae_states)

  do {
    const ak_enumpair *found;

    if (ak_xml_begin(&xest))
      break;

    found = bsearch(xst->nodeName,
                    stateMap,
                    stateMapLen,
                    sizeof(stateMap[0]),
                    ak_enumpair_cmp2);
    if (!found) {
      ak_xml_skipelm(xst);
      goto skip;
    }

    switch (found->val) {
      case AK_RENDER_STATE_ALPHA_FUNC:
        dae_fxStateAlphaFunc(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_FUNC:
        dae_fxStateBlend(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_FUNC_SEPARATE:
        dae_fxStateBlendSep(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_BLEND_EQUATION:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_BLEND_EQUATION,
                         AK_GL_BLEND_EQUATION_FUNC_ADD,
                         dae_fxEnumBlendEq);
        break;
      case AK_RENDER_STATE_BLEND_EQUATION_SEPARATE:
        dae_fxStateBlendEqSep(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_COLOR_MATERIAL:
        dae_fxStateColorMaterial(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_CULL_FACE:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_CULL_FACE,
                         AK_GL_FACE_BACK,
                         dae_fxEnumGLFace);
        break;
      case AK_RENDER_STATE_DEPTH_FUNC:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_DEPTH_FUNC,
                         AK_GL_FUNC_ALWAYS,
                         dae_fxEnumGlFunc);
        break;
      case AK_RENDER_STATE_FOG_MODE:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_FOG_MODE,
                         AK_GL_FOG_EXP,
                         dae_fxEnumFog);
        break;
      case AK_RENDER_STATE_FOG_COORD_SRC:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_FOG_COORD_SRC,
                         AK_GL_FOG_COORD_SRC_FOG_COORDINATE,
                         dae_fxEnumFogCoordSrc);
        break;
      case AK_RENDER_STATE_FRONT_FACE:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_FRONT_FACE,
                         AK_GL_FRONT_FACE_CCW,
                         dae_fxEnumFrontFace);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_LIGHT_MODEL_COLOR_CONTROL,
                         AK_GL_LIGHT_MODEL_COLOR_CONTROL_SINGLE_COLOR,
                         dae_fxEnumLightModelColorCtl);
        break;
      case AK_RENDER_STATE_LOGIC_OP:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_LOGIC_OP,
                         AK_GL_LOGIC_OP_COPY,
                         dae_fxEnumLogicOp);
        break;
      case AK_RENDER_STATE_POLYGON_MODE:
        dae_fxStatePolyMode(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_SHADE_MODEL:
        dae_fxState_enum(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_SHADE_MODEL,
                         AK_GL_SHADE_MODEL_SMOOTH,
                         dae_fxEnumShadeModel);
        break;
      case AK_RENDER_STATE_STENCIL_FUNC:
        dae_fxStateStencilFunc(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_OP:
        dae_fxStateStencilOp(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_FUNC_SEPARATE:
        dae_fxStateStencilFuncSep(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_OP_SEPARATE:
        dae_fxStateStencilOpSep(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_STENCIL_MASK_SEPARATE:
        dae_fxStateStencilMaskSep(xst, &last_state, &states);
        break;
      case AK_RENDER_STATE_LIGHT_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LIGHT_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LIGHT_AMBIENT:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_AMBIENT,
                             (AkFloat4){0.0f, 0.0f, 0.0f, 1.0f},
                             sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHT_DIFFUSE:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_DIFFUSE,
                             (AkFloat4){0.0f, 0.0f, 0.0f, 0.0f},
                             sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHT_SPECULAR:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_SPECULAR,
                             (AkFloat4){0.0f, 0.0f, 0.0f, 0.0f},
                             sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHT_POSITION:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_POSITION,
                             (AkFloat4){0.0f, 0.0f, 1.0f, 0.0f},
                             sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION:
        dae_fxState_float_i(xst,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_LIGHT_CONSTANT_ATTENUATION,
                            1);
        break;
      case AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION:
        dae_fxState_float_i(xst,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_LIGHT_LINEAR_ATTENUATION,
                            0);
        break;
      case AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION:
        dae_fxState_float_i(xst,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_LIGHT_QUADRIC_ATTENUATION,
                            0);
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_CUTOFF:
        dae_fxState_float_i(xst,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_LIGHT_SPOT_CUTOFF,
                            180);
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_DIRECTION:
        dae_fxState_float3_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_LIGHT_SPOT_DIRECTION,
                             (AkFloat3){0.0f, 0.0f, -1.0f},
                             sizeof(AkFloat3));
        break;
      case AK_RENDER_STATE_LIGHT_SPOT_EXPONENT:
        dae_fxState_float_i(xst,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_LIGHT_SPOT_EXPONENT,
                            0);
        break;
      case AK_RENDER_STATE_TEXTURE1D:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURE1D);
        break;
      case AK_RENDER_STATE_TEXTURE2D:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURE2D);
        break;
      case AK_RENDER_STATE_TEXTURE3D:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURE3D);
        break;
      case AK_RENDER_STATE_TEXTURECUBE:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURECUBE);
        break;
      case AK_RENDER_STATE_TEXTURERECT:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTURERECT);
        break;
      case AK_RENDER_STATE_TEXTUREDEPTH:
        dae_fxState_sampler(xst,
                            (const char *)xst->nodeName,
                            &last_state,
                            &states,
                            AK_RENDER_STATE_TEXTUREDEPTH);
        break;
      case AK_RENDER_STATE_TEXTURE1D_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTURE1D_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTURE2D_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTURE2D_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTURE3D_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTURE3D_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTURECUBE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTURECUBE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTURERECT_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTURERECT_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTUREDEPTH_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_TEXTUREDEPTH_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_TEXTURE_ENV_COLOR:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_TEXTURE_ENV_COLOR,
                             NULL,
                             0);
        break;
      case AK_RENDER_STATE_TEXTURE_ENV_MODE:
        dae_fxState_str(xst,
                        &last_state,
                        &states,
                        AK_RENDER_STATE_TEXTURE_ENV_MODE);
        break;
      case AK_RENDER_STATE_CLIP_PLANE:
        dae_fxState_float4_i(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_CLIP_PLANE,
                             (AkFloat4){0.0f, 0.0f, 0.0f, 0.0f},
                             sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_CLIP_PLANE_ENABLE:
        dae_fxState_ul_i(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_CLIP_PLANE_ENABLE,
                         0);
        break;
      case AK_RENDER_STATE_BLEND_COLOR:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_BLEND_COLOR,
                           (AkFloat4){0.0f, 0.0f, 0.0f, 0.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_COLOR_MASK:
        dae_fxState_bool4(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_COLOR_MASK,
                          (AkBool4){1, 1, 1, 1},
                          sizeof(AkBool4));
        break;
      case AK_RENDER_STATE_DEPTH_BOUNDS:
        dae_fxState_float2(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_BOUNDS,
                           NULL,
                           0);
        break;
      case AK_RENDER_STATE_DEPTH_MASK:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_DEPTH_MASK,
                       1);
        break;
      case AK_RENDER_STATE_DEPTH_RANGE:
        dae_fxState_float2(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_DEPTH_RANGE,
                           (AkFloat2){0, 1},
                           sizeof(AkFloat2));
        break;
      case AK_RENDER_STATE_FOG_DENSITY:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_FOG_DENSITY,
                          1.0f);
        break;
      case AK_RENDER_STATE_FOG_START:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_FOG_START,
                          0.0f);
        break;
      case AK_RENDER_STATE_FOG_END:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_FOG_END,
                          1.0f);
        break;
      case AK_RENDER_STATE_FOG_COLOR:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_FOG_COLOR,
                           (AkFloat4){0.0f, 0.0f, 0.0f, 0.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_AMBIENT:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_LIGHT_MODEL_AMBIENT,
                           (AkFloat4){0.2f, 0.2f, 0.2f, 1.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_LIGHTING_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LIGHTING_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LINE_STIPPLE:
        dae_fxState_int2(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_LINE_STIPPLE,
                         (AkInt2){1, 65536},
                         sizeof(AkInt2));
        break;
      case AK_RENDER_STATE_LINE_WIDTH:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_LINE_WIDTH,
                          1.0f);
        break;
      case AK_RENDER_STATE_MATERIAL_AMBIENT:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_MATERIAL_AMBIENT,
                           (AkFloat4){0.2f, 0.2f, 0.2f, 1.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_MATERIAL_DIFFUSE:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_MATERIAL_DIFFUSE,
                           (AkFloat4){0.8f, 0.8f, 0.8f, 1.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_MATERIAL_EMISSION:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_MATERIAL_EMISSION,
                           (AkFloat4){0.0f, 0.0f, 0.0f, 1.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_MATERIAL_SHININESS:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_MATERIAL_SHININESS,
                          0.0f);
        break;
      case AK_RENDER_STATE_MATERIAL_SPECULAR:
        dae_fxState_float4(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_MATERIAL_SPECULAR,
                           (AkFloat4){0.0f, 0.0f, 0.0f, 1.0f},
                           sizeof(AkFloat4));
        break;
      case AK_RENDER_STATE_MODEL_VIEW_MATRIX:
        dae_fxState_float4x4(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_MODEL_VIEW_MATRIX,
                             *(AkFloat4x4){
                               {1.0f, 0.0f, 0.0f, 0.0f},
                               {0.0f, 1.0f, 0.0f, 0.0f},
                               {0.0f, 0.0f, 1.0f, 0.0f},
                               {0.0f, 0.0f, 0.0f, 1.0f}
                             },
                             sizeof(AkFloat4x4));
        break;
      case AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION:
        dae_fxState_float3(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POINT_DISTANCE_ATTENUATION,
                           (AkFloat3){1.0f, 0.0f, 0.0f},
                           sizeof(AkFloat3));
        break;
      case AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_POINT_FADE_THRESOLD_SIZE,
                          1.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_POINT_SIZE,
                          1.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE_MIN:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_POINT_SIZE_MIN,
                          0.0f);
        break;
      case AK_RENDER_STATE_POINT_SIZE_MAX:
        dae_fxState_float(xst,
                          &last_state,
                          &states,
                          AK_RENDER_STATE_POINT_SIZE_MAX,
                          1.0f);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET:
        dae_fxState_float2(xst,
                           &last_state,
                           &states,
                           AK_RENDER_STATE_POLYGON_OFFSET,
                           (AkFloat2){0.0f, 0.0f},
                           sizeof(AkFloat2));
        break;
      case AK_RENDER_STATE_PROJECTION_MATRIX:
        dae_fxState_float4x4(xst,
                             &last_state,
                             &states,
                             AK_RENDER_STATE_PROJECTION_MATRIX,
                             *(AkFloat4x4){
                               {1.0f, 0.0f, 0.0f, 0.0f},
                               {0.0f, 1.0f, 0.0f, 0.0f},
                               {0.0f, 0.0f, 1.0f, 0.0f},
                               {0.0f, 0.0f, 0.0f, 1.0f}
                             },
                             sizeof(AkFloat4x4));
        break;
      case AK_RENDER_STATE_SCISSOR:
        dae_fxState_int4(xst,
                         &last_state,
                         &states,
                         AK_RENDER_STATE_SCISSOR,
                         NULL,
                         0);
        break;
      case AK_RENDER_STATE_STENCIL_MASK:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_STENCIL_MASK,
                       4294967295);
        break;
      case AK_RENDER_STATE_ALPHA_TEST_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_ALPHA_TEST_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_BLEND_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_BLEND_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_COLOR_LOGIC_OP_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_COLOR_MATERIAL_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_COLOR_MATERIAL_ENABLE,
                       1);
        break;
      case AK_RENDER_STATE_CULL_FACE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_CULL_FACE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_DEPTH_BOUNDS_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_DEPTH_CLAMP_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_DEPTH_CLAMP_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_DEPTH_TEST_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_DEPTH_TEST_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_DITHER_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_DITHER_ENABLE,
                       1);
        break;
      case AK_RENDER_STATE_FOG_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_FOG_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LIGHT_MODEL_LOCAL_VIEWER_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LIGHT_MODEL_TWO_SIDE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LINE_SMOOTH_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LINE_SMOOTH_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LINE_STIPPLE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LINE_STIPPLE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_LOGIC_OP_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_LOGIC_OP_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_MULTISAMPLE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_MULTISAMPLE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_NORMALIZE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_NORMALIZE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POINT_SMOOTH_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POINT_SMOOTH_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POLYGON_OFFSET_FILL_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POLYGON_OFFSET_LINE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POLYGON_OFFSET_POINT_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POLYGON_SMOOTH_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_POLYGON_STIPPLE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_RESCALE_NORMAL_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_RESCALE_NORMAL_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_SAMPLE_ALPHA_TO_COVERAGE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_SAMPLE_ALPHA_TO_ONE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_SAMPLE_COVERAGE_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_SCISSOR_TEST_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_SCISSOR_TEST_ENABLE,
                       0);
        break;
      case AK_RENDER_STATE_STENCIL_TEST_ENABLE:
        dae_fxState_ul(xst,
                       &last_state,
                       &states,
                       AK_RENDER_STATE_STENCIL_TEST_ENABLE,
                       0);
        break;

      default:
        ak_xml_skipelm(xst);
        break;
    }

  skip:
    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = states;

  return AK_OK;
}
