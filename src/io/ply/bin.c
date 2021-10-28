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

AK_HIDE
void
ply_bin(char * __restrict src, PLYState * __restrict pst, bool le) {
  char        *p;
  float       *b;
  PLYElement  *elem;
  PLYProperty *prop;
  AkBuffer    *buff;
  uint32_t     i, stride, vertcount;
  
  p         = src;
  elem      = pst->element;
  vertcount = pst->vertcount;

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"

            ply_val(p, prop->typeDesc, le, float, b[prop->slot], 0.0f);
            
#pragma GCC diagnostic pop
          }
          prop = prop->next;
        }

        b += stride;
      }
    } else if (elem->type == PLY_ELEM_FACE) {
      char   *e;
      AkUInt *f, center, fc, j, count, last_fc, valid, elemc;

      pst->dc_ind = ak_data_new(pst->tmp, 128, sizeof(AkUInt), NULL);
      elemc       = elem->count;
      e           = pst->end;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;

      while (i++ < elemc) {
        prop = elem->property;
        
        /* iterate thorough list and other properties */
        while (prop) {
          if (!prop->ignore && prop->islist) { /* TODO: */
            if ((p + prop->typeDesc->size) > e)
              goto fns;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
            
            ply_val(p, prop->listCountTypeDesc, le, AkUInt, fc, 0);
            
#pragma GCC diagnostic pop

            if (fc >= 3) {
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
                center = f[0];
                for (j = 0; j < fc - 2; j++) {
                  ak_data_append(pst->dc_ind, &center);
                  ak_data_append(pst->dc_ind, &f[j + 1]);
                  ak_data_append(pst->dc_ind, &f[j + 2]);
                  count += 3;
                }
              }
            } else if (fc > 0) {
              for (j = 0; j < fc; j++)
                p += prop->typeDesc->size;
            }
            
            last_fc = fc;
          } else {
            /* do ignore */
            /* TODO: */
          }

          prop = prop->next;
        }
      }

      pst->count = count;
    } else {
      /* skip unsupported elements */
      /* TODO: */
    }
    elem = elem->next;
  }
  
fns:
  ply_finish(pst);
}
