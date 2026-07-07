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

#include "scenekit.h"
#include "../../../mat/internal.h"
#include "../../../string_fast.h"

#include <ctype.h>
#include <string.h>

static
bool
dae_strcase_contains(const char * __restrict str,
                     const char * __restrict needle) {
  const char *s, *n, *match;

  if (!str || !needle || !needle[0])
    return false;

  for (; *str; str++) {
    s     = str;
    n     = needle;
    match = str;

    while (*s && *n
           && tolower((unsigned char)*s) == tolower((unsigned char)*n)) {
      s++;
      n++;
    }

    if (!*n)
      return true;

    str = match;
  }

  return false;
}

static
bool
dae_scenekit_authored(AkDoc * __restrict doc) {
  AkContributor *contr;

  if (!doc || !doc->inf)
    return false;

  for (contr = doc->inf->base.contributor; contr; contr = contr->next) {
    if (dae_strcase_contains(contr->authoringTool, "scenekit"))
      return true;
  }

  return false;
}

static
bool
dae_colordesc_has_texture(DAEState    * __restrict dst,
                          AkColorDesc * __restrict color) {
  if (!color)
    return false;

  return color->texture || (dst->texmap && rb_find(dst->texmap, color));
}

static
bool
dae_scenekit_is_red_fill(AkMaterial          * __restrict material,
                         AkTechniqueFxCommon * __restrict common) {
  AkColor *color;

  if (!material
      || !material->name
      || !ak_str_eq_cstr_fast(material->name,
                              _s_dae_material,
                              _s_dae_material_len)
      || !common
      || !common->diffuse
      || !common->diffuse->color)
    return false;

  color = common->diffuse->color;

  return color->rgba.R > 0.45f
         && color->rgba.R < 0.90f
         && color->rgba.G < 0.40f
         && color->rgba.B < 0.50f
         && color->rgba.R > color->rgba.G + 0.20f
         && color->rgba.R > color->rgba.B + 0.15f
         && color->rgba.A > 0.95f;
}

static
AkTechniqueFxCommon*
dae_scenekit_primitive_common(DAEState            * __restrict dst,
                              AkInstanceGeometry  * __restrict instGeom,
                              AkMeshPrimitive     * __restrict prim,
                              AkMaterial         ** __restrict materialOut) {
  AkResolvedMaterial resolved;
  AkEffect          *effect;

  *materialOut = NULL;

  if (!ak_materialResolve(prim, instGeom, UINT32_MAX, &resolved)
      || !resolved.material)
    return NULL;

  effect = dae_material_effect(dst, resolved.material);
  if (!effect)
    return NULL;

  *materialOut = resolved.material;
  return ak_getProfileTechniqueCommon(effect);
}

static
bool
dae_scenekit_primitive_has_texture(DAEState           * __restrict dst,
                                   AkInstanceGeometry * __restrict instGeom,
                                   AkMeshPrimitive    * __restrict prim) {
  AkTechniqueFxCommon *common;
  AkMaterial          *material;

  common = dae_scenekit_primitive_common(dst, instGeom, prim, &material);

  return common && dae_colordesc_has_texture(dst, common->diffuse);
}

static
bool
dae_scenekit_is_textured_twin(DAEState           * __restrict dst,
                              AkInstanceGeometry * __restrict instGeom,
                              AkMeshPrimitive    * __restrict prim,
                              AkMeshPrimitive    * __restrict other) {
  return other
         && other != prim
         && other->type == AK_PRIMITIVE_TRIANGLES
         && other->nPolygons == prim->nPolygons
         && dae_scenekit_primitive_has_texture(dst, instGeom, other);
}

static
bool
dae_scenekit_should_drop_primitive(DAEState           * __restrict dst,
                                   AkInstanceGeometry * __restrict instGeom,
                                   AkMeshPrimitive    * __restrict prim,
                                   AkMeshPrimitive    * __restrict prev,
                                   AkMeshPrimitive    * __restrict next) {
  AkTechniqueFxCommon *common;
  AkMaterial          *material;

  if (prim->type != AK_PRIMITIVE_TRIANGLES || prim->nPolygons == 0)
    return false;

  common = dae_scenekit_primitive_common(dst, instGeom, prim, &material);
  if (!common || dae_colordesc_has_texture(dst, common->diffuse))
    return false;

  return dae_scenekit_is_red_fill(material, common)
         && (dae_scenekit_is_textured_twin(dst, instGeom, prim, prev)
             || dae_scenekit_is_textured_twin(dst, instGeom, prim, next));
}

static
void
dae_scenekit_unmap_material(AkGeometry      * __restrict geom,
                            AkMeshPrimitive * __restrict prim) {
  AkMapItem *head, *item;

  if (!geom || !geom->materialMap || !prim->bindmaterial)
    return;

  head = ak_map_findm(geom->materialMap, (void *)prim->bindmaterial);
  if (!head)
    return;

  item = head->data;
  while (item) {
    if (item->data == prim) {
      if (item->prev)
        item->prev->next = item->next;
      else
        head->data = item->next;

      if (item->next)
        item->next->prev = item->prev;

      return;
    }

    item = item->next;
  }
}

static
void
dae_scenekit_fix_mesh(DAEState           * __restrict dst,
                      AkInstanceGeometry * __restrict instGeom,
                      AkGeometry         * __restrict geom,
                      AkMesh             * __restrict mesh) {
  AkMeshPrimitive *prim, *prev, *next;

  prev = NULL;
  prim = mesh->primitive;

  while (prim) {
    next = prim->next;

    if (dae_scenekit_should_drop_primitive(dst, instGeom, prim, prev, next)) {
      if (prev)
        prev->next = next;
      else
        mesh->primitive = next;

      dae_scenekit_unmap_material(geom, prim);

      prim->next = NULL;
      if (mesh->primitiveCount > 0)
        mesh->primitiveCount--;

    } else {
      prev = prim;
    }

    prim = next;
  }
}

static
void
dae_scenekit_fix_node(DAEState * __restrict dst, AkNode * __restrict node) {
  AkInstanceGeometry *instGeom;
  AkGeometry         *geom;
  AkObject           *geomData;

  for (; node; node = node->next) {
    for (instGeom = node->geometry; instGeom;
         instGeom = (AkInstanceGeometry *)instGeom->base.next) {
      geom = ak_instanceObject(&instGeom->base);
      if (!geom || ak_typeid(geom) != AKT_GEOMETRY)
        continue;

      geomData = geom->gdata;
      if (!geomData || (AkGeometryType)geomData->type != AK_GEOMETRY_MESH)
        continue;

      dae_scenekit_fix_mesh(dst, instGeom, geom, ak_objGet(geomData));
    }

    if (node->chld)
      dae_scenekit_fix_node(dst, node->chld);

    if (node->node) {
      AkInstanceNode *instNode;

      for (instNode = node->node; instNode; instNode = instNode->next) {
        AkNode *target;

        target = ak_instanceNodeTarget(instNode);
        if (target)
          dae_scenekit_fix_node(dst, target);
      }
    }
  }
}

AK_HIDE
void
dae_bugfix_scenekit_material_surfaces(DAEState * __restrict dst) {
  AkMaterial *material;

  if (!dst
      || !dst->doc
      || !ak_opt_get(AK_OPT_BUGFIXES)
      || !dae_scenekit_authored(dst->doc))
    return;

  for (material = dst->doc->lib.materials.first; material; material = material->next) {
    if (material->surface)
      material->surface->flags |= AK_MATERIAL_FLAG_DOUBLE_SIDED;
  }
}

AK_HIDE
void
dae_bugfix_scenekit_backfaces(DAEState * __restrict dst) {
  AkScene *vscn;

  if (!dst
      || !dst->doc
      || !ak_opt_get(AK_OPT_BUGFIXES)
      || !dae_scenekit_authored(dst->doc)
      || !dst->doc->lib.scenes.first)
    return;

  for (vscn = dst->doc->lib.scenes.first;
       vscn;
       vscn = vscn->next) {
    dae_scenekit_fix_node(dst, vscn->node);
  }
}
