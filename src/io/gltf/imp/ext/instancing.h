/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef gltf_imp_ext_instancing_h
#define gltf_imp_ext_instancing_h

#include "../common.h"

AK_HIDE
AkGpuInstancing*
gltf_ext_meshGPUInstancing(AkGLTFState * __restrict gst,
                           AkNode      * __restrict node,
                           const json_t * __restrict jinstancing);

#endif /* gltf_imp_ext_instancing_h */
