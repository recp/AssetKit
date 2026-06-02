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
AkMaterialInput*
wobj_colorInput(WOState             * __restrict wst,
                void                * __restrict parent,
                const char          * __restrict semantic,
                float               *            rgb,
                char                * __restrict map,
                AkTextureColorSpace              colorSpace,
                AkTextureChannels                channels) {
  AkMaterialInput *input;

  input             = ak_heap_calloc(wst->heap, parent, sizeof(*input));
  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_COLOR;
  input->colorSpace = colorSpace;
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

  input             = ak_heap_calloc(wst->heap, parent, sizeof(*input));
  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_FLOAT;
  input->value[0]   = value;
  input->colorSpace = colorSpace;
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
  localurl = ak_getFileFrom(wst->doc, name);

  if (ak_readfile(localurl, NULL, &mtlstr, &mtlstrSize) != AK_OK
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

              mtl->map_d = ak_heap_strndup(heap, mtl, begin, end - begin);
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
                
                mtl->map_Ka = ak_heap_strndup(heap, mtl, begin, end - begin);
                break;
              case 'd':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Kd = ak_heap_strndup(heap, mtl, begin, end - begin);
                
                break;
              case 's':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Ks = ak_heap_strndup(heap, mtl, begin, end - begin);
                
                break;
              case 'e':
                p += 2;
                SKIP_SPACES

                begin = p;
                while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
                end = p;
                
                mtl->map_Ke = ak_heap_strndup(heap, mtl, begin, end - begin);
                
                break;
              default: break;
            }
            break;
          default: break;
        }
      } else if (ak_str_pack4_fast(p, 4) == WOBJ_MTL_KW_BUMP) {
        p += 4;

        SKIP_SPACES

        begin = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
        end = p;

        mtl->bump = ak_heap_strndup(heap, mtl, begin, end - begin);
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
  surface = ak_heap_calloc(heap, mat,    sizeof(*surface));
  classic = ak_heap_calloc(heap, surface, sizeof(*classic));

  switch (mtl->illum) {
    case 0: /* Constant */
      surface->type = AK_MATERIAL_TYPE_CONSTANT;
      break;
    case 1: /* Lambert */
      surface->type = AK_MATERIAL_TYPE_LAMBERT;
      break;
    case 2: /* TODO: Currently all others are Blinn */
//    case 3:
//    case 4:
      surface->type = AK_MATERIAL_TYPE_BLINN;
    default:
      break;
  }

  if (surface->type == AK_MATERIAL_TYPE_NONE)
    surface->type = AK_MATERIAL_TYPE_BLINN;

  surface->baseColor = wobj_colorInput(wst,
                                       surface,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_BASE_COLOR),
                                       mtl->Kd,
                                       mtl->map_Kd,
                                       AK_TEXTURE_COLORSPACE_SRGB,
                                       AK_TEXTURE_CHANNEL_RGBA);
  surface->emissive = wobj_colorInput(wst,
                                      surface,
                                      ak_materialSemanticName(AK_MATERIAL_SEMANTIC_EMISSIVE),
                                      mtl->Ke,
                                      mtl->map_Ke,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGB);
  surface->alphaCutoff = 0.5f;
  surface->ior = mtl->Ni;
  surface->emissiveStrength = 1.0f;

  if (mtl->bump) {
    surface->normal = wobj_scalarInput(wst,
                                       surface,
                                       ak_materialSemanticName(AK_MATERIAL_SEMANTIC_NORMAL),
                                       1.0f,
                                       mtl->bump,
                                       AK_TEXTURE_COLORSPACE_LINEAR,
                                       AK_TEXTURE_CHANNEL_RGB);
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
                                        AK_TEXTURE_CHANNEL_R);
    surface->flags |= AK_MATERIAL_FLAG_ALPHA_BLEND;
  }

  classic->base.type = AK_MATERIAL_FEATURE_CLASSIC;
  classic->ambient = wobj_colorInput(wst,
                                     classic,
                                     _s_ak_ambient,
                                     mtl->Ka,
                                     mtl->map_Ka,
                                     AK_TEXTURE_COLORSPACE_SRGB,
                                     AK_TEXTURE_CHANNEL_RGB);
  classic->diffuse = surface->baseColor;
  classic->specular = wobj_colorInput(wst,
                                      classic,
                                      _s_ak_specular,
                                      mtl->Ks,
                                      mtl->map_Ks,
                                      AK_TEXTURE_COLORSPACE_SRGB,
                                      AK_TEXTURE_CHANNEL_RGB);
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
  classic->shininess = mtl->Ns;
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
  sampler        = ak_heap_calloc(heap, doc, sizeof(*sampler));
  sampler->wrapS = AK_WRAP_MODE_WRAP;
  sampler->wrapT = AK_WRAP_MODE_WRAP;
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
