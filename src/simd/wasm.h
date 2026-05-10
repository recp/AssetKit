/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0.
 */

#ifndef assetkit_simd_wasm_h
#define assetkit_simd_wasm_h

#if defined(__wasm__) && defined(__wasm_simd128__)
#  include <wasm_simd128.h>
#  define AK_SIMD_WASM 1
#endif

#endif /* assetkit_simd_wasm_h */
