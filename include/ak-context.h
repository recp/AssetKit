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

typedef struct AkContext {
  AkDoc           *doc;
  AkTechniqueHint *techniqueHint;
} AkContext;

#ifdef __cplusplus
}
#endif
#endif /* ak_context_h */
