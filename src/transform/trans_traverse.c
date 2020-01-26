/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
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

  ak_transformCombine(node, mat[0]);

  parentNode = node->parent;
  while (parentNode) {
    ak_transformCombine(parentNode, ptrans[0]);

    glm_mat4_mul(ptrans, mat, mat);
    parentNode = parentNode->parent;
  }

  glm_mat4_copy(mat, (vec4 *)matrix);
}
