/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "common.h"
#include "../strpool.h"
#include "../../common/text_number.h"

static
void
gltf_imp_prefixed_id(char       * __restrict dst,
                     const char * __restrict prefix,
                     size_t                  prefixLen,
                     int64_t                 index) {
  char    *p;
  uint64_t absIndex;

  memcpy(dst, prefix, prefixLen);
  p = dst + prefixLen;
  if (index < 0) {
    *p++ = '-';
    absIndex = (uint64_t)(-(index + 1)) + 1u;
  } else {
    absIndex = (uint64_t)index;
  }

  p  = ak_io_text_format_uint64(p, absIndex);
  *p = '\0';
}

AK_HIDE
void
gltf_imp_skin_id(char * __restrict dst, int64_t index) {
  gltf_imp_prefixed_id(dst, _s_gltf_skin, _s_gltf_skin_len, index);
}

AK_HIDE
void
gltf_imp_texcoord_id(char * __restrict dst, int64_t index) {
  gltf_imp_prefixed_id(dst,
                       _s_gltf_texcoordPrefix,
                       _s_gltf_texcoordPrefix_len,
                       index);
}
