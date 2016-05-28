/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_fx_states_detail_h_
#define __libassetkit__ak_collada_fx_states_detail_h_

#include "../../../include/assetkit.h"
#include "../ak_collada_common.h"

typedef AkEnum (*ak_dae_fxEnumFn_t)(const char * name);

AkResult _assetkit_hide
ak_dae_fxState_enum(AkHeap * __restrict heap,
                    xmlTextReaderPtr reader,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkEnum defaultEnumVal,
                    ak_dae_fxEnumFn_t enumFn);

AkResult _assetkit_hide
ak_dae_fxState_bool4(AkHeap * __restrict heap,
                     xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkBool * defaultVal,
                     size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_ul(AkHeap * __restrict heap,
                  xmlTextReaderPtr reader,
                  AkRenderState ** __restrict last_state,
                  AkStates ** __restrict states,
                  AkRenderStateType state_type,
                  unsigned long defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_ul_i(AkHeap * __restrict heap,
                    xmlTextReaderPtr reader,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    unsigned long defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_int2(AkHeap * __restrict heap,
                    xmlTextReaderPtr reader,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_int4(AkHeap * __restrict heap,
                    xmlTextReaderPtr reader,
                    AkRenderState ** __restrict last_state,
                    AkStates ** __restrict states,
                    AkRenderStateType state_type,
                    AkInt * defaultVal,
                    size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float(AkHeap * __restrict heap,
                     xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     AkStates ** __restrict states,
                     AkRenderStateType state_type,
                     AkFloat defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_float_i(AkHeap * __restrict heap,
                       xmlTextReaderPtr reader,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type,
                       AkFloat defaultVal);

AkResult _assetkit_hide
ak_dae_fxState_float2(AkHeap * __restrict heap,
                      xmlTextReaderPtr reader,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float3(AkHeap * __restrict heap,
                      xmlTextReaderPtr reader,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float3_i(AkHeap * __restrict heap,
                        xmlTextReaderPtr reader,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4(AkHeap * __restrict heap,
                      xmlTextReaderPtr reader,
                      AkRenderState ** __restrict last_state,
                      AkStates ** __restrict states,
                      AkRenderStateType state_type,
                      AkFloat * defaultVal,
                      size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4_i(AkHeap * __restrict heap,
                        xmlTextReaderPtr reader,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_float4x4(AkHeap * __restrict heap,
                        xmlTextReaderPtr reader,
                        AkRenderState ** __restrict last_state,
                        AkStates ** __restrict states,
                        AkRenderStateType state_type,
                        AkFloat * defaultVal,
                        size_t defaultValSize);

AkResult _assetkit_hide
ak_dae_fxState_sampler(AkHeap * __restrict heap,
                       xmlTextReaderPtr reader,
                       const char *elm,
                       AkRenderState ** __restrict last_state,
                       AkStates ** __restrict states,
                       AkRenderStateType state_type);

AkResult _assetkit_hide
ak_dae_fxState_str(AkHeap * __restrict heap,
                   xmlTextReaderPtr reader,
                   AkRenderState ** __restrict last_state,
                   AkStates ** __restrict states,
                   AkRenderStateType state_type);

#define _ak_DAE_ST_FUN_(ST_FN)                                                \
AkResult _assetkit_hide                                                       \
ST_FN(AkHeap * __restrict,                                                    \
      xmlTextReaderPtr,                                                       \
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


#endif /* __libassetkit__ak_collada_fx_states_detail_h_ */
