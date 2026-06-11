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

#include "material.h"
#include "writer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static
bool
wobj_name_char_ok(unsigned char ch) {
  return ch > 0x20
         && ch != '#'
         && ch != '/'
         && ch != '\\'
         && ch != ':';
}

static
char*
wobj_material_name_dup(AkMaterial * __restrict mat, uint32_t index) {
  const char *src;
  char       *dst;
  size_t      srcLen;
  size_t      prefixLen;
  size_t      len;
  size_t      i;
  int         n;
  char        prefix[32];

  n = snprintf(prefix, sizeof(prefix), "mat_%u", index);
  if (n <= 0 || (size_t)n >= sizeof(prefix))
    return NULL;

  src = mat && mat->name ? mat->name : NULL;
  if (!src || !*src) {
    dst = malloc((size_t)n + 1u);
    if (!dst)
      return NULL;
    memcpy(dst, prefix, (size_t)n + 1u);
    return dst;
  }

  srcLen    = strlen(src);
  prefixLen = (size_t)n;
  if (srcLen > (size_t)-1 - prefixLen - 2u)
    return NULL;

  len = prefixLen + 1u + srcLen;
  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, prefix, prefixLen);
  dst[prefixLen] = '_';

  for (i = 0; i < srcLen; i++) {
    unsigned char ch;

    ch = (unsigned char)src[i];
    dst[prefixLen + 1u + i] = wobj_name_char_ok(ch) ? (char)ch : '_';
  }
  dst[len] = '\0';

  return dst;
}

static
uintptr_t
wobj_ptr_hash(const void * __restrict key) {
  uintptr_t h;

  h  = (uintptr_t)key >> 4;
  h ^= h >> 16;
  h *= (uintptr_t)0x45d9f3bu;
  h ^= h >> 16;
  return h ? h : 1u;
}

static
WOBJExpMaterialSlot*
wobj_material_lookup_slot(WOBJExpMaterialSlot * __restrict slots,
                          uint32_t                         capacity,
                          const AkMaterial    * __restrict material) {
  uintptr_t h;
  uintptr_t mask;

  h    = wobj_ptr_hash(material);
  mask = capacity - 1u;

  for (;;) {
    WOBJExpMaterialSlot *slot;

    slot = &slots[h & mask];
    if (!slot->material || slot->material == material)
      return slot;
    h++;
  }
}

static
bool
wobj_prepare_material_lookup(WOBJExpState * __restrict st) {
  uint32_t capacity;
  uint32_t i;

  if (st->materialCount == 0)
    return true;

  if (st->materialCount > (1u << 29))
    return false;

  capacity = 16u;
  while (capacity < st->materialCount * 4u)
    capacity <<= 1u;

  st->materialLookup = calloc(capacity, sizeof(*st->materialLookup));
  if (!st->materialLookup)
    return false;

  st->materialLookupCapacity = capacity;
  for (i = 0; i < st->materialCount; i++) {
    WOBJExpMaterialSlot *slot;

    slot           = wobj_material_lookup_slot(st->materialLookup,
                                               capacity,
                                               st->materials[i].material);
    slot->material = st->materials[i].material;
    slot->index    = i;
  }

  return true;
}

static
void
wobj_input_rgb(const AkMaterialInput * __restrict input,
               float                              fallback,
               float                 * __restrict out) {
  out[0] = fallback;
  out[1] = fallback;
  out[2] = fallback;

  if (!input)
    return;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
      out[0] = input->color.rgba.R;
      out[1] = input->color.rgba.G;
      out[2] = input->color.rgba.B;
      break;
    case AK_MATERIAL_VALUE_FLOAT4:
    case AK_MATERIAL_VALUE_FLOAT3:
      out[0] = input->value[0];
      out[1] = input->value[1];
      out[2] = input->value[2];
      break;
    case AK_MATERIAL_VALUE_FLOAT2:
      out[0] = input->value[0];
      out[1] = input->value[1];
      out[2] = fallback;
      break;
    case AK_MATERIAL_VALUE_FLOAT:
      out[0] = input->value[0];
      out[1] = input->value[0];
      out[2] = input->value[0];
      break;
    default:
      break;
  }
}

static
float
wobj_input_alpha(const AkMaterialInput * __restrict input,
                 float                              fallback) {
  if (!input)
    return fallback;

  if (input->valueType == AK_MATERIAL_VALUE_COLOR)
    return input->color.rgba.A;
  if (input->valueType == AK_MATERIAL_VALUE_FLOAT4)
    return input->value[3];

  return fallback;
}

static
AkImage*
wobj_input_image(const AkMaterialInput * __restrict input) {
  AkTextureRef   *ref;
  AkTexture      *tex;

  ref = ak_materialInputTexture(input);
  tex = ref ? ref->texture : NULL;
  return tex ? tex->image : NULL;
}

static
AkImageSource*
wobj_image_source(AkImage * __restrict image) {
  if (!image)
    return NULL;

  if (image->source)
    return image->source;

  return image->image ? image->image->source : NULL;
}

static
uint32_t
wobj_image_index(WOBJExpState * __restrict st,
                 AkImage      * __restrict image) {
  AkImage *it;
  uint32_t i;

  if (!st || !image)
    return UINT32_MAX;

  i = 0;
  for (it = st->doc->lib.images.first; it; it = it->next, i++) {
    if (it == image)
      return i;
  }

  return UINT32_MAX;
}

static
const char*
wobj_path_basename(const char * __restrict path) {
  const char *slash;
  const char *backslash;

  if (!path || !*path)
    return "texture";

  slash     = strrchr(path, '/');
  backslash = strrchr(path, '\\');

  if (slash && backslash)
    return slash > backslash ? slash + 1 : backslash + 1;
  if (slash)
    return slash + 1;
  if (backslash)
    return backslash + 1;
  return path;
}

static
bool
wobj_uri_has_scheme(const char * __restrict uri) {
  const char *it;

  if (!uri)
    return false;

  for (it = uri; *it; it++) {
    if (*it == ':' && it != uri)
      return true;
    if (*it == '/' || *it == '\\' || *it == '?' || *it == '#')
      return false;
  }

  return false;
}

static
bool
wobj_uri_is_data(const char * __restrict uri) {
  return uri
         && uri[0] == 'd'
         && uri[1] == 'a'
         && uri[2] == 't'
         && uri[3] == 'a'
         && uri[4] == ':';
}

static
bool
wobj_path_is_absolute(const char * __restrict path) {
  unsigned char drive;

  if (!path || !path[0])
    return false;

  if (path[0] == '/' || path[0] == '\\')
    return true;

  drive = (unsigned char)path[0];
  return ((drive >= 'A' && drive <= 'Z') || (drive >= 'a' && drive <= 'z'))
         && path[1] == ':'
         && (path[2] == '/' || path[2] == '\\');
}

static
bool
wobj_join_path_parts(const char * __restrict dir,
                     const char * __restrict rel,
                     size_t     * __restrict dirLen,
                     size_t     * __restrict relLen,
                     bool       * __restrict sep,
                     size_t     * __restrict need) {
  size_t dlen;
  size_t rlen;
  size_t slen;

  if (!dir || !rel)
    return false;

  dlen = strlen(dir);
  rlen = strlen(rel);
  slen = dlen > 0 && dir[dlen - 1u] != '/' && dir[dlen - 1u] != '\\';
  if (dlen > (size_t)-1 - rlen
      || dlen + rlen > (size_t)-1 - slen
      || dlen + rlen + slen > (size_t)-1 - 1u)
    return false;

  *dirLen = dlen;
  *relLen = rlen;
  *sep    = slen != 0;
  *need   = dlen + rlen + slen + 1u;
  return true;
}

static
char*
wobj_join_path(const char * __restrict dir,
               const char * __restrict rel) {
  char  *path;
  size_t dirLen;
  size_t relLen;
  size_t need;
  bool   sep;

  if (!wobj_join_path_parts(dir, rel, &dirLen, &relLen, &sep, &need))
    return NULL;

  path = malloc(need);
  if (!path)
    return NULL;

  memcpy(path, dir, dirLen);
  if (sep)
    path[dirLen++] = '/';
  memcpy(path + dirLen, rel, relLen);
  path[dirLen + relLen] = '\0';

  return path;
}

static
bool
wobj_copy_file(const char * __restrict src,
               const char * __restrict dst) {
  FILE          *infile;
  FILE          *outfile;
  unsigned char  buf[64u * 1024u];
  size_t         nread;
  bool           ok;

  if (!src || !dst)
    return false;

  if (strcmp(src, dst) == 0)
    return true;

  infile = fopen(src, "rb");
  if (!infile)
    return false;

  outfile = fopen(dst, "wb");
  if (!outfile) {
    fclose(infile);
    return false;
  }

  ok = true;
  while ((nread = fread(buf, 1, sizeof(buf), infile)) > 0) {
    if (fwrite(buf, 1, nread, outfile) != nread) {
      ok = false;
      break;
    }
  }

  if (ferror(infile))
    ok = false;
  if (fclose(outfile) != 0)
    ok = false;
  fclose(infile);

  if (!ok)
    remove(dst);

  return ok;
}

static
bool
wobj_texture_cacheable(WOBJExpState * __restrict st, uint32_t imageIdx) {
  return st && imageIdx != UINT32_MAX && imageIdx < st->imageUriCount;
}

static
char*
wobj_texture_cache_get(WOBJExpState * __restrict st,
                       uint32_t                  imageIdx,
                       bool       * __restrict failed) {
  char *uri;

  if (!wobj_texture_cacheable(st, imageIdx))
    return NULL;

  if (st->imageUriFailed[imageIdx]) {
    if (failed)
      *failed = true;
    return NULL;
  }

  uri = st->imageUris[imageIdx];
  if (!uri)
    return NULL;

  uri = strdup(uri);
  if (!uri && failed)
    *failed = true;

  return uri;
}

static
void
wobj_texture_cache_set(WOBJExpState * __restrict st,
                       uint32_t                  imageIdx,
                       const char * __restrict   uri) {
  if (!wobj_texture_cacheable(st, imageIdx) || !uri || st->imageUris[imageIdx])
    return;

  st->imageUris[imageIdx] = strdup(uri);
}

static
void
wobj_texture_cache_fail(WOBJExpState * __restrict st, uint32_t imageIdx) {
  if (wobj_texture_cacheable(st, imageIdx))
    st->imageUriFailed[imageIdx] = true;
}

static
char*
wobj_export_texture_uri(WOBJExpState         * __restrict st,
                        const AkMaterialInput * __restrict input,
                        bool                 * __restrict failed) {
  AkImage       *image;
  AkImageSource *source;
  const char    *uri;
  const char    *base;
  const char    *srcPath;
  char          *srcOwned;
  char          *dstPath;
  char          *exportUri;
  size_t         baseLen;
  uint32_t       imageIdx;
  uint32_t       exportImageIdx;
  int            n;
  char           prefix[32];

  if (failed)
    *failed = false;

  image  = wobj_input_image(input);
  source = wobj_image_source(image);
  if (!source)
    return NULL;

  imageIdx = wobj_image_index(st, image);
  if ((exportUri = wobj_texture_cache_get(st, imageIdx, failed)))
    return exportUri;
  if (failed && *failed)
    return NULL;

  uri = source->uri;
  if (source->type != AK_IMAGE_SOURCE_URI || !uri || !*uri) {
    if (source->resolvedPath && *source->resolvedPath)
      uri = source->resolvedPath;
    else
      return NULL;
  }

  if (wobj_uri_is_data(uri))
    return NULL;

  if (!st->outDir || wobj_uri_has_scheme(uri)) {
    exportUri = strdup(uri);
    if (!exportUri) {
      wobj_texture_cache_fail(st, imageIdx);
      if (failed)
        *failed = true;
      return NULL;
    }
    wobj_texture_cache_set(st, imageIdx, exportUri);
    return exportUri;
  }

  base = wobj_path_basename(uri);
  if (!base || !*base)
    base = "texture";

  exportImageIdx = imageIdx == UINT32_MAX ? 0u : imageIdx;
  n = snprintf(prefix, sizeof(prefix), "image_%u_", exportImageIdx);
  if (n <= 0 || (size_t)n >= sizeof(prefix))
    return NULL;

  baseLen = strlen(base);
  if (baseLen > (size_t)-1 - (size_t)n - 1u)
    return NULL;

  exportUri = malloc((size_t)n + baseLen + 1u);
  if (!exportUri)
    return NULL;

  memcpy(exportUri, prefix, (size_t)n);
  memcpy(exportUri + (size_t)n, base, baseLen + 1u);

  srcOwned = NULL;
  srcPath  = source->resolvedPath && *source->resolvedPath
             ? source->resolvedPath
             : NULL;
  if (!srcPath && wobj_path_is_absolute(source->uri))
    srcPath = source->uri;
  if (!srcPath && source->uri) {
    srcOwned = ak_getFileFrom(st->doc, source->uri);
    srcPath  = srcOwned;
  }

  if (!srcPath) {
    wobj_texture_cache_fail(st, imageIdx);
    if (failed)
      *failed = true;
    free(exportUri);
    return NULL;
  }

  dstPath = wobj_join_path(st->outDir, exportUri);
  if (!dstPath) {
    if (srcOwned)
      ak_free(srcOwned);
    free(exportUri);
    wobj_texture_cache_fail(st, imageIdx);
    if (failed)
      *failed = true;
    return NULL;
  }

  if (!wobj_copy_file(srcPath, dstPath)) {
    free(dstPath);
    if (srcOwned)
      ak_free(srcOwned);
    free(exportUri);
    wobj_texture_cache_fail(st, imageIdx);
    if (failed)
      *failed = true;
    return NULL;
  }

  free(dstPath);
  if (srcOwned)
    ak_free(srcOwned);

  wobj_texture_cache_set(st, imageIdx, exportUri);
  return exportUri;
}

static
void
wobj_w_float3(WOBJExpWriter * __restrict w,
              const float   * __restrict v) {
  wobj_w_float(w, v[0]);
  wobj_w_ch(w, ' ');
  wobj_w_float(w, v[1]);
  wobj_w_ch(w, ' ');
  wobj_w_float(w, v[2]);
}

static
void
wobj_w_map(WOBJExpState         * __restrict st,
           WOBJExpWriter        * __restrict w,
           const char           * __restrict key,
           const AkMaterialInput * __restrict input) {
  char *uri;
  bool  failed;

  uri = wobj_export_texture_uri(st, input, &failed);
  if (!uri) {
    (void)failed;
    return;
  }

  wobj_w_lit(w, key);
  wobj_w_ch(w, ' ');
  wobj_w_name(w, uri);
  wobj_w_ch(w, '\n');
  free(uri);
}

static
float
wobj_surface_ns(AkMaterialSurface * __restrict surface,
                AkMaterialClassicFeature * __restrict classic) {
  float roughness;
  float ns;

  if (classic && classic->shininess > 0.0f && isfinite(classic->shininess))
    return classic->shininess;

  roughness = glm_clamp_zo(ak_materialRoughnessFactor(surface));
  if (roughness <= 0.001f)
    return 1000.0f;

  ns = 2.0f / (roughness * roughness) - 2.0f;
  if (!isfinite(ns) || ns < 0.0f)
    return 0.0f;
  if (ns > 1000.0f)
    return 1000.0f;
  return ns;
}

static
uint32_t
wobj_surface_illum(AkMaterialSurface * __restrict surface,
                   AkMaterialClassicFeature * __restrict classic) {
  if (classic && classic->illum)
    return classic->illum;
  if (ak_materialUnlit(surface))
    return 0u;
  if (ak_materialAlphaBlend(surface) || ak_materialAlphaMask(surface))
    return 4u;
  return 2u;
}

static
float
wobj_input_scalar_or_avg(const AkMaterialInput * __restrict input,
                         float                              fallback) {
  float rgb[3];

  if (!input)
    return fallback;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_FLOAT:
    case AK_MATERIAL_VALUE_FLOAT2:
    case AK_MATERIAL_VALUE_FLOAT3:
    case AK_MATERIAL_VALUE_FLOAT4:
      return input->value[0];
    case AK_MATERIAL_VALUE_COLOR:
      return (input->color.rgba.R
              + input->color.rgba.G
              + input->color.rgba.B) / 3.0f;
    default:
      wobj_input_rgb(input, fallback, rgb);
      return (rgb[0] + rgb[1] + rgb[2]) / 3.0f;
  }
}

static
void
wobj_write_material(WOBJExpState    * __restrict st,
                    WOBJExpWriter   * __restrict w,
                    WOBJExpMaterial * __restrict out) {
  AkMaterialSurface        *surface;
  AkMaterialClassicFeature *classic;
  AkMaterialClearcoatFeature *clearcoat;
  AkMaterialSheenFeature   *sheen;
  AkMaterialAnisotropyFeature *anisotropy;
  const AkMaterialInput    *baseColor;
  const AkMaterialInput    *emissive;
  const AkMaterialInput    *metallic;
  const AkMaterialInput    *roughness;
  const AkMaterialInput    *opacity;
  const AkMaterialInput    *normal;
  float                     ka[3];
  float                     kd[3];
  float                     ks[3];
  float                     ke[3];
  float                     alpha;

  (void)st;

  surface   = out->material ? out->material->surface : NULL;
  classic   = (AkMaterialClassicFeature *)ak_materialFeature(
    surface, AK_MATERIAL_FEATURE_CLASSIC);
  clearcoat = (AkMaterialClearcoatFeature *)ak_materialFeature(
    surface, AK_MATERIAL_FEATURE_CLEARCOAT);
  sheen = (AkMaterialSheenFeature *)ak_materialFeature(
    surface, AK_MATERIAL_FEATURE_SHEEN);
  anisotropy = (AkMaterialAnisotropyFeature *)ak_materialFeature(
    surface, AK_MATERIAL_FEATURE_ANISOTROPY);
  baseColor = surface ? surface->baseColor : NULL;
  emissive  = surface ? surface->emissive : NULL;
  metallic  = surface ? surface->metallic : NULL;
  roughness = surface ? surface->roughness : NULL;
  opacity   = surface ? surface->opacity : NULL;
  normal    = surface ? surface->normal : NULL;

  wobj_input_rgb(classic ? classic->ambient : NULL, 0.0f, ka);
  wobj_input_rgb(baseColor, 0.8f, kd);
  wobj_input_rgb(classic ? classic->specular : NULL, 0.2f, ks);
  wobj_input_rgb(emissive, 0.0f, ke);
  alpha = glm_clamp_zo(ak_materialOpacityFactor(surface)
                       * wobj_input_alpha(baseColor, 1.0f));

  wobj_w_lit(w, "newmtl ");
  wobj_w_name(w, out->name);
  wobj_w_ch(w, '\n');

  wobj_w_lit(w, "Ka ");
  wobj_w_float3(w, ka);
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Kd ");
  wobj_w_float3(w, kd);
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Ks ");
  wobj_w_float3(w, ks);
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Ke ");
  wobj_w_float3(w, ke);
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Ns ");
  wobj_w_float(w, wobj_surface_ns(surface, classic));
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Pr ");
  wobj_w_float(w, glm_clamp_zo(ak_materialRoughnessFactor(surface)));
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "Pm ");
  wobj_w_float(w, glm_clamp_zo(ak_materialMetallicFactor(surface)));
  wobj_w_ch(w, '\n');
  if (sheen && sheen->color) {
    wobj_w_lit(w, "Ps ");
    wobj_w_float(w, glm_clamp_zo(wobj_input_scalar_or_avg(sheen->color, 0.0f)));
    wobj_w_ch(w, '\n');
  }
  if (clearcoat && clearcoat->factor) {
    wobj_w_lit(w, "Pc ");
    wobj_w_float(w, glm_clamp_zo(ak_materialInputScalar(clearcoat->factor, 0.0f))
                    * 4.0f);
    wobj_w_ch(w, '\n');
  }
  if (clearcoat && clearcoat->roughness) {
    wobj_w_lit(w, "Pcr ");
    wobj_w_float(w, glm_clamp_zo(ak_materialInputScalar(clearcoat->roughness, 0.0f)));
    wobj_w_ch(w, '\n');
  }
  if (anisotropy && anisotropy->strength) {
    wobj_w_lit(w, "aniso ");
    wobj_w_float(w, glm_clamp_zo(ak_materialInputScalar(anisotropy->strength, 0.0f)));
    wobj_w_ch(w, '\n');
  }
  if (anisotropy && anisotropy->rotation) {
    wobj_w_lit(w, "anisor ");
    wobj_w_float(w, glm_clamp_zo(ak_materialInputScalar(anisotropy->rotation, 0.0f)));
    wobj_w_ch(w, '\n');
  }
  wobj_w_lit(w, "Ni ");
  wobj_w_float(w, classic && classic->ior > 0.0f
                  ? classic->ior
                  : ak_materialIor(surface));
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "d ");
  wobj_w_float(w, alpha);
  wobj_w_ch(w, '\n');
  wobj_w_lit(w, "illum ");
  wobj_w_uint(w, wobj_surface_illum(surface, classic));
  wobj_w_ch(w, '\n');

  wobj_w_map(st, w, "map_Ka", classic ? classic->ambient : NULL);
  wobj_w_map(st, w, "map_Kd", baseColor);
  wobj_w_map(st, w, "map_Ks", classic ? classic->specular : NULL);
  wobj_w_map(st, w, "map_Pr", roughness);
  wobj_w_map(st, w, "map_Pm", metallic);
  wobj_w_map(st, w, "map_Ps", sheen ? sheen->color : NULL);
  wobj_w_map(st, w, "map_d", opacity);
  wobj_w_map(st, w, "map_Ke", emissive);
  wobj_w_map(st, w, "bump", normal);
  wobj_w_ch(w, '\n');
}

AK_HIDE
bool
wobj_prepare_materials(WOBJExpState * __restrict st) {
  AkMaterial *mat;
  uint32_t    i;

  st->materialCount = st->doc ? st->doc->lib.materials.count : 0u;
  if (st->materialCount == 0)
    return true;

  st->materials = calloc(st->materialCount, sizeof(*st->materials));
  if (!st->materials)
    return false;

  i = 0;
  for (mat = st->doc->lib.materials.first; mat; mat = mat->next) {
    if (i >= st->materialCount)
      break;
    st->materials[i].material = mat;
    st->materials[i].name     = wobj_material_name_dup(mat, i);
    if (!st->materials[i].name)
      return false;
    i++;
  }

  st->materialCount = i;
  return wobj_prepare_material_lookup(st);
}

AK_HIDE
void
wobj_destroy_materials(WOBJExpState * __restrict st) {
  uint32_t i;

  if (!st || !st->materials)
    return;

  for (i = 0; i < st->materialCount; i++)
    free(st->materials[i].name);

  free(st->materials);
  free(st->materialLookup);
  st->materials              = NULL;
  st->materialLookup         = NULL;
  st->materialCount          = 0;
  st->materialUsedCount      = 0;
  st->materialLookupCapacity = 0;
}

AK_HIDE
bool
wobj_write_mtl(WOBJExpState * __restrict st) {
  WOBJExpWriter w;
  FILE         *file;
  uint32_t      i;

  if (!st->mtlPath || st->materialUsedCount == 0)
    return true;

  file = fopen(st->mtlPath, "wb");
  if (!file)
    return false;
  (void)setvbuf(file, NULL, _IOFBF, 1024u * 1024u);

  memset(&w, 0, sizeof(w));
  w.file   = file;
  w.result = AK_OK;

  wobj_w_lit(&w, "# Generated by AssetKit\n\n");
  for (i = 0; i < st->materialCount; i++) {
    if (!st->materials[i].used)
      continue;
    wobj_write_material(st, &w, &st->materials[i]);
  }

  wobj_w_flush(&w);
  if (fclose(file) != 0 && w.result == AK_OK)
    w.result = AK_ERR;

  return w.result == AK_OK;
}

AK_HIDE
bool
wobj_use_material(WOBJExpState * __restrict st,
                  uint32_t                  matIdx) {
  WOBJExpMaterial *material;

  if (!st || matIdx >= st->materialCount || !st->materials)
    return false;

  material = &st->materials[matIdx];
  if (!material->used) {
    material->used = true;
    st->materialUsedCount++;
  }

  if (!st->wroteMtllib) {
    st->wroteMtllib = true;
    if (st->mtlBaseName) {
      wobj_w_lit(&st->w, "mtllib ");
      wobj_w_name(&st->w, st->mtlBaseName);
      wobj_w_ch(&st->w, '\n');
    }
  }

  wobj_w_lit(&st->w, "usemtl ");
  wobj_w_name(&st->w, material->name);
  wobj_w_ch(&st->w, '\n');

  return st->w.result == AK_OK;
}

AK_HIDE
uint32_t
wobj_material_index(WOBJExpState * __restrict st,
                    AkMaterial   * __restrict material) {
  WOBJExpMaterialSlot *slot;

  if (!st || !material || !st->materialLookup)
    return UINT32_MAX;

  slot = wobj_material_lookup_slot(st->materialLookup,
                                   st->materialLookupCapacity,
                                   material);
  if (slot->material)
    return slot->index;

  return UINT32_MAX;
}
