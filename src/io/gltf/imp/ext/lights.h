/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef gltf_imp_ext_lights_h
#define gltf_imp_ext_lights_h

#include "../common.h"

AK_HIDE
void
gltf_ext_lights(AkGLTFState * __restrict gst,
                json_t      * __restrict jlights);

AK_HIDE
bool
gltf_ext_nodeLight(AkGLTFState * __restrict gst,
                   AkNode      * __restrict node,
                   const json_t * __restrict jext);

#endif /* gltf_imp_ext_lights_h */
