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

#include "ply.h"
#include "../common/util.h"

static
PLYProperty*
ply_splat_property(PLYElement *elem, const char *name) {
  PLYProperty *prop, *found;

  found = NULL;

  for (prop = elem->property; prop; prop = prop->next) {
    if (strcmp(prop->name, name))
      continue;

    if (found || prop->islist || !prop->typeDesc)
      return NULL;

    found = prop;
  }

  return found;
}

AK_HIDE
bool
ply_splat_prepare(PLYState *pst) {
  static const char *names[] = {
    "x", "y", "z", "rot_1", "rot_2", "rot_3", "rot_0",
    "scale_0", "scale_1", "scale_2", "opacity", "f_dc_0", "f_dc_1", "f_dc_2"
  };
  PLYProperty *props[86], *prop;
  PLYElement  *elem, *vertex;
  AkAccessor  *acc;
  char        *end;
  long         index;
  uint32_t     i, count, degree, shCount, slot, components;
  char         name[32];

  vertex = NULL;

  for (elem = pst->element; elem; elem = elem->next) {
    if (elem->count && (elem->type == PLY_ELEM_FACE || elem->type == PLY_ELEM_EDGE
                       || elem->type == PLY_ELEM_TRISTRIPS))
      return true;

    if (elem->type == PLY_ELEM_VERTEX) {
      if (vertex)
        return true;

      vertex = elem;
    }
  }

  if (!vertex || !vertex->count)
    return true;

  for (i = 0; i < 14; i++)
    if (!(props[i] = ply_splat_property(vertex, names[i])))
      return true; /* ordinary PLY, not a Gaussian field */

  count = 0;

  for (prop = vertex->property; prop; prop = prop->next) {
    if (strncmp(prop->name, "f_rest_", 7))
      continue;

    index = strtol(prop->name + 7, &end, 10);
    if (*end || end == prop->name + 7 || index < 0 || index >= 72)
      return false;

    count++;
  }

  for (degree = 0; degree <= 4; degree++)
    if (count == degree * (degree + 2) * 3)
      break;

  if (degree > 4)
    return false;

  shCount = degree * (degree + 2);

  for (i = 0; i < count; i++) {
    /* PLY stores all red coefficients, then green, then blue. */
    snprintf(name, sizeof(name), "f_rest_%u", i);
    slot = 14 + (i % shCount) * 3 + i / shCount;
    if (!(props[slot] = ply_splat_property(vertex, name)))
      return false;
  }

  pst->splatElement  = vertex;
  pst->shDegree      = degree;
  vertex->knownCount = 14 + count;
  pst->byteStride    = vertex->knownCount * sizeof(float);

  if (vertex->count > SIZE_MAX / pst->byteStride)
    return false;

  pst->vertBuffsize = (size_t)vertex->count * pst->byteStride;
  if (!(vertex->buff = ak_heap_calloc(pst->heap, pst->doc, sizeof(*vertex->buff))))
    return false;

  vertex->buff->length = pst->vertBuffsize;
  if (!(vertex->buff->data = ak_heap_alloc(pst->heap, vertex->buff, pst->vertBuffsize)))
    return false;

  AK_LIB_PREPEND(pst->doc->lib.buffers, vertex->buff, next);

  for (prop = vertex->property; prop; prop = prop->next)
    prop->ignore = true;

  for (i = 0; i < vertex->knownCount; i++) {
    props[i]->ignore = false;
    props[i]->slot   = i;
    props[i]->off    = i * sizeof(float);
  }

  slot = 0;

  for (i = 0; i < 5 + shCount; i++) {
    components = i == 1 ? 4 : (i == 3 ? 1 : 3);
    if (!(acc = io_acc(pst->heap, pst->doc, (AkComponentSize)components,
                       AKT_FLOAT, vertex->count, vertex->buff)))
      return false;

    acc->byteOffset = slot * sizeof(float);
    acc->byteStride = pst->byteStride;
    acc->byteLength = (size_t)(vertex->count - 1) * pst->byteStride + components * sizeof(float);

    if (i == 0)
      pst->ac_pos = acc;
    else
      pst->ac_splat[i - 1] = acc;

    slot += components;
  }

  return true;
}

AK_HIDE
bool
ply_splat_finish(PLYState *pst, AkMeshPrimitive *prim) {
  static const AkInputSemantic semantics[] = {AK_INPUT_ROTATION, AK_INPUT_SCALE, AK_INPUT_OPACITY};
  static const char *names[] = {"ROTATION", "SCALE", "OPACITY"};
  AkGaussianSplat *gs;
  AkInput        *input;
  float          *row;
  double          norm;
  uint32_t        i, c, degree, coefficient, stride, shCount;

  if (pst->vertexRows != pst->vertcount)
    return false;

  stride  = pst->splatElement->knownCount;
  shCount = (pst->shDegree + 1) * (pst->shDegree + 1);
  row     = pst->splatElement->buff->data;

  for (i = 0; i < pst->vertcount; i++, row += stride) {
    for (c = 0; c < stride; c++)
      if (!isfinite(row[c]))
        return false;

    /* Graphdeco PLY uses RDF. Rotate 180 degrees about Z to glTF LUF;
       rotate SH by the same basis change, not just the point centers. */
    row[0] = -row[0];
    row[1] = -row[1];
    row[3] = -row[3];
    row[4] = -row[4];
    norm   = 0.0;

    for (c = 3; c < 7; c++)
      norm += (double)row[c] * row[c];

    if (norm <= 0.0)
      return false;

    norm = 1.0 / sqrt(norm);

    for (c = 3; c < 7; c++)
      row[c] = (float)(row[c] * norm);

    for (c = 7; c < 10; c++) {
      row[c] = expf(row[c]);
      if (!isfinite(row[c]))
        return false;
    }

    row[10] = 1.0f / (1.0f + expf(-row[10]));

    for (degree = 1; degree <= pst->shDegree; degree++) {
      for (coefficient = 0; coefficient <= 2 * degree; coefficient++) {
        if ((coefficient + degree) & 1u)
          for (c = 0; c < 3; c++)
            row[11 + (degree * degree + coefficient) * 3 + c] *= -1.0f;
      }
    }
  }

  if (!(gs = ak_heap_calloc(pst->heap, prim, sizeof(*gs))))
    return false;

  gs->kernel       = AK_GSPLAT_KERNEL_ELLIPSE;
  gs->colorSpace   = AK_GSPLAT_COLOR_SRGB_REC709_DISPLAY;
  gs->decodedCount = pst->vertcount;
  gs->shDegree     = (uint8_t)pst->shDegree;
  prim->gsplat     = gs;
  prim->pos        = io_input(pst->heap, prim, pst->ac_pos, AK_INPUT_POSITION, "POSITION", 0);

  for (i = 0; i < 3; i++)
    io_input(pst->heap, prim, pst->ac_splat[i], semantics[i], names[i], 0);

  for (i = 0; i < shCount; i++) {
    input      = io_input(pst->heap, prim, pst->ac_splat[3 + i], AK_INPUT_SH, "SH", 0);
    input->set = i;
  }

  return true;
}
