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

#ifndef assetkit_options_h
#define assetkit_options_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

typedef enum AkDaeExportIndexMode {
  /* Native COLLADA-style multi-index output. Lowest export overhead and
     preserves the current AssetKit accessor/input layout. */
  AK_DAE_EXPORT_INDEX_MULTI  = 0,

  /* Unified vertex-index output. This explicitly normalizes exported mesh
     primitives to AssetKit's single-index layout before writing, so callers
     that need a readonly export should keep the default MULTI mode. */
  AK_DAE_EXPORT_INDEX_SINGLE = 1,

  /* Let the exporter choose. Currently resolves to MULTI. */
  AK_DAE_EXPORT_INDEX_AUTO   = 2
} AkDaeExportIndexMode;

typedef enum AkDaeExportVersion {
  /* Choose the lowest compatible COLLADA version. Currently 1.4.1 unless
     the document contains B-rep data, which requires the 1.5 schema. */
  AK_DAE_EXPORT_VERSION_AUTO = 0,

  /* Force COLLADA 1.4.1. Export fails for data that requires the 1.5 schema. */
  AK_DAE_EXPORT_VERSION_1_4  = 1,

  /* Force the latest COLLADA 1.5 specification revision (1.5.1).
     COLLADA 1.5.1 intentionally retains the 1.5.0 schema, namespace, and
     document version attribute. */
  AK_DAE_EXPORT_VERSION_1_5_1 = 2,

  /* Backward-compatible name for AK_DAE_EXPORT_VERSION_1_5_1. */
  AK_DAE_EXPORT_VERSION_1_5   = AK_DAE_EXPORT_VERSION_1_5_1
} AkDaeExportVersion;

typedef enum AkGltfExportVersion {
  /* Use AssetKit's stable glTF writer profile. Currently resolves to 2.0
     until the glTF 2.1 specification syntax is finalized. */
  AK_GLTF_EXPORT_VERSION_AUTO = 0,

  /* Force asset.version = "2.0". */
  AK_GLTF_EXPORT_VERSION_2_0  = 1,

  /* Force asset.version = "2.1". GLB files still use the compatible v2
     binary container unless a future feature requires GLB v3. */
  AK_GLTF_EXPORT_VERSION_2_1  = 2
} AkGltfExportVersion;

typedef enum AkStlExportFormat {
  AK_STL_EXPORT_BINARY = 0,
  AK_STL_EXPORT_ASCII  = 1
} AkStlExportFormat;

typedef enum AkPlyExportFormat {
  AK_PLY_EXPORT_BINARY_LITTLE = 0,
  AK_PLY_EXPORT_ASCII         = 1
} AkPlyExportFormat;

typedef enum AkPlyExportColorMode {
  AK_PLY_EXPORT_COLOR_NONE   = 0,
  AK_PLY_EXPORT_COLOR_SRGB   = 1,
  AK_PLY_EXPORT_COLOR_LINEAR = 2
} AkPlyExportColorMode;

typedef enum AkPlyImportColorMode {
  /* PLY does not declare a transfer function. AUTO applies the widespread
     byte-as-display-color convention (integer RGB is sRGB, float RGB is
     linear). SRGB matches interoperable DCC display-color behavior and is
     AssetKit's default. LINEAR preserves RGB numerically apart from integer
     0..1 normalization. */
  AK_PLY_IMPORT_COLOR_AUTO   = 0,
  AK_PLY_IMPORT_COLOR_SRGB   = 1,
  AK_PLY_IMPORT_COLOR_LINEAR = 2
} AkPlyImportColorMode;

/* Global import/export options. */
typedef enum AkOption {
  AK_OPT_INDICES_DEFAULT            = 0,  /* false    */
  AK_OPT_INDICES_SINGLE_INTERLEAVED = 1,  /* false    */
  AK_OPT_INDICES_SINGLE_SEPARATE    = 2,  /* false    */
  AK_OPT_INDICES_SINGLE             = 3,  /* false    */
  AK_OPT_NOINDEX_INTERLEAVED        = 4,  /* true     */
  AK_OPT_NOINDEX_SEPARATE           = 5,  /* false    */
  AK_OPT_COORD                      = 6,  /* Y_UP     */
  AK_OPT_DEFAULT_ID_PREFIX          = 7,  /* id-      */
  AK_OPT_COMPUTE_BBOX               = 8,  /* false    */
  AK_OPT_TRIANGULATE                = 9,  /* true     */
  AK_OPT_GEN_NORMALS_IF_NEEDED      = 10, /* true     */
  AK_OPT_DEFAULT_PROFILE            = 11, /* COMMON   */
  AK_OPT_EFFECT_PROFILE             = 12, /* true     */
  AK_OPT_TECHNIQUE                  = 13, /* "common" */
  AK_OPT_TECHNIQUE_FX               = 14, /* "common" */
  AK_OPT_ZERO_INDEXED_INPUT         = 15, /* false    */
  AK_OPT_IMAGE_LOAD_FLIP_VERTICALLY = 16, /* true     */
  AK_OPT_ADD_DEFAULT_CAMERA         = 17, /* true     */
  AK_OPT_ADD_DEFAULT_LIGHT          = 18, /* false    */
  AK_OPT_COORD_CONVERT_TYPE         = 19, /* DEFAULT  */
  AK_OPT_BUGFIXES                   = 20, /* TRUE     */
  AK_OPT_COMPUTE_EXACT_CENTER       = 21, /* FALSE    */
  AK_OPT_USE_MMAP                   = 22, /* TRUE     */

  /* Generate MikkTSpace-compatible tangents when needed. */
  AK_OPT_GEN_TANGENTS_IF_NEEDED     = 23, /* true     */

  /* Convert triangle strips/fans to triangle lists when the consumer does
     not support native strip/fan primitives. */
  AK_OPT_CVT_TRIANGLESTRIP          = 24, /* false    */
  AK_OPT_CVT_TRIANGLEFAN            = 25, /* false    */

  /* Convert line loops/strips to line lists when the consumer does not
     support native loop/strip primitives. */
  AK_OPT_CVT_LINELOOP               = 26, /* false    */
  AK_OPT_CVT_LINESTRIP              = 27, /* false    */

  /* keep KHR_mesh_quantization accessors in their authored integer form. */
  AK_OPT_PRESERVE_QUANTIZED_ATTRS   = 28, /* false    */

  /* optional glTF extension decoder settings. */
  AK_OPT_GLTF_EXT_DECODER_AUTOLOAD   = 29, /* true     */
  AK_OPT_GLTF_MESHOPT_DECODER_PATH   = 30, /* NULL     */
  AK_OPT_GLTF_DRACO_DECODER_PATH     = 31, /* NULL     */
  AK_OPT_GLTF_GSPLAT_DECODER_PATH    = 32, /* NULL     */
  AK_OPT_GLTF_KTX2_DECODER_PATH      = 33, /* NULL     */

  /* deduplicate position-only triangle soup into indexed mesh storage when
     the importer can preserve the remaining semantics. Currently applied to
     colorless STL triangle soup. */
  AK_OPT_MESH_POSITION_DEDUP_INDEX    = 34, /* true     */

  /* Preserve format-authored custom metadata. glTF extensions are preserved
     independently because they carry typed extension semantics. */
  AK_OPT_PRESERVE_EXTRAS              = 35, /* false    */

  /* AkDaeExportIndexMode. Default MULTI keeps DAE export low-latency and
     avoids vertex remap work unless SINGLE is explicitly requested. */
  AK_OPT_DAE_EXPORT_INDEX_MODE        = 36, /* MULTI    */

  /* AkDaeExportVersion. Default AUTO writes COLLADA 1.4.1 unless the
     content requires the 1.5 schema. */
  AK_OPT_DAE_EXPORT_VERSION           = 37, /* AUTO     */

  /* const char*. Exporter generator/authoring-tool string. */
  AK_OPT_EXPORT_AUTHORING_TOOL        = 38, /* AssetKit vX.Y.Z */

  AK_OPT_STL_EXPORT_FORMAT            = 39, /* BINARY   */
  AK_OPT_PLY_EXPORT_FORMAT            = 40, /* BINARY_LITTLE */
  AK_OPT_PLY_EXPORT_NORMALS           = 41, /* false    */
  AK_OPT_PLY_EXPORT_UV                = 42, /* true     */
  AK_OPT_PLY_EXPORT_COLOR_MODE        = 43, /* SRGB     */
  AK_OPT_PLY_EXPORT_TRIANGULATED      = 44, /* false    */

  /* AkGltfExportVersion. Default AUTO writes glTF 2.0 for maximum
     compatibility; callers may opt into the draft 2.1 asset version. */
  AK_OPT_GLTF_EXPORT_VERSION           = 45, /* AUTO     */

  /* ZIP container export compression level. 0 stores entries, 1 is fastest
     deflate, 12 is maximum libdeflate compression. Currently used by 3MF. */
  AK_OPT_ZIP_EXPORT_COMPRESSION_LEVEL  = 46, /* 1        */

  /* AkPlyImportColorMode. */
  AK_OPT_PLY_IMPORT_COLOR_MODE         = 47, /* SRGB     */
} AkOption;

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value);

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option);

AK_EXPORT
void
ak_opt_set_str(AkOption option, const char *value);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_options_h */
