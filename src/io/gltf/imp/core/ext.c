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

#include "ext.h"
#include "../../../../platform/dylib.h"

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

typedef enum AkGLTFMeshoptMode {
  AK_GLTF_MESHOPT_MODE_UNKNOWN = 0,
  AK_GLTF_MESHOPT_MODE_ATTRIBUTES,
  AK_GLTF_MESHOPT_MODE_TRIANGLES,
  AK_GLTF_MESHOPT_MODE_INDICES
} AkGLTFMeshoptMode;

typedef enum AkGLTFMeshoptFilter {
  AK_GLTF_MESHOPT_FILTER_NONE = 0,
  AK_GLTF_MESHOPT_FILTER_OCTAHEDRAL,
  AK_GLTF_MESHOPT_FILTER_QUATERNION,
  AK_GLTF_MESHOPT_FILTER_EXPONENTIAL
} AkGLTFMeshoptFilter;

static const char *AK_MESHOPT_LIBS[] = {
  "assetkit_meshoptimizer.dll",
  "libassetkit_meshoptimizer.dylib",
  "libassetkit_meshoptimizer.so"
};

static const char *AK_DRACO_LIBS[] = {
  "assetkit_draco.dll",
  "libassetkit_draco.dylib",
  "libassetkit_draco.so"
};

static
void*
gltf_ext_openLib(AkOption                   opt,
                 const char * const       * __restrict names,
                 uint32_t                              nameCount) {
  const char *path;
  void       *lib;
  uint32_t    i;

  path = (const char *)ak_opt_get(opt);
  if (path && (lib = ak_dylib_open(path)))
    return lib;

  if (!ak_opt_get(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD))
    return NULL;

  for (i = 0; i < nameCount; i++) {
    if ((lib = ak_dylib_open(names[i])))
      return lib;
  }

  return NULL;
}

static
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
                         AK_MESHOPT_LIBS,
                         AK_ARRAY_LEN(AK_MESHOPT_LIBS));
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

static
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
                         AK_DRACO_LIBS,
                         AK_ARRAY_LEN(AK_DRACO_LIBS));
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

static
bool
gltf_ext_supported(AkGLTFState      * __restrict gst,
                   const json_t     * __restrict ext) {
  if (!ext)
    return false;

  if (json_val_eq(ext, _s_gltf_KHR_texture_transform))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_clearcoat))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_emissive_strength))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_ior))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_sheen))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_iridescence))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_volume))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_anisotropy))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_dispersion))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_diffuse_transmission))
    return true;
  if (json_val_eq(ext, _s_gltf_ext_pbrSpecGloss))
    return true;
  if (json_val_eq(ext, _s_gltf_ext_KHR_materials_specular))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_transmission))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_materials_unlit))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_animation_pointer))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_node_visibility))
    return true;
  if (json_val_eq(ext, _s_gltf_KHR_mesh_quantization))
    return true;
  if (json_val_eq(ext, _s_gltf_EXT_meshopt_compression)
      || json_val_eq(ext, _s_gltf_KHR_meshopt_compression))
    return gltf_ext_meshopt(gst);
  if (json_val_eq(ext, _s_gltf_KHR_draco_mesh_compression))
    return gltf_ext_draco(gst);
  if (json_val_eq(ext, _s_gltf_KHR_lights_punctual))
    return true;

  return false;
}

static
AkLightType
gltf_ext_lightType(const json_t * __restrict jtype) {
  if (!jtype)
    return AK_LIGHT_TYPE_POINT;

  if (json_val_eq(jtype, _s_gltf_directional))
    return AK_LIGHT_TYPE_DIRECTIONAL;
  if (json_val_eq(jtype, _s_gltf_spot))
    return AK_LIGHT_TYPE_SPOT;
  if (json_val_eq(jtype, _s_gltf_point))
    return AK_LIGHT_TYPE_POINT;

  return AK_LIGHT_TYPE_POINT;
}

static
void
gltf_ext_lights(AkGLTFState * __restrict gst,
                json_t      * __restrict jlights) {
  const json_array_t *jarr;
  json_t             *jlight;
  json_t             *it;
  AkLight            *light;
  AkLightBase        *base;
  AkSpotLight        *spot;

  if (!gst || !(jarr = json_array(jlights)))
    return;

  jlight = jarr->base.value;
  while (jlight) {
    it    = json_get(jlight, _s_gltf_type);
    light = ak_lightMake(gst->doc, gst->doc, gltf_ext_lightType(it));
    if (!light)
      goto nxt;

    base = light->tcommon;
    if ((it = json_get(jlight, _s_gltf_name)))
      light->name = json_strdup(it, gst->heap, light);

    if ((it = json_get(jlight, _s_gltf_color))) {
      json_array_float(base->color.vec, it, 1.0f, 3, true);
      base->color.vec[3] = 1.0f;
    }

    base->intensity = json_float(json_get(jlight, _s_gltf_intensity), 1.0f);
    base->range     = json_float(json_get(jlight, _s_gltf_range),     0.0f);

    if (base->type == AK_LIGHT_TYPE_SPOT
        && (it = json_get(jlight, _s_gltf_spot))) {
      spot = (AkSpotLight *)base;
      spot->innerConeAngle = json_float(json_get(it, _s_gltf_innerConeAngle),
                                        0.0f);
      spot->outerConeAngle = json_float(json_get(it, _s_gltf_outerConeAngle),
                                        GLM_PI_4f);
    }

  nxt:
    jlight = jlight->next;
  }
}

static
AkGLTFMeshoptMode
gltf_ext_meshoptMode(const json_t * __restrict jmode) {
  if (!jmode)
    return AK_GLTF_MESHOPT_MODE_UNKNOWN;

  if (json_val_eq(jmode, _s_gltf_ATTRIBUTES))
    return AK_GLTF_MESHOPT_MODE_ATTRIBUTES;
  if (json_val_eq(jmode, _s_gltf_TRIANGLES))
    return AK_GLTF_MESHOPT_MODE_TRIANGLES;
  if (json_val_eq(jmode, _s_gltf_INDICES))
    return AK_GLTF_MESHOPT_MODE_INDICES;

  return AK_GLTF_MESHOPT_MODE_UNKNOWN;
}

static
AkGLTFMeshoptFilter
gltf_ext_meshoptFilter(const json_t * __restrict jfilter) {
  if (!jfilter || json_val_eq(jfilter, _s_gltf_NONE))
    return AK_GLTF_MESHOPT_FILTER_NONE;

  if (json_val_eq(jfilter, _s_gltf_OCTAHEDRAL))
    return AK_GLTF_MESHOPT_FILTER_OCTAHEDRAL;
  if (json_val_eq(jfilter, _s_gltf_QUATERNION))
    return AK_GLTF_MESHOPT_FILTER_QUATERNION;
  if (json_val_eq(jfilter, _s_gltf_EXPONENTIAL))
    return AK_GLTF_MESHOPT_FILTER_EXPONENTIAL;

  return AK_GLTF_MESHOPT_FILTER_NONE;
}

static
bool
gltf_ext_meshoptDecode(AkGLTFState          * __restrict gst,
                       void                 * __restrict dst,
                       size_t                            dstSize,
                       const unsigned char  * __restrict src,
                       size_t                            srcSize,
                       size_t                            count,
                       size_t                            stride,
                       AkGLTFMeshoptMode                 mode,
                       AkGLTFMeshoptFilter               filter) {
  AkGLTFMeshoptLib *mo;
  int               res;

  if (!gltf_ext_meshopt(gst))
    return false;

  mo  = gst->meshopt;

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
gltf_exts(json_t * __restrict jext,
          void   * __restrict userdata) {
  AkGLTFState        *gst;
  const json_array_t *jexts;
  json_t             *it;

  gst = userdata;

  if (!(jexts = json_array(jext)))
    return;

  for (it = (void *)jexts->base.value; it; it = it->next) {
    if (json_val_eq(it, _s_gltf_KHR_animation_pointer))
      gst->animPointerRequired = true;

    if (!gltf_ext_supported(gst, it)) {
      gst->stop = true;
      return;
    }
  }
}

AK_HIDE
void
gltf_ext_root(json_t * __restrict jext,
              void   * __restrict userdata) {
  AkGLTFState *gst;
  json_t      *jpunctual;
  json_t      *jlights;

  gst = userdata;
  if (!jext)
    return;

  if ((jpunctual = json_get(jext, _s_gltf_KHR_lights_punctual))
      && (jlights = json_get(jpunctual, _s_gltf_lights))) {
    gltf_ext_lights(gst, jlights);
  }
}

AK_HIDE
bool
gltf_ext_node(AkGLTFState * __restrict gst,
              AkNode      * __restrict node,
              const json_t * __restrict jext) {
  json_t  *jpunctual;
  json_t  *jlight;
  json_t  *jvis;
  json_t  *jvisible;
  AkLight *light;
  int32_t  lightIndex;

  if (!gst || !node || !jext)
    return true;

  jvis     = json_get(jext, _s_gltf_KHR_node_visibility);
  jvisible = jvis ? json_get(jvis, _s_gltf_visible) : NULL;
  if (jvisible)
    node->visible = json_bool(jvisible, true);

  jpunctual = json_get(jext, _s_gltf_KHR_lights_punctual);
  jlight    = jpunctual ? json_get(jpunctual, _s_gltf_light) : NULL;
  if (!jlight)
    return true;

  lightIndex = json_int32(jlight, -1);
  if (lightIndex < 0 || !gst->doc->lib.lights)
    return false;

  light = (void *)gst->doc->lib.lights->chld;
  while (light && lightIndex > 0) {
    light = light->next;
    lightIndex--;
  }

  if (!light)
    return false;

  return ak_nodeAttachLight(node, light) != NULL;
}

AK_HIDE
void
gltf_ext_close(AkGLTFState * __restrict gst) {
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
}

AK_HIDE
bool
gltf_ext_bufferView(AkGLTFState  * __restrict gst,
                    AkBufferView * __restrict buffView,
                    const json_t * __restrict jext) {
  const json_t          *jmo;
  const json_t          *it;
  AkBuffer              *srcBuff;
  AkBuffer              *dstBuff;
  const unsigned char   *src;
  size_t                 srcOff;
  size_t                 srcLen;
  size_t                 stride;
  size_t                 count;
  size_t                 dstLen;
  int32_t                buffIdx;
  AkGLTFMeshoptMode      mode;
  AkGLTFMeshoptFilter    filter;

  if (!gst || !buffView || !jext)
    return true;

  jmo = json_get(jext, _s_gltf_EXT_meshopt_compression);
  if (!jmo)
    jmo = json_get(jext, _s_gltf_KHR_meshopt_compression);
  if (!jmo)
    return true;

  if (!gltf_ext_meshopt(gst)) {
    if (buffView->buffer && buffView->buffer->data)
      return true;
    return false;
  }

  it      = json_get(jmo, _s_gltf_buffer);
  buffIdx = it ? json_int32(it, -1) : -1;
  if (buffIdx < 0
      || !(srcBuff = flist_sp_at(&gst->buffers, buffIdx))
      || !srcBuff->data)
    return false;

  srcOff = (it = json_get(jmo, _s_gltf_byteOffset))
             ? (size_t)json_uint64(it, 0) : 0;
  srcLen = (it = json_get(jmo, _s_gltf_byteLength))
             ? (size_t)json_uint64(it, 0) : 0;
  stride = (it = json_get(jmo, _s_gltf_byteStride))
             ? (size_t)json_uint64(it, 0) : 0;
  count  = (it = json_get(jmo, _s_gltf_count))
             ? (size_t)json_uint64(it, 0) : 0;
  mode   = gltf_ext_meshoptMode(json_get(jmo, _s_gltf_mode));
  filter = gltf_ext_meshoptFilter(json_get(jmo, _s_gltf_filter));

  if (srcOff > srcBuff->length
      || srcLen == 0
      || srcLen > srcBuff->length - srcOff
      || stride == 0
      || count == 0)
    return false;

  dstLen = buffView->byteLength;
  if (dstLen == 0) {
    if (count > SIZE_MAX / stride)
      return false;
    dstLen = count * stride;
  }

  dstBuff         = ak_heap_calloc(gst->heap, gst->doc, sizeof(*dstBuff));
  dstBuff->data   = ak_heap_alloc(gst->heap, dstBuff, dstLen);
  dstBuff->length = dstLen;
  src             = (const unsigned char *)srcBuff->data + srcOff;

  if (!gltf_ext_meshoptDecode(gst,
                              dstBuff->data,
                              dstLen,
                              src,
                              srcLen,
                              count,
                              stride,
                              mode,
                              filter))
    return false;

  buffView->buffer     = dstBuff;
  buffView->byteOffset = 0;
  buffView->byteLength = dstLen;
  buffView->byteStride = stride;

  return true;
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

  jext = json_get(jprim, _s_gltf_extensions);
  if (!jext)
    return true;

  jdraco = json_get(jext, _s_gltf_KHR_draco_mesh_compression);
  if (!jdraco)
    return true;

  if (!gltf_ext_draco(gst))
    return true;

  return gst->draco->decodePrimitive(gst, prim, jprim, jdraco) == 0;
}
