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
