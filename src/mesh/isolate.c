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

#include "../common.h"

AK_EXPORT
bool
ak_meshIsPrimIsolated(AkMeshPrimitive *prim) {
  AkInput    *inp;
  AkAccessor *acc;
  AkBuffer   *buff;

  if (!(inp = prim->input))
    return true;
  
  do {
    /* check accessor reference count */
    if ((acc = inp->accessor) && ak_refc(acc) > 1)
      return false;
    
    /* check buffer reference count */
    if ((buff = acc->buffer) && ak_refc(buff) > 1)
      return false;
  } while ((inp = inp->next));

  return true;
}

AK_EXPORT
bool
ak_meshIsIsolated(AkMesh *mesh) {
  AkMeshPrimitive  *prim;

  if (mesh && (prim = mesh->primitive)) {
    do {
      if (!ak_meshIsPrimIsolated(prim))
        return false;
    } while ((prim = prim->next));
  }
  
  return true;
}

AK_EXPORT
void
ak_meshIsolatePrim(AkMeshPrimitive *prim) {
  if (!prim)
    return;
  
  /* detach accessors */
  
  /* detach buffers */
}

AK_EXPORT
void
ak_meshIsolate(AkMesh *mesh) {
  AkMeshPrimitive  *prim;
  
  if (!mesh)
    return;

  if ((prim = mesh->primitive)) {
    do {
      ak_meshIsolatePrim(prim);
    } while ((prim = prim->next));
  }
}
