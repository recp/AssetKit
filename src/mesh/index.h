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

#ifndef ak_mesh_index_h
#define ak_mesh_index_h

#include "../common.h"

typedef struct AkDuplicatorBuild {
  AkMeshPrimitive    *primitive;
  const AkIndexArray *indices;
  const AkAccessor   *positionAccessor;
  uint32_t           *ordinals;
  uint32_t           *positionCopies;
  uint32_t           *lookup;
  void               *storage;

  size_t             indexCount;
  size_t             vertexCount;
  size_t             lookupCount;
  size_t             storageSize;
  size_t             duplicateCount;
  size_t             bufferCount;
  AkUInt             duplicateMax;
  uint32_t           indexStride;
  uint32_t           positionOffset;
  bool               computed;
  bool               valid;
  bool               ownsStorage;
} AkDuplicatorBuild;

AK_HIDE
AkResult
ak_meshFixIndices(AkMesh *mesh);

AK_HIDE
AkResult
ak_primFixIndicesRetainDuplicator(AkMesh          *mesh,
                                  AkMeshPrimitive *prim,
                                  bool             retainDuplicator);

AK_HIDE
AkResult
ak_primFixIndicesWithBuild(AkMesh            *mesh,
                           AkMeshPrimitive   *prim,
                           bool               retainDuplicator,
                           AkDuplicatorBuild *build);

AK_HIDE
bool
ak_primCollapseIdentityIndices(AkMeshPrimitive *prim);

AK_HIDE
AkResult
ak_meshFixIndicesDefaultRetainDuplicators(AkMesh *mesh,
                                          bool    retainDuplicators);

AK_HIDE
AkDuplicator*
ak_meshDuplicatorForIndicesRetained(AkMesh          * __restrict mesh,
                                    AkMeshPrimitive * __restrict prim,
                                    bool                         retain);

AK_HIDE
bool
ak_meshDuplicatorBuildPrepare(AkMeshPrimitive   * __restrict prim,
                              AkDuplicatorBuild * __restrict build);

AK_HIDE
void
ak_meshDuplicatorBuildCompute(void *userdata);

AK_HIDE
AkDuplicator*
ak_meshDuplicatorBuildFinish(AkMesh            * __restrict mesh,
                             AkMeshPrimitive   * __restrict prim,
                             bool                           retain,
                             AkDuplicatorBuild * __restrict build);

AK_HIDE
void
ak_meshDuplicatorBuildRelease(AkDuplicatorBuild *build);

AK_HIDE
AkResult
ak_movePositions(AkMesh          *mesh,
                 AkMeshPrimitive *prim,
                 AkDuplicator    *duplicator);

#endif /* ak_mesh_index_h */
