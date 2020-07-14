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

#ifndef ply_strpool_h
#  define ply_strpool_h

#ifndef _ply_STRPOOL_
#  define _AK_EXTERN extern
#else
#  define _AK_EXTERN
#endif

_AK_EXTERN const char _s_ply_pool_0[];

#define _s_ply_0(x) (_s_ply_pool_0 + x)

/* _s_ply_pool_0 */
#define _s_ply_binary_little_endian _s_ply_0(0)
#define _s_ply_binary_big_endian _s_ply_0(21)
#define _s_ply_char _s_ply_0(39)
#define _s_ply_uchar _s_ply_0(44)
#define _s_ply_short _s_ply_0(50)
#define _s_ply_ushort _s_ply_0(56)
#define _s_ply_int _s_ply_0(63)
#define _s_ply_uint _s_ply_0(67)
#define _s_ply_float _s_ply_0(72)
#define _s_ply_double _s_ply_0(78)
#define _s_ply_int8 _s_ply_0(85)
#define _s_ply_uint8 _s_ply_0(90)
#define _s_ply_int16 _s_ply_0(96)
#define _s_ply_uint16 _s_ply_0(102)
#define _s_ply_int32 _s_ply_0(109)
#define _s_ply_uint32 _s_ply_0(115)
#define _s_ply_float32 _s_ply_0(122)
#define _s_ply_float64 _s_ply_0(130)

#endif /* ply_strpool_h */
