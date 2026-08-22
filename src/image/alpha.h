/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef ak_src_image_alpha_h
#define ak_src_image_alpha_h

#include "../common.h"

typedef enum AkImageAlphaPresence {
  AK_IMAGE_ALPHA_UNKNOWN = 0,
  AK_IMAGE_ALPHA_ABSENT,
  AK_IMAGE_ALPHA_PRESENT
} AkImageAlphaPresence;

/* Inspects encoded image metadata only. This never decodes pixel data. */
AK_HIDE
AkImageAlphaPresence
ak_imageAlphaPresence(AkImage * __restrict image);

#endif /* ak_src_image_alpha_h */
