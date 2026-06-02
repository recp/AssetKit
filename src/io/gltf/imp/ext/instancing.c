/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "instancing.h"

static
bool
gltf_ext_instanceAccOK(AkAccessor * __restrict acc,
                       uint32_t                componentCount) {
  return acc
         && acc->componentType == AKT_FLOAT
         && acc->componentCount == componentCount
         && acc->count > 0;
}

/* EXT_mesh_gpu_instancing: pull the per-instance TRS accessor refs out of
   the extension's `attributes` object. Each attribute is optional; present
   attributes must all have the same accessor count per the spec. */
AK_HIDE
AkGpuInstancing*
gltf_ext_meshGPUInstancing(AkGLTFState * __restrict gst,
                           AkNode      * __restrict node,
                           const json_t * __restrict jinstancing) {
  AkGpuInstancing *instancing;
  json_t          *jattrs;
  json_t          *jattr;
  AkAccessor      *acc;
  int32_t          accIdx;
  uint32_t         count;
  uint32_t         compCount;
  uint32_t         attrKind;

  if (!(jattrs = GLTF_JSON_GET(jinstancing, attributes)))
    return NULL;

  instancing = ak_heap_calloc(gst->heap, node, sizeof(*instancing));
  count      = 0;

  for (jattr = jattrs->value; jattr; jattr = jattr->next) {
    accIdx    = json_int32(jattr, -1);
    attrKind  = 0;
    compCount = 0;

    if (ak_str_eq_fast(jattr->key,
                       (size_t)jattr->keysize,
                       _s_gltf_TRANSLATION,
                       _s_gltf_TRANSLATION_len)) {
      attrKind  = 1;
      compCount = 3;
    } else if (ak_str_eq_packed_fast(jattr->key,
                                     (size_t)jattr->keysize,
                                     _s_gltf_ROTATION_u64_exact,
                                     _s_gltf_ROTATION_len)) {
      attrKind  = 2;
      compCount = 4;
    } else if (ak_str_eq_packed_fast(jattr->key,
                                     (size_t)jattr->keysize,
                                     _s_gltf_SCALE_u64_exact,
                                     _s_gltf_SCALE_len)) {
      attrKind  = 3;
      compCount = 3;
    }

    if (!attrKind)
      continue;

    if (accIdx < 0) {
      gst->stop = true;
      return NULL;
    }
    if (!(acc = gltf_accessor_at(gst, accIdx)))
      goto malformed;
    if (!gltf_ext_instanceAccOK(acc, compCount))
      goto malformed;

    if (count == 0) {
      count = (uint32_t)acc->count;
    } else if (count != (uint32_t)acc->count) {
      goto malformed;
    }

    ak_retain(acc);

    switch (attrKind) {
      case 1: instancing->translation = acc; break;
      case 2: instancing->rotation    = acc; break;
      case 3: instancing->scale       = acc; break;
      default: break;
    }
  }

  if (count == 0)
    return NULL;

  instancing->count = count;
  return instancing;

malformed:
  gst->stop = true;
  return NULL;
}
