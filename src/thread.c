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

#include "thread.h"

#if !defined(AK_WINAPI)
#  include <unistd.h>
#endif

#if defined(AK_WINAPI)
static
DWORD
WINAPI
ak_thread_entry(void *arg) {
  AkThread *thread;

  thread = arg;
  thread->func(thread->userdata);
  return 0;
}
#else
static
void*
ak_thread_entry(void *arg) {
  AkThread *thread;

  thread = arg;
  thread->func(thread->userdata);
  return NULL;
}
#endif

AK_HIDE
bool
ak_thread_start(AkThread    * __restrict thread,
                AkThreadFunc             func,
                void        * __restrict userdata) {
  if (!thread || !func)
    return false;

  memset(thread, 0, sizeof(*thread));
  thread->func     = func;
  thread->userdata = userdata;

#if defined(AK_WINAPI)
  thread->handle = CreateThread(NULL, 0, ak_thread_entry, thread, 0, NULL);
  thread->started = thread->handle != NULL;
#else
  thread->started = pthread_create(&thread->handle, NULL, ak_thread_entry, thread) == 0;
#endif

  return thread->started;
}

AK_HIDE
void
ak_thread_join(AkThread * __restrict thread) {
  if (!thread || !thread->started)
    return;

#if defined(AK_WINAPI)
  WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
  thread->handle = NULL;
#else
  pthread_join(thread->handle, NULL);
#endif

  thread->started = false;
}

AK_HIDE
uint32_t
ak_thread_cpu_count(void) {
#if defined(AK_WINAPI)
  SYSTEM_INFO info;

  GetSystemInfo(&info);
  return info.dwNumberOfProcessors > 0u
         ? (uint32_t)info.dwNumberOfProcessors
         : 1u;
#elif defined(_SC_NPROCESSORS_ONLN)
  long count;

  count = sysconf(_SC_NPROCESSORS_ONLN);
  return count > 0 ? (uint32_t)count : 1u;
#else
  return 1u;
#endif
}
