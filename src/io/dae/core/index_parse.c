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

#include "index_parse.h"
#include "../../../thread.h"

#define DAE_INDEX_PARSE_PARALLEL_MIN_VALUES 131072u
#define DAE_INDEX_PARSE_MAX_THREADS 8u

typedef struct DaeIndexParseWorker {
  DaeIndexParseJob *jobs;
  uint32_t         *scratch;
  size_t            jobCount;
  uint32_t          workerIndex;
} DaeIndexParseWorker;

static
void
dae_parse_index_job(DaeIndexParseJob * __restrict job,
                    uint32_t         * __restrict scratch) {
  if (!job || !job->source || !scratch || job->count == 0u)
    return;

  job->maxValue = 0u;
  job->remaining = xml_strtoindex_u32_fast_max(
    job->source,
    scratch + job->scratchOffset,
    job->count,
    &job->maxValue);
}

static
void
dae_parse_index_worker(void *userdata) {
  DaeIndexParseWorker *worker;
  size_t               i;

  worker = userdata;
  for (i = 0u; i < worker->jobCount; i++) {
    if (worker->jobs[i].workerIndex == worker->workerIndex)
      dae_parse_index_job(&worker->jobs[i], worker->scratch);
  }
}

static
int
dae_index_parse_job_compare_weight(const void *left, const void *right) {
  const DaeIndexParseJob *a;
  const DaeIndexParseJob *b;

  a = left;
  b = right;

  return (a->sourceByteCount < b->sourceByteCount)
       - (a->sourceByteCount > b->sourceByteCount);
}

static
int
dae_index_parse_job_compare_sequence(const void *left, const void *right) {
  const DaeIndexParseJob *a;
  const DaeIndexParseJob *b;

  a = left;
  b = right;

  return (a->sequence > b->sequence) - (a->sequence < b->sequence);
}

static
void
dae_parse_index_jobs_serial(DAEState * __restrict dst) {
  size_t i;

  for (i = 0u; i < dst->indexParseJobCount; i++) {
    DaeIndexParseJob *job;
    AkIndexArray     *indices;
    AkUInt            maxValue;

    job      = &dst->indexParseJobs[i];
    maxValue = 0u;
    if (xml_strtoindex_arrayN_max(dst->heap,
                                  job->parent,
                                  job->source,
                                  job->count,
                                  &indices,
                                  &maxValue) == AK_OK)
      job->primitive->indices = indices;
  }
}

static
void
dae_copy_index_values(AkIndexArray  * __restrict indices,
                      const uint32_t * __restrict values,
                      size_t                      count) {
  size_t i;

  switch (indices->componentType) {
    case AKT_UBYTE: {
      uint8_t *dst;

      dst = (uint8_t *)indices->items;
      for (i = 0u; i < count; i++)
        dst[i] = (uint8_t)values[i];
      break;
    }
    case AKT_USHORT: {
      uint16_t *dst;

      dst = (uint16_t *)indices->items;
      for (i = 0u; i < count; i++)
        dst[i] = (uint16_t)values[i];
      break;
    }
    case AKT_UINT:
      memcpy(indices->items, values, sizeof(*values) * count);
      break;
    default:
      break;
  }
}

static
void
dae_finalize_index_job(DAEState        * __restrict dst,
                       DaeIndexParseJob * __restrict job,
                       const uint32_t   * __restrict scratch) {
  AkIndexArray *indices;

  if (job->remaining != 0u) {
    AkUInt maxValue;

    maxValue = 0u;
    if (xml_strtoindex_array_max(dst->heap,
                                 job->parent,
                                 job->source,
                                 &indices,
                                 &maxValue) == AK_OK)
      job->primitive->indices = indices;
    return;
  }

  indices = ak_indexArrayAlloc(dst->heap,
                               job->parent,
                               job->count,
                               ak_indexComponentTypeForMax(job->maxValue));
  if (!indices)
    return;

  dae_copy_index_values(indices,
                        scratch + job->scratchOffset,
                        job->count);
  indices->max = job->maxValue;
  job->primitive->indices = indices;
}

AK_HIDE
void
dae_parse_index_arrays(DAEState * __restrict dst) {
  AkThreadTask        tasks[DAE_INDEX_PARSE_MAX_THREADS];
  DaeIndexParseWorker workers[DAE_INDEX_PARSE_MAX_THREADS];
  uint32_t           *scratch;
  size_t              jobCount;
  size_t              scratchOffset;
  uint64_t            workerLoads[DAE_INDEX_PARSE_MAX_THREADS];
  uint32_t            threadCount;
  uint32_t            i;

  if (!dst || !dst->indexParseJobs || dst->indexParseJobCount == 0u)
    return;

  jobCount = dst->indexParseJobCount;
  if (jobCount < 2u
      || jobCount > UINT32_MAX
      || dst->indexValueCount < DAE_INDEX_PARSE_PARALLEL_MIN_VALUES
      || dst->indexValueCount > SIZE_MAX / sizeof(*scratch)) {
    dae_parse_index_jobs_serial(dst);
    goto done;
  }

  scratch = malloc(sizeof(*scratch) * (size_t)dst->indexValueCount);
  if (!scratch) {
    dae_parse_index_jobs_serial(dst);
    goto done;
  }

  /*
   * Parse longest arrays first for balanced fixed-stride worker groups.
   * Restore source order before assigning results so malformed files with
   * repeated <p> elements keep the previous last-write-wins behavior.
   */
  qsort(dst->indexParseJobs,
        jobCount,
        sizeof(*dst->indexParseJobs),
        dae_index_parse_job_compare_weight);

  scratchOffset = 0u;
  for (i = 0u; i < jobCount; i++) {
    dst->indexParseJobs[i].scratchOffset = scratchOffset;
    scratchOffset += dst->indexParseJobs[i].count;
  }

  threadCount = ak_thread_cpu_count();
  if (threadCount > DAE_INDEX_PARSE_MAX_THREADS)
    threadCount = DAE_INDEX_PARSE_MAX_THREADS;
  if ((size_t)threadCount > jobCount)
    threadCount = (uint32_t)jobCount;

  memset(workerLoads, 0, sizeof(workerLoads));
  for (i = 0u; i < jobCount; i++) {
    uint32_t lightest;
    uint32_t workerIndex;

    lightest = 0u;
    for (workerIndex = 1u; workerIndex < threadCount; workerIndex++) {
      if (workerLoads[workerIndex] < workerLoads[lightest])
        lightest = workerIndex;
    }

    dst->indexParseJobs[i].workerIndex = lightest;
    workerLoads[lightest] += dst->indexParseJobs[i].sourceByteCount;
  }

  for (i = 0u; i < threadCount; i++) {
    workers[i].jobs        = dst->indexParseJobs;
    workers[i].scratch     = scratch;
    workers[i].jobCount    = jobCount;
    workers[i].workerIndex = i;
    tasks[i].func          = dae_parse_index_worker;
    tasks[i].userdata      = &workers[i];
  }

  if (!ak_thread_run_tasks(tasks, threadCount)) {
    for (i = 0u; i < jobCount; i++)
      dae_parse_index_job(&dst->indexParseJobs[i], scratch);
  }

  qsort(dst->indexParseJobs,
        jobCount,
        sizeof(*dst->indexParseJobs),
        dae_index_parse_job_compare_sequence);

  for (i = 0u; i < jobCount; i++)
    dae_finalize_index_job(dst, &dst->indexParseJobs[i], scratch);

  free(scratch);

done:
  free(dst->indexParseJobs);
  dst->indexParseJobs        = NULL;
  dst->indexParseJobCount    = 0u;
  dst->indexParseJobCapacity = 0u;
  dst->indexValueCount       = 0u;
}

AK_HIDE
bool
dae_defer_index_array(DAEState        * __restrict dst,
                      const xml_t     * __restrict source,
                      AkMeshPrimitive * __restrict primitive,
                      void            * __restrict parent,
                      unsigned long                count) {
  DaeIndexParseJob *jobs;
  size_t            capacity;

  if (!dst || !source || !primitive || !parent || count == 0u)
    return false;
  if (dst->indexValueCount > UINT64_MAX - count)
    return false;

  if (dst->indexParseJobCount == dst->indexParseJobCapacity) {
    if (dst->indexParseJobCapacity > SIZE_MAX / 2u)
      return false;

    capacity = dst->indexParseJobCapacity
               ? dst->indexParseJobCapacity * 2u
               : 64u;
    if (capacity > SIZE_MAX / sizeof(*jobs))
      return false;

    jobs = realloc(dst->indexParseJobs, sizeof(*jobs) * capacity);
    if (!jobs)
      return false;

    dst->indexParseJobs        = jobs;
    dst->indexParseJobCapacity = capacity;
  }

  jobs = &dst->indexParseJobs[dst->indexParseJobCount];
  jobs->source          = source;
  jobs->primitive       = primitive;
  jobs->parent          = parent;
  jobs->scratchOffset   = 0u;
  jobs->sequence        = dst->indexParseJobCount;
  jobs->sourceByteCount = xmls_sumlen(source);
  jobs->count           = count;
  jobs->remaining       = count;
  jobs->maxValue        = 0u;
  jobs->workerIndex     = 0u;

  dst->indexParseJobCount++;
  dst->indexValueCount += count;
  return true;
}
