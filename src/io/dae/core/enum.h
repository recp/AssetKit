/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef dae_enums_h
#define dae_enums_h

#include "../../../xml.h"

AkEnum _assetkit_hide
dae_semantic(const char * name);

AkEnum _assetkit_hide
dae_morphMethod(const xml_attr_t * __restrict xatt);

AkEnum _assetkit_hide
dae_nodeType(const xml_attr_t * __restrict xatt);

AkEnum _assetkit_hide
dae_animBehavior(const xml_attr_t * __restrict xatt);

AkEnum _assetkit_hide
dae_animInterp(const char * name, size_t len);

AkEnum _assetkit_hide
dae_wrap(const xml_t * __restrict xml);

AkEnum _assetkit_hide
dae_minfilter(const xml_t * __restrict xml);

AkEnum _assetkit_hide
dae_mipfilter(const xml_t * __restrict xml);

AkEnum _assetkit_hide
dae_magfilter(const xml_t * __restrict xml);

AkEnum _assetkit_hide
dae_face(const xml_attr_t * __restrict xatt);

AkEnum _assetkit_hide
dae_opaque(const xml_attr_t * __restrict xatt);

AkEnum _assetkit_hide
dae_enumChannel(const char *name, size_t len);

AkEnum _assetkit_hide
dae_range(const char *name, size_t len);

AkEnum _assetkit_hide
dae_precision(const char *name, size_t len);

#endif /* dae_enums_h */
