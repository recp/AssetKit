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
AkInstanceAttribs*
gltf_ext_meshGPUInstancing(AkGLTFState * __restrict gst,
                           AkNode      * __restrict node,
                           const json_t * __restrict jinstancing) {
  AkInstanceAttribs *attribs;
  json_t            *jattrs;
  json_t            *jattr;
  AkAccessor        *acc;
  int32_t            accIdx;
  uint32_t           count;
  uint32_t           compCount;
  bool               known;

  if (!(jattrs = json_get(jinstancing, _s_gltf_attributes)))
    return NULL;

  attribs = ak_heap_calloc(gst->heap, node, sizeof(*attribs));
  count   = 0;

  for (jattr = jattrs->value; jattr; jattr = jattr->next) {
    accIdx    = json_int32(jattr, -1);
    known     = false;
    compCount = 0;

    if (jattr->keysize == 11
        && memcmp(jattr->key, "TRANSLATION", 11) == 0) {
      known     = true;
      compCount = 3;
    } else if (jattr->keysize == 8
               && memcmp(jattr->key, "ROTATION", 8) == 0) {
      known     = true;
      compCount = 4;
    } else if (jattr->keysize == 5
               && memcmp(jattr->key, "SCALE", 5) == 0) {
      known     = true;
      compCount = 3;
    }

    if (!known)
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

    if (jattr->keysize == 11
        && memcmp(jattr->key, "TRANSLATION", 11) == 0) {
      attribs->translation = acc;
    } else if (jattr->keysize == 8
               && memcmp(jattr->key, "ROTATION", 8) == 0) {
      attribs->rotation = acc;
    } else if (jattr->keysize == 5
               && memcmp(jattr->key, "SCALE", 5) == 0) {
      attribs->scale = acc;
    }
  }

  if (count == 0)
    return NULL;

  attribs->count = count;
  return attribs;

malformed:
  gst->stop = true;
  return NULL;
}
