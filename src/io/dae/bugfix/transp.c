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
#include "../../../image/alpha.h"
#include "../../../string_fast.h"

#include <limits.h>
#include <string.h>

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
  if (ak_imageAlphaPresence(image) != AK_IMAGE_ALPHA_PRESENT)
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
