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

#include "mesh_fixup.h"

/* create indices to fix topology,
   an alternative way could be work with each input,
   this can be provided by an option maybe in the future. */
AK_HIDE
void
topofix_noind(AkHeap          * __restrict heap,
              AkMeshPrimitive * __restrict prim,
              uint8_t                      trig_fan,
              uint8_t                      trig_strip,
              uint8_t                      line_loop,
              uint8_t                      line_strip) {
  /* TODO: no indices, handle inputs... */
  AkInput     *input;
  AkAccessor  *acc;
  AkBuffer    *buff;
  AkUIntArray *indices;
  AkUInt      *it, nVertices, i, j;

  if (!(input = prim->pos)
      || !(acc  = input->accessor)
      || !(buff = acc->buffer)) {
    return;
  }

  nVertices = acc->count;

  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES: {
      if (trig_fan || trig_strip) {
        AkTriangles *trig;
        AkUInt       ntrigs;

        trig = (AkTriangles *)prim;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
          case AK_TRIANGLE_STRIP: break;
          default:                return;
        }

        ntrigs         = nVertices - 2;
        indices        = ak_heap_calloc(heap, prim, sizeof(*indices) + sizeof(AkUInt) * ntrigs * 3);
        indices->count = ntrigs * 3;
        it             = indices->items;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
            if (trig_fan) {
              for (i = 0, j = 0; i < ntrigs; ++i) {
                it[j++] = 0; /* center vertex */
                it[j++] = i + 1;
                it[j++] = i + 2;
              }
            }
            break;
          case AK_TRIANGLE_STRIP:
            if (trig_strip) {
              for (i = 0, j = 0; i < ntrigs; ++i) {
                if (i % 2 == 0) {
                  it[j++] = i;
                  it[j++] = i + 1;
                  it[j++] = i + 2;
                } else {
                  it[j++] = i + 2;
                  it[j++] = i + 1;
                  it[j++] = i;
                }
              }
            }
            break;
          default: break;
        }

        trig->mode        = AK_TRIANGLES;
        prim->indices     = indices;
        prim->indexStride = 1;
      }
      break;
    }
    case AK_PRIMITIVE_LINES:
      if (line_loop || line_strip) {
        AkLines *lines;
        AkUInt   nlines;

        lines = (AkLines *)prim;

        switch (lines->mode) {
          case AK_LINE_LOOP:
            if (line_loop) {
              nlines         = nVertices;
              indices        = ak_heap_calloc(heap, prim, sizeof(*indices) + sizeof(AkUInt) * nlines * 2);
              indices->count = nlines * 2;
              it             = indices->items;

              for (i = 0; i < nVertices - 1; ++i) {
                it[i * 2]     = i;
                it[i * 2 + 1] = i + 1;
              }

              /* close the loop */
              it[(nVertices - 1) * 2] = nVertices - 1;
              it[(nVertices - 1) * 2 + 1] = 0;

              lines->mode       = AK_LINES;
              prim->indices     = indices;
              prim->indexStride = 1;
            }
            break;
          case AK_LINE_STRIP:
            if (line_strip) {
              nlines         = nVertices - 1;
              indices        = ak_heap_calloc(heap, prim, sizeof(*indices) + sizeof(AkUInt) * nlines * 2);
              indices->count = nlines * 2;
              it             = indices->items;

              for (i = 0; i < nlines; ++i) {
                it[i * 2]     = i;
                it[i * 2 + 1] = i + 1;
              }

              lines->mode       = AK_LINES;
              prim->indices     = indices;
              prim->indexStride = 1;
            }
            break;
          default: break;
        }
      }
      break;
    default: break;
  }
}

AK_HIDE
void
topofix_ind(AkHeap          * __restrict heap,
            AkMeshPrimitive * __restrict prim,
            uint8_t                      trig_fan,
            uint8_t                      trig_strip,
            uint8_t                      line_loop,
            uint8_t                      line_strip) {
  AkUInt      *it, *newIt, nIndices, i, j;
  AkUIntArray *newIndices;

  it       = prim->indices->items;
  nIndices = (AkUInt)prim->indices->count;

  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES: {
      if (trig_fan || trig_strip) {
        AkTriangles *trig;
        AkUInt       nTriangles;

        trig = (AkTriangles *)prim;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
          case AK_TRIANGLE_STRIP: break;
          default:                return;
        }

        nTriangles        = nIndices - 2;
        newIndices        = ak_heap_calloc(heap,
                                           prim,
                                           sizeof(*newIndices)
                                           + sizeof(AkUInt) * nTriangles * 3);
        newIndices->count = nTriangles * 3;
        newIt             = newIndices->items;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
            if (trig_fan) {
              for (i = 0, j = 0; i < nTriangles; ++i) {
                newIt[j++] = it[0];
                newIt[j++] = it[i + 1];
                newIt[j++] = it[i + 2];
              }

              trig->mode = AK_TRIANGLES;
            }
            break;
          case AK_TRIANGLE_STRIP:
            if (trig_strip) {
              for (i = 0, j = 0; i < nTriangles; ++i) {
                newIt[j++] = it[i];

                if (i % 2 == 0) {
                  newIt[j++] = it[i + 1];
                  newIt[j++] = it[i + 2];
                } else {
                  newIt[j++] = it[i + 2];
                  newIt[j++] = it[i + 1];
                }
              }

              trig->mode = AK_TRIANGLES;
            }
            break;
          default: break;
        }

        ak_free(prim->indices);
        prim->indices = newIndices;
      }
      break;
    }
    case AK_PRIMITIVE_LINES:
      if (line_loop || line_strip) {
        AkLines *lines;
        AkUInt   nLines;

        lines = (AkLines *)prim;

        switch (lines->mode) {
          case AK_LINE_LOOP:
            if (line_loop) {
              nLines            = nIndices;
              newIndices        = ak_heap_calloc(heap,
                                                 prim,
                                                 sizeof(*newIndices)
                                                 + sizeof(AkUInt) * nLines * 2);
              newIndices->count = nLines * 2;
              newIt             = newIndices->items;

              for (i = 0; i < nIndices - 1; ++i) {
                newIt[i * 2]     = it[i];
                newIt[i * 2 + 1] = it[i + 1];
              }

              /* close the loop */
              newIt[(nIndices - 1) * 2]     = it[nIndices - 1];
              newIt[(nIndices - 1) * 2 + 1] = it[0];

              ak_free(prim->indices);
              prim->indices = newIndices;

              lines->mode = AK_LINES;
            }
            break;
          case AK_LINE_STRIP:
            if (line_strip) {
              nLines            = nIndices - 1;
              newIndices        = ak_heap_calloc(heap,
                                                 prim,
                                                 sizeof(*newIndices)
                                                 + sizeof(AkUInt) * nLines * 2);
              newIndices->count = nLines * 2;
              newIt             = newIndices->items;

              for (i = 0; i < nLines; ++i) {
                newIt[i * 2]     = it[i];
                newIt[i * 2 + 1] = it[i + 1];
              }

              ak_free(prim->indices);
              prim->indices = newIndices;

              lines->mode = AK_LINES;
            }
            break;
          default: break;
        }
      }
      break;
    default: break;
  }
}

AK_HIDE
void
topofix(AkMesh * mesh) {
  AkHeap          *heap;
  AkMeshPrimitive *prim;
  uint8_t          trig_fan, trig_strip, line_loop, line_strip;

  trig_fan   = (int)ak_opt_get(AK_OPT_CVT_TRIANGLEFAN);
  trig_strip = (int)ak_opt_get(AK_OPT_CVT_TRIANGLEFAN);
  line_loop  = (int)ak_opt_get(AK_OPT_CVT_LINELOOP);
  line_strip = (int)ak_opt_get(AK_OPT_CVT_LINESTRIP);

  if (!trig_fan && !trig_strip && !line_loop && !line_strip)
    return;

  heap       = ak_heap_getheap(mesh->geom);
  prim       = mesh->primitive;

  while (prim) {
    if (prim->indices) {
      topofix_ind(heap, prim, trig_fan, trig_strip, line_loop, line_strip);
    } else {
      topofix_noind(heap, prim, trig_fan, trig_strip, line_loop, line_strip);
    }

    prim = prim->next;
  }
}
