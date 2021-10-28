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
#include "../../strpool.h"

/*
 Resources:
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
   http://paulbourke.net/dataformats/obj/
   http://paulbourke.net/dataformats/mtl/
   https://en.wikipedia.org/wiki/Wavefront_.obj_file#Material_template_library
*/

static
AkProfileCommon*
wobj_cmnEffect(WOState * __restrict wst);

static
void
wobj_handleMaterial(WOState  * __restrict wst,
                    WOMtlLib * __restrict mtllib,
                    WOMtl    * __restrict mtl);

static
AkTextureRef*
wobj_texref(WOState * __restrict wst, void * __restrict memp, char* name);

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
    
    if (p[0] == 'n'
        && p[1] == 'e'
        && p[2] == 'w'
        && p[3] == 'm'
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
      } else if (p[0] == 'm'
                 && p[1] == 'a'
                 && p[2] == 'p'
                 && p[3] == '_') {
        p += 4;
        switch (p[0]) {
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
      } else if (p[0] == 'b'
                 && p[1] == 'u'
                 && p[2] == 'm'
                 && p[3] == 'p') {
        p += 4;

        SKIP_SPACES

        begin = p;
        while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
        end = p;

        mtl->bump = ak_heap_strndup(heap, mtl, begin, end - begin);
      } else if (p[0] == 'i'
                 && p[1] == 'l'
                 && p[2] == 'l'
                 && p[3] == 'u'
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
AkProfileCommon*
wobj_cmnEffect(WOState * __restrict wst) {
  AkLibrary       *lib;
  AkEffect        *effect;
  AkProfileCommon *profile;

  if (!(lib = wst->doc->lib.effects)) {
    lib = ak_heap_calloc(wst->heap, wst->doc, sizeof(*lib));
    wst->doc->lib.effects = lib;
  }

  effect        = ak_heap_calloc(wst->heap, lib,    sizeof(*effect));
  profile       = ak_heap_calloc(wst->heap, effect, sizeof(*profile));
  profile->type = AK_PROFILE_TYPE_COMMON;

  lib->count++;

  effect->profile = profile;
  effect->next    = (void *)lib->chld;
  lib->chld       = (void *)effect;

  ak_setypeid(profile, AKT_PROFILE);
  ak_setypeid(effect,  AKT_EFFECT);

  return effect->profile;
}

AK_INLINE
AkColorDesc*
wobj_clrtex(WOState    * __restrict wst,
            void       * __restrict memp,
            float      *            rgb,
            char       * __restrict map) {
  AkColorDesc *clr;

  clr = ak_heap_calloc(wst->heap, memp, sizeof(*clr));
  
  if (rgb) {
    clr->color = ak_heap_calloc(wst->heap, clr,  sizeof(*clr->color));
    glm_vec3_copy(rgb, clr->color->vec);
    clr->color->vec[3] = 1.0f;
  }

  if (map) {
    clr->texture = wobj_texref(wst, memp, map);
  }

  return clr;
}

AK_INLINE
AkFloatOrParam*
wobj_flt(AkHeap * __restrict heap,
         void   * __restrict memp,
         float               val) {
  AkFloatOrParam *flt;

  flt       = ak_heap_calloc(heap, memp, sizeof(*flt));
  flt->val  = ak_heap_calloc(heap, flt, sizeof(*flt->val));
  *flt->val = val;

  return flt;
}

static
void
wobj_handleMaterial(WOState  * __restrict wst,
                    WOMtlLib * __restrict mtllib,
                    WOMtl    * __restrict mtl) {
  AkHeap               *heap;
  AkDoc                *doc;
  AkLibrary            *libmat;
  AkProfileCommon      *pcommon;
  AkTechniqueFx        *technfx;
  AkTechniqueFxCommon  *cmnTechn;
  AkEffect             *effect;
  AkInstanceEffect     *ieff;
  AkMaterial           *mat;
 
  heap = wst->heap;
  doc  = wst->doc;
  
  if (!(libmat = doc->lib.materials)) {
    libmat             = ak_heap_calloc(heap, wst->doc, sizeof(*libmat));
    doc->lib.materials = libmat;
  }

  pcommon = wobj_cmnEffect(wst);
  effect  = ak_mem_parent(pcommon);
  technfx = ak_heap_calloc(heap, pcommon, sizeof(*technfx));
  mat     = ak_heap_calloc(heap, libmat,  sizeof(*mat));

  ak_setypeid(technfx, AKT_TECHNIQUE_FX);

  cmnTechn = ak_heap_calloc(heap, technfx, sizeof(*cmnTechn));
  switch (mtl->illum) {
    case 0: /* Constant */
      cmnTechn->type = AK_MATERIAL_CONSTANT;
      break;
    case 1: /* Lambert */
      cmnTechn->type = AK_MATERIAL_LAMBERT;
      break;
    case 2: /* TODO: Currently all others are Blinn */
      cmnTechn->type = AK_MATERIAL_BLINN;
    default:
      break;
  }
  
  cmnTechn->ambient  = wobj_clrtex(wst, cmnTechn, mtl->Ka, mtl->map_Ka);
  cmnTechn->diffuse  = wobj_clrtex(wst, cmnTechn, mtl->Kd, mtl->map_Kd);
  cmnTechn->specular = wobj_clrtex(wst, cmnTechn, mtl->Ks, mtl->map_Ks);
  cmnTechn->emission = wobj_clrtex(wst, cmnTechn, mtl->Ke, mtl->map_Ke);

  cmnTechn->shininess         = wobj_flt(heap, cmnTechn, mtl->Ns);
  cmnTechn->indexOfRefraction = wobj_flt(heap, cmnTechn, mtl->Ni);

  if (mtl->bump) {
    cmnTechn->normal        = ak_heap_calloc(heap, cmnTechn, sizeof(*cmnTechn->normal));
    cmnTechn->normal->scale = 1.0f;
    cmnTechn->normal->tex   = wobj_texref(wst, cmnTechn, mtl->bump);
  }
  
  if (mtl->Tr > 0.0f || mtl->d < 1.0f) {
    AkTransparent *transp;
    float          t;

    if (mtl->d < 1.0f)
      t = mtl->d;
    else
      t = 1.0f - mtl->Tr;

    transp         = ak_heap_calloc(heap, cmnTechn, sizeof(*transp));
    transp->amount = wobj_flt(heap, transp, t);
    transp->opaque = AK_OPAQUE_BLEND;

    cmnTechn->transparent = transp;
  }

  technfx->common    = cmnTechn;
  technfx->next      = pcommon->technique;
  pcommon->technique = technfx;
  
  ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
  ieff->base.type    = AK_INSTANCE_EFFECT;
  ieff->base.url.ptr = effect;
  mat->effect        = ieff;
  
  mat->base.next     = libmat->chld;
  libmat->chld       = (void *)mat;
  libmat->count++;

  rb_insert(mtllib->materials, mtl->name, mat);
}

static
AkTextureRef*
wobj_texref(WOState * __restrict wst, void * __restrict memp, char* name) {
  AkHeap       *heap;
  AkDoc        *doc;
  AkImage      *image;
  AkInitFrom   *initFrom;
  AkTexture    *tex;
  AkSampler    *sampler;
  AkTextureRef *texref;
 
  heap = wst->heap;
  doc  = wst->doc;
  
  /* create image */
  image           = ak_heap_calloc(heap, doc, sizeof(*image));
  initFrom        = ak_heap_calloc(heap, image, sizeof(*initFrom));
  initFrom->ref   = name;
  image->initFrom = initFrom;

  ak_mem_setp(name, initFrom);
  flist_sp_insert(&doc->lib.images, image);

  /* create sampler */
  sampler        = ak_heap_calloc(heap, doc, sizeof(*sampler));
  sampler->wrapS = AK_WRAP_MODE_WRAP;
  sampler->wrapT = AK_WRAP_MODE_WRAP;
  ak_setypeid(sampler, AKT_SAMPLER2D);
  
  /* create texture */
  tex          = ak_heap_calloc(heap, doc, sizeof(*tex));
  tex->type    = AKT_SAMPLER2D;
  tex->image   = image;
  tex->sampler = sampler;

  flist_sp_insert(&doc->lib.textures, tex);

  /* create texture ref */
  texref = ak_heap_calloc(heap, memp, sizeof(*texref));
  ak_setypeid(texref, AKT_TEXTURE_REF);
  
  texref->coordInputName = _s_TEXCOORD;
  texref->texture        = tex;
  texref->slot           = 0;

  return texref;
}
