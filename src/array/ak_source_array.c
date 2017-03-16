/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../ak_common.h"

AK_EXPORT
AkObject*
ak_sourceDetachArray(AkAccessor * __restrict acc) {
  AkHeap            *heap;
  AkSourceArrayBase *array, *newarray;
  AkObject          *data,  *newdata;
  AkDataParam       *dp;
  size_t             size, icount, i;

  heap = ak_heap_getheap(acc);
  data = ak_getObjectByUrl(&acc->source);
  if (!data)
    return NULL;

  if (data->type == AK_SOURCE_ARRAY_TYPE_STRING) {
    /* TODO: */
    return NULL;
  }

  array = ak_objGet(data);
  size  = ak_sourceArraySize(data->type)
             + ak_sourceArrayItemSize(data->type)
             + acc->count * acc->bound;

  newdata = ak_objAlloc(heap,
                        NULL,
                        size,
                        data->type,
                        false);
  newarray = ak_objGet(newdata);
  newarray->count = acc->count;
  newarray->type  = data->type;

  icount = newarray->count;

  switch (data->type) {
    case AK_SOURCE_ARRAY_TYPE_FLOAT: {
      AkFloat *olditms, *newitms;
      AkUInt   j;

      newitms = newarray->items;
      olditms = array->items;

      for (i = 0; i < icount; i++) {
        j  = 0;
        dp = acc->param;

        while (dp) {
          if (!dp->name)
            continue;

          newitms[i + j++] = olditms[i + acc->offset + dp->offset];
          dp = dp->next;
        }
      }
      break;
    }
    case AK_SOURCE_ARRAY_TYPE_INT: {
      AkInt *olditms, *newitms;
      AkUInt j;

      newitms = newarray->items;
      olditms = array->items;

      for (i = 0; i < icount; i++) {
        j  = 0;
        dp = acc->param;

        while (dp) {
          if (!dp->name)
            continue;

          newitms[i + j++] = olditms[i + acc->offset + dp->offset];
          dp = dp->next;
        }
      }
      break;
    }
    case AK_SOURCE_ARRAY_TYPE_STRING: {
      /* TODO: */
      break;
    }
    case AK_SOURCE_ARRAY_TYPE_BOOL: {
      bool  *olditms, *newitms;
      AkUInt j;

      newitms = newarray->items;
      olditms = array->items;

      for (i = 0; i < icount; i++) {
        j  = 0;
        dp = acc->param;

        while (dp) {
          if (!dp->name)
            continue;

          newitms[i + j++] = olditms[i + acc->offset + dp->offset];
          dp = dp->next;
        }
      }
      break;
    }
    default:
      return NULL;
  }

  return newdata;
}
