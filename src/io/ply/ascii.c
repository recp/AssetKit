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

AK_HIDE
void
ply_ascii(char * __restrict src, PLYState * __restrict pst) {
  char        *p;
  float       *b;
  PLYElement  *elem;
  PLYProperty *prop;
  AkBuffer    *buff;
  char         c;
  uint32_t     i, stride;
  
  p    = src;
  elem = pst->element;

  while (elem) {
    if (elem->type == PLY_ELEM_VERTEX) {
      buff   = elem->buff;
      b      = buff->data; /* TODO: all vertices are floats for now */
      stride = elem->knownCount;
      i      = 0;
      c      = *p;

      /* stop */
      if (!elem->buff || elem->buff->length == 0)
        return;

      do {
        SKIP_SPACES

        prop = elem->property;
        while (prop) {
          if (!prop->ignore)
            b[prop->slot] = strtof(p, &p);
          prop = prop->next;
        }

        b += stride;

        NEXT_LINE

        if (++i >= elem->count)
          break;
      } while (p && p[0] != '\0');
    } else if (elem->type == PLY_ELEM_FACE) {
      AkUInt *f, center, fc, j, count, last_fc;
      
      pst->dc_ind = ak_data_new(pst->tmp, 128, sizeof(AkUInt), NULL);
      c           = *p;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;

      do {
        SKIP_SPACES
        
        fc = (AkUInt)strtol(p, &p, 10);
        if (fc >= 3) {
          if (!f || last_fc < fc)
            f = alloca(sizeof(AkUInt) * fc);
          
          for (j = 0; j < fc; j++)
            f[j] = (AkUInt)strtol(p, &p, 10);
          
          center = f[0];
          for (j = 0; j < fc - 2; j++) {
            ak_data_append(pst->dc_ind, &center);
            ak_data_append(pst->dc_ind, &f[j + 1]);
            ak_data_append(pst->dc_ind, &f[j + 2]);
            count += 3;
          }
        }

        last_fc = fc;

        NEXT_LINE

        if (++i >= elem->count)
          break;
      } while (p && p[0] != '\0');
      
      pst->count = count;
    } else {
      /* skip unsupported elements */
      for (i = 0; i < elem->count; i++) {
        NEXT_LINE
      }
    }
    elem = elem->next;
  }
  
  ply_finish(pst);
}
