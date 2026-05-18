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

#include "topo.h"

#define AK_TOPO_WRITE_NOIND_TRIFAN(TYPE)                                     \
  do {                                                                        \
    TYPE *dst_;                                                               \
                                                                              \
    dst_ = (TYPE *)(void *)indices->items;                                    \
    for (i = 0, j = 0; i < ntrigs; ++i) {                                     \
      dst_[j++] = 0;                                                          \
      dst_[j++] = (TYPE)(i + 1);                                              \
      dst_[j++] = (TYPE)(i + 2);                                              \
    }                                                                         \
  } while (0)

#define AK_TOPO_WRITE_NOIND_TRISTRIP(TYPE)                                   \
  do {                                                                        \
    TYPE *dst_;                                                               \
                                                                              \
    dst_ = (TYPE *)(void *)indices->items;                                    \
    for (i = 0, j = 0; i < ntrigs; ++i) {                                     \
      if (i % 2 == 0) {                                                       \
        dst_[j++] = (TYPE)i;                                                  \
        dst_[j++] = (TYPE)(i + 1);                                            \
        dst_[j++] = (TYPE)(i + 2);                                            \
      } else {                                                                \
        dst_[j++] = (TYPE)(i + 2);                                            \
        dst_[j++] = (TYPE)(i + 1);                                            \
        dst_[j++] = (TYPE)i;                                                  \
      }                                                                       \
    }                                                                         \
  } while (0)

#define AK_TOPO_WRITE_NOIND_LINE_LOOP(TYPE)                                  \
  do {                                                                        \
    TYPE *dst_;                                                               \
                                                                              \
    dst_ = (TYPE *)(void *)indices->items;                                    \
    for (i = 0; i < nVertices - 1; ++i) {                                     \
      dst_[i * 2]     = (TYPE)i;                                              \
      dst_[i * 2 + 1] = (TYPE)(i + 1);                                        \
    }                                                                         \
                                                                              \
    dst_[(nVertices - 1) * 2]     = (TYPE)(nVertices - 1);                    \
    dst_[(nVertices - 1) * 2 + 1] = 0;                                        \
  } while (0)

#define AK_TOPO_WRITE_NOIND_LINE_STRIP(TYPE)                                 \
  do {                                                                        \
    TYPE *dst_;                                                               \
                                                                              \
    dst_ = (TYPE *)(void *)indices->items;                                    \
    for (i = 0; i < nlines; ++i) {                                            \
      dst_[i * 2]     = (TYPE)i;                                              \
      dst_[i * 2 + 1] = (TYPE)(i + 1);                                        \
    }                                                                         \
  } while (0)

#define AK_TOPO_WRITE_IND_TRIFAN(TYPE)                                       \
  do {                                                                        \
    const TYPE *src_;                                                         \
    TYPE       *dst_;                                                         \
                                                                              \
    src_ = (const TYPE *)(const void *)prim->indices->items;                  \
    dst_ = (TYPE *)(void *)newIndices->items;                                 \
    for (i = 0, j = 0; i < nTriangles; ++i) {                                 \
      dst_[j++] = src_[0];                                                    \
      dst_[j++] = src_[i + 1];                                                \
      dst_[j++] = src_[i + 2];                                                \
    }                                                                         \
  } while (0)

#define AK_TOPO_WRITE_IND_TRISTRIP(TYPE)                                     \
  do {                                                                        \
    const TYPE *src_;                                                         \
    TYPE       *dst_;                                                         \
                                                                              \
    src_ = (const TYPE *)(const void *)prim->indices->items;                  \
    dst_ = (TYPE *)(void *)newIndices->items;                                 \
    for (i = 0, j = 0; i < nTriangles; ++i) {                                 \
      dst_[j++] = src_[i];                                                    \
                                                                              \
      if (i % 2 == 0) {                                                       \
        dst_[j++] = src_[i + 1];                                              \
        dst_[j++] = src_[i + 2];                                              \
      } else {                                                                \
        dst_[j++] = src_[i + 2];                                              \
        dst_[j++] = src_[i + 1];                                              \
      }                                                                       \
    }                                                                         \
  } while (0)

#define AK_TOPO_WRITE_IND_LINE_LOOP(TYPE)                                    \
  do {                                                                        \
    const TYPE *src_;                                                         \
    TYPE       *dst_;                                                         \
                                                                              \
    src_ = (const TYPE *)(const void *)prim->indices->items;                  \
    dst_ = (TYPE *)(void *)newIndices->items;                                 \
    for (i = 0; i < nIndices - 1; ++i) {                                      \
      dst_[i * 2]     = src_[i];                                              \
      dst_[i * 2 + 1] = src_[i + 1];                                          \
    }                                                                         \
                                                                              \
    dst_[(nIndices - 1) * 2]     = src_[nIndices - 1];                        \
    dst_[(nIndices - 1) * 2 + 1] = src_[0];                                   \
  } while (0)

#define AK_TOPO_WRITE_IND_LINE_STRIP(TYPE)                                   \
  do {                                                                        \
    const TYPE *src_;                                                         \
    TYPE       *dst_;                                                         \
                                                                              \
    src_ = (const TYPE *)(const void *)prim->indices->items;                  \
    dst_ = (TYPE *)(void *)newIndices->items;                                 \
    for (i = 0; i < nLines; ++i) {                                            \
      dst_[i * 2]     = src_[i];                                              \
      dst_[i * 2 + 1] = src_[i + 1];                                          \
    }                                                                         \
  } while (0)

static
bool
topofix_prim_needs(AkMeshPrimitive * __restrict prim,
                   uint8_t                      trig_fan,
                   uint8_t                      trig_strip,
                   uint8_t                      line_loop,
                   uint8_t                      line_strip) {
  switch (prim->type) {
    case AK_PRIMITIVE_TRIANGLES: {
      AkTriangles *trig;

      if (!trig_fan && !trig_strip)
        return false;

      trig = (AkTriangles *)prim;
      return (trig_fan   && trig->mode == AK_TRIANGLE_FAN)
             || (trig_strip && trig->mode == AK_TRIANGLE_STRIP);
    }
    case AK_PRIMITIVE_LINES: {
      AkLines *lines;

      if (!line_loop && !line_strip)
        return false;

      lines = (AkLines *)prim;
      return (line_loop  && lines->mode == AK_LINE_LOOP)
             || (line_strip && lines->mode == AK_LINE_STRIP);
    }
    default:
      return false;
  }
}

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
  AkIndexArray *indices;
  AkTypeId      componentType;
  AkUInt        nVertices, i, j;

  if (!(input = prim->pos)
      || !(acc  = input->accessor)
      || !(buff = acc->buffer)) {
    return;
  }

  nVertices = acc->count;
  if (nVertices == 0)
    return;

  componentType = ak_indexComponentTypeForMax(nVertices - 1);

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

        if (nVertices < 3)
          return;

        ntrigs         = nVertices - 2;
        indices = ak_indexArrayAlloc(heap, prim, ntrigs * 3, componentType);
        if (!indices)
          return;
        indices->max = nVertices - 1;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
            if (trig_fan) {
              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_NOIND_TRIFAN(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_NOIND_TRIFAN(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_NOIND_TRIFAN(uint32_t); break;
                default:         ak_free(indices); return;
              }
            }
            break;
          case AK_TRIANGLE_STRIP:
            if (trig_strip) {
              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_NOIND_TRISTRIP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_NOIND_TRISTRIP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_NOIND_TRISTRIP(uint32_t); break;
                default:         ak_free(indices); return;
              }
            }
            break;
          default: break;
        }

        trig->mode        = AK_TRIANGLES;
        prim->indices     = indices;
        prim->indexStride = 1;
        prim->indexAccessor = NULL;
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
              if (nVertices < 2)
                return;

              nlines         = nVertices;
              indices = ak_indexArrayAlloc(heap,
                                           prim,
                                           nlines * 2,
                                           componentType);
              if (!indices)
                return;
              indices->max = nVertices - 1;

              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_NOIND_LINE_LOOP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_NOIND_LINE_LOOP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_NOIND_LINE_LOOP(uint32_t); break;
                default:         ak_free(indices); return;
              }

              lines->mode       = AK_LINES;
              prim->indices     = indices;
              prim->indexStride = 1;
              prim->indexAccessor = NULL;
            }
            break;
          case AK_LINE_STRIP:
            if (line_strip) {
              if (nVertices < 2)
                return;

              nlines         = nVertices - 1;
              indices = ak_indexArrayAlloc(heap,
                                           prim,
                                           nlines * 2,
                                           componentType);
              if (!indices)
                return;
              indices->max = nVertices - 1;

              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_NOIND_LINE_STRIP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_NOIND_LINE_STRIP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_NOIND_LINE_STRIP(uint32_t); break;
                default:         ak_free(indices); return;
              }

              lines->mode       = AK_LINES;
              prim->indices     = indices;
              prim->indexStride = 1;
              prim->indexAccessor = NULL;
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
  AkUInt        nIndices, i, j;
  AkTypeId      componentType;
  AkIndexArray *newIndices;

  nIndices      = (AkUInt)prim->indices->count;
  componentType = prim->indices->componentType;

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

        if (nIndices < 3)
          return;

        nTriangles        = nIndices - 2;
        newIndices = ak_indexArrayAlloc(heap,
                                        prim,
                                        nTriangles * 3,
                                        componentType);
        if (!newIndices)
          return;
        newIndices->max = prim->indices->max;

        switch (trig->mode) {
          case AK_TRIANGLE_FAN:
            if (trig_fan) {
              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_IND_TRIFAN(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_IND_TRIFAN(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_IND_TRIFAN(uint32_t); break;
                default:         ak_free(newIndices); return;
              }

              trig->mode = AK_TRIANGLES;
            }
            break;
          case AK_TRIANGLE_STRIP:
            if (trig_strip) {
              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_IND_TRISTRIP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_IND_TRISTRIP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_IND_TRISTRIP(uint32_t); break;
                default:         ak_free(newIndices); return;
              }

              trig->mode = AK_TRIANGLES;
            }
            break;
          default: break;
        }

        ak_free(prim->indices);
        prim->indices = newIndices;
        prim->indexAccessor = NULL;
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
              if (nIndices < 2)
                return;

              nLines            = nIndices;
              newIndices = ak_indexArrayAlloc(heap,
                                              prim,
                                              nLines * 2,
                                              componentType);
              if (!newIndices)
                return;
              newIndices->max = prim->indices->max;

              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_IND_LINE_LOOP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_IND_LINE_LOOP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_IND_LINE_LOOP(uint32_t); break;
                default:         ak_free(newIndices); return;
              }

              ak_free(prim->indices);
              prim->indices = newIndices;
              prim->indexAccessor = NULL;

              lines->mode = AK_LINES;
            }
            break;
          case AK_LINE_STRIP:
            if (line_strip) {
              if (nIndices < 2)
                return;

              nLines            = nIndices - 1;
              newIndices = ak_indexArrayAlloc(heap,
                                              prim,
                                              nLines * 2,
                                              componentType);
              if (!newIndices)
                return;
              newIndices->max = prim->indices->max;

              switch (componentType) {
                case AKT_UBYTE:  AK_TOPO_WRITE_IND_LINE_STRIP(uint8_t);  break;
                case AKT_USHORT: AK_TOPO_WRITE_IND_LINE_STRIP(uint16_t); break;
                case AKT_UINT:   AK_TOPO_WRITE_IND_LINE_STRIP(uint32_t); break;
                default:         ak_free(newIndices); return;
              }

              ak_free(prim->indices);
              prim->indices = newIndices;
              prim->indexAccessor = NULL;

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
  trig_strip = (int)ak_opt_get(AK_OPT_CVT_TRIANGLESTRIP);
  line_loop  = (int)ak_opt_get(AK_OPT_CVT_LINELOOP);
  line_strip = (int)ak_opt_get(AK_OPT_CVT_LINESTRIP);

  if (!trig_fan && !trig_strip && !line_loop && !line_strip)
    return;

  heap       = ak_heap_getheap(mesh->geom);
  prim       = mesh->primitive;

  while (prim) {
    if (!topofix_prim_needs(prim, trig_fan, trig_strip, line_loop, line_strip))
      goto next;

    if (prim->indices || prim->indexAccessor) {
      ak_meshPrimitiveMaterializeIndices(prim);

      if (prim->indices)
        topofix_ind(heap, prim, trig_fan, trig_strip, line_loop, line_strip);
    } else {
      topofix_noind(heap, prim, trig_fan, trig_strip, line_loop, line_strip);
    }

  next:
    prim = prim->next;
  }
}
