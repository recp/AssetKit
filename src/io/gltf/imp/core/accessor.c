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

#include "accessor.h"
#include "enum.h"
#include "buffer.h"
#include "../../../../accessor.h"

#define k_gltf_bufferView    0
#define k_gltf_byteOffset    1
#define k_gltf_componentType 2
#define k_gltf_normalized    3
#define k_gltf_count         4
#define k_gltf_type          5
#define k_gltf_max           6
#define k_gltf_min           7
#define k_gltf_sparse        8
#define k_gltf_name          9

AK_HIDE
void
gltf_accessors(json_t * __restrict json,
               void   * __restrict userdata) {
  AkGLTFState        *gst;
  AkDoc              *doc;
  AkHeap             *heap;
  const json_array_t *jaccessors, *jarr;
  const json_t       *jitem, *it;
  AkAccessor         *acc;
  int                 componentLen, count, bound;

  if (!(jaccessors = json_array(json)))
    return;

  gst          = userdata;
  doc          = gst->doc;
  heap         = gst->heap;
  json         = jaccessors->base.value;
  componentLen = 1;

  while (json) {
    acc = ak_heap_calloc(heap, doc, sizeof(*acc));

    ak_setypeid(acc, AKT_ACCESSOR);

    json_objmap_t accMap[] = {
      JSON_OBJMAP_OBJ(_s_gltf_bufferView,    I2P k_gltf_bufferView),
      JSON_OBJMAP_OBJ(_s_gltf_byteOffset,    I2P k_gltf_byteOffset),
      JSON_OBJMAP_OBJ(_s_gltf_componentType, I2P k_gltf_componentType),
      JSON_OBJMAP_OBJ(_s_gltf_normalized,    I2P k_gltf_normalized),
      JSON_OBJMAP_OBJ(_s_gltf_count,         I2P k_gltf_count),
      JSON_OBJMAP_OBJ(_s_gltf_type,          I2P k_gltf_type),
      JSON_OBJMAP_OBJ(_s_gltf_max,           I2P k_gltf_max),
      JSON_OBJMAP_OBJ(_s_gltf_min,           I2P k_gltf_min),
      JSON_OBJMAP_OBJ(_s_gltf_sparse,        I2P k_gltf_sparse),
      JSON_OBJMAP_OBJ(_s_gltf_name,          I2P k_gltf_name)
    };

    json_objmap(json, accMap, JSON_ARR_LEN(accMap));
  
    if ((it = accMap[k_gltf_name].object)) {
      acc->name = json_strdup(it, heap, acc);
    }
    
    /*  merge bufferView with acessor and buffer */
    if ((it = accMap[k_gltf_bufferView].object)) {
      AkBuffer     *buff, *tmpbuff;
      AkBufferView *buffView;
      int32_t       buffViewIndex;
      
      if ((buffViewIndex = json_int32(it, -1)) > -1
          && (buffView = flist_sp_at(&gst->bufferViews, buffViewIndex))
          && (tmpbuff = buffView->buffer)
          /* tmpbuff->data is NULL when the source buffer is supplied via
             an unsupported extension (EXT_meshopt_compression,
             KHR_draco_mesh_compression, ...). Skip — accessor will be
             buffer-less rather than reading from a NULL pointer. */
          && tmpbuff->data) {
        if (!(buff = rb_find(gst->bufferMap, buffView))) {
          buff         = ak_heap_calloc(heap, doc, sizeof(*buff));
          buff->data   = ak_heap_alloc(heap, buff, buffView->byteLength);
          buff->length = buffView->byteLength;

          memcpy(buff->data,
                 (char *)tmpbuff->data + buffView->byteOffset,
                 buffView->byteLength);

          rb_insert(gst->bufferMap, buffView, buff);
        }

        acc->byteStride = buffView->byteStride;
        acc->buffer     = buff;

        flist_sp_insert(&doc->lib.buffers, buff);
      }
    }

    if ((it = accMap[k_gltf_byteOffset].object)) {
      acc->byteOffset = json_uint64(it, 0);
    }
    
    if ((it = accMap[k_gltf_componentType].object)) {
      int componentType;
      componentType      = json_int32(it, -1);
      acc->componentType = gltf_componentType(componentType);
      componentLen       = gltf_componentLen(componentType);
    }
    
    if ((it = accMap[k_gltf_normalized].object)) {
      acc->normalized = json_bool(it, false);
    }
    
    if ((it = accMap[k_gltf_count].object)) {
      acc->count = json_uint32(it, 0);
    }
    
    if ((it = accMap[k_gltf_type].object)) {
      acc->componentSize = gltf_type(it);
    }

    /* prepare for min and max */
    if (acc->componentSize < 5) {
      bound                  = acc->componentSize;
      acc->bytesPerComponent = componentLen;
      acc->componentCount    = bound;
    } else {
      bound                  = acc->componentSize >> 3;
      acc->bytesPerComponent = componentLen;
      acc->componentCount    = bound;
    }
    
    acc->byteLength   = acc->bytesPerComponent * bound * acc->count;
    acc->fillByteSize = acc->bytesPerComponent * bound;

    /* glTF spec 3.6.2.4: bufferView.byteStride undefined → tightly packed.
       Normalize once, up front, so all downstream code (sparse densify,
       dequantize, consumer reads) can use acc->byteStride uniformly without
       a special case for "0 means packed". */
    if (acc->byteStride == 0) {
      acc->byteStride = acc->fillByteSize;
    }

    /* Snapshot the source-side encoding before any in-place dequantize
       below mutates componentType/normalized. These fields stay valid
       for the lifetime of the accessor so callers can reason about how
       the buffer was originally laid out, even after a widen-to-float
       pass. With AK_OPT_PRESERVE_QUANTIZED_ATTRS the buffer is left as
       integers and these mirror componentType/normalized. */
    acc->originalComponentType = acc->componentType;
    acc->originallyNormalized  = acc->normalized;

    if (acc->componentSize != AK_COMPONENT_SIZE_UNKNOWN
        && acc->fillByteSize > 0) {
      if ((it = accMap[k_gltf_min].object) && it->value) {
        acc->min = ak_heap_alloc(heap, acc, acc->fillByteSize);

        if ((jarr = json_array(it))) {
          jitem = jarr->base.value;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->min, acc->componentType, --count, it);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->min, acc->componentType, 0, it);
        }
      }

      if ((it = accMap[k_gltf_max].object) && it->value) {
        acc->max = ak_heap_alloc(heap, acc, acc->fillByteSize);

        if ((jarr = json_array(it))) {
          jitem = jarr->base.value;
          count = jarr->count;

          while (jitem) {
            json_array_set(acc->max, acc->componentType, --count, it);
            jitem = jitem->next;
          }
        } else {
          json_array_set(acc->max, acc->componentType, 0, it);
        }
      }
    }

    /* Sparse accessor — densify here so consumers always see a flat,
       tightly-packed buffer.
       glTF spec 3.6.2.3: when accessor.bufferView is undefined the dense
       buffer is initialized to zeros; otherwise it starts as a copy of the
       referenced bufferView slice. Then sparse.values overwrite the entries
       at sparse.indices. */
    if ((it = accMap[k_gltf_sparse].object)) {
      json_t       *jsCount, *jsIndices, *jsValues, *node;
      AkBufferView *idxBV, *valBV;
      AkBuffer     *denseBuff;
      char         *denseData, *idxPtr, *valPtr;
      size_t        totalSize, idxByteOffset, valByteOffset;
      uint32_t      sparseCount, sparseIdx, i;
      int32_t       idxBVIdx, valBVIdx;
      AkTypeId      idxComponentType;

      jsCount   = json_get(it, "count");
      jsIndices = json_get(it, "indices");
      jsValues  = json_get(it, "values");

      if (jsCount && jsIndices && jsValues
          && (sparseCount = json_uint32(jsCount, 0)) > 0
          && acc->fillByteSize > 0
          && acc->count > 0) {

        /* indices descriptor */
        idxBVIdx         = (node = json_get(jsIndices, _s_gltf_bufferView))
                              ? json_int32(node, -1) : -1;
        idxByteOffset    = (node = json_get(jsIndices, _s_gltf_byteOffset))
                              ? json_uint64(node, 0) : 0;
        idxComponentType = (node = json_get(jsIndices, _s_gltf_componentType))
                              ? gltf_componentType(json_int32(node, -1))
                              : AKT_USHORT;

        /* values descriptor */
        valBVIdx      = (node = json_get(jsValues, _s_gltf_bufferView))
                          ? json_int32(node, -1) : -1;
        valByteOffset = (node = json_get(jsValues, _s_gltf_byteOffset))
                          ? json_uint64(node, 0) : 0;

        idxBV = (idxBVIdx >= 0)
                  ? flist_sp_at(&gst->bufferViews, idxBVIdx) : NULL;
        valBV = (valBVIdx >= 0)
                  ? flist_sp_at(&gst->bufferViews, valBVIdx) : NULL;

        if (idxBV && valBV && idxBV->buffer && valBV->buffer) {
          totalSize         = (size_t)acc->fillByteSize * acc->count;
          denseBuff         = ak_heap_calloc(heap, doc, sizeof(*denseBuff));
          denseBuff->data   = ak_heap_alloc(heap, denseBuff, totalSize);
          denseBuff->length = totalSize;
          denseData         = denseBuff->data;

          /* initialize from main bufferView slice (if any) or zeros.
             Source may be interleaved (byteStride > fillByteSize), so copy
             element by element when needed instead of one big memcpy. */
          if (acc->buffer && acc->buffer->data) {
            if (acc->byteStride == 0
                || acc->byteStride == acc->fillByteSize) {
              memcpy(denseData,
                     (char *)acc->buffer->data + acc->byteOffset,
                     totalSize);
            } else {
              uint32_t v;
              char    *baseSrc = (char *)acc->buffer->data + acc->byteOffset;
              for (v = 0; v < acc->count; v++) {
                memcpy(denseData + (size_t)v * acc->fillByteSize,
                       baseSrc   + (size_t)v * acc->byteStride,
                       acc->fillByteSize);
              }
            }
          } else {
            memset(denseData, 0, totalSize);
          }

          /* overlay sparse values at the given indices */
          idxPtr = (char *)idxBV->buffer->data
                 + idxBV->byteOffset + idxByteOffset;
          valPtr = (char *)valBV->buffer->data
                 + valBV->byteOffset + valByteOffset;

          for (i = 0; i < sparseCount; i++) {
            switch (idxComponentType) {
              case AKT_UBYTE:  sparseIdx = ((uint8_t  *)idxPtr)[i]; break;
              case AKT_USHORT: sparseIdx = ((uint16_t *)idxPtr)[i]; break;
              case AKT_UINT:   sparseIdx = ((uint32_t *)idxPtr)[i]; break;
              default:         continue;
            }
            if (sparseIdx >= acc->count) continue;

            memcpy(denseData + (size_t)sparseIdx * acc->fillByteSize,
                   valPtr    + (size_t)i        * acc->fillByteSize,
                   acc->fillByteSize);
          }

          /* swap: accessor now points at the dense buffer, tightly packed */
          acc->buffer     = denseBuff;
          acc->byteOffset = 0;
          acc->byteStride = acc->fillByteSize;

          flist_sp_insert(&doc->lib.buffers, denseBuff);
        }
      }
    }

    /* Dequantize normalized integer attributes to packed float buffer.
       Triggered when accessor.normalized == true on byte/short data — the
       common case for COLOR_n (UBYTE), TEXCOORD_n (UBYTE/USHORT), and
       quantized POSITION/NORMAL/TANGENT under KHR_mesh_quantization.
       Non-normalized integers (e.g. JOINTS_n) are left intact so vertex
       shaders can index by them. After conversion the accessor owns a
       tightly-packed float buffer.
       glTF spec 3.6.1.1: integer-to-float scale factors.

       Opt-out: AK_OPT_PRESERVE_QUANTIZED_ATTRS keeps the integer buffer
       intact for renderers that decode on the GPU. originalComponentType
       and originallyNormalized still describe the source encoding, and
       ak_accessorAsFloat / ak_accessorMakeFloat let consumers dequantize
       on demand. */
    if (acc->normalized
        && acc->buffer && acc->buffer->data
        && acc->componentType != AKT_FLOAT
        && acc->fillByteSize > 0
        && acc->count > 0
        && !ak_opt_get(AK_OPT_PRESERVE_QUANTIZED_ATTRS)) {
      AkBuffer *fbuf;
      char     *src;
      float    *dst;
      size_t    floatBufSize, stride, perComp;
      uint32_t  v, c, comps;

      comps        = acc->componentCount;
      stride       = acc->byteStride;            /* normalized up front      */
      perComp      = acc->bytesPerComponent;
      floatBufSize = (size_t)comps * acc->count * sizeof(float);

      fbuf         = ak_heap_calloc(heap, doc, sizeof(*fbuf));
      fbuf->data   = ak_heap_alloc(heap, fbuf, floatBufSize);
      fbuf->length = floatBufSize;
      dst          = fbuf->data;
      src          = (char *)acc->buffer->data + acc->byteOffset;

      for (v = 0; v < acc->count; v++) {
        char *vsrc = src + (size_t)v * stride;
        for (c = 0; c < comps; c++) {
          void  *cp = vsrc + (size_t)c * perComp;
          float  f;
          switch (acc->componentType) {
            case AKT_BYTE:   f = (float)(*(int8_t  *)cp) / 127.0f;
                             if (f < -1.0f) f = -1.0f;
                             break;
            case AKT_UBYTE:  f = (float)(*(uint8_t *)cp) / 255.0f;   break;
            case AKT_SHORT:  f = (float)(*(int16_t *)cp) / 32767.0f;
                             if (f < -1.0f) f = -1.0f;
                             break;
            case AKT_USHORT: f = (float)(*(uint16_t*)cp) / 65535.0f; break;
            default:         f = 0.0f; break;
          }
          dst[(size_t)v * comps + c] = f;
        }
      }

      acc->buffer            = fbuf;
      acc->byteOffset        = 0;
      acc->bytesPerComponent = sizeof(float);
      acc->fillByteSize      = (size_t)comps * sizeof(float);
      acc->byteStride        = acc->fillByteSize;
      acc->componentType     = AKT_FLOAT;
      acc->normalized        = false;

      flist_sp_insert(&doc->lib.buffers, fbuf);
    }

    /* KHR_mesh_quantization: non-normalized integer POSITION (vec3) or
       TEXCOORD (vec2). Spec allows raw integer values that map linearly to
       model space via a node-level transform — but most engines expect
       float vertex attributes, so we widen the integers to float in
       place. The asset's node transform already encodes the correct
       scale/offset from quantized space, so the result reaches the GPU
       at the intended position.

       Heuristic: only widen vec2/vec3 attributes — JOINTS_n is non-normalized
       integer too but is vec4 (and must stay int for skinning), so it's
       skipped. TANGENT is vec4 in the quantization spec but is required to
       be normalized, so it goes through the previous branch instead.

       Opt-out: AK_OPT_PRESERVE_QUANTIZED_ATTRS keeps the integer buffer
       intact (see notes on the previous block). */
    if (!acc->normalized
        && acc->buffer && acc->buffer->data
        && acc->componentType != AKT_FLOAT
        && acc->componentType != AKT_UINT
        && (acc->componentCount == 2 || acc->componentCount == 3)
        && acc->fillByteSize > 0
        && acc->count > 0
        && !ak_opt_get(AK_OPT_PRESERVE_QUANTIZED_ATTRS)) {
      AkBuffer *fbuf;
      char     *src;
      float    *dst;
      size_t    floatBufSize, stride, perComp;
      uint32_t  v, c, comps;

      comps        = acc->componentCount;
      stride       = acc->byteStride;            /* normalized up front      */
      perComp      = acc->bytesPerComponent;
      floatBufSize = (size_t)comps * acc->count * sizeof(float);

      fbuf         = ak_heap_calloc(heap, doc, sizeof(*fbuf));
      fbuf->data   = ak_heap_alloc(heap, fbuf, floatBufSize);
      fbuf->length = floatBufSize;
      dst          = fbuf->data;
      src          = (char *)acc->buffer->data + acc->byteOffset;

      for (v = 0; v < acc->count; v++) {
        char *vsrc = src + (size_t)v * stride;
        for (c = 0; c < comps; c++) {
          void *cp = vsrc + (size_t)c * perComp;
          float f;
          switch (acc->componentType) {
            case AKT_BYTE:   f = (float)(*(int8_t  *)cp); break;
            case AKT_UBYTE:  f = (float)(*(uint8_t *)cp); break;
            case AKT_SHORT:  f = (float)(*(int16_t *)cp); break;
            case AKT_USHORT: f = (float)(*(uint16_t*)cp); break;
            default:         f = 0.0f; break;
          }
          dst[(size_t)v * comps + c] = f;
        }
      }

      acc->buffer            = fbuf;
      acc->byteOffset        = 0;
      acc->bytesPerComponent = sizeof(float);
      acc->fillByteSize      = (size_t)comps * sizeof(float);
      acc->byteStride        = acc->fillByteSize;
      acc->componentType     = AKT_FLOAT;

      flist_sp_insert(&doc->lib.buffers, fbuf);
    }

    /* (byteStride normalization done up front, before sparse + dequantize) */

    flist_sp_insert(&gst->doc->lib.accessors, acc);

    json = json->next;
  }
}
