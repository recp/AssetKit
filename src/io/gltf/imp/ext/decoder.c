/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "decoder.h"
#include "../../../../platform/dylib.h"
#include "../../../../../include/ak/gsplat.h"

typedef int
(*AkMeshoptDecodeBufferFn)(void                *destination,
                           size_t               destination_size,
                           const unsigned char *buffer,
                           size_t               buffer_size,
                           size_t               count,
                           size_t               stride,
                           int                  mode,
                           int                  filter);

typedef int
(*AkDracoDecodePrimitiveFn)(AkGLTFState     *gst,
                            AkMeshPrimitive *prim,
                            const json_t    *jprim,
                            const json_t    *jext);

struct AkGLTFMeshoptLib {
  void                    *lib;
  AkMeshoptDecodeBufferFn  decodeBuffer;
  bool                     tried;
};

struct AkGLTFDracoLib {
  void                       *lib;
  AkDracoDecodePrimitiveFn    decodePrimitive;
  bool                        tried;
};

struct AkGLTFSPZLib {
  void                            *lib;
  AkGaussianSplatDecoder           decoder;
  bool                             tried;
};

struct AkGLTFKTX2Lib {
  void          *lib;
  AkKTX2DecodeFn decode;
  bool           tried;
};

static
void*
gltf_ext_openLib(AkOption      opt,
                 const char * __restrict name) {
  const char *path;
  void       *lib;

  path = (const char *)ak_opt_get(opt);
  if (path && (lib = ak_dylib_open(path)))
    return lib;

  if (!ak_opt_get(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD))
    return NULL;

  return ak_dylib_openName(name);
}

AK_HIDE
bool
gltf_ext_meshopt(AkGLTFState * __restrict gst) {
  AkGLTFMeshoptLib *mo;
  void             *lib;

  if (gst->meshopt)
    return gst->meshopt->lib != NULL;

  mo = ak_calloc(NULL, sizeof(*mo));
  mo->tried = true;
  gst->meshopt = mo;

  lib = gltf_ext_openLib(AK_OPT_GLTF_MESHOPT_DECODER_PATH,
                         "assetkit_meshoptimizer");
  if (!lib)
    return false;

  mo->decodeBuffer = (AkMeshoptDecodeBufferFn)
    ak_dylib_sym(lib, "ak_meshopt_decode_gltf_buffer");

  if (!mo->decodeBuffer) {
    ak_dylib_close(lib);
    return false;
  }

  mo->lib = lib;
  return true;
}

AK_HIDE
bool
gltf_ext_spz(AkGLTFState * __restrict gst) {
  AkGLTFSPZLib                    *sp;
  AkGaussianSplatDecoderCreateFn   createFn;
  void                            *lib;

  if (gst->spz)
    return gst->spz->lib != NULL;

  sp        = ak_calloc(NULL, sizeof(*sp));
  sp->tried = true;
  gst->spz  = sp;

  lib = gltf_ext_openLib(AK_OPT_GLTF_GSPLAT_DECODER_PATH,
                         "assetkit_spz");
  if (!lib)
    return false;

  createFn = (AkGaussianSplatDecoderCreateFn)
              ak_dylib_sym(lib, "assetkit_gsplat_create");
  if (!createFn || createFn(&sp->decoder) != 0
      || (!sp->decoder.decodeBytes && !sp->decoder.decodePrimitive)) {
    ak_dylib_close(lib);
    return false;
  }

  sp->lib = lib;
  return true;
}

AK_HIDE
bool
gltf_ext_ktx2(AkGLTFState * __restrict gst) {
  AkGLTFKTX2Lib *kx;
  void          *lib;

  if (gst->ktx2)
    return gst->ktx2->lib != NULL;

  kx        = ak_calloc(NULL, sizeof(*kx));
  kx->tried = true;
  gst->ktx2 = kx;

  lib = gltf_ext_openLib(AK_OPT_GLTF_KTX2_DECODER_PATH,
                         "assetkit_ktx2");
  if (!lib)
    return false;

  kx->decode = (AkKTX2DecodeFn)ak_dylib_sym(lib, "assetkit_ktx2_decode");
  if (!kx->decode) {
    ak_dylib_close(lib);
    return false;
  }

  kx->lib = lib;
  return true;
}

AK_HIDE
bool
gltf_ext_draco(AkGLTFState * __restrict gst) {
  AkGLTFDracoLib *dr;
  void           *lib;

  if (gst->draco)
    return gst->draco->lib != NULL;

  dr = ak_calloc(NULL, sizeof(*dr));
  dr->tried = true;
  gst->draco = dr;

  lib = gltf_ext_openLib(AK_OPT_GLTF_DRACO_DECODER_PATH,
                         "assetkit_draco");
  if (!lib)
    return false;

  dr->decodePrimitive = (AkDracoDecodePrimitiveFn)
    ak_dylib_sym(lib, "ak_draco_decode_gltf_primitive");
  if (!dr->decodePrimitive) {
    ak_dylib_close(lib);
    return false;
  }

  dr->lib = lib;
  return true;
}

AK_HIDE
bool
gltf_ext_spzDecodeBytes(AkGLTFState     * __restrict gst,
                        AkMeshPrimitive * __restrict prim,
                        const uint8_t   * __restrict data,
                        size_t                       size) {
  if (!gltf_ext_spz(gst) || !gst->spz->decoder.decodeBytes)
    return false;

  return gst->spz->decoder.decodeBytes(gst->heap, prim, data, size) == 0;
}

AK_HIDE
bool
gltf_ext_dracoPrimitive(AkGLTFState     * __restrict gst,
                        AkMeshPrimitive * __restrict prim,
                        const json_t    * __restrict jprim) {
  const json_t *jext;
  const json_t *jdraco;

  if (!gst || !prim || !jprim)
    return true;

  jext = GLTF_JSON_GET(jprim, extensions);
  if (!jext)
    return true;

  jdraco = GLTF_JSON_GET(jext, KHR_draco_mesh_compression);
  if (!jdraco)
    return true;

  if (!gltf_ext_draco(gst))
    return false;

  return gst->draco->decodePrimitive(gst, prim, jprim, jdraco) == 0;
}

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
                       int                               filter) {
  AkGLTFMeshoptLib *mo;
  int               res;

  if (!gltf_ext_meshopt(gst))
    return false;

  mo = gst->meshopt;

  if (!dst
      || !src
      || stride == 0
      || count > SIZE_MAX / stride
      || dstSize < count * stride)
    return false;

  res = mo->decodeBuffer(dst,
                         dstSize,
                         src,
                         srcSize,
                         count,
                         stride,
                         mode,
                         filter);

  return res == 0;
}

AK_HIDE
void
gltf_ext_decoderClose(AkGLTFState * __restrict gst) {
  if (!gst)
    return;

  if (gst->meshopt) {
    if (gst->meshopt->lib)
      ak_dylib_close(gst->meshopt->lib);
    ak_free(gst->meshopt);
    gst->meshopt = NULL;
  }

  if (gst->draco) {
    if (gst->draco->lib)
      ak_dylib_close(gst->draco->lib);
    ak_free(gst->draco);
    gst->draco = NULL;
  }

  if (gst->spz) {
    if (gst->spz->decoder.close)
      gst->spz->decoder.close(gst->spz->decoder.userdata);
    if (gst->spz->lib)
      ak_dylib_close(gst->spz->lib);
    ak_free(gst->spz);
    gst->spz = NULL;
  }

  if (gst->ktx2) {
    if (gst->ktx2->lib)
      ak_dylib_close(gst->ktx2->lib);
    ak_free(gst->ktx2);
    gst->ktx2 = NULL;
  }
}
