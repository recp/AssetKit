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

#ifndef assetkit_gsplat_h
#define assetkit_gsplat_h
#ifdef __cplusplus
extern "C" {
#endif

#include "common.h"

/*!
 * @brief Gaussian splat metadata for SPZ, L-GSC, Gaussian PLY and glTF.
 *
 * The base extension stores splats as POINT primitives. Per-splat data
 * stays in AkMeshPrimitive.input: AK_INPUT_POSITION, AK_INPUT_ROTATION
 * (unit quaternion, xyzw), AK_INPUT_SCALE (linear), AK_INPUT_OPACITY
 * (0..1), and AK_INPUT_SH (unmodified RGB coefficients). SH input sets
 * use degree * degree + coefficient: DC is set 0, degree 1 is 1..3,
 * degree 2 is 4..8, degree 3 is 9..15, degree 4 is 16..24.
 * Evaluate all SH terms before converting the reconstructed color from
 * colorSpace. The DC-only color is 0.5 + 0.28209479177387814 * SH[0].
 * All inputs and buffers belong to the document heap.
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

  /* Optional opaque payload; the built-in decoder uses primitive inputs. */
  void                        *decodedData;     /* opaque, decoder-owned */
  uint32_t                     decodedCount;    /* decoded splat count */
  uint8_t                      shDegree;        /* highest complete SH degree */
  bool                         antialiased;     /* trained with mip-splat filtering */
  uint8_t                      reserved[2];
} AkGaussianSplat;

/*---------------------------------------------------------------------*/
/* External decoder interface.                                         */
/*                                                                     */
/* Apps provide a side library exporting assetkit_gsplat_create, the   */
/* same pattern used by Draco / meshoptimizer / KTX2 shims. AssetKit   */
/* dlopens it from AK_OPT_GLTF_GSPLAT_DECODER_PATH or, when autoload   */
/* is enabled, from the standard side-library name.                    */
/*                                                                     */
/* L-GSC and the uncompressed base KHR_gaussian_splatting extension do */
/* not need an external decoder. Renderers read primitive accessors.   */
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
