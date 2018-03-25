/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_fx_states_detail_h_
#define __libassetkit__dae_fx_states_detail_h_

#include "../../../include/ak/assetkit.h"
#include "../dae_common.h"

typedef AkEnum (*ak_dae_fxEnumFn_t)(const char * name);

AkResult _assetkit_hide
ak_dae_fxState_enum(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkEnum defaultEnumVal,
                    ak_dae_fxEnumFn_t enumFn);

AkResult _assetkit_hide
ak_dae_fxState_bool4(AkXmlState * __restrict xst,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkBool * defaultVal,
                     size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_ul(AkXmlState * __restrict xst,
                  AkRenderState ** __restrict last_state,
                  AkStates ** __restrict states,
                  AkRenderStateType state_type,
                  unsigned long defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_ul_i(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    unsigned long defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_int2(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_int4(AkXmlState * __restrict xst,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float(AkXmlState * __restrict xst,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkFloat defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_float_i(AkXmlState * __restrict xst,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type,
                       AkFloat defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_float2(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float3(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float3_i(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4(AkXmlState * __restrict xst,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4_i(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4x4(AkXmlState * __restrict xst,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_sampler(AkXmlState * __restrict xst,
                       const char *elm,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type);

AkResult _assetkit_hide
ak_dae_fxState_str(AkXmlState * __restrict xst,
                   AkRenderState ** __restrict last_state,
                   AkStates ** __restrict states,
                   AkRenderStateType state_type);

#define _ak_DAE_ST_FUN_(ST_FN)                                                \
AkResult _assetkit_hide                                                       \
ST_FN(AkXmlState * __restrict xst,                                       \
      AkRenderState ** __restrict,                                            \
      AkStates ** __restrict)

_ak_DAE_ST_FUN_(ak_dae_fxStateAlphaFunc);
_ak_DAE_ST_FUN_(ak_dae_fxStateBlend);
_ak_DAE_ST_FUN_(ak_dae_fxStateBlendSep);
_ak_DAE_ST_FUN_(ak_dae_fxStateBlendEqSep);
_ak_DAE_ST_FUN_(ak_dae_fxStateColorMaterial);
_ak_DAE_ST_FUN_(ak_dae_fxStatePolyMode);
_ak_DAE_ST_FUN_(ak_dae_fxStateStencilFunc);
_ak_DAE_ST_FUN_(ak_dae_fxStateStencilOp);
_ak_DAE_ST_FUN_(ak_dae_fxStateStencilFuncSep);
_ak_DAE_ST_FUN_(ak_dae_fxStateStencilOpSep);
_ak_DAE_ST_FUN_(ak_dae_fxStateStencilMaskSep);

#undef _ak_DAE_ST_FUN_


#endif /* __libassetkit__dae_fx_states_detail_h_ */
