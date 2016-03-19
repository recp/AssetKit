/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__ak_collada_fx_states_detail_h_
#define __libassetkit__ak_collada_fx_states_detail_h_

#include "../../../include/assetkit.h"

typedef struct _xmlNode xmlNode;
typedef struct _xmlTextReader *xmlTextReaderPtr;

typedef AkEnum (*ak_dae_fxEnumFn_t)(const char * name);

AkEnum _assetkit_hide
ak_dae_fxState_enum(xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     ak_states ** __restrict states,
                     AkRenderStateType state_type,
                     AkEnum defaultEnumVal,
                     ak_dae_fxEnumFn_t enumFn);

int _assetkit_hide
ak_dae_fxState_bool4(xmlTextReaderPtr reader,
                      AkRenderState ** __restrict last_state,
                      ak_states ** __restrict states,
                      AkRenderStateType state_type,
                      ak_bool * defaultVal,
                      size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_ul(xmlTextReaderPtr reader,
                   AkRenderState ** __restrict last_state,
                   ak_states ** __restrict states,
                   AkRenderStateType state_type,
                   unsigned long defaultVal);

int _assetkit_hide
ak_dae_fxState_ul_i(xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     ak_states ** __restrict states,
                     AkRenderStateType state_type,
                     unsigned long defaultVal);

int _assetkit_hide
ak_dae_fxState_int2(xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     ak_states ** __restrict states,
                     AkRenderStateType state_type,
                     ak_int * defaultVal,
                     size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_int4(xmlTextReaderPtr reader,
                     AkRenderState ** __restrict last_state,
                     ak_states ** __restrict states,
                     AkRenderStateType state_type,
                     ak_int * defaultVal,
                     size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float(xmlTextReaderPtr reader,
                      AkRenderState ** __restrict last_state,
                      ak_states ** __restrict states,
                      AkRenderStateType state_type,
                      ak_float defaultVal);

int _assetkit_hide
ak_dae_fxState_float_i(xmlTextReaderPtr reader,
                        AkRenderState ** __restrict last_state,
                        ak_states ** __restrict states,
                        AkRenderStateType state_type,
                        ak_float defaultVal);

int _assetkit_hide
ak_dae_fxState_float2(xmlTextReaderPtr reader,
                       AkRenderState ** __restrict last_state,
                       ak_states ** __restrict states,
                       AkRenderStateType state_type,
                       ak_float * defaultVal,
                       size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float3(xmlTextReaderPtr reader,
                       AkRenderState ** __restrict last_state,
                       ak_states ** __restrict states,
                       AkRenderStateType state_type,
                       ak_float * defaultVal,
                       size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float3_i(xmlTextReaderPtr reader,
                         AkRenderState ** __restrict last_state,
                         ak_states ** __restrict states,
                         AkRenderStateType state_type,
                         ak_float * defaultVal,
                         size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float4(xmlTextReaderPtr reader,
                       AkRenderState ** __restrict last_state,
                       ak_states ** __restrict states,
                       AkRenderStateType state_type,
                       ak_float * defaultVal,
                       size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float4_i(xmlTextReaderPtr reader,
                         AkRenderState ** __restrict last_state,
                         ak_states ** __restrict states,
                         AkRenderStateType state_type,
                         ak_float * defaultVal,
                         size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_float4x4(xmlTextReaderPtr reader,
                         AkRenderState ** __restrict last_state,
                         ak_states ** __restrict states,
                         AkRenderStateType state_type,
                         ak_float * defaultVal,
                         size_t defaultValSize);

int _assetkit_hide
ak_dae_fxState_sampler(xmlTextReaderPtr reader,
                        const char *elm,
                        AkRenderState ** __restrict last_state,
                        ak_states ** __restrict states,
                        AkRenderStateType state_type);

int _assetkit_hide
ak_dae_fxState_str(xmlTextReaderPtr reader,
                    AkRenderState ** __restrict last_state,
                    ak_states ** __restrict states,
                    AkRenderStateType state_type);

#define _ak_DAE_ST_FUN_(ST_FN)                                               \
int _assetkit_hide                                                             \
ST_FN(xmlTextReaderPtr,                                                       \
      AkRenderState ** __restrict,                                         \
      ak_states ** __restrict)

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
