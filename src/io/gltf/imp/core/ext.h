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

#ifndef gltf_imp_core_ext_h
#define gltf_imp_core_ext_h

#include "../common.h"

AK_HIDE
void
gltf_exts(json_t * __restrict jext,
          void   * __restrict userdata);

AK_HIDE
void
gltf_ext_root(json_t * __restrict jext,
              void   * __restrict userdata);

AK_HIDE
bool
gltf_ext_node(AkGLTFState * __restrict gst,
              AkNode      * __restrict node,
              const json_t * __restrict jext);

AK_HIDE
void
gltf_ext_close(AkGLTFState * __restrict gst);

AK_HIDE
bool
gltf_ext_bufferView(AkGLTFState  * __restrict gst,
                    AkBufferView * __restrict buffView,
                    const json_t * __restrict jext);

AK_HIDE
bool
gltf_ext_dracoPrimitive(AkGLTFState     * __restrict gst,
                        AkMeshPrimitive * __restrict prim,
                        const json_t    * __restrict jprim);

#endif /* gltf_imp_core_ext_h */
