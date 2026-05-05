/*
 * Copyright (C) 2026 Recep Aslantas
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

#ifndef assetkit_gsplat_h
#define assetkit_gsplat_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*!
 * @brief KHR_gaussian_splatting metadata.
 *
 * The base extension stores splats as POINT primitives. Per-splat data
 * stays in AkMeshPrimitive.input (POSITION, ROTATION, SCALE, OPACITY,
 * COLOR_0..COLOR_15). This struct stores only the extension-level
 * rendering hints and optional decoder output for compressed payloads.
 */

typedef enum AkGaussianSplatKernel {
  AK_GSPLAT_KERNEL_UNKNOWN = 0,
  AK_GSPLAT_KERNEL_ELLIPSE = 1   /* 2D ellipse projection of an ellipsoid */
} AkGaussianSplatKernel;

typedef enum AkGaussianSplatColorSpace {
  AK_GSPLAT_COLOR_UNKNOWN              = 0,
  AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY  = 1, /* "srgb_rec709_display" */
  AK_GSPLAT_COLOR_LIN_REC709_DISPLAY   = 2  /* "lin_rec709_display"  */
} AkGaussianSplatColorSpace;

typedef enum AkGaussianSplatProjection {
  AK_GSPLAT_PROJECTION_PERSPECTIVE  = 0, /* default */
  AK_GSPLAT_PROJECTION_ORTHOGRAPHIC = 1
} AkGaussianSplatProjection;

typedef enum AkGaussianSplatSortingMethod {
  AK_GSPLAT_SORTING_CAMERA_DISTANCE = 0, /* default */
  AK_GSPLAT_SORTING_NONE            = 1
} AkGaussianSplatSortingMethod;

typedef struct AkGaussianSplat {
  AkGaussianSplatKernel        kernel;
  AkGaussianSplatColorSpace    colorSpace;
  AkGaussianSplatProjection    projection;
  AkGaussianSplatSortingMethod sortingMethod;

  /* Filled by an optional decoder when a compression extension is present. */
  void                        *decodedData;     /* opaque, decoder-owned */
  uint32_t                     decodedCount;     /* decoded splat count */
  uint32_t                     reserved;
} AkGaussianSplat;

/*---------------------------------------------------------------------*/
/* External decoder interface.                                         */
/*                                                                     */
/* Apps provide a side library exporting assetkit_gsplat_create, the   */
/* same pattern used by Draco / meshoptimizer / KTX2 shims. AssetKit   */
/* dlopens it from AK_OPT_GLTF_GSPLAT_DECODER_PATH or, when autoload   */
/* is enabled, from the standard side-library name.                    */
/*                                                                     */
/* The uncompressed base KHR_gaussian_splatting extension does not     */
/* need this decoder; renderers read primitive accessors directly.     */
/*---------------------------------------------------------------------*/

struct AkHeap;
struct AkGLTFState;
struct AkMeshPrimitive;
struct json_t;

/* Decoders may implement either entrypoint. decodeBytes is preferred when
   the compression extension references a bufferView directly; decodePrimitive
   is kept for formats that need broader glTF state. */
typedef int
(*AkGaussianSplatDecodeBytesFn)(struct AkHeap          * heap,
                                struct AkMeshPrimitive * prim,
                                const uint8_t          * data,
                                size_t                   size);

typedef int
(*AkGaussianSplatDecodePrimitiveFn)(struct AkGLTFState     * gst,
                                    struct AkMeshPrimitive * prim,
                                    const struct json_t    * jprim,
                                    const struct json_t    * jcompression);

typedef struct AkGaussianSplatDecoder {
  void                              *userdata;
  AkGaussianSplatDecodeBytesFn       decodeBytes;
  AkGaussianSplatDecodePrimitiveFn   decodePrimitive;
  void                             (*close)(void *ud);
} AkGaussianSplatDecoder;

/*!
 * @brief Decoder-library entrypoint. Returns 0 on success.
 */
typedef int
(*AkGaussianSplatDecoderCreateFn)(AkGaussianSplatDecoder * out);

#ifdef __cplusplus
}
#endif
#endif /* assetkit_gsplat_h */
