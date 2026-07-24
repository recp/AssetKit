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

#ifndef dae_mesh_fixup_h
#define dae_mesh_fixup_h

#include "../common.h"
#include "../../../mesh/index.h"

typedef struct DaeDuplicatorJob {
  AkMesh            *mesh;
  AkDuplicatorBuild  build;
  size_t             scratchOffset;
  size_t             sequence;
  size_t             weight;
  uint32_t           workerIndex;
} DaeDuplicatorJob;

AK_HIDE
AkResult
dae_mesh_fixup(AkMesh * mesh, bool retainDuplicators);

AK_HIDE
AkResult
dae_mesh_fixup_with_builds(AkMesh           *mesh,
                           bool              retainDuplicators,
                           DaeDuplicatorJob *jobs,
                           size_t            jobCount);

#endif /* dae_mesh_fixup_h */
