/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "variants.h"
#include "../core/ext.h"

/* KHR_materials_variants — root-level: capture the document's variant list
   so primitive parser can resolve `variants[]` indices later. The doc
   field stays in declaration-order (head→tail = index 0→N), matching the
   indices the primitive `mappings[].variants[]` arrays use. */
AK_HIDE
void
gltf_ext_materialVariants(AkGLTFState * __restrict gst,
                          json_t      * __restrict jvariants) {
  json_array_t       *jarr;
  json_t             *jvariant;
  AkMaterialVariant  *variant;
  AkDoc              *doc;
  uint32_t            count;
  json_t             *jname;

  if (!(jarr = json_array(jvariants)) || jarr->count == 0)
    return;

  doc      = gst->doc;
  jvariant = jarr->base.value;
  count    = 0;

  doc->materialVariants = NULL;
  while (jvariant) {
    variant = ak_heap_calloc(gst->heap, doc, sizeof(*variant));

    if ((jname = json_get(jvariant, _s_gltf_name)))
      variant->name = json_strdup(jname, gst->heap, variant);

    /* json_parse(..., true) gives array children in reverse. Prepend
       restores source/index order: variants[0] is list head. */
    variant->next         = doc->materialVariants;
    doc->materialVariants = variant;

    count++;
    jvariant = jvariant->next;
  }

  doc->materialVariantCount = count;
}

AK_HIDE
bool
gltf_ext_primitiveVariants(AkGLTFState     * __restrict gst,
                           AkMeshPrimitive * __restrict prim,
                           const json_t    * __restrict jprim) {
  const json_t             *jext;
  const json_t             *jvariantsExt;
  const json_t             *jmappings;
  json_array_t             *jmapArr;
  json_t                   *jmap;
  json_t                   *jmaterial;
  json_t                   *jvariants;
  json_array_t             *jvarsArr;
  json_t                   *jvarIdx;
  AkMaterialVariantMapping *mapping;
  AkMaterialVariantMapping *tail;
  AkMaterial               *material;
  int32_t                   matIdx;
  uint32_t                  variantIdx;
  uint32_t                  count;

  if (!gst || !prim || !jprim)
    return true;

  jext         = json_get(jprim, _s_gltf_extensions);
  jvariantsExt = jext ? json_get(jext, _s_gltf_KHR_materials_variants) : NULL;
  jmappings    = jvariantsExt ? json_get(jvariantsExt, _s_gltf_mappings) : NULL;
  if (!jmappings || !(jmapArr = json_array(jmappings)) || jmapArr->count == 0)
    return true;

  count = 0;
  tail  = NULL;

  /* Each `mapping` entry references a single material and the list of
     variant indices that should resolve to it. We unroll the variants
     list into a flat per-(variant, material) chain so runtime swap is
     O(numVariants) — typical N is small (<10) so the unroll is cheap. */
  for (jmap = jmapArr->base.value; jmap; jmap = jmap->next) {
    jmaterial = json_get(jmap, _s_gltf_material);
    jvariants = json_get(jmap, _s_gltf_variants);
    if (!jmaterial || !jvariants)
      continue;

    matIdx = json_int32(jmaterial, -1);
    if (matIdx < 0)
      continue;

    material = gltf_material_at(gst, matIdx);
    if (!material)
      continue;

    if (!(jvarsArr = json_array(jvariants)))
      continue;

    for (jvarIdx = jvarsArr->base.value; jvarIdx; jvarIdx = jvarIdx->next) {
      variantIdx = (uint32_t)json_int32(jvarIdx, -1);
      if ((int32_t)variantIdx < 0
          || variantIdx >= gst->doc->materialVariantCount)
        continue;

      mapping = ak_heap_calloc(gst->heap, prim, sizeof(*mapping));
      mapping->material     = material;
      mapping->variantIndex = variantIdx;

      if (tail) {
        tail->next = mapping;
      } else {
        prim->variantMappings = mapping;
      }
      tail = mapping;

      count++;
    }
  }

  prim->variantMappingCount = count;
  return true;
}
