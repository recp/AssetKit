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

#include "mtl.h"
#include "../../string_fast.h"
#include "../../strpool.h"

#include <math.h>

/*
 Resources:
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
   http://paulbourke.net/dataformats/obj/
   http://paulbourke.net/dataformats/mtl/
   https://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library
*/

static
void
wobj_handleMaterial(WOState  * __restrict wst,
                    WOMtlLib * __restrict mtllib,
                    WOMtl    * __restrict mtl);

static
AkTextureRef*
wobj_texref(WOState            * __restrict wst,
            void               * __restrict memp,
            char               *            name,
            AkTextureColorSpace             colorSpace,
            AkTextureChannels               channels);

#define WOBJ_MTL_KW_NEWM AK_STR_PACK4_CHARS('n', 'e', 'w', 'm')
#define WOBJ_MTL_KW_MAP_ AK_STR_PACK4_CHARS('m', 'a', 'p', '_')
#define WOBJ_MTL_KW_BUMP AK_STR_PACK4_CHARS('b', 'u', 'm', 'p')
#define WOBJ_MTL_KW_ILLU AK_STR_PACK4_CHARS('i', 'l', 'l', 'u')

static
AkTextureChannels
wobj_mtl_imfchan(char ch) {
  switch (ch) {
    case 'r':
    case 'R': return AK_TEXTURE_CHANNEL_R;
    case 'g':
    case 'G': return AK_TEXTURE_CHANNEL_G;
    case 'b':
    case 'B': return AK_TEXTURE_CHANNEL_B;
    case 'a':
    case 'A': return AK_TEXTURE_CHANNEL_A;
    default:  return AK_TEXTURE_CHANNEL_NONE;
  }
}

static
AkTextureChannels
wobj_mtl_map_channels(AkTextureChannels parsed,
                      AkTextureChannels fallback) {
  return parsed != AK_TEXTURE_CHANNEL_NONE ? parsed : fallback;
}

static
char*
wobj_mtl_map_value(AkHeap             * __restrict heap,
                   WOMtl              * __restrict mtl,
                   const char         * __restrict begin,
                   const char         * __restrict end,
                   AkTextureChannels  * __restrict channels) {
  const char *p;
  const char *tokBegin;
  const char *tokEnd;
  const char *pathBegin;
  const char *pathEnd;
  size_t      tokLen;
  bool        sawOption;

  if (channels)
    *channels = AK_TEXTURE_CHANNEL_NONE;
  if (!begin || !end || end <= begin)
    return NULL;

  p         = begin;
  pathBegin = NULL;
  pathEnd   = NULL;
  sawOption = false;
  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t'))
      p++;
    if (p >= end)
      break;

    tokBegin = p;
    while (p < end && *p != ' ' && *p != '\t')
      p++;
    tokEnd = p;
    tokLen = (size_t)(tokEnd - tokBegin);

    if (ak_str_eq_fast(tokBegin, tokLen, "-imfchan", 8u)) {
      sawOption = true;
      while (p < end && (*p == ' ' || *p == '\t'))
        p++;
      if (p < end && channels) {
        *channels = wobj_mtl_imfchan(*p);
        while (p < end && *p != ' ' && *p != '\t')
          p++;
      }
      continue;
    }

    if (tokLen > 0 && tokBegin[0] == '-') {
      sawOption = true;
      continue;
    }

    pathBegin = tokBegin;
    pathEnd   = tokEnd;
  }

  if (!sawOption) {
    while (begin < end && (*begin == ' ' || *begin == '\t'))
      begin++;
    while (end > begin && (end[-1] == ' ' || end[-1] == '\t'))
      end--;
    pathBegin = begin;
    pathEnd   = end;
  }

  if (!pathBegin || pathEnd <= pathBegin)
    return NULL;
  return ak_heap_strndup(heap, mtl, pathBegin, (size_t)(pathEnd - pathBegin));
}

static
void
wobj_featurePush(AkMaterialSurface * __restrict surface,
                 AkMaterialFeature * __restrict feature) {
  if (!surface || !feature)
    return;

  feature->next     = surface->features;
  surface->features = feature;
  if ((uint32_t)feature->type < 32)
    surface->featureMask |= 1u << (uint32_t)feature->type;
}

static
AkMaterialFeature*
wobj_feature(WOState              * __restrict wst,
             AkMaterialSurface    * __restrict surface,
             AkMaterialFeatureType             type,
             size_t                            size) {
  AkMaterialFeature *feature;

  feature = ak_materialFeature(surface, type);
  if (feature)
    return feature;

  feature       = ak_heap_calloc(wst->heap, surface, size);
  feature->type = type;
  wobj_featurePush(surface, feature);

  return feature;
}

static
AkMaterialInput*
wobj_colorInput(WOState             * __restrict wst,
                void                * __restrict parent,
                const char          * __restrict semantic,
                float               *            rgb,
                char                * __restrict map,
                AkTextureColorSpace              colorSpace,
                AkTextureChannels                channels) {
  AkMaterialInput *input;

  input             = ak_heap_aligned_calloc(wst->heap,
                                              parent,
                                              AK_ALIGNOF(AkMaterialInput),
                                              sizeof(*input));
  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_COLOR;
  input->colorSpace = map ? colorSpace : AK_TEXTURE_COLORSPACE_LINEAR;
  input->channels   = channels;
  input->color.rgba.A = 1.0f;

  if (rgb)
    glm_vec3_copy(rgb, input->color.vec);

  if (map) {
    input->texture = wobj_texref(wst, parent, map, colorSpace, channels);
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

static
AkMaterialInput*
wobj_scalarInput(WOState             * __restrict wst,
                 void                * __restrict parent,
                 const char          * __restrict semantic,
                 float                            value,
                 char                * __restrict map,
                 AkTextureColorSpace              colorSpace,
                 AkTextureChannels                channels) {
  AkMaterialInput *input;

  input             = ak_heap_aligned_calloc(wst->heap,
                                              parent,
                                              AK_ALIGNOF(AkMaterialInput),
                                              sizeof(*input));
  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_FLOAT;
  input->value[0]   = value;
  input->colorSpace = map ? colorSpace : AK_TEXTURE_COLORSPACE_LINEAR;
  input->channels   = channels;

  if (map) {
    input->texture = wobj_texref(wst, parent, map, colorSpace, channels);
    input->source  = AK_MATERIAL_INPUT_TEXTURE;
  }

  return input;
}

AK_HIDE
WOMtlLib*
wobj_mtl(WOState    * __restrict wst,
         const char * __restrict name) {
  AkHeap   *heap;
  void     *mtlstr;
  char     *p, *localurl, *begin, *end;
  WOMtlLib *mtllib;
  WOMtl    *mtl;
  size_t    mtlstrSize;
  char      c;

  mtllib   = NULL;
  mtlstr   = NULL;
  localurl = ak_getFileFrom(wst->doc, name);

  if (!localurl
      || ak_readfile(localurl, NULL, &mtlstr, &mtlstrSize) != AK_OK
      || !((p = mtlstr) && (c = *p) != '\0'))
    goto ret;

  heap              = wst->heap;
  mtl               = NULL;
  mtllib            = ak_heap_calloc(heap, wst->tmp, sizeof(*mtllib));
  mtllib->materials = rb_newtree_str();
  
  /* parse .mtl */
  do {
    /* skip spaces */
    SKIP_SPACES
    
    if (ak_str_pack4_fast(p, 4) == WOBJ_MTL_KW_NEWM
        && p[4] == 't'
        && p[5] == 'l'
        && (p[6] == ' ' || p[6] == '\t')) {
      p += 6;
      SKIP_SPACES
      
      begin = p;
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
      end = p;
      
      if (end > begin) {
        if (mtl)
          wobj_handleMaterial(wst, mtllib, mtl);
        
        mtl       = ak_heap_calloc(heap, wst->tmp, sizeof(*mtl));
        mtl->name = ak_heap_strndup(heap, mtl, begin, end - begin);
        
        /* default params */
        mtl->Tr    = 0.0f;
        mtl->d     = 1.0f;
        mtl->illum = 1;
      }
    } else if (mtl) {
      if (p[1] == ' ' || p[1] == '\t') {
        if (p[0] == 'd') {
          p++;
          ak_strtof_line(p, 0, 1, &mtl->d);
        }
      } else if (p[2] == ' ' || p[2] == '\t') {
        switch (p[0]) {
          case 'K':
            switch (p[1]) {
              case 'a':
                p += 2;
                ak_strtof_line(p, 0, 3, mtl->Ka);
                break;
              case 'd':
                p += 2;
                ak_strtof_line(p, 0, 3, mtl->Kd);
                break;
              case 's':
                p += 2;
                ak_strtof_line(p, 0, 3, mtl->Ks);
                break;
              case 'e':
                p += 2;
                ak_strtof_line(p, 0, 3, mtl->Ke);
                break;
              default:
                p += 2;
                break;
            }
            break;
          case 'N':
            switch (p[1]) {
              case 's':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Ns);
                mtl->has_Ns = true;
                break;
              case 'i':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Ni);
                break;
              default:
                p += 2;
                break;
            }
            break;
          case 'P':
            switch (p[1]) {
              case 'r':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Pr);
                mtl->has_Pr = true;
                break;
              case 'm':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Pm);
                mtl->has_Pm = true;
                break;
              case 's':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Ps);
                mtl->has_Ps = true;
                break;
              case 'c':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Pc);
                mtl->has_Pc = true;
                break;
              default:
                p += 2;
                break;
            }
            break;
          case 'T':
            switch (p[1]) {
              case 'f':
                p += 2;
                ak_strtof_line(p, 0, 3, mtl->Tf);
                mtl->has_Tf = true;
                break;
              case 'r':
                p += 2;
                ak_strtof_line(p, 0, 1, &mtl->Tr);
                break;
              default:
                p += 2;
                break;
            }
            break;
          default:
            p += 2;
            break;
        }
      } else if (ak_str_pack4_fast(p, 4) == WOBJ_MTL_KW_MAP_) {
        p += 4;
        switch (p[0]) {
          case 'd':
            if (p[1] == ' ' || p[1] == '\t') {
              p++;
              SKIP_SPACES

              begin = p;
              while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
              end = p;

              mtl->map_d = wobj_mtl_map_value(heap,
                                               mtl,
                                               begin,
                                               end,
                                               &mtl->map_d_channels);
            }
            break;
          case 'K':
            switch (p[1]) {
              case 'a':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Ka = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Ka_channels);
                break;
              case 'd':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Kd = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Kd_channels);
                
                break;
              case 's':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Ks = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Ks_channels);
                
                break;
              case 'e':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Ke = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Ke_channels);
                
                break;
              default: break;
            }
            break;
          case 'P':
            switch (p[1]) {
              case 'r':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;

                mtl->map_Pr = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Pr_channels);
                break;
              case 'm':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;

                mtl->map_Pm = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Pm_channels);
                break;
              case 's':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;

                mtl->map_Ps = wobj_mtl_map_value(heap,
                                                  mtl,
                                                  begin,
                                                  end,
                                                  &mtl->map_Ps_channels);
                break;
              default: break;
            }
            break;
          default: break;
        }
      } else if (p[0] == 'P'
                 && p[1] == 'c'
                 && p[2] == 'r'
                 && (p[3] == ' ' || p[3] == '\t')) {
        p += 3;
        ak_strtof_line(p, 0, 1, &mtl->Pcr);
        mtl->has_Pcr = true;
      } else if (p[0] == 'a'
                 && p[1] == 'n'
                 && p[2] == 'i'
                 && p[3] == 's'
                 && p[4] == 'o') {
        if (p[5] == 'r' && (p[6] == ' ' || p[6] == '\t')) {
          p += 6;
          ak_strtof_line(p, 0, 1, &mtl->anisor);
          mtl->has_anisor = true;
        } else if (p[5] == ' ' || p[5] == '\t') {
          p += 5;
          ak_strtof_line(p, 0, 1, &mtl->aniso);
          mtl->has_aniso = true;
        }
      } else if (ak_str_pack4_fast(p, 4) == WOBJ_MTL_KW_BUMP) {
        p += 4;

        SKIP_SPACES

        begin = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
        end = p;

        mtl->bump = wobj_mtl_map_value(heap,
                                       mtl,
                                       begin,
                                       end,
                                       &mtl->bump_channels);
      } else if (ak_str_pack4_fast(p, 4) == WOBJ_MTL_KW_ILLU
                 && p[4] == 'm'
                 && (p[5] == ' ' || p[5] == '\t')) {
        p += 5;
        ak_strtoi_line(p, 0, 1, &mtl->illum);
      }
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  if (mtl)
    wobj_handleMaterial(wst, mtllib, mtl);

ret:
  if (mtlstr)
    ak_releasefile(mtlstr, mtlstrSize);
  ak_free((void *)localurl);
  return mtllib;
}

static
void
wobj_handleMaterial(WOState  * __restrict wst,
                    WOMtlLib * __restrict mtllib,
                    WOMtl    * __restrict mtl) {
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkMaterial               *mat;
  AkMaterialSurface        *surface;
  AkMaterialClassicFeature *classic;

  heap = wst->heap;
  doc  = wst->doc;
  
  mat     = ak_heap_calloc(heap, doc, sizeof(*mat));
  ak_setypeid(mat, AKT_MATERIAL);
  surface = ak_heap_calloc(heap, mat,    sizeof(*surface));
  classic = ak_heap_calloc(heap, surface, sizeof(*classic));

  if (mtl->has_Pr
      || mtl->map_Pr
      || mtl->has_Pm
      || mtl->map_Pm
      || mtl->has_Ps
      || mtl->map_Ps
      || mtl->has_Pc
      || mtl->has_Pcr
      || mtl->has_aniso
      || mtl->has_anisor) {
    /* The Exocortex/Adobe PBR MTL fields extend, rather than merely decorate,
       the classic illum model. Consumers must route these materials through
       the metallic-roughness path or map_Pm/map_Pr are silently ignored. */
    surface->type = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  } else {
    switch (mtl->illum) {
      case 0: /* Constant */
        surface->type = AK_MATERIAL_TYPE_CONSTANT;
        break;
      case 1: /* Lambert */
        surface->type = AK_MATERIAL_TYPE_LAMBERT;
        break;
      case 2: /* OBJ Ns is a Phong-style specular exponent. */
//      case 3:
//      case 4:
        surface->type = AK_MATERIAL_TYPE_PHONG;
      default:
        break;
    }

    if (surface->type == AK_MATERIAL_TYPE_NONE)
      surface->type = AK_MATERIAL_TYPE_PHONG;
  }

  surface->baseColor = wobj_colorInput(wst,
                                       surface,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR),
                                       mtl->Kd,
                                       mtl->map_Kd,
                                       AK_TEXTURE_COLORSPACE_SRGB,
                                       wobj_mtl_map_channels(mtl->map_Kd_channels,
                                                             AK_TEXTURE_CHANNEL_RGBA));
  surface->emissive = wobj_colorInput(wst,
                                      surface,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                                      mtl->Ke,
                                      mtl->map_Ke,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      wobj_mtl_map_channels(mtl->map_Ke_channels,
                                                            AK_TEXTURE_CHANNEL_RGB));
  surface->alphaCutoff = 0.5f;
  surface->ior = mtl->Ni;
  surface->emissiveStrength = 1.0f;

  if ((mtl->has_Pr && isfinite(mtl->Pr)) || mtl->map_Pr) {
    float value;

    value = mtl->has_Pr && isfinite(mtl->Pr) ? glm_clamp_zo(mtl->Pr) : 1.0f;
    surface->roughness = wobj_scalarInput(wst,
                                          surface,
                                          ak_materialSemanticName(AK_MATERIAL_SEMANTIC_ROUGHNESS),
                                          value,
                                          mtl->map_Pr,
                                          AK_TEXTURE_COLORSPACE_LINEAR,
                                          wobj_mtl_map_channels(mtl->map_Pr_channels,
                                                                AK_TEXTURE_CHANNEL_R));
  }

  if ((mtl->has_Pm && isfinite(mtl->Pm)) || mtl->map_Pm) {
    float value;

    value = mtl->has_Pm && isfinite(mtl->Pm) ? glm_clamp_zo(mtl->Pm) : 1.0f;
    surface->metallic = wobj_scalarInput(wst,
                                         surface,
                                         ak_materialSemanticName(AK_MATERIAL_SEMANTIC_METALLIC),
                                         value,
                                         mtl->map_Pm,
                                         AK_TEXTURE_COLORSPACE_LINEAR,
                                         wobj_mtl_map_channels(mtl->map_Pm_channels,
                                                               AK_TEXTURE_CHANNEL_R));
  }

  if ((mtl->has_Ps && isfinite(mtl->Ps)) || mtl->map_Ps) {
    AkMaterialSheenFeature *sheen;
    float                   value;

    sheen = (AkMaterialSheenFeature *)wobj_feature(wst,
                                                   surface,
                                                   AK_MATERIAL_FEATURE_SHEEN,
                                                   sizeof(*sheen));
    value = mtl->has_Ps && isfinite(mtl->Ps) ? glm_clamp_zo(mtl->Ps) : 1.0f;
    sheen->color = wobj_scalarInput(wst,
                                    sheen,
                                    _s_ak_sheenColor,
                                    value,
                                    mtl->map_Ps,
                                    AK_TEXTURE_COLORSPACE_LINEAR,
                                    wobj_mtl_map_channels(mtl->map_Ps_channels,
                                                          AK_TEXTURE_CHANNEL_R));
  }

  if ((mtl->has_Pc && isfinite(mtl->Pc))
      || (mtl->has_Pcr && isfinite(mtl->Pcr))) {
    AkMaterialClearcoatFeature *clearcoat;

    clearcoat = (AkMaterialClearcoatFeature *)wobj_feature(
      wst, surface, AK_MATERIAL_FEATURE_CLEARCOAT, sizeof(*clearcoat));

    if (mtl->has_Pc && isfinite(mtl->Pc)) {
      clearcoat->factor = wobj_scalarInput(wst,
                                           clearcoat,
                                           _s_ak_clearcoat,
                                           glm_clamp_zo(mtl->Pc * 0.25f),
                                           NULL,
                                           AK_TEXTURE_COLORSPACE_LINEAR,
                                           AK_TEXTURE_CHANNEL_R);
    }

    if (mtl->has_Pcr && isfinite(mtl->Pcr)) {
      clearcoat->roughness = wobj_scalarInput(wst,
                                              clearcoat,
                                              _s_ak_clearcoatRoughness,
                                              glm_clamp_zo(mtl->Pcr),
                                              NULL,
                                              AK_TEXTURE_COLORSPACE_LINEAR,
                                              AK_TEXTURE_CHANNEL_R);
    }
  }

  if ((mtl->has_aniso && isfinite(mtl->aniso))
      || (mtl->has_anisor && isfinite(mtl->anisor))) {
    AkMaterialAnisotropyFeature *anisotropy;

    anisotropy = (AkMaterialAnisotropyFeature *)wobj_feature(
      wst, surface, AK_MATERIAL_FEATURE_ANISOTROPY, sizeof(*anisotropy));

    if (mtl->has_aniso && isfinite(mtl->aniso)) {
      anisotropy->strength = wobj_scalarInput(wst,
                                              anisotropy,
                                              _s_ak_anisotropyStrength,
                                              glm_clamp_zo(mtl->aniso),
                                              NULL,
                                              AK_TEXTURE_COLORSPACE_LINEAR,
                                              AK_TEXTURE_CHANNEL_R);
    }

    if (mtl->has_anisor && isfinite(mtl->anisor)) {
      anisotropy->rotation = wobj_scalarInput(wst,
                                              anisotropy,
                                              _s_ak_anisotropyRotation,
                                              glm_clamp_zo(mtl->anisor),
                                              NULL,
                                              AK_TEXTURE_COLORSPACE_LINEAR,
                                              AK_TEXTURE_CHANNEL_R);
    }
  }

  if (mtl->bump) {
    surface->normal = wobj_scalarInput(wst,
                                       surface,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_NORMAL),
                                       1.0f,
                                       mtl->bump,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       wobj_mtl_map_channels(mtl->bump_channels,
                                                             AK_TEXTURE_CHANNEL_RGB));
  }
  
  if (mtl->Tr > 0.0f || mtl->d < 1.0f || mtl->map_d) {
    float t;

    if (mtl->d < 1.0f)
      t = mtl->d;
    else
      t = 1.0f - mtl->Tr;

    surface->opacity = wobj_scalarInput(wst,
                                        surface,
                                        ak_materialSemanticName(AK_MATERIAL_SEMANTIC_OPACITY),
                                        t,
                                        mtl->map_d,
                                        AK_TEXTURE_COLORSPACE_LINEAR,
                                        wobj_mtl_map_channels(mtl->map_d_channels,
                                                              AK_TEXTURE_CHANNEL_R));
    surface->flags |= AK_MATERIAL_FLAG_ALPHA_BLEND;
  }

  classic->base.type = AK_MATERIAL_FEATURE_CLASSIC;
  classic->ambient = wobj_colorInput(wst,
                                     classic,
                                     _s_ak_ambient,
                                     mtl->Ka,
                                     mtl->map_Ka,
                                     AK_TEXTURE_COLORSPACE_SRGB,
                                     wobj_mtl_map_channels(mtl->map_Ka_channels,
                                                           AK_TEXTURE_CHANNEL_RGB));
  classic->diffuse = surface->baseColor;
  classic->specular = wobj_colorInput(wst,
                                      classic,
                                      _s_ak_specular,
                                      mtl->Ks,
                                      mtl->map_Ks,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      wobj_mtl_map_channels(mtl->map_Ks_channels,
                                                            AK_TEXTURE_CHANNEL_RGB));
  classic->emission = surface->emissive;
  if (mtl->has_Tf) {
    classic->transparency = wobj_colorInput(wst,
                                            classic,
                                            _s_ak_transparency,
                                            mtl->Tf,
                                            NULL,
                                            AK_TEXTURE_COLORSPACE_SRGB,
                                            AK_TEXTURE_CHANNEL_RGB);
  }
  if (mtl->has_Ns && isfinite(mtl->Ns))
    classic->shininess = glm_max(mtl->Ns, 0.0f);
  else
    classic->shininess = 20.0f;
  classic->ior = mtl->Ni;
  classic->illum = mtl->illum;
  wobj_featurePush(surface, &classic->base);

  mat->name = mtl->name;
  if (mat->name)
    ak_mem_setp((void *)mat->name, mat);
  mat->surface = surface;
  
  AK_LIB_PREPEND(doc->lib.materials, mat, next);

  rb_insert(mtllib->materials, mtl->name, mat);
}

static
AkTextureRef*
wobj_texref(WOState            * __restrict wst,
            void               * __restrict memp,
            char               *            name,
            AkTextureColorSpace             colorSpace,
            AkTextureChannels               channels) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkImage       *image;
  AkImageSource *source;
  AkTexture     *tex;
  AkSampler     *sampler;
  AkTextureRef  *texref;
 
  heap = wst->heap;
  doc  = wst->doc;
  
  /* create image */
  image         = ak_heap_calloc(heap, doc, sizeof(*image));
  source        = ak_heap_calloc(heap, image, sizeof(*source));
  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = name;
  image->source = source;

  ak_mem_setp(name, source);
  AK_LIB_PREPEND(doc->lib.images, image, next);

  /* create sampler */
  sampler            = ak_heap_calloc(heap, doc, sizeof(*sampler));
  sampler->wrapS     = AK_WRAP_MODE_WRAP;
  sampler->wrapT     = AK_WRAP_MODE_WRAP;
  sampler->minfilter = AK_MINFILTER_UNSPECIFIED;
  sampler->magfilter = AK_MAGFILTER_UNSPECIFIED;
  sampler->mipfilter = AK_MIPFILTER_UNSPECIFIED;
  ak_setypeid(sampler, AKT_SAMPLER2D);
  AK_LIB_PREPEND(doc->lib.samplers, sampler, next);
  
  /* create texture */
  tex          = ak_heap_calloc(heap, doc, sizeof(*tex));
  tex->type    = AKT_SAMPLER2D;
  tex->image   = image;
  tex->sampler = sampler;

  AK_LIB_PREPEND(doc->lib.textures, tex, next);

  /* create texture ref */
  texref = ak_heap_calloc(heap, memp, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);
  
  texref->coordInputName = _s_TEXCOORD;
  texref->texture        = tex;
  texref->slot           = 0;
  ak_texref_usage(texref, colorSpace, channels);

  return texref;
}
