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

/*
 Resources:
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
   http://paulbourke.net/dataformats/obj/
   http://paulbourke.net/dataformats/mtl/
*/

static
AkProfileCommon* _assetkit_hide
wobj_cmnEffect(WOState * __restrict wst);

static
void
wobj_handleMaterial(WOState  * __restrict wst,
                    WOMtlLib * __restrict mtllib,
                    WOMtl    * __restrict mtl);

WOMtlLib* _assetkit_hide
wobj_mtl(WOState    * __restrict wst,
         const char * __restrict name) {
  AkHeap   *heap;
  void     *mtlstr;
  char     *p, *localurl, *begin, *end;
  WOMtlLib *mtllib;
  WOMtl    *mtl;
  size_t    mtlstrSize;
  AkResult  ret;
  char      c;

  c        = '\0';
  localurl = ak_getFileFrom(wst->doc, name);

  if ((ret = ak_readfile(localurl, "rb", &mtlstr, &mtlstrSize)) != AK_OK
      || !((p = mtlstr) && (c = *p) != '\0'))
    goto ret;

  heap              = wst->heap;
  mtl               = NULL;
  mtllib            = ak_heap_calloc(heap, wst->tmpParent, sizeof(*mtllib));
  mtllib->materials = rb_newtree_str();
  
  /* parse .mtl */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (mtl && (p[2] == ' ' || p[2] == '\t')) {
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
        default:
          p += 2;
          break;
      }
    } else if (p[0] == 'n'
               && p[1] == 'e'
               && p[2] == 'w'
               && p[3] == 'm'
               && p[4] == 't'
               && p[5] == 'l'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 6;
      SKIP_SPACES

      begin = ++p;
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
      end = p;

      if (end > begin) {
        if (mtl)
          wobj_handleMaterial(wst, mtllib, mtl);
        
        mtl       = ak_heap_calloc(heap, wst->tmpParent, sizeof(*mtl));
        mtl->name = ak_heap_strndup(heap, mtl, begin, end - begin);
      }
    }

    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  if (mtl)
    wobj_handleMaterial(wst, mtllib, mtl);

ret:
  ak_free((void *)localurl);
  return NULL;
}

static
AkProfileCommon* _assetkit_hide
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
wobj_color_rgb(AkHeap * __restrict heap,
               void   * __restrict memp,
               vec3                rgb) {
  AkColorDesc *colorDesc;

  colorDesc        = ak_heap_calloc(heap, memp, sizeof(*colorDesc));
  colorDesc->color = ak_heap_calloc(heap, colorDesc, sizeof(*colorDesc->color));
  glm_vec3_copy(rgb, colorDesc->color->vec);
  colorDesc->color->vec[3] = 1.0f;

  return colorDesc;
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
  
  cmnTechn->ambient           = wobj_color_rgb(heap, cmnTechn, mtl->Ka);
  cmnTechn->diffuse           = wobj_color_rgb(heap, cmnTechn, mtl->Kd);
  cmnTechn->specular          = wobj_color_rgb(heap, cmnTechn, mtl->Ks);
  cmnTechn->emission          = wobj_color_rgb(heap, cmnTechn, mtl->Ke);

  cmnTechn->shininess         = wobj_flt(heap, cmnTechn, mtl->Ns);
  cmnTechn->indexOfRefraction = wobj_flt(heap, cmnTechn, mtl->Ni);
  
  technfx->next      = pcommon->technique;
  pcommon->technique = technfx;
  
  technfx->common    = cmnTechn;
  ieff               = ak_heap_calloc(heap, mat, sizeof(*ieff));
  ieff->base.type    = AK_INSTANCE_EFFECT;
  ieff->base.url.ptr = effect;
  mat->effect        = ieff;
  
  mat->base.next     = libmat->chld;
  libmat->chld       = (void *)mat;
  libmat->count++;

  rb_insert(mtllib->materials, mtl->name, mat);
}
