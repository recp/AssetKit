/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_buffer_h
#define gltf_buffer_h

#include "../common.h"

void
gltf_buffers(json_t * __restrict json,
             void   * __restrict userdata);

void
gltf_bufferViews(json_t * __restrict json,
                 void   * __restrict userdata);

#endif /* gltf_buffer_h */
