/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetio__aio_collada_fx_states_detail_h_
#define __libassetio__aio_collada_fx_states_detail_h_

#include "../../../include/assetio.h"

typedef struct _xmlNode xmlNode;
typedef struct _xmlTextReader *xmlTextReaderPtr;

typedef long (*aio_dae_fxEnumFn_t)(const char * name);

int _assetio_hide
aio_dae_fxState_enum(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     long defaultEnumVal,
                     aio_dae_fxEnumFn_t enumFn);

int _assetio_hide
aio_dae_fxState_bool4(xmlTextReaderPtr __restrict reader,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_bool * defaultVal,
                      size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_ul(xmlTextReaderPtr __restrict reader,
                   aio_render_state ** __restrict last_state,
                   aio_states ** __restrict states,
                   long state_type,
                   unsigned long defaultVal);

int _assetio_hide
aio_dae_fxState_ul_i(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     unsigned long defaultVal);

int _assetio_hide
aio_dae_fxState_int2(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_int4(xmlTextReaderPtr __restrict reader,
                     aio_render_state ** __restrict last_state,
                     aio_states ** __restrict states,
                     long state_type,
                     aio_int * defaultVal,
                     size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float(xmlTextReaderPtr __restrict reader,
                      aio_render_state ** __restrict last_state,
                      aio_states ** __restrict states,
                      long state_type,
                      aio_float defaultVal);

int _assetio_hide
aio_dae_fxState_float_i(xmlTextReaderPtr __restrict reader,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type,
                        aio_float defaultVal);

int _assetio_hide
aio_dae_fxState_float2(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float3(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float3_i(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float4(xmlTextReaderPtr __restrict reader,
                       aio_render_state ** __restrict last_state,
                       aio_states ** __restrict states,
                       long state_type,
                       aio_float * defaultVal,
                       size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float4_i(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_float4x4(xmlTextReaderPtr __restrict reader,
                         aio_render_state ** __restrict last_state,
                         aio_states ** __restrict states,
                         long state_type,
                         aio_float * defaultVal,
                         size_t defaultValSize);

int _assetio_hide
aio_dae_fxState_sampler(xmlTextReaderPtr __restrict reader,
                        const char *elm,
                        aio_render_state ** __restrict last_state,
                        aio_states ** __restrict states,
                        long state_type);

int _assetio_hide
aio_dae_fxState_str(xmlTextReaderPtr __restrict reader,
                    aio_render_state ** __restrict last_state,
                    aio_states ** __restrict states,
                    long state_type);

#define _AIO_DAE_ST_FUN_(ST_FN)                                               \
int _assetio_hide                                                             \
ST_FN(xmlTextReaderPtr __restrict,                                            \
      aio_render_state ** __restrict,                                         \
      aio_states ** __restrict)

_AIO_DAE_ST_FUN_(aio_dae_fxStateAlphaFunc);
_AIO_DAE_ST_FUN_(aio_dae_fxStateBlend);
_AIO_DAE_ST_FUN_(aio_dae_fxStateBlendSep);
_AIO_DAE_ST_FUN_(aio_dae_fxStateBlendEqSep);
_AIO_DAE_ST_FUN_(aio_dae_fxStateColorMaterial);
_AIO_DAE_ST_FUN_(aio_dae_fxStatePolyMode);
_AIO_DAE_ST_FUN_(aio_dae_fxStateStencilFunc);
_AIO_DAE_ST_FUN_(aio_dae_fxStateStencilOp);
_AIO_DAE_ST_FUN_(aio_dae_fxStateStencilFuncSep);
_AIO_DAE_ST_FUN_(aio_dae_fxStateStencilOpSep);
_AIO_DAE_ST_FUN_(aio_dae_fxStateStencilMaskSep);

#undef _AIO_DAE_ST_FUN_


#endif /* __libassetio__aio_collada_fx_states_detail_h_ */
