/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef gltf_imp_ext_decoder_h
#define gltf_imp_ext_decoder_h

#include "../common.h"

AK_HIDE
bool
gltf_ext_meshopt(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_draco(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_spz(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_ktx2(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_textureBasisu(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_spzDecodeBytes(AkGLTFState     * __restrict gst,
                        AkMeshPrimitive * __restrict prim,
                        const uint8_t   * __restrict data,
                        size_t                       size);

AK_HIDE
bool
gltf_ext_meshoptDecode(AkGLTFState          * __restrict gst,
                       void                 * __restrict dst,
                       size_t                            dstSize,
                       const unsigned char  * __restrict src,
                       size_t                            srcSize,
                       size_t                            count,
                       size_t                            stride,
                       int                               mode,
                       int                               filter);

AK_HIDE
void
gltf_ext_decoderClose(AkGLTFState * __restrict gst);

#endif /* gltf_imp_ext_decoder_h */
