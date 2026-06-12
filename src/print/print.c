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

#include "../common.h"

#include <string.h>

static
const char*
ak_print_strdup(AkHeap * __restrict heap,
                void   * __restrict parent,
                const char * __restrict str) {
  return str && heap ? ak_heap_strdup(heap, parent, str) : NULL;
}

AK_EXPORT
AkPrintDocument*
ak_printDocument(AkDoc * __restrict doc) {
  AkDocPrivate *priv;

  priv = ak__docPrivate(doc, false);
  return priv ? priv->print : NULL;
}

AK_EXPORT
AkPrintDocument*
ak_printDocumentEnsure(AkDoc * __restrict doc) {
  AkDocPrivate    *priv;
  AkHeap          *heap;
  AkPrintDocument *print;

  priv = ak__docPrivate(doc, true);
  if (!priv)
    return NULL;

  if (priv->print)
    return priv->print;

  heap = ak_heap_getheap(priv);
  if (!heap)
    return NULL;

  print = ak_heap_calloc(heap, priv, sizeof(*print));
  if (!print)
    return NULL;

  priv->print = print;
  return print;
}

AK_EXPORT
bool
ak_printHasFeature(const AkPrintDocument * __restrict print,
                   AkPrintFeatureFlags                 features) {
  return print && (print->features & features) == features;
}

AK_EXPORT
void
ak_printSetFeature(AkPrintDocument   * __restrict print,
                   AkPrintFeatureFlags             features) {
  if (print)
    print->features |= features;
}

AK_EXPORT
void
ak_printSetUnsupportedFeature(AkPrintDocument   * __restrict print,
                              AkPrintFeatureFlags             features) {
  if (!print)
    return;

  print->unsupportedFeatures |= features;
  print->validationFlags     |= AK_PRINT_VALIDATION_UNSUPPORTED_FEATURE;
}

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePart(AkDoc                  * __restrict doc,
                       AkPrintPackagePartType              type,
                       const char            * __restrict name,
                       const char            * __restrict contentType,
                       const char            * __restrict relationshipType) {
  AkPrintDocument    *print;
  AkPrintPackagePart *part;
  AkHeap             *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  part = ak_heap_calloc(heap, print, sizeof(*part));
  if (!part)
    return NULL;

  part->type = type;
  if (name)
    part->name = ak_heap_strdup(heap, part, name);
  if (contentType)
    part->contentType = ak_heap_strdup(heap, part, contentType);
  if (relationshipType)
    part->relationshipType = ak_heap_strdup(heap, part, relationshipType);

  if (print->lastPart)
    print->lastPart->next = part;
  else
    print->parts = part;

  print->lastPart = part;
  print->packagePartCount++;
  print->features |= AK_PRINT_FEATURE_PACKAGE;

  return part;
}

AK_EXPORT
bool
ak_printSetPackagePartData(AkDoc              * __restrict doc,
                           AkPrintPackagePart * __restrict part,
                           const void         * __restrict data,
                           size_t                          size) {
  AkHeap *heap;
  void   *copy;

  if (!doc || !part)
    return false;
  if (!data && size > 0u)
    return false;

  part->data = NULL;
  part->size = 0u;
  if (size == 0u)
    return true;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return false;

  copy = ak_heap_alloc(heap, part, size);
  if (!copy)
    return false;

  memcpy(copy, data, size);
  part->data = copy;
  part->size = size;
  return true;
}

AK_EXPORT
AkPrintPackagePart*
ak_printAddPackagePartData(AkDoc                  * __restrict doc,
                           AkPrintPackagePartType              type,
                           const char            * __restrict name,
                           const char            * __restrict contentType,
                           const char            * __restrict relationshipType,
                           const void            * __restrict data,
                           size_t                             size) {
  AkPrintPackagePart *part;

  part = ak_printAddPackagePart(doc, type, name, contentType, relationshipType);
  if (!part)
    return NULL;
  if (!ak_printSetPackagePartData(doc, part, data, size))
    return NULL;

  return part;
}

AK_EXPORT
AkPrintProductionItem*
ak_printAddProductionItem(AkDoc                  * __restrict doc,
                          AkPrintProductionItemType           type,
                          const char            * __restrict uuid,
                          const char            * __restrict path,
                          const char            * __restrict partNumber,
                          const char            * __restrict modelResolution,
                          uint32_t                           objectId,
                          uint32_t                           parentObjectId) {
  AkPrintDocument       *print;
  AkPrintProductionItem *item;
  AkHeap                *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  item = ak_heap_calloc(heap, print, sizeof(*item));
  if (!item)
    return NULL;

  item->type           = type;
  item->objectId       = objectId;
  item->parentObjectId = parentObjectId;
  if (uuid)
    item->uuid = ak_heap_strdup(heap, item, uuid);
  if (path)
    item->path = ak_heap_strdup(heap, item, path);
  if (partNumber)
    item->partNumber = ak_heap_strdup(heap, item, partNumber);
  if (modelResolution)
    item->modelResolution = ak_heap_strdup(heap, item, modelResolution);

  if (print->lastProductionItem)
    print->lastProductionItem->next = item;
  else
    print->productionItems = item;

  print->lastProductionItem = item;
  print->productionItemCount++;
  print->features |= AK_PRINT_FEATURE_PRODUCTION;

  return item;
}

AK_EXPORT
AkPrintSliceStack*
ak_printAddSliceStack(AkDoc      * __restrict doc,
                      const char * __restrict path,
                      uint32_t                id,
                      float                   zBottom) {
  AkPrintDocument  *print;
  AkPrintSliceStack *stack;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  stack = ak_heap_calloc(heap, print, sizeof(*stack));
  if (!stack)
    return NULL;

  stack->id      = id;
  stack->zBottom = zBottom;
  stack->path    = ak_print_strdup(heap, stack, path);

  if (print->lastSliceStack)
    print->lastSliceStack->next = stack;
  else
    print->sliceStacks = stack;

  print->lastSliceStack = stack;
  print->sliceStackCount++;
  print->features |= AK_PRINT_FEATURE_SLICE;

  return stack;
}

AK_EXPORT
AkPrintSliceRef*
ak_printAddSliceRef(AkDoc      * __restrict doc,
                    const char * __restrict path,
                    uint32_t                stackId,
                    float                   zTop) {
  AkPrintDocument *print;
  AkPrintSliceRef *ref;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  ref = ak_heap_calloc(heap, print, sizeof(*ref));
  if (!ref)
    return NULL;

  ref->stackId = stackId;
  ref->zTop    = zTop;
  ref->path    = ak_print_strdup(heap, ref, path);

  if (print->lastSliceRef)
    print->lastSliceRef->next = ref;
  else
    print->sliceRefs = ref;

  print->lastSliceRef = ref;
  print->sliceRefCount++;
  print->features |= AK_PRINT_FEATURE_SLICE;

  return ref;
}

AK_EXPORT
AkPrintSlice*
ak_printAddSlice(AkDoc      * __restrict doc,
                 const char * __restrict path,
                 uint32_t                stackId,
                 float                   zTop,
                 uint32_t                vertexCount,
                 uint32_t                polygonCount,
                 uint32_t                segmentCount) {
  AkPrintDocument *print;
  AkPrintSlice    *slice;
  AkHeap          *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  slice = ak_heap_calloc(heap, print, sizeof(*slice));
  if (!slice)
    return NULL;

  slice->path         = ak_print_strdup(heap, slice, path);
  slice->stackId      = stackId;
  slice->zTop         = zTop;
  slice->vertexCount  = vertexCount;
  slice->polygonCount = polygonCount;
  slice->segmentCount = segmentCount;

  if (print->lastSlice)
    print->lastSlice->next = slice;
  else
    print->slices = slice;

  print->lastSlice = slice;
  print->sliceCount++;
  print->features |= AK_PRINT_FEATURE_SLICE;

  return slice;
}

AK_EXPORT
AkPrintSliceObject*
ak_printAddSliceObject(AkDoc      * __restrict doc,
                       const char * __restrict path,
                       const char * __restrict slicePath,
                       const char * __restrict meshResolution,
                       uint32_t                objectId,
                       uint32_t                sliceStackId) {
  AkPrintDocument  *print;
  AkPrintSliceObject *object;
  AkHeap           *heap;

  print = ak_printDocumentEnsure(doc);
  if (!print)
    return NULL;

  heap = ak_heap_getheap(print);
  if (!heap)
    return NULL;

  object = ak_heap_calloc(heap, print, sizeof(*object));
  if (!object)
    return NULL;

  object->path           = ak_print_strdup(heap, object, path);
  object->slicePath      = ak_print_strdup(heap, object, slicePath);
  object->meshResolution = ak_print_strdup(heap, object, meshResolution);
  object->objectId       = objectId;
  object->sliceStackId   = sliceStackId;

  if (print->lastSliceObject)
    print->lastSliceObject->next = object;
  else
    print->sliceObjects = object;

  print->lastSliceObject = object;
  print->sliceObjectCount++;
  print->features |= AK_PRINT_FEATURE_SLICE;

  return object;
}
