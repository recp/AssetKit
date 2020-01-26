/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "value.h"
#include "../fx/sampler.h"

#include "../1.4/surface.h"

#define AK_CUSTOM_TYPE_SURFACE 1

typedef struct {
  const char *key;
  AkTypeId    val;
  int         m; /* this is userData for _CUSTOM */
  int         n;
  int         size;
} ak_value_pair;

static
int
valuePairCmp(const void *, const void *);

static
int
valuePairCmp2(const void *, const void *);

static ak_value_pair valueMap[] = {
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

static size_t valueMapLen = 0;

void _assetkit_hide
dae_dtype(const char *typeName, AkTypeDesc *type) {
  ak_value_pair *found;

  if (valueMapLen == 0) {
    valueMapLen = AK_ARRAY_LEN(valueMap);
    qsort(valueMap,
          valueMapLen,
          sizeof(valueMap[0]),
          valuePairCmp);
  }

  found = bsearch(typeName,
                  valueMap,
                  valueMapLen,
                  sizeof(valueMap[0]),
                  valuePairCmp2);

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

AkResult _assetkit_hide
dae_value(AkXmlState * __restrict xst,
          void       * __restrict memParent,
          AkValue   ** __restrict dest) {
  AkValue       *val;
  ak_value_pair *found;
  AkXmlElmState  xest;

  if (valueMapLen == 0) {
    valueMapLen = AK_ARRAY_LEN(valueMap);
    qsort(valueMap,
          valueMapLen,
          sizeof(valueMap[0]),
          valuePairCmp);
  }

  found = bsearch(xst->nodeName,
                  valueMap,
                  valueMapLen,
                  sizeof(valueMap[0]),
                  valuePairCmp2);

  if (!found) {
    *dest = NULL;
    return AK_EFOUND;
  }

  val = ak_heap_calloc(xst->heap, memParent, sizeof(*val));
  val->type.size     = found->size;
  val->type.typeId   = found->val;
  val->type.typeName = found->key;

  ak_xest_init(xest, found->key)

  switch (found->val) {
    case AKT_STRING:
      val->value = ak_xml_val(xst, NULL);
      break;
    case AKT_BOOL:
    case AKT_BOOL2:
    case AKT_BOOL3:
    case AKT_BOOL4:{
      AkBool *boolVal;
      char   *nodeVal;

      nodeVal = ak_xml_rawcval(xst);
      boolVal = ak_heap_calloc(xst->heap,
                               memParent,
                               sizeof(*boolVal) * found->m * found->n);
      ak_strtomb(&nodeVal, boolVal, found->m, found->n);

      val->value = boolVal;
      break;
    }
    case AKT_INT:
    case AKT_INT2:
    case AKT_INT3:
    case AKT_INT4:{
      AkInt *intVal;
      char  *nodeVal;

      nodeVal = ak_xml_rawcval(xst);
      intVal = ak_heap_calloc(xst->heap,
                              memParent,
                              sizeof(*intVal) * found->m * found->n);
      ak_strtomi(&nodeVal, intVal, found->m, found->n);

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
      char    *nodeVal;

      nodeVal  = ak_xml_rawcval(xst);
      floatVal = ak_heap_calloc(xst->heap,
                                memParent,
                                sizeof(*floatVal) * found->m * found->n);
      ak_strtomf(&nodeVal, floatVal, found->m, found->n);

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
      AkResult   ret;

      sampler = NULL;
      ret     = dae_fxSampler(xst, memParent, found->key, &sampler);

      if (ret == AK_OK) {
        AkTexture *tex;

        tex = ak_heap_calloc(xst->heap, memParent, sizeof(*tex));
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
          AkDae14Surface *surface;
          AkResult        ret;

          surface = NULL;
          ret = dae14_fxSurface(xst, memParent, &surface);
          if (ret == AK_OK)
            val->value = surface;
          break;
        }
      }
      break;
    }
    default:
      break;
  }

  /* end element */
  ak_xml_end(&xest);

  *dest = val;

  return AK_OK;
}

static
int
valuePairCmp(const void * a, const void * b) {
  const ak_value_pair * _a = a;
  const ak_value_pair * _b = b;

  return strcmp(_a->key, _b->key);
}

static
int
valuePairCmp2(const void * a, const void * b) {
  const char * _a = a;
  const ak_value_pair * _b = b;
  
  return strcmp(_a, _b->key);
}
