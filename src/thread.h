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

#ifndef ak_src_thread_h
#define ak_src_thread_h

#include "common.h"

#if defined(AK_WINAPI)
#  include <windows.h>
#else
#  include <pthread.h>
#endif

typedef void (*AkThreadFunc)(void *userdata);

typedef struct AkThreadTask {
  AkThreadFunc func;
  void        *userdata;
} AkThreadTask;

typedef struct AkThread {
  AkThreadFunc func;
  void        *userdata;
#if defined(AK_WINAPI)
  HANDLE       handle;
#else
  pthread_t    handle;
#endif
  bool         started;
} AkThread;

AK_HIDE
bool
ak_thread_start(AkThread    * __restrict thread,
                AkThreadFunc             func,
                void        * __restrict userdata);

AK_HIDE
void
ak_thread_join(AkThread * __restrict thread);

AK_HIDE
uint32_t
ak_thread_cpu_count(void);

AK_HIDE
bool
ak_thread_run_tasks(const AkThreadTask * __restrict tasks,
                    uint32_t                         taskCount);

#endif /* ak_src_thread_h */
