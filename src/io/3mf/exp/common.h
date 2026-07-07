/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ak_3mf_exp_common_h
#define ak_3mf_exp_common_h

#include "../3mf.h"
#include "../../common/buffer.h"
#include "../../common/primitive.h"
#include "../../common/text_number.h"
#include "../../common/zip.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define AK_3MF_MAX_NODE_DEPTH 512u

typedef IOFloatRows AK3MFRows;

#define ak_3mf_rows_init     io_float_rows_init
#define ak_3mf_rows_destroy  io_float_rows_destroy
#define ak_3mf_rows_get      io_float_rows_get
#define ak_3mf_row_component io_float_row_component

typedef IOBuffer AK3MFBuffer;

typedef struct AK3MFExportState {
  AkDoc           *doc;
  AkPrintDocument *print;
  AK3MFBuffer      resources;
  AK3MFBuffer      build;
  uint32_t         objectCount;
  uint32_t         nextObjectId;
  AkResult         result;
  bool             usesMaterialExtension;
  bool             usesProductionExtension;
  bool             usesProductionAlternativeExtension;
  bool             usesSliceExtension;
  bool             usesBeamLatticeExtension;
  bool             usesBeamBallExtension;
  bool             usesBooleanExtension;
  bool             usesDisplacementExtension;
  bool             usesVolumetricExtension;
  bool             usesImplicitExtension;
  bool             suppressBuildItems;
} AK3MFExportState;

typedef struct AK3MFDisplacementWrite {
  const AkPrintDisplacementTriangle *triangle;
  uint32_t                           remaining;
} AK3MFDisplacementWrite;

#define AK_3MF_BUF_INITIAL_CAP 4096u
#define ak_3mf_buf_reserve(BUF, EXTRA)                                        \
  io_buffer_reserve((BUF), (EXTRA), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_raw(BUF, DATA, LEN)                                        \
  io_buffer_raw((BUF), (DATA), (LEN), AK_3MF_BUF_INITIAL_CAP)
#define AK_3MF_BUF_LIT(BUF, LIT)                                              \
  IO_BUFFER_LIT((BUF), LIT, AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_lit(BUF, LIT)                                              \
  io_buffer_cstr((BUF), (LIT), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_ch(BUF, CH)                                                \
  io_buffer_ch((BUF), (CH), AK_3MF_BUF_INITIAL_CAP)
#define ak_3mf_buf_free(BUF) io_buffer_free((BUF))

AK_HIDE
void
ak_3mf_buf_attr(AK3MFBuffer * __restrict buf,
                const char  * __restrict value);

AK_HIDE
void
ak_3mf_buf_3mf_path_attr(AK3MFBuffer * __restrict buf,
                         const char  * __restrict path);

AK_HIDE
void
ak_3mf_buf_u32(AK3MFBuffer * __restrict buf, uint32_t value);

AK_HIDE
void
ak_3mf_buf_float(AK3MFBuffer * __restrict buf, float value);

AK_HIDE
void
ak_3mf_append_transform(AK3MFBuffer * __restrict buf, mat4 world);

AK_HIDE
void
ak_3mf_append_flat_transform(AK3MFBuffer         * __restrict buf,
                             const float          matrix[16]);

AK_HIDE
void
ak_3mf_append_optional_3mf_path(AK3MFBuffer * __restrict buf,
                                const char  * __restrict path);

AK_HIDE
bool
ak_3mf_uses_production_alternative_extension(
                                      const AkPrintDocument * __restrict print);

AK_HIDE
bool
ak_3mf_uses_beam_ball_extension(const AkPrintDocument * __restrict print);

AK_HIDE
bool
ak_3mf_write_slice_stacks(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_displacement_resources(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_volumetric_resources(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_scene(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_library_fallback(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_level_sets(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_write_boolean_shapes(AK3MFExportState * __restrict st);

AK_HIDE
bool
ak_3mf_build_model_xml(AK3MFExportState * __restrict st,
                       AK3MFBuffer      * __restrict model);

#endif /* ak_3mf_exp_common_h */
