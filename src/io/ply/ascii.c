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
#include "../../string_fast.h"

static
bool
ply_ascii_vertex_direct(PLYElement * __restrict elem) {
  PLYProperty *prop;
  uint32_t     slot;

  prop = elem->property;
  slot = 0;

  while (prop) {
    if (prop->ignore
        || !prop->typeDesc
        || prop->typeDesc->typeId != AKT_FLOAT
        || prop->slot != slot)
      return false;

    slot++;
    prop = prop->next;
  }

  return slot == elem->knownCount && slot > 0;
}

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

      if (ply_ascii_vertex_direct(elem)) {
        uint32_t j;

        do {
          SKIP_SPACES

          for (j = 0; j < stride; j++)
            p = ak_strtof_one_fast(p, &b[j]);

          b += stride;

          NEXT_LINE

          if (++i >= elem->count)
            break;
        } while (p && p[0] != '\0');
      } else {
        do {
          SKIP_SPACES

          prop = elem->property;
          while (prop) {
            AkFloat value;

            p = ak_strtof_one_fast(p, &value);
            if (!prop->ignore)
              b[prop->slot] = value;
            prop = prop->next;
          }

          b += stride;

          NEXT_LINE

          if (++i >= elem->count)
            break;
        } while (p && p[0] != '\0');
      }
    } else if (elem->type == PLY_ELEM_FACE) {
      AkUInt *f, fc, j, count, last_fc;
      
      pst->dc_ind = ply_index_data_new(pst);
      c           = *p;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;

      do {
        SKIP_SPACES
        
        p = ak_strtoui_one_fast(p, &fc);
        if (fc == 3) {
          AkUInt f0, f1, f2;

          p = ak_strtoui_one_fast(p, &f0);
          p = ak_strtoui_one_fast(p, &f1);
          p = ak_strtoui_one_fast(p, &f2);
          PLY_INDEX_APPEND_TRI(pst, f0, f1, f2, count);
        } else if (fc > 3) {
          if (!f || last_fc < fc)
            f = alloca(sizeof(AkUInt) * fc);
          
          for (j = 0; j < fc; j++)
            p = ak_strtoui_one_fast(p, &f[j]);
          
          PLY_INDEX_APPEND_FACE(pst, f, fc, count);
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
