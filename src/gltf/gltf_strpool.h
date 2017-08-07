/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef gltf_strpool_h
#  define gltf_strpool_h

#ifndef _AK_GLTF_STRPOOL_
#  define _AK_EXTERN extern
#else
#  define _AK_EXTERN
#endif

_AK_EXTERN const char _s_gltf_pool_0[];

#define _s_gltf_0(x) (_s_gltf_pool_0 + x)

/* _s_gltf_pool_0 */
#define _s_gltf_space _s_gltf_0(0)
#define _s_gltf_gltf _s_gltf_0(2)
#define _s_gltf_asset _s_gltf_0(7)
#define _s_gltf_version _s_gltf_0(13)
#define _s_gltf_generator _s_gltf_0(21)
#define _s_gltf_copyright _s_gltf_0(31)
#define _s_gltf_meter _s_gltf_0(41)

#endif /* ak_gltf_strpool_h */
