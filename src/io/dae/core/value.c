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

#include "value.h"
#include "../fx/samp.h"
#include "../1.4/surface.h"

#define AK_CUSTOM_TYPE_SURFACE 1

typedef struct valpair {
  const char *key;
  AkTypeId    val;
  int         m; /* this is userData for _CUSTOM */
  int         n;
  int         size;
} valpair;

static int valpair_cmp1(const void *, const void *);
static int valpair_cmp2(const void *, const void *);
static int valpair_cmpxt(const void *, const void *);

static valpair valmap[] = {
  {_s_dae_string,   AKT_STRING,   1, 1, sizeof(char *)},
  {_s_dae_bool,     AKT_BOOL,     1, 1, sizeof(bool)},
  {_s_dae_bool2,    AKT_BOOL2,    1, 2, sizeof(bool[2])},
  {_s_dae_bool3,    AKT_BOOL3,    1, 3, sizeof(bool[3])},
  {_s_dae_bool4,    AKT_BOOL4,    1, 4, sizeof(bool[4])},
  {_s_dae_int,      AKT_INT,      1, 1, sizeof(int)},
  {_s_dae_int2,     AKT_INT2,     1, 2, sizeof(int[2])},
  {_s_dae_int3,     AKT_INT3,     1, 3, sizeof(int[3])},
  {_s_dae_int4,     AKT_INT4,     1, 4, sizeof(int[4])},
  {_s_dae_float,    AKT_FLOAT,    1, 1, sizeof(float)},
  {_s_dae_float2,   AKT_FLOAT2,   1, 2, sizeof(float[2])},
  {_s_dae_float3,   AKT_FLOAT3,   1, 3, sizeof(float[3])},
  {_s_dae_float4,   AKT_FLOAT4,   1, 4, sizeof(float[4])},
  {_s_dae_float2x2, AKT_FLOAT2x2, 2, 2, sizeof(float[2][2])},
  {_s_dae_float3x3, AKT_FLOAT3x3, 3, 3, sizeof(float[3][3])},
  {_s_dae_float4x4, AKT_FLOAT4x4, 4, 4, sizeof(float[4][4])},

  /* samplers */
  {_s_dae_sampler1d,     AKT_SAMPLER1D,     1, 1, sizeof(AkSampler)},
  {_s_dae_sampler2d,     AKT_SAMPLER2D,     1, 1, sizeof(AkSampler)},
  {_s_dae_sampler3d,     AKT_SAMPLER3D,     1, 1, sizeof(AkSampler)},
  {_s_dae_sampler_cube,  AKT_SAMPLER_CUBE,  1, 1, sizeof(AkSampler)},
  {_s_dae_sampler_rect,  AKT_SAMPLER_RECT,  1, 1, sizeof(AkSampler)},
  {_s_dae_sampler_depth, AKT_SAMPLER_DEPTH, 1, 1, sizeof(AkSampler)},

  /* COLLADA 1.4 */
  {
    _s_dae_surface,
    AKT_CUSTOM,
    AK_CUSTOM_TYPE_SURFACE,
    1,
    sizeof(AkDae14Surface)
  },
};

static size_t valmapLen = 0;

AK_HIDE
void
dae_dtype(const char *typeName, AkTypeDesc *type) {
  valpair *found;

  if (valmapLen == 0) {
    valmapLen = AK_ARRAY_LEN(valmap);
    qsort(valmap,
          valmapLen,
          sizeof(valmap[0]),
          valpair_cmp1);
  }

  found = bsearch(typeName,
                  valmap,
                  valmapLen,
                  sizeof(valmap[0]),
                  valpair_cmp2);

  if (!found) {
    type->typeId   = AKT_UNKNOWN;
    type->typeName = NULL;
    type->size     = 0;
    return;
  }

  type->size     = found->size;
  type->typeId   = found->val;
  type->typeName = found->key;
}

AK_HIDE
AkValue*
dae_value(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap        *heap;
  AkValue       *val;
  valpair       *found;
  const xml_t   *sval;

  heap = dst->heap;
  sval = xmls(xml);
  
  if (valmapLen == 0) {
    valmapLen = AK_ARRAY_LEN(valmap);
    qsort(valmap, valmapLen, sizeof(valmap[0]), valpair_cmp1);
  }
  
  found = bsearch(xml, valmap, valmapLen, sizeof(valmap[0]), valpair_cmpxt);

  if (!found)
    return NULL;

  val                = ak_heap_calloc(heap, memp, sizeof(*val));
  val->type.size     = found->size;
  val->type.typeId   = found->val;
  val->type.typeName = found->key;

  switch (found->val) {
    case AKT_STRING:
      val->value = xml_strdup(xml, heap, val);
      break;
    case AKT_BOOL:
    case AKT_BOOL2:
    case AKT_BOOL3:
    case AKT_BOOL4:{
      AkBool *boolVal;

      boolVal = ak_heap_calloc(heap,
                               val,
                               sizeof(*boolVal) * found->m * found->n);
      xml_strtob_fast(sval, boolVal, found->m * found->n);

      val->value = boolVal;
      break;
    }
    case AKT_INT:
    case AKT_INT2:
    case AKT_INT3:
    case AKT_INT4:{
      AkInt *intVal;
  
      intVal = ak_heap_calloc(heap,
                              memp,
                              sizeof(*intVal) * found->m * found->n);
      xml_strtoi_fast(sval, intVal, found->m * found->n);

      val->value = intVal;
      break;
    }
    case AKT_FLOAT:
    case AKT_FLOAT2:
    case AKT_FLOAT3:
    case AKT_FLOAT4:
    case AKT_FLOAT2x2:
    case AKT_FLOAT3x3:
    case AKT_FLOAT4x4:{
      AkFloat *floatVal;

      floatVal = ak_heap_calloc(heap,
                                memp,
                                sizeof(*floatVal) * found->m * found->n);
      xml_strtof_fast(sval, floatVal, found->m * found->n);

      val->value = floatVal;
      break;
    }
    case AKT_SAMPLER1D:
    case AKT_SAMPLER2D:
    case AKT_SAMPLER3D:
    case AKT_SAMPLER_CUBE:
    case AKT_SAMPLER_RECT:
    case AKT_SAMPLER_DEPTH: {
      AkSampler *sampler;

      if ((sampler = dae_sampler(dst, xml, val))) {
        AkTexture *tex;
        
        tex = ak_heap_calloc(heap, val, sizeof(*tex));
        ak_setypeid(tex, AKT_TEXTURE);

        tex->sampler = sampler;
        tex->type    = found->val;
        val->value   = tex;
      }

      break;
    }
    case AKT_CUSTOM: {
      switch (found->m) {
        case AK_CUSTOM_TYPE_SURFACE: {
          val->value = dae14_surface(dst, xml, val);
          break;
        }
      }
      break;
    }
    default:
      break;
  }

  return val;
}

static
int
valpair_cmp1(const void * a, const void * b) {
  return strcmp(((const valpair *)a)->key, ((const valpair *)b)->key);
}

static
int
valpair_cmp2(const void * a, const void * b) {  
  return strcmp(a, ((const valpair *)b)->key);
}

static int
valpair_cmpxt(const void *a, const void *b) {
  return xml_tag_cmp(a, ((const valpair *)b)->key);
}
