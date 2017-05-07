/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_context_h
#define ak_context_h
#ifdef __cplusplus
extern "C" {
#endif

#include "ak-common.h"
#include "ak-map.h"
#include "ak-util.h"

typedef struct AkContext {
  AkDoc              *doc;
  AkTechniqueHint    *techniqueHint;
  AkInstanceMaterial *instanceMaterial;
  AkMap              *bindVertexInputIndex;
} AkContext;

#ifdef __cplusplus
}
#endif
#endif /* ak_context_h */
