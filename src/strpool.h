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

#ifndef ak_strpool_h
#  define ak_strpool_h

#ifndef AK_STRPOOL_
#  define _AK_EXTERN extern
#else
#  define _AK_EXTERN
#endif

_AK_EXTERN const char _s_ak_pool_0[];

#define _s_ak_0(x) (_s_ak_pool_0 + x)

/* _s_ak_pool_0 */
#define _s_POSITION _s_ak_0(0)
#define _s_NORMAL _s_ak_0(9)
#define _s_TEXCOORD _s_ak_0(16)
#define _s_COLOR _s_ak_0(25)

#endif /* ak_strpool_h */
