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

#define AK_THREAD_POOL_MAX_WORKERS 7u

typedef struct AkThreadPool {
  const AkThreadTask *tasks;
  uint32_t            taskCount;
  uint32_t            nextTask;
  uint32_t            remaining;
  uint32_t            workerCount;
  bool                active;
  bool                stop;
#if defined(AK_WINAPI)
  CRITICAL_SECTION    mutex;
  CONDITION_VARIABLE  workCond;
  CONDITION_VARIABLE  doneCond;
  HANDLE              workers[AK_THREAD_POOL_MAX_WORKERS];
#else
  pthread_mutex_t     mutex;
  pthread_cond_t      workCond;
  pthread_cond_t      doneCond;
  pthread_t           workers[AK_THREAD_POOL_MAX_WORKERS];
#endif
} AkThreadPool;

static AkThreadPool ak_thread_pool;
static uint32_t     ak_thread_cached_cpu_count;

AK_HIDE
void
ak_mutex_init(AkMutex * __restrict mutex) {
#if defined(AK_WINAPI)
  InitializeCriticalSection(mutex);
#else
  pthread_mutex_init(mutex, NULL);
#endif
}

AK_HIDE
void
ak_mutex_destroy(AkMutex * __restrict mutex) {
#if defined(AK_WINAPI)
  DeleteCriticalSection(mutex);
#else
  pthread_mutex_destroy(mutex);
#endif
}

AK_HIDE
void
ak_mutex_lock(AkMutex * __restrict mutex) {
#if defined(AK_WINAPI)
  EnterCriticalSection(mutex);
#else
  pthread_mutex_lock(mutex);
#endif
}

AK_HIDE
void
ak_mutex_unlock(AkMutex * __restrict mutex) {
#if defined(AK_WINAPI)
  LeaveCriticalSection(mutex);
#else
  pthread_mutex_unlock(mutex);
#endif
}

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

#if defined(AK_WINAPI)
static
void
ak_thread_pool_lock(AkThreadPool *pool) {
  EnterCriticalSection(&pool->mutex);
}

static
void
ak_thread_pool_unlock(AkThreadPool *pool) {
  LeaveCriticalSection(&pool->mutex);
}

static
void
ak_thread_pool_wait_work(AkThreadPool *pool) {
  SleepConditionVariableCS(&pool->workCond, &pool->mutex, INFINITE);
}

static
void
ak_thread_pool_wait_done(AkThreadPool *pool) {
  SleepConditionVariableCS(&pool->doneCond, &pool->mutex, INFINITE);
}

static
void
ak_thread_pool_signal_done(AkThreadPool *pool) {
  WakeConditionVariable(&pool->doneCond);
}

static
void
ak_thread_pool_wake_workers(AkThreadPool *pool) {
  WakeAllConditionVariable(&pool->workCond);
}
#else
static
void
ak_thread_pool_lock(AkThreadPool *pool) {
  pthread_mutex_lock(&pool->mutex);
}

static
void
ak_thread_pool_unlock(AkThreadPool *pool) {
  pthread_mutex_unlock(&pool->mutex);
}

static
void
ak_thread_pool_wait_work(AkThreadPool *pool) {
  pthread_cond_wait(&pool->workCond, &pool->mutex);
}

static
void
ak_thread_pool_wait_done(AkThreadPool *pool) {
  pthread_cond_wait(&pool->doneCond, &pool->mutex);
}

static
void
ak_thread_pool_signal_done(AkThreadPool *pool) {
  pthread_cond_signal(&pool->doneCond);
}

static
void
ak_thread_pool_wake_workers(AkThreadPool *pool) {
  pthread_cond_broadcast(&pool->workCond);
}
#endif

static
bool
ak_thread_pool_take_task(AkThreadPool * __restrict pool,
                         AkThreadTask * __restrict task) {
  if (pool->nextTask >= pool->taskCount)
    return false;

  *task = pool->tasks[pool->nextTask++];
  return true;
}

static
void
ak_thread_pool_finish_task(AkThreadPool * __restrict pool) {
  pool->remaining--;
  if (pool->remaining == 0u)
    ak_thread_pool_signal_done(pool);
}

static
void
ak_thread_pool_worker_loop(void) {
  AkThreadPool *pool;

  pool = &ak_thread_pool;

  for (;;) {
    AkThreadTask task;
    bool         hasTask;

    ak_thread_pool_lock(pool);
    while (!pool->stop
           && (!pool->active || pool->nextTask >= pool->taskCount))
      ak_thread_pool_wait_work(pool);

    if (pool->stop) {
      ak_thread_pool_unlock(pool);
      return;
    }

    hasTask = ak_thread_pool_take_task(pool, &task);
    ak_thread_pool_unlock(pool);
    if (!hasTask)
      continue;

    task.func(task.userdata);

    ak_thread_pool_lock(pool);
    ak_thread_pool_finish_task(pool);
    ak_thread_pool_unlock(pool);
  }
}

#if defined(AK_WINAPI)
static
DWORD
WINAPI
ak_thread_pool_worker_entry(void *arg) {
  (void)arg;
  ak_thread_pool_worker_loop();
  return 0;
}
#else
static
void*
ak_thread_pool_worker_entry(void *arg) {
  (void)arg;
  ak_thread_pool_worker_loop();
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

static
uint32_t
ak_thread_detect_cpu_count(void) {
  uint32_t detected;

#if defined(AK_WINAPI)
  SYSTEM_INFO info;

  GetSystemInfo(&info);
  detected = info.dwNumberOfProcessors > 0u
             ? (uint32_t)info.dwNumberOfProcessors
             : 1u;
#elif defined(_SC_NPROCESSORS_ONLN)
  long count;

  count = sysconf(_SC_NPROCESSORS_ONLN);
  detected = count > 0 ? (uint32_t)count : 1u;
#else
  detected = 1u;
#endif

  return detected;
}

#if defined(AK_WINAPI)
static INIT_ONCE ak_thread_cpu_count_once = INIT_ONCE_STATIC_INIT;

static
BOOL
CALLBACK
ak_thread_cpu_count_init_once(PINIT_ONCE once,
                              PVOID      param,
                              PVOID     *context) {
  (void)once;
  (void)param;
  (void)context;

  ak_thread_cached_cpu_count = ak_thread_detect_cpu_count();
  return TRUE;
}
#else
static pthread_once_t ak_thread_cpu_count_once = PTHREAD_ONCE_INIT;

static
void
ak_thread_cpu_count_init_once(void) {
  ak_thread_cached_cpu_count = ak_thread_detect_cpu_count();
}
#endif

AK_HIDE
uint32_t
ak_thread_cpu_count(void) {
#if defined(AK_WINAPI)
  InitOnceExecuteOnce(&ak_thread_cpu_count_once,
                      ak_thread_cpu_count_init_once,
                      NULL,
                      NULL);
#else
  pthread_once(&ak_thread_cpu_count_once, ak_thread_cpu_count_init_once);
#endif
  return ak_thread_cached_cpu_count;
}

#if defined(AK_WINAPI)
static INIT_ONCE ak_thread_pool_once = INIT_ONCE_STATIC_INIT;

static
BOOL
CALLBACK
ak_thread_pool_init_once(PINIT_ONCE once, PVOID param, PVOID *context) {
  AkThreadPool *pool;
  uint32_t      cpuCount;
  uint32_t      maxWorkers;
  uint32_t      i;

  (void)once;
  (void)param;
  (void)context;

  pool = &ak_thread_pool;
  InitializeCriticalSection(&pool->mutex);
  InitializeConditionVariable(&pool->workCond);
  InitializeConditionVariable(&pool->doneCond);

  cpuCount   = ak_thread_cpu_count();
  maxWorkers = cpuCount > 1u ? cpuCount - 1u : 0u;
  if (maxWorkers > AK_THREAD_POOL_MAX_WORKERS)
    maxWorkers = AK_THREAD_POOL_MAX_WORKERS;

  for (i = 0u; i < maxWorkers; i++) {
    pool->workers[i] = CreateThread(NULL,
                                    0,
                                    ak_thread_pool_worker_entry,
                                    NULL,
                                    0,
                                    NULL);
    if (!pool->workers[i])
      break;
    pool->workerCount++;
  }

  return TRUE;
}

static
void
ak_thread_pool_init(void) {
  InitOnceExecuteOnce(&ak_thread_pool_once,
                      ak_thread_pool_init_once,
                      NULL,
                      NULL);
}
#else
static pthread_once_t ak_thread_pool_once = PTHREAD_ONCE_INIT;

static
void
ak_thread_pool_init_once(void) {
  AkThreadPool *pool;
  uint32_t      cpuCount;
  uint32_t      maxWorkers;
  uint32_t      i;

  pool = &ak_thread_pool;
  pthread_mutex_init(&pool->mutex, NULL);
  pthread_cond_init(&pool->workCond, NULL);
  pthread_cond_init(&pool->doneCond, NULL);

  cpuCount   = ak_thread_cpu_count();
  maxWorkers = cpuCount > 1u ? cpuCount - 1u : 0u;
  if (maxWorkers > AK_THREAD_POOL_MAX_WORKERS)
    maxWorkers = AK_THREAD_POOL_MAX_WORKERS;

  for (i = 0u; i < maxWorkers; i++) {
    if (pthread_create(&pool->workers[i],
                       NULL,
                       ak_thread_pool_worker_entry,
                       NULL) != 0)
      break;
    pool->workerCount++;
  }
}

static
void
ak_thread_pool_init(void) {
  pthread_once(&ak_thread_pool_once, ak_thread_pool_init_once);
}
#endif

AK_HIDE
bool
ak_thread_run_tasks(const AkThreadTask * __restrict tasks,
                    uint32_t                         taskCount) {
  AkThreadPool *pool;

  if (!tasks)
    return false;
  if (taskCount == 0u)
    return true;
  if (taskCount == 1u) {
    tasks[0].func(tasks[0].userdata);
    return true;
  }

  ak_thread_pool_init();
  pool = &ak_thread_pool;

  if (pool->workerCount == 0u) {
    uint32_t i;

    for (i = 0u; i < taskCount; i++)
      tasks[i].func(tasks[i].userdata);
    return true;
  }

  ak_thread_pool_lock(pool);
  if (pool->active) {
    uint32_t i;

    ak_thread_pool_unlock(pool);
    for (i = 0u; i < taskCount; i++)
      tasks[i].func(tasks[i].userdata);
    return true;
  }

  pool->tasks     = tasks;
  pool->taskCount = taskCount;
  pool->nextTask  = 0u;
  pool->remaining = taskCount;
  pool->active    = true;
  ak_thread_pool_wake_workers(pool);

  for (;;) {
    AkThreadTask task;
    bool         hasTask;

    hasTask = ak_thread_pool_take_task(pool, &task);
    if (!hasTask) {
      while (pool->remaining > 0u)
        ak_thread_pool_wait_done(pool);
      break;
    }

    ak_thread_pool_unlock(pool);
    task.func(task.userdata);
    ak_thread_pool_lock(pool);
    ak_thread_pool_finish_task(pool);
  }

  pool->active    = false;
  pool->tasks     = NULL;
  pool->taskCount = 0u;
  pool->nextTask  = 0u;
  ak_thread_pool_unlock(pool);
  return true;
}
