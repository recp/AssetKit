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
