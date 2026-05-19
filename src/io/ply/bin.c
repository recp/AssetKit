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
#include "common.h"
#include "util.h"
#include "../common/util.h"
#include "../../data.h"
#include "../../endian.h"

static
bool
ply_bin_skip_property(char        ** __restrict src,
                      char         * __restrict end,
                      PLYProperty  * __restrict prop,
                      bool                       le) {
  char   *p;
  AkUInt  count;
  size_t  itemSize;

  p = *src;

  if (!prop->islist) {
    if (!prop->typeDesc || p + prop->typeDesc->size > end)
      return false;

    *src = p + prop->typeDesc->size;
    return true;
  }

  if (!prop->listCountTypeDesc || !prop->typeDesc)
    return false;

  if (p + prop->listCountTypeDesc->size > end)
    return false;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

  ply_val(p, prop->listCountTypeDesc, le, AkUInt, count, 0);

#pragma GCC diagnostic pop

  itemSize = prop->typeDesc->size;
  if ((uint64_t)count * itemSize > (uint64_t)(end - p))
    return false;

  *src = p + (size_t)count * itemSize;

  return true;
}

AK_HIDE
void
ply_bin(char * __restrict src, PLYState * __restrict pst, bool le) {
  char        *p;
  float       *b;
  PLYElement  *elem;
  PLYProperty *prop;
  AkBuffer    *buff;
  char        *e;
  uint32_t     i, stride, vertcount;
  
  p         = src;
  elem      = pst->element;
  vertcount = pst->vertcount;
  e         = pst->end;

  while (elem) {
    if (elem->type == PLY_ELEM_VERTEX) {
      AkUInt elemc;
      
      elemc  = elem->count;
      buff   = elem->buff;
      b      = buff->data; /* TODO: all vertices are floats for now */
      stride = elem->knownCount;
      i      = 0;

      /* stop */
      if (!elem->buff || elem->buff->length == 0)
        return;

      while (i++ < elemc) {
        prop = elem->property;
        while (prop) {
          if (!prop->ignore) {
            if (!prop->typeDesc || p + prop->typeDesc->size > e)
              goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

            ply_val(p, prop->typeDesc, le, float, b[prop->slot], 0.0f);
            
#pragma GCC diagnostic pop
          } else if (!ply_bin_skip_property(&p, e, prop, le)) {
            goto fns;
          }
          prop = prop->next;
        }

        b += stride;
      }
    } else if (elem->type == PLY_ELEM_FACE) {
      AkUInt *f, fc, j, count, last_fc, valid, elemc;

      pst->dc_ind = ply_index_data_new(pst);
      elemc       = elem->count;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;

      while (i++ < elemc) {
        prop = elem->property;
        
        /* iterate thorough list and other properties */
        while (prop) {
          if (!prop->ignore && prop->islist) { /* TODO: */
            if (!prop->listCountTypeDesc || !prop->typeDesc)
              goto fns;

            if ((p + prop->listCountTypeDesc->size) > e)
              goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
            
            ply_val(p, prop->listCountTypeDesc, le, AkUInt, fc, 0);
            
#pragma GCC diagnostic pop

            if (fc == 3) {
              AkUInt f0, f1, f2;

              if ((p + prop->typeDesc->size * 3) > e)
                goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

              ply_val(p, prop->typeDesc, le, uint32_t, f0, 0);
              ply_val(p, prop->typeDesc, le, uint32_t, f1, 0);
              ply_val(p, prop->typeDesc, le, uint32_t, f2, 0);

#pragma GCC diagnostic pop

              if (f0 < vertcount && f1 < vertcount && f2 < vertcount)
                PLY_INDEX_APPEND_TRI(pst, f0, f1, f2, count);
            } else if (fc > 3) {
              if (!f || fc > last_fc)
                f = alloca(sizeof(*f) * fc);

              valid = 0;

              /* copy data */
              for (j = 0; j < fc; j++) {
                if ((p + prop->typeDesc->size) > e)
                  goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
                
                ply_val(p, prop->typeDesc, le, uint32_t, f[j], 0);
                
#pragma GCC diagnostic pop

                valid += f[j] < vertcount;
              }
              
              /* check valid loop */
              if (valid == fc) {
                PLY_INDEX_APPEND_FACE(pst, f, fc, count);
              }
            } else if (fc > 0) {
              for (j = 0; j < fc; j++)
                p += prop->typeDesc->size;
            }
            
            last_fc = fc;
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }
      }

      pst->count = count;
    } else if (elem->type == PLY_ELEM_TRISTRIPS) {
      AkUInt elemc, fc, j, count, vertcount;

      pst->dc_ind = ply_index_data_new(pst);
      elemc       = elem->count;
      i           = 0;
      count       = 0;
      vertcount   = pst->vertcount;

      while (i++ < elemc) {
        prop = elem->property;

        while (prop) {
          if (prop->semantic == PLY_PROP_VERTEX_INDICES && prop->islist) {
            AkUInt prev0, prev1, stripLen;
            size_t itemSize;

            if (!prop->listCountTypeDesc || !prop->typeDesc)
              goto fns;

            if ((p + prop->listCountTypeDesc->size) > e)
              goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

            ply_val(p, prop->listCountTypeDesc, le, AkUInt, fc, 0);

#pragma GCC diagnostic pop

            itemSize = prop->typeDesc->size;
            if ((uint64_t)fc * itemSize > (uint64_t)(e - p))
              goto fns;

            prev0 = prev1 = 0;
            stripLen = 0;
            for (j = 0; j < fc; j++) {
              AkInt value;
              AkUInt index;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

              ply_val(p, prop->typeDesc, le, AkInt, value, -1);

#pragma GCC diagnostic pop

              if (value < 0 || (AkUInt)value >= vertcount) {
                stripLen = 0;
                continue;
              }

              index = (AkUInt)value;
              if (stripLen == 0) {
                prev0 = index;
                stripLen = 1;
              } else if (stripLen == 1) {
                prev1 = index;
                stripLen = 2;
              } else {
                PLY_INDEX_APPEND_STRIP_TRI(pst,
                                           prev0,
                                           prev1,
                                           index,
                                           stripLen,
                                           count);
                prev0 = prev1;
                prev1 = index;
                stripLen++;
              }
            }
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }
      }

      pst->count = count;
    } else if (elem->type == PLY_ELEM_EDGE) {
      AkUInt elemc, vertcount;

      elemc     = elem->count;
      i         = 0;
      vertcount = pst->vertcount;

      while (i++ < elemc) {
        AkUInt v0, v1;
        bool   hasV0, hasV1;

        v0 = v1 = 0;
        hasV0 = hasV1 = false;
        prop = elem->property;

        while (prop) {
          if (!prop->islist
              && (prop->semantic == PLY_PROP_VERTEX1
                  || prop->semantic == PLY_PROP_VERTEX2)) {
            AkUInt value;

            if (!prop->typeDesc || p + prop->typeDesc->size > e)
              goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

            ply_val(p, prop->typeDesc, le, AkUInt, value, UINT32_MAX);

#pragma GCC diagnostic pop

            if (prop->semantic == PLY_PROP_VERTEX1) {
              v0 = value;
              hasV0 = true;
            } else {
              v1 = value;
              hasV1 = true;
            }
          } else {
            if (!ply_bin_skip_property(&p, e, prop, le))
              goto fns;
          }

          prop = prop->next;
        }

        if (hasV0 && hasV1 && v0 < vertcount && v1 < vertcount) {
          ply_edge_append(pst, v0, v1);
        }
      }
    } else {
      /* skip unsupported elements */
      AkUInt elemc;

      elemc = elem->count;
      i     = 0;
      while (i++ < elemc) {
        prop = elem->property;
        while (prop) {
          if (!ply_bin_skip_property(&p, e, prop, le))
            goto fns;
          prop = prop->next;
        }
      }
    }
    elem = elem->next;
  }
  
fns:
  ply_finish(pst);
}
