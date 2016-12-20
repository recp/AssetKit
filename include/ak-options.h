/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_options_h
#define ak_options_h
#ifdef __cplusplus
extern "C" {
#endif

typedef enum AkOption {
  AK_OPT_INDICES_NONE               = 0,  /* false */
  AK_OPT_INDICES_SINGLE_INTERLEAVED = 1,  /* true  */
  AK_OPT_INDICES_SINGLE_SEPARATE    = 2,  /* true  */
  AK_OPT_INDICES_SINGLE             = 3,  /* true  */
  AK_OPT_COORD                      = 4,  /* Y_UP  */
  AK_OPT_DEFAULT_ID_PREFIX          = 5   /* id prefix */
} AkOption;

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value);

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option);

#ifdef __cplusplus
}
#endif
#endif /* ak_options_h */
