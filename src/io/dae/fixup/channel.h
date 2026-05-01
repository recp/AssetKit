/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef dae_fixup_channel_h
#define dae_fixup_channel_h

#include "../common.h"

/*
 * Resolve DAE animation channel targets that ak_sid_resolve cannot handle
 * by itself — currently the parenthesized index syntax used for morph
 * weights, e.g. <channel target="morph-weights(0)"/>.
 *
 * For matched patterns this builds an AkResolvedTarget pointing at the
 * AkInstanceMorph (target), the index inside the parentheses (off), and
 * isPartial=true; ak_channelTarget then returns it directly. Channels
 * with conventional "node/transform.attr" SID syntax are left alone.
 *
 * Must run AFTER dae_fixup_instctlr so AkInstanceMorph instances exist.
 */
AK_HIDE
void
dae_fixup_channel(DAEState * __restrict dst);

#endif /* dae_fixup_channel_h */
