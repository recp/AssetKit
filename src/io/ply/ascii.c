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

AK_INLINE
char*
ply_ascii_parse_index(char   * __restrict p,
                      AkUInt * __restrict dest) {
  AkUInt value;

  while (*p && ak_str_sep_fast(*p))
    p++;

  if (!ak_str_isdigit_fast(*p))
    return ak_str_parse_uint_fast(p, NULL, dest);

  value = 0;
  do {
    value = value * 10u + (AkUInt)(*p++ - '0');
  } while (ak_str_isdigit_fast(*p));

  *dest = value;

  return p;
}

static
char*
ply_ascii_skip_property(char        * __restrict p,
                        PLYProperty * __restrict prop) {
  AkFloat value;
  AkUInt  count, i;

  if (!prop->islist)
    return ak_strtof_one_fast(p, &value);

  p = ply_ascii_parse_index(p, &count);
  for (i = 0; i < count; i++)
    p = ak_strtof_one_fast(p, &value);

  return p;
}

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

            if (prop->islist) {
              p = ply_ascii_skip_property(p, prop);
            } else {
              p = ak_strtof_one_fast(p, &value);
            }

            if (!prop->ignore && !prop->islist)
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
      AkUInt *f, fc, j, count, last_fc, valid, vertcount;
      
      pst->dc_ind = ply_index_data_new_estimated(pst, (size_t)elem->count * 3u);
      c           = *p;
      f           = NULL;
      i           = 0;
      count       = 0;
      last_fc     = 0;
      vertcount   = pst->vertcount;

      while (i++ < elem->count) {
        SKIP_SPACES

        prop = elem->property;
        while (prop) {
          if (prop->semantic == PLY_PROP_VERTEX_INDICES && prop->islist) {
            p = ply_ascii_parse_index(p, &fc);
            if (fc == 3) {
              AkUInt f0, f1, f2;

              p = ply_ascii_parse_index(p, &f0);
              p = ply_ascii_parse_index(p, &f1);
              p = ply_ascii_parse_index(p, &f2);
              if (f0 < vertcount && f1 < vertcount && f2 < vertcount)
                PLY_INDEX_APPEND_TRI(pst, f0, f1, f2, count);
            } else if (fc > 3) {
              if (!f || last_fc < fc)
                f = alloca(sizeof(AkUInt) * fc);

              valid = 0;
              for (j = 0; j < fc; j++)
                p = ply_ascii_parse_index(p, &f[j]);
              for (j = 0; j < fc; j++)
                valid += f[j] < vertcount;

              if (valid == fc)
                PLY_INDEX_APPEND_FACE(pst, f, fc, count);
            } else {
              AkUInt unused;

              for (j = 0; j < fc; j++)
                p = ply_ascii_parse_index(p, &unused);
            }

            last_fc = fc;
          } else {
            p = ply_ascii_skip_property(p, prop);
          }

          prop = prop->next;
        }

        NEXT_LINE
      }
      
      pst->count = count;
    } else if (elem->type == PLY_ELEM_TRISTRIPS) {
      AkUInt fc, j, count, vertcount;

      c           = *p;
      i           = 0;
      count       = 0;
      vertcount   = pst->vertcount;

      while (i++ < elem->count) {
        SKIP_SPACES

        prop = elem->property;
        while (prop) {
          if (prop->semantic == PLY_PROP_VERTEX_INDICES && prop->islist) {
            PLYTriSeen seen;
            AkUInt prev0, prev1, stripLen;

            p = ply_ascii_parse_index(p, &fc);
            if (!pst->dc_ind)
              pst->dc_ind = ply_index_data_new_estimated(
                pst,
                fc > 2 ? ((size_t)fc - 2u) * 3u : 0);
            ply_tri_seen_init(&seen, pst, fc);
            prev0 = prev1 = 0;
            stripLen = 0;
            for (j = 0; j < fc; j++) {
              AkInt value;
              AkUInt index;

              p = ak_strtoi_one_fast(p, &value);
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
                PLY_INDEX_APPEND_STRIP_TRI_SEEN(pst,
                                                prev0,
                                                prev1,
                                                index,
                                                stripLen,
                                                count,
                                                &seen);
                prev0 = prev1;
                prev1 = index;
                stripLen++;
              }
            }
          } else {
            p = ply_ascii_skip_property(p, prop);
          }

          prop = prop->next;
        }

        NEXT_LINE
      }

      pst->count = count;
    } else if (elem->type == PLY_ELEM_EDGE) {
      AkUInt vertcount;

      c         = *p;
      i         = 0;
      vertcount = pst->vertcount;

      while (i++ < elem->count) {
        AkUInt v0, v1;
        bool   hasV0, hasV1;

        SKIP_SPACES

        v0 = v1 = 0;
        hasV0 = hasV1 = false;
        prop = elem->property;
        while (prop) {
          if (!prop->islist
              && prop->semantic == PLY_PROP_VERTEX1) {
            p = ply_ascii_parse_index(p, &v0);
            hasV0 = true;
          } else if (!prop->islist
                     && prop->semantic == PLY_PROP_VERTEX2) {
            p = ply_ascii_parse_index(p, &v1);
            hasV1 = true;
          } else {
            p = ply_ascii_skip_property(p, prop);
          }

          prop = prop->next;
        }

        if (hasV0 && hasV1 && v0 < vertcount && v1 < vertcount) {
          ply_edge_append(pst, v0, v1);
        }

        NEXT_LINE
      }
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
