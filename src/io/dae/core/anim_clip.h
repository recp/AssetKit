/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#ifndef dae_anim_clip_h
#define dae_anim_clip_h

#include "../common.h"

AK_HIDE
void
dae_animationClips(DAEState * __restrict dst,
                   xml_t    * __restrict xml);

AK_HIDE
void
dae_fixupAnimationClips(DAEState * __restrict dst);

#endif /* dae_anim_clip_h */
