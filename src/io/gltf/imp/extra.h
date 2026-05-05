/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef gltf_imp_extra_h
#define gltf_imp_extra_h

#include "common.h"

AK_HIDE
void
gltf_extra(AkGLTFState * __restrict gst,
           void        * __restrict owner,
           const json_t * __restrict jextras,
           const json_t * __restrict jextensions);

#endif /* gltf_imp_extra_h */
