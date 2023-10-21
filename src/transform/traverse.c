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
#include <cglm/cglm.h>

AK_EXPORT
void
ak_transformCombineWorld(AkNode * __restrict node,
                         float  * matrix) {
  AkNode *parentNode;
  mat4    mat;
  mat4    ptrans;

  ak_transformCombine(node->transform, mat[0]);

  parentNode = node->parent;
  while (parentNode) {
    ak_transformCombine(parentNode->transform, ptrans[0]);

    glm_mat4_mul(ptrans, mat, mat);
    parentNode = parentNode->parent;
  }

  glm_mat4_copy(mat, (vec4 *)matrix);
}
