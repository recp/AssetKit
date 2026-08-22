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

#include "transp.h"
#include "../../../string_fast.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const uint8_t dae_png_signature[8] = {
  0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'
};

static
uint32_t
dae_png_u32be(const uint8_t * __restrict bytes) {
  return ((uint32_t)bytes[0] << 24)
         | ((uint32_t)bytes[1] << 16)
         | ((uint32_t)bytes[2] << 8)
         | (uint32_t)bytes[3];
}

static
bool
dae_png_alpha_color_type(uint8_t colorType) {
  return colorType == 4u || colorType == 6u;
}

static
bool
dae_png_has_alpha_memory(const uint8_t * __restrict data,
                         size_t                      length) {
  size_t offset;

  if (!data
      || length < sizeof(dae_png_signature)
      || memcmp(data, dae_png_signature, sizeof(dae_png_signature)) != 0)
    return false;

  offset = sizeof(dae_png_signature);
  while (length - offset >= 12u) {
    uint32_t       chunkLength;
    const uint8_t *chunkType;
    const uint8_t *chunkData;

    chunkLength = dae_png_u32be(data + offset);
    chunkType   = data + offset + 4u;
    chunkData   = data + offset + 8u;
    if ((size_t)chunkLength > length - offset - 12u)
      return false;

    if (memcmp(chunkType, "IHDR", 4u) == 0) {
      if (chunkLength != 13u)
        return false;
      if (dae_png_alpha_color_type(chunkData[9]))
        return true;
    } else if (memcmp(chunkType, "tRNS", 4u) == 0) {
      return chunkLength > 0u;
    } else if (memcmp(chunkType, "IDAT", 4u) == 0
               || memcmp(chunkType, "IEND", 4u) == 0) {
      return false;
    }

    offset += (size_t)chunkLength + 12u;
  }

  return false;
}

static
bool
dae_png_has_alpha_file(const char * __restrict path) {
  FILE    *file;
  uint8_t  signature[sizeof(dae_png_signature)];
  bool     hasAlpha;

  if (!path || !(file = fopen(path, "rb")))
    return false;

  hasAlpha = false;
  if (fread(signature, 1u, sizeof(signature), file) != sizeof(signature)
      || memcmp(signature, dae_png_signature, sizeof(signature)) != 0)
    goto done;

  for (;;) {
    uint8_t  chunkHeader[8];
    uint8_t  ihdr[13];
    uint32_t chunkLength;

    if (fread(chunkHeader, 1u, sizeof(chunkHeader), file)
        != sizeof(chunkHeader))
      break;

    chunkLength = dae_png_u32be(chunkHeader);
    if (memcmp(chunkHeader + 4u, "IHDR", 4u) == 0) {
      if (chunkLength != sizeof(ihdr)
          || fread(ihdr, 1u, sizeof(ihdr), file) != sizeof(ihdr))
        break;
      if (dae_png_alpha_color_type(ihdr[9])) {
        hasAlpha = true;
        break;
      }
    } else if (memcmp(chunkHeader + 4u, "tRNS", 4u) == 0) {
      hasAlpha = chunkLength > 0u;
      break;
    } else if (memcmp(chunkHeader + 4u, "IDAT", 4u) == 0
               || memcmp(chunkHeader + 4u, "IEND", 4u) == 0) {
      break;
    } else if (chunkLength > (uint32_t)(LONG_MAX - 4L)
               || fseek(file, (long)chunkLength, SEEK_CUR) != 0) {
      break;
    }

    if (fseek(file, 4L, SEEK_CUR) != 0)
      break;
  }

done:
  fclose(file);
  return hasAlpha;
}

static
bool
dae_image_source_has_png_alpha(AkImage * __restrict image) {
  AkImageSource *source;

  if (!image)
    return false;

  source = image->source ? image->source
                         : image->image ? image->image->source : NULL;
  if (!source)
    return false;

  switch (source->type) {
    case AK_IMAGE_SOURCE_URI:
      return dae_png_has_alpha_file(ak_imageResolvePath(image));
    case AK_IMAGE_SOURCE_BUFFER:
      return source->buffer
             && dae_png_has_alpha_memory(source->buffer->data,
                                         source->buffer->length);
    default:
      break;
  }

  return false;
}

static
char
dae_ascii_tolower(char c) {
  if (c >= 'A' && c <= 'Z')
    return (char)(c + ('a' - 'A'));

  return c;
}

static
bool
dae_contains_nocase(const char * __restrict str,
                    const char * __restrict needle) {
  const char *s, *n, *p;

  if (!str || !needle || !needle[0])
    return false;

  for (p = str; *p; p++) {
    s = p;
    n = needle;

    while (*s
           && *n
           && dae_ascii_tolower(*s) == dae_ascii_tolower(*n)) {
      s++;
      n++;
    }

    if (!*n)
      return true;
  }

  return false;
}

static
bool
dae_is_version_token_char(char c) {
  return (c >= '0' && c <= '9')
         || (c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z')
         || c == '.'
         || c == '_'
         || c == '-';
}

static
bool
dae_contains_exact_version_nocase(const char * __restrict str,
                                  const char * __restrict signature) {
  const char *s, *n, *p;

  if (!str || !signature || !signature[0])
    return false;

  for (p = str; *p; p++) {
    if (p != str && dae_is_version_token_char(p[-1]))
      continue;

    s = p;
    n = signature;
    while (*s && *n
           && dae_ascii_tolower(*s) == dae_ascii_tolower(*n)) {
      s++;
      n++;
    }

    if (!*n && !dae_is_version_token_char(*s))
      return true;
  }

  return false;
}

static
const char*
dae_parse_next_uint(const char * __restrict p,
                    int        * __restrict out) {
  AkUInt value;

  while (*p && (*p < '0' || *p > '9'))
    p++;
  if (!*p)
    return NULL;

  p = ak_str_parse_uint_fast((char *)p, NULL, &value);
  if (value > (AkUInt)INT_MAX)
    return NULL;

  *out = (int)value;
  return p;
}

AK_HIDE
void
dae_bugfix_transp(AkTransparent * __restrict transp,
                  bool                       opaqueSpecified) {
  AkContributor *contr;
  const char    *tool;

  if (!(contr = ak_getAssetInfo(transp, offsetof(AkAssetInf, contributor)))
      || !(tool = (const char *)contr->authoringTool))
    return;

  /* ColladaMaya v2.03b / FCollada v1.13 wrote this exact opaque-material
     sentinel with the schema-default A_ONE mode.  The zero factor makes the
     material invisible under the COLLADA equation even though black was the
     exporter's opaque transparency color.  Explicit opaque modes are authored
     semantics and must not be reinterpreted. */
  if (!opaqueSpecified
      && transp->opaque == AK_OPAQUE_A_ONE
      && transp->amount == 0.0f
      && transp->color
      && transp->color->color
      && !transp->color->param
      && !transp->color->texture
      && transp->color->color->rgba.R == 0.0f
      && transp->color->color->rgba.G == 0.0f
      && transp->color->color->rgba.B == 0.0f
      && transp->color->color->rgba.A == 1.0f
      && dae_contains_exact_version_nocase(tool, "ColladaMaya v2.03b")
      && dae_contains_exact_version_nocase(tool, "FCollada v1.13")) {
    transp->amount = 1.0f;
    return;
  }

  /* fix old SketchUp transparency bug */
  if (dae_contains_nocase(tool, _s_dae_sketchup)) {
    const char *p;
    int         major, minor, patch;

    major = minor = patch = 0;
    if ((p = dae_parse_next_uint(tool, &major))) {
      if ((p = dae_parse_next_uint(p, &minor)))
        dae_parse_next_uint(p, &patch);

      /* don't flip >= 7.1.1 */
      if (major < 7
          || (major == 7
              && (minor < 1 || (minor == 1 && patch < 1)))) {
        transp->amount = 1.0f - transp->amount;
      }
    }
  } /* _s_dae_sketchup */
}

AK_HIDE
bool
dae_bugfix_transp_infer_diffuse_alpha(DAEState            * __restrict dst,
                                      AkTechniqueFxCommon * __restrict techn) {
  AkContributor *contr;
  AkColorDesc   *diffuse;
  AkTextureRef  *diffuseRef;
  AkTextureRef  *opacityRef;
  AkTransparent *transp;
  AkColorDesc   *color;
  AkImage       *image;
  const char    *tool;

  if (!dst || !dst->heap || !techn || techn->transparent)
    return false;

  if (!(contr = ak_getAssetInfo(techn, offsetof(AkAssetInf, contributor)))
      || !(tool = (const char *)contr->authoringTool)
      || !dae_contains_nocase(tool, _s_dae_sketchup))
    return false;

  diffuse    = techn->albedo ? techn->albedo : techn->constantDiffuse;
  diffuseRef = diffuse ? diffuse->texture : NULL;
  image      = diffuseRef && diffuseRef->texture
                 ? diffuseRef->texture->image
                 : NULL;
  if (!dae_image_source_has_png_alpha(image))
    return false;

  transp     = ak_heap_calloc(dst->heap, techn, sizeof(*transp));
  color      = ak_heap_calloc(dst->heap, transp, sizeof(*color));
  opacityRef = ak_heap_calloc(dst->heap, color, sizeof(*opacityRef));
  if (!transp || !color || !opacityRef)
    return false;

  *opacityRef            = *diffuseRef;
  opacityRef->channels   = AK_TEXTURE_CHANNEL_A;
  opacityRef->colorSpace = AK_TEXTURE_COLORSPACE_LINEAR;

  color->texture     = opacityRef;
  transp->color      = color;
  transp->amount     = 1.0f;
  transp->opaque     = AK_OPAQUE_A_ONE;
  techn->transparent = transp;

  return true;
}
