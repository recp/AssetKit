/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_options_h
#define ak_options_h

typedef enum AkOption {
  AK_OPT_INDICES_NONE               = 0,  /* false */
  AK_OPT_INDICES_SINGLE_INTERLEAVED = 1,  /* true  */
  AK_OPT_INDICES_SINGLE             = 2,  /* true  */
  AK_OPT_COORD                      = 3   /* Y_UP  */
} AkOption;

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value);

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option);

#endif /* ak_options_h */