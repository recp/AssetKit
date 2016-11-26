/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_options_h
#define ak_options_h

typedef enum AkOption {
  AK_OPTION_SOURCE_FIX_NONE        = 0,  /* false */
  AK_OPTION_SOURCE_FIX_INTERLEAVED = 1,  /* true  */
  AK_OPTION_SOURCE_FIX_INDIVIDUAL  = 2,  /* true  */
  AK_OPTION_COORD                  = 3   /* Y_UP  */
} AkOption;

AK_EXPORT
void
ak_opt_set(AkOption option, uintptr_t value);

AK_EXPORT
uintptr_t
ak_opt_get(AkOption option);

#endif /* ak_options_h */
