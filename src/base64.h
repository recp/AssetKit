/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 *
 * Modified by Recep Aslantas (github @recp)
 */

#ifndef BASE64_H
#define BASE64_H

#include <stdlib.h>
#include <memory.h>
#include "../include/ak/memory.h"
#include "../include/ak/source.h"

AK_HIDE
unsigned char*
base64_encode(AkHeap              * __restrict heap,
              void                * __restrict memparent,
              const unsigned char * __restrict src,
              size_t                           len,
              size_t              * __restrict out_len);

AK_HIDE
unsigned char*
base64_decode(AkHeap              * __restrict heap,
              void                * __restrict memparent,
              const unsigned char * __restrict src,
              size_t                          len,
              size_t              * __restrict out_len);

AK_HIDE
void
base64_buff(const char * __restrict b64,
            size_t                  len,
            AkBuffer   * __restrict buff);

#endif /* BASE64_H */
