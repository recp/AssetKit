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

#include "source.h"
#include "../core/asset.h"
#include "../core/techn.h"
#include "../core/enum.h"
#include "../core/value.h"
#include "../../../thread.h"

#define DAE_FLOAT_PARSE_PARALLEL_MIN_VALUES 131072u
#define DAE_FLOAT_PARSE_MAX_THREADS 8u

typedef struct DaeFloatParseWorker {
  DaeFloatParseJob *jobs;
  size_t            jobCount;
  uint32_t          workerIndex;
} DaeFloatParseWorker;

static
uint64_t
dae_float_parse_job_weight(const DaeFloatParseJob *job) {
  uint64_t sourceBytes;
  uint64_t bufferBytes;

  sourceBytes = job->sourceByteCount;
  bufferBytes = job->buffer->length;
  if (sourceBytes > UINT64_MAX - bufferBytes)
    return UINT64_MAX;
  return sourceBytes + bufferBytes;
}

static
void
dae_parse_float_source_task(void *userdata) {
  DaeFloatParseJob *job;
  size_t            count;

  job = userdata;
  if (!job || !job->source || !job->buffer || !job->buffer->data)
    return;

  count = job->buffer->length / sizeof(AkFloat);
  xml_strtof_fast(job->source,
                  job->buffer->data,
                  (unsigned long)count);
}

static
int
dae_float_parse_job_compare(const void *left, const void *right) {
  const DaeFloatParseJob *a;
  const DaeFloatParseJob *b;

  a = left;
  b = right;

  return (dae_float_parse_job_weight(a) < dae_float_parse_job_weight(b))
       - (dae_float_parse_job_weight(a) > dae_float_parse_job_weight(b));
}

static
void
dae_parse_float_source_worker(void *userdata) {
  DaeFloatParseWorker *worker;
  size_t               i;

  worker = userdata;
  for (i = 0u; i < worker->jobCount; i++) {
    if (worker->jobs[i].workerIndex == worker->workerIndex)
      dae_parse_float_source_task(&worker->jobs[i]);
  }
}

static
void
dae_parse_float_sources_serial(DAEState * __restrict dst) {
  size_t i;

  for (i = 0u; i < dst->floatParseJobCount; i++)
    dae_parse_float_source_task(&dst->floatParseJobs[i]);
}

AK_HIDE
void
dae_parse_float_sources(DAEState * __restrict dst) {
  AkThreadTask          tasks[DAE_FLOAT_PARSE_MAX_THREADS];
  DaeFloatParseWorker  workers[DAE_FLOAT_PARSE_MAX_THREADS];
  size_t                jobCount;
  uint64_t              workerLoads[DAE_FLOAT_PARSE_MAX_THREADS];
  uint32_t              threadCount;
  uint32_t              i;

  if (!dst || !dst->floatParseJobs || dst->floatParseJobCount == 0u)
    return;

  jobCount = dst->floatParseJobCount;
  if (jobCount < 2u
      || jobCount > UINT32_MAX
      || dst->floatValueCount < DAE_FLOAT_PARSE_PARALLEL_MIN_VALUES) {
    dae_parse_float_sources_serial(dst);
    goto done;
  }

  /* Longest jobs first improves greedy worker load balancing. */
  qsort(dst->floatParseJobs,
        jobCount,
        sizeof(*dst->floatParseJobs),
        dae_float_parse_job_compare);

  threadCount = ak_thread_cpu_count();
  if (threadCount > DAE_FLOAT_PARSE_MAX_THREADS)
    threadCount = DAE_FLOAT_PARSE_MAX_THREADS;
  if ((size_t)threadCount > jobCount)
    threadCount = (uint32_t)jobCount;

  memset(workerLoads, 0, sizeof(workerLoads));
  for (i = 0u; i < jobCount; i++) {
    uint64_t weight;
    uint32_t lightest;
    uint32_t workerIndex;

    lightest = 0u;
    for (workerIndex = 1u; workerIndex < threadCount; workerIndex++) {
      if (workerLoads[workerIndex] < workerLoads[lightest])
        lightest = workerIndex;
    }

    weight = dae_float_parse_job_weight(&dst->floatParseJobs[i]);
    dst->floatParseJobs[i].workerIndex = lightest;
    workerLoads[lightest] = weight <= UINT64_MAX - workerLoads[lightest]
                             ? workerLoads[lightest] + weight
                             : UINT64_MAX;
  }

  for (i = 0u; i < threadCount; i++) {
    workers[i].jobs        = dst->floatParseJobs;
    workers[i].jobCount    = jobCount;
    workers[i].workerIndex = i;
    tasks[i].func          = dae_parse_float_source_worker;
    tasks[i].userdata      = &workers[i];
  }

  if (!ak_thread_run_tasks(tasks, threadCount))
    dae_parse_float_sources_serial(dst);

done:
  free(dst->floatParseJobs);
  dst->floatParseJobs        = NULL;
  dst->floatParseJobCount    = 0u;
  dst->floatParseJobCapacity = 0u;
  dst->floatValueCount       = 0u;
}

static
bool
dae_defer_float_source(DAEState   * __restrict dst,
                       const xml_t * __restrict source,
                       AkBuffer    * __restrict buffer,
                       uint32_t                 count) {
  DaeFloatParseJob *jobs;
  size_t            capacity;

  if (dst->floatValueCount > UINT64_MAX - count)
    return false;

  if (dst->floatParseJobCount == dst->floatParseJobCapacity) {
    if (dst->floatParseJobCapacity > SIZE_MAX / 2u)
      return false;

    capacity = dst->floatParseJobCapacity
               ? dst->floatParseJobCapacity * 2u
               : 64u;
    if (capacity > SIZE_MAX / sizeof(*jobs))
      return false;

    jobs = realloc(dst->floatParseJobs, sizeof(*jobs) * capacity);
    if (!jobs)
      return false;

    dst->floatParseJobs        = jobs;
    dst->floatParseJobCapacity = capacity;
  }

  jobs = &dst->floatParseJobs[dst->floatParseJobCount++];
  jobs->source          = source;
  jobs->buffer          = buffer;
  jobs->sourceByteCount = xmls_sumlen(source);
  jobs->workerIndex     = 0u;
  dst->floatValueCount += count;
  return true;
}

AK_HIDE
DaeSource*
dae_source(DAEState * __restrict dst,
           xml_t    * __restrict xml,
           AkEnum              (*asEnum)(const char *name, size_t nameLen),
           AkTypeId              enumType) {
  AkHeap        *heap;
  DaeSource     *source;
  AkBuffer      *buffer;
  AkTechnique   *tq;
  AkAccessor    *acc;
  AkAccessorDAE *accdae;
  const xml_t   *sval;
  void          *rootmemp, *tempmem;
  uint32_t       count;
  AkTypeId       t;
  bool           isName;

  heap     = dst->heap;
  rootmemp = ak_heap_data(heap->data);
  tempmem  = dst->tempmem;
  isName   = false;
  buffer   = NULL;
  source   = ak_heap_calloc(heap, tempmem, sizeof(*source));
  ak_setypeid(source, DAE_TYPE_SOURCE);

  xmla_setid(xml, heap, source);
  source->name = DAE_XMLA_STRDUP8(xml, heap, name, source);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, source, NULL);
    } else if (DAE_XML_TAG_EQ(xml, techniquec)) {
      xml_t       *xacc;
      AkDataParam *dp_last;

      if ((xacc = DAE_XML_ELEM8(xml, accessor))) {
        acc         = ak_heap_calloc(heap, rootmemp, sizeof(*acc));
        accdae      = ak_heap_calloc(heap, tempmem,  sizeof(*accdae));
        
        ak_heap_setUserData(heap, acc, accdae);
        
        acc->count     = xmla_u32(DAE_XMLA8(xacc, count),  0);
        accdae->offset = xmla_u32(DAE_XMLA8(xacc, offset), 0);
        accdae->stride = xmla_u32(DAE_XMLA8(xacc, stride), 1);

        ak_setypeid(acc, AKT_ACCESSOR);
        DAE_URL_SET(dst, xacc, source, accdae, &accdae->source);

        xacc    = xacc->val;
        dp_last = NULL;

        while (xacc) {
          AkDataParam *dp;
          
          dp = ak_heap_calloc(heap, accdae, sizeof(*dp));
          sid_set(xacc, heap, dp);

          dp->name = DAE_XMLA_STRDUP8(xacc, heap, name, dp);
          dae_dtype_attr(DAE_XMLA8(xacc, type),  &dp->type);
          
          AK_APPEND_FLINK(accdae->param, dp_last, dp);
          xacc = xacc->next;
        }

        source->accessor = acc;

        /* append accessor to global list */
        /* this will be prepared in postprocess */
        flist_sp_insert(&dst->accessors, acc);
      }
    } else if (DAE_XML_TAG_EQ(xml, technique)) {
      tq       = dae_techn(xml, heap, source);
      tq->next = (AkTechnique *)source->reserved;
      source->reserved = tq;
    } else if (xml_valtype(xml) == XML_STRING && (sval = xmls(xml))) {
      size_t availableCount;

      count = xmla_u32(DAE_XMLA8(xml, count), 0);
      availableCount = xml_strtok_count_fast(sval, NULL);
      if (availableCount < count)
        count = (uint32_t)availableCount;
      buffer           = ak_heap_alloc(heap, rootmemp, sizeof(*buffer));
      if (!buffer) {
        xml = xml->next;
        continue;
      }
      memset(buffer, 0, sizeof(*buffer));
      buffer->name     = DAE_XMLA_STRDUP8(xml, heap, name, buffer);
      source->buffer   = buffer;
      
      xmla_setid(xml, heap, buffer);
      
      if (DAE_XML_TAG_EQ(xml, float_array)) {
        if ((size_t)count > (size_t)-1 / sizeof(float))
          count = 0u;
        buffer->length = sizeof(float) * (size_t)count;
        buffer->data   = buffer->length
                           ? ak_heap_alloc(heap, buffer, buffer->length)
                           : NULL;
        /* Float sources remain independent until DAE post-processing. */
        if (buffer->data && !dae_defer_float_source(dst, sval, buffer, count))
          xml_strtof_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_FLOAT);
      } else if (DAE_XML_TAG_EQ(xml, int_array)) {
        if ((size_t)count > (size_t)-1 / sizeof(uint32_t))
          count = 0u;
        buffer->length = sizeof(uint32_t) * (size_t)count;
        buffer->data   = buffer->length
                           ? ak_heap_alloc(heap, buffer, buffer->length)
                           : NULL;
        if (buffer->data)
          xml_strtoi_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_INT);
      } else if (DAE_XML_TAG_EQ(xml, bool_array)) {
        if ((size_t)count > (size_t)-1 / sizeof(bool))
          count = 0u;
        buffer->length = sizeof(bool) * (size_t)count;
        buffer->data   = buffer->length
                           ? ak_heap_alloc(heap, buffer, buffer->length)
                           : NULL;
        if (buffer->data)
          xml_strtob_fast(sval, buffer->data, count);
        
        ak_setUserData(buffer, (void *)(uintptr_t)AKT_BOOL);
      } else if ((DAE_XML_TAG_EQ(xml, Name_array)   & (1|(t = AKT_NAME)))
              || (DAE_XML_TAG_EQ(xml, IDREF_array)  & (1|(t = AKT_IDREF)))
              || (DAE_XML_TAG_EQ(xml, SIDREF_array) & (1|(t = AKT_SIDREF)))
              || (DAE_XML_TAG_EQ(xml, token_array)  & (1|(t = AKT_TOKEN)))) {
        char        *pData, **iter, *tok, *tok_begin, *end, c;
        const xml_t *v;
        size_t       srclen, toklen, enumLen;
        uint32_t     idx;
        AkEnum       enumValue;

        /*
         |pSTR1|pSTR2|pSTR3|STR1\0STR2\0STR3|
         
         the last one is pointer to all data
         */
        
        isName = true;
        idx    = 0;

        if (asEnum) {
          ak_setUserData(buffer, (void *)(uintptr_t)enumType);
          
          enumLen        = ak_typeDesc(enumType)->size;
          if (!enumLen || (size_t)count > (size_t)-1 / enumLen)
            count = 0u;
          buffer->length = enumLen * (size_t)count;
          buffer->data   = buffer->length
                             ? ak_heap_alloc(heap, buffer, buffer->length)
                             : NULL;
          pData          = buffer->data;

          if (pData && (v = sval) && (tok = v->val)) {
            do {
              if (idx >= count)
                break;

              srclen = v->valsize;
              end    = tok + srclen;

              do {
                while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                tok_begin = tok;
                
                while (tok < end && !((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                toklen    = tok - tok_begin;
                enumValue = asEnum(tok_begin, toklen);
                memcpy(pData + idx * enumLen, &enumValue, enumLen);

                idx++;
              } while (idx < count && tok < end);
            } while ((v = xmls_next(v)) && (tok = v->val));
          }
        } else {
          size_t pointerBytes, stringBytes;

          ak_setUserData(buffer, (void *)(uintptr_t)t);

          stringBytes = xmls_sumlen(sval);
          if ((size_t)count > (size_t)-1 / 2u
              || (size_t)count * 2u > (size_t)-1 / sizeof(char *)
              || stringBytes == (size_t)-1) {
            count = 0u;
            pointerBytes = 0u;
            buffer->length = 0u;
          } else {
            /* Preserve the established buffer layout/size: two pointer slots
               per declared token, with the packed strings beginning after
               the count+1 pointer table. */
            pointerBytes = sizeof(char *)
                           * (count ? (size_t)count * 2u : 1u);
            if (pointerBytes > (size_t)-1 - stringBytes - 1u) {
              count = 0u;
              pointerBytes = 0u;
              buffer->length = 0u;
            } else {
              buffer->length = pointerBytes + stringBytes + 1u;
            }
          }
          iter = buffer->length
                   ? ak_heap_alloc(heap, buffer, buffer->length)
                   : NULL;
          buffer->data = iter;
          pData = iter
                    ? (char *)iter + sizeof(char *) * ((size_t)count + 1u)
                    : NULL;
          
          if (iter)
            iter[count] = pData;

          if (iter && (v = sval) && (tok = v->val)) {
            do {
              if (idx >= count)
                break;

              srclen = v->valsize;
              end    = tok + srclen;

              do {
                while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;
                
                tok_begin = tok;
                
                while (tok < end && !((void)(c = *tok), AK_ARRAY_SEP_CHECK))
                  tok++;

                toklen = tok - tok_begin;
                memcpy(pData, tok_begin, toklen);
                iter[idx++] = pData;
                
                pData += toklen;
                *pData++ = '\0';
              } while (idx < count && tok < end);
            } while ((v = xmls_next(v)) && (tok = v->val));
          }
        } /* if asEnum */
      }
    }
    
    xml = xml->next;
  }

  if (source->accessor
      && isName
      && asEnum
      && (accdae = ak_userData(source->accessor))) {

    accdae->bound  = 1;
    accdae->stride = 1;
  }

  return source;
}
