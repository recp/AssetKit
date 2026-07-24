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

#include "geom.h"
#include "mesh.h"
#include "../../../mesh/index.h"
#include "../../../thread.h"
#include "../../../topo/topo.h"

#define DAE_DUPLICATOR_PARALLEL_MIN_INDICES 131072
#define DAE_DUPLICATOR_PARALLEL_MAX_THREADS 8

typedef struct DaeDuplicatorWorker {
  DaeDuplicatorJob *jobs;
  size_t            jobCount;
  uint32_t          workerIndex;
} DaeDuplicatorWorker;

static
bool
dae_mesh_indices_stable_for_build(AkMesh *mesh) {
  AkMeshPrimitive *prim;
  AkInput         *input;
  bool             foundNormal;

  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (ak_opt_get(AK_OPT_TRIANGULATE)
        && prim->type == AK_PRIMITIVE_POLYGONS)
      return false;

    if (!ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED)
        || (prim->type != AK_PRIMITIVE_TRIANGLES
            && prim->type != AK_PRIMITIVE_POLYGONS))
      continue;

    foundNormal = false;
    for (input = prim->input; input; input = input->next) {
      if (input->semantic != AK_INPUT_NORMAL)
        continue;

      foundNormal = input->accessor && input->accessor->buffer;
      break;
    }
    if (!foundNormal)
      return false;
  }

  return true;
}

static
bool
dae_primitive_prepare_for_build(AkMeshPrimitive *prim,
                                size_t          *indexCount) {
  uint32_t stride;

  if (prim->indexStride <= 1
      || (!prim->indices && !prim->indexAccessor))
    return false;

  if (ak_primCollapseIdentityIndices(prim))
    return false;

  if (!prim->indices)
    ak_meshPrimitiveMaterializeIndices(prim);
  if (!prim->indices)
    return false;

  stride = prim->indexStride ? prim->indexStride : 1;
  if (!prim->pos
      || !prim->pos->accessor
      || prim->pos->indexOffset >= stride
      || prim->indices->count % stride != 0)
    return false;

  *indexCount = prim->indices->count / stride;
  return *indexCount > 0;
}

static
int
dae_duplicator_job_compare_weight(const void *a, const void *b) {
  const DaeDuplicatorJob *ja;
  const DaeDuplicatorJob *jb;

  ja = a;
  jb = b;
  if (ja->weight != jb->weight)
    return ja->weight < jb->weight ? 1 : -1;
  return (ja->sequence > jb->sequence) - (ja->sequence < jb->sequence);
}

static
int
dae_duplicator_job_compare_sequence(const void *a, const void *b) {
  const DaeDuplicatorJob *ja;
  const DaeDuplicatorJob *jb;

  ja = a;
  jb = b;
  return (ja->sequence > jb->sequence) - (ja->sequence < jb->sequence);
}

static
void
dae_duplicator_worker(void *userdata) {
  DaeDuplicatorWorker *worker;
  size_t               i;

  worker = userdata;
  for (i = 0; i < worker->jobCount; i++) {
    if (worker->jobs[i].workerIndex == worker->workerIndex)
      ak_meshDuplicatorBuildCompute(&worker->jobs[i].build);
  }
}

AK_HIDE
AkResult
dae_geom_fixup_all(AkDoc * doc, bool retainDuplicators) {
  AkGeometry          *geom;
  AkObject            *primitive;
  AkMesh              *mesh;
  AkMeshPrimitive     *meshPrimitive;
  DaeDuplicatorJob    *jobs;
  void                *scratchArena;
  DaeDuplicatorWorker  workers[DAE_DUPLICATOR_PARALLEL_MAX_THREADS];
  AkThreadTask         tasks[DAE_DUPLICATOR_PARALLEL_MAX_THREADS];
  size_t               workerLoads[DAE_DUPLICATOR_PARALLEL_MAX_THREADS];
  size_t               candidateCount, totalIndexCount, indexCount;
  size_t               jobCount, jobIndex, sequence, meshJobStart,
                       scratchSize, alignedOffset, i, w;
  uint32_t             workerCount, minWorker;

  candidateCount  = 0;
  totalIndexCount = 0;
  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    primitive = geom->gdata;
    if (!primitive || primitive->type != AK_GEOMETRY_MESH)
      continue;

    mesh = ak_objGet(primitive);
    topofix(mesh);
    if (!dae_mesh_indices_stable_for_build(mesh))
      continue;

    for (meshPrimitive = mesh->primitive;
         meshPrimitive;
         meshPrimitive = meshPrimitive->next) {
      if (!dae_primitive_prepare_for_build(meshPrimitive, &indexCount))
        continue;

      candidateCount++;
      totalIndexCount = indexCount <= SIZE_MAX - totalIndexCount
                        ? totalIndexCount + indexCount
                        : SIZE_MAX;
    }
  }

  if (candidateCount < 2
      || totalIndexCount < DAE_DUPLICATOR_PARALLEL_MIN_INDICES)
    goto serial;

  jobs = calloc(candidateCount, sizeof(*jobs));
  if (!jobs)
    goto serial;

  jobCount = sequence = 0;
  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    primitive = geom->gdata;
    if (!primitive || primitive->type != AK_GEOMETRY_MESH)
      continue;

    mesh = ak_objGet(primitive);
    if (!dae_mesh_indices_stable_for_build(mesh))
      continue;

    for (meshPrimitive = mesh->primitive;
         meshPrimitive;
         meshPrimitive = meshPrimitive->next) {
      if (meshPrimitive->indexStride <= 1
          || !meshPrimitive->indices
          || !ak_meshDuplicatorBuildPrepare(meshPrimitive,
                                            &jobs[jobCount].build))
        continue;

      jobs[jobCount].mesh     = mesh;
      jobs[jobCount].sequence = sequence++;
      jobs[jobCount].weight   = jobs[jobCount].build.indices->count;
      if (jobs[jobCount].build.vertexCount <= SIZE_MAX / 3
          && jobs[jobCount].weight
               <= SIZE_MAX - jobs[jobCount].build.vertexCount * 3) {
        jobs[jobCount].weight += jobs[jobCount].build.vertexCount * 3;
      } else {
        jobs[jobCount].weight = SIZE_MAX;
      }
      jobCount++;
    }
  }

  if (jobCount < 2) {
    free(jobs);
    goto serial;
  }

  /*
   * Workers must not touch AkHeap: its ownership tree and search context are
   * mutable. Give each build a cache-line-separated slice of one import-scope
   * arena, then attach the finished arrays to AkHeap serially below.
   */
  scratchArena = NULL;
  scratchSize  = 0;
  for (i = 0; i < jobCount; i++) {
    if (scratchSize > SIZE_MAX - 63u)
      break;
    alignedOffset = (scratchSize + 63u) & ~(size_t)63u;
    if (jobs[i].build.storageSize > SIZE_MAX - alignedOffset)
      break;

    jobs[i].scratchOffset = alignedOffset;
    scratchSize           = alignedOffset + jobs[i].build.storageSize;
  }
  if (i == jobCount) {
    scratchArena = malloc(scratchSize);
    if (scratchArena) {
      for (i = 0; i < jobCount; i++) {
        jobs[i].build.storage = (uint8_t *)scratchArena
                                + jobs[i].scratchOffset;
      }
    }
  }

  qsort(jobs,
        jobCount,
        sizeof(*jobs),
        dae_duplicator_job_compare_weight);

  workerCount = ak_thread_cpu_count();
  if (workerCount > DAE_DUPLICATOR_PARALLEL_MAX_THREADS)
    workerCount = DAE_DUPLICATOR_PARALLEL_MAX_THREADS;
  if (workerCount > jobCount)
    workerCount = (uint32_t)jobCount;
  if (workerCount < 1)
    workerCount = 1;

  memset(workerLoads, 0, sizeof(workerLoads));
  for (i = 0; i < jobCount; i++) {
    minWorker = 0;
    for (w = 1; w < workerCount; w++) {
      if (workerLoads[w] < workerLoads[minWorker])
        minWorker = (uint32_t)w;
    }
    jobs[i].workerIndex = minWorker;
    workerLoads[minWorker] = jobs[i].weight
                               <= SIZE_MAX - workerLoads[minWorker]
                             ? workerLoads[minWorker] + jobs[i].weight
                             : SIZE_MAX;
  }

  for (w = 0; w < workerCount; w++) {
    workers[w].jobs        = jobs;
    workers[w].jobCount    = jobCount;
    workers[w].workerIndex = (uint32_t)w;
    tasks[w].func          = dae_duplicator_worker;
    tasks[w].userdata      = &workers[w];
  }

  if (!ak_thread_run_tasks(tasks, workerCount)) {
    for (i = 0; i < jobCount; i++)
      ak_meshDuplicatorBuildCompute(&jobs[i].build);
  }

  qsort(jobs,
        jobCount,
        sizeof(*jobs),
        dae_duplicator_job_compare_sequence);

  jobIndex = 0;
  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    primitive = geom->gdata;
    if (!primitive)
      continue;
    if (primitive->type != AK_GEOMETRY_MESH) {
      dae_geom_fixup(geom, retainDuplicators);
      continue;
    }

    mesh         = ak_objGet(primitive);
    meshJobStart = jobIndex;
    while (jobIndex < jobCount && jobs[jobIndex].mesh == mesh)
      jobIndex++;

    if (jobIndex > meshJobStart) {
      dae_mesh_fixup_with_builds(mesh,
                                 retainDuplicators,
                                 &jobs[meshJobStart],
                                 jobIndex - meshJobStart);
    } else {
      dae_mesh_fixup(mesh, retainDuplicators);
    }
  }

  for (i = 0; i < jobCount; i++)
    ak_meshDuplicatorBuildRelease(&jobs[i].build);
  free(scratchArena);
  free(jobs);

  return AK_OK;

serial:
  for (geom = doc->lib.geometries.first; geom; geom = geom->next)
    dae_geom_fixup(geom, retainDuplicators);

  return AK_OK;
}

AK_HIDE
AkResult
dae_geom_fixup(AkGeometry * geom, bool retainDuplicators) {
  AkObject *primitive;

  primitive = geom->gdata;
  if (!primitive)
    return AK_OK;

  switch ((AkGeometryType)primitive->type) {
    case AK_GEOMETRY_MESH:
      dae_mesh_fixup(ak_objGet(primitive), retainDuplicators);
    default:
      break;
  }

  return AK_OK;
}
