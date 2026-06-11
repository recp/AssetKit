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

#include "controller.h"
#include "mesh.h"

static
AkNode**
dae_skin_joints(AkInstanceSkin * __restrict skinner,
                size_t         * __restrict count) {
  AkSkin *skin;

  *count = 0;
  if (!skinner || !(skin = skinner->skin))
    return NULL;

  *count = skin->nJoints;
  return skinner->overrideJoints ? skinner->overrideJoints : skin->joints;
}

AK_HIDE
bool
dae_instance_skin_supported(AkInstanceGeometry * __restrict inst) {
  AkInstanceSkin  *skinner;
  AkSkin          *skin;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkNode         **joints;
  size_t           jointCount;
  size_t           i;
  uint32_t         primIdx;

  skinner = inst ? inst->skinner : NULL;
  skin    = skinner ? skinner->skin : NULL;
  geom    = dae_instance_geometry_object(inst);
  if (!skin
      || !geom
      || !geom->gdata
      || geom->gdata->type != AK_GEOMETRY_MESH)
    return false;

  mesh = ak_objGet(geom->gdata);
  if (!mesh || mesh->primitiveCount == 0 || !(prim = mesh->primitive))
    return false;

  primIdx = 0;
  for (; prim; prim = prim->next, primIdx++) {
    if (!prim->pos || !prim->pos->accessor || prim->pos->accessor->count == 0)
      return false;

    if (skin->weights && skin->nPrims > 0) {
      AkBoneWeights *weights;

      if (primIdx >= skin->nPrims)
        return false;

      weights = skin->weights[primIdx];
      if (!weights
          || !weights->counts
          || !weights->indexes
          || weights->nVertex != prim->pos->accessor->count)
        return false;
    }
  }

  if (primIdx != mesh->primitiveCount)
    return false;

  joints = dae_skin_joints(skinner, &jointCount);
  if (!joints || jointCount == 0 || jointCount > UINT32_MAX)
    return false;

  for (i = 0; i < jointCount; i++) {
    if (!joints[i])
      return false;
  }

  return true;
}

AK_HIDE
void
dae_w_skin_id(DAEExpWriter * __restrict w, uint32_t skinIdx) {
  dae_w_id(w, DAE_EXP_NAME(skin), skinIdx);
}

static
void
dae_w_skin_source_id(DAEExpWriter     * __restrict w,
                     uint32_t                      skinIdx,
                     DAEExpName                    suffix) {
  dae_w_skin_id(w, skinIdx);
  dae_w_ch(w, '_');
  dae_w_name(w, suffix);
}

static
AkNode**
dae_skin_export_joints(AkSkin         * __restrict skin,
                       AkInstanceSkin * __restrict skinner,
                       size_t         * __restrict count) {
  *count = 0;
  if (!skin)
    return NULL;

  *count = skin->nJoints;
  return skinner && skinner->overrideJoints
         ? skinner->overrideJoints
         : skin->joints;
}

static
AkMesh*
dae_skin_base_mesh(AkGeometry * __restrict geom) {
  AkMesh *mesh;

  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return NULL;

  mesh = ak_objGet(geom->gdata);
  return mesh && mesh->primitiveCount > 0 ? mesh : NULL;
}

static
bool
dae_write_skin_joint_source(DAEExpState    * __restrict st,
                            AkSkin         * __restrict skin,
                            AkInstanceSkin * __restrict skinner,
                            uint32_t                    skinIdx) {
  DAEExpWriter *w;
  AkNode      **joints;
  size_t        jointCount;
  size_t        i;

  joints = dae_skin_export_joints(skin, skinner, &jointCount);
  if (!joints || jointCount == 0)
    return false;

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME(joints));
  dae_w_lit(w, "\"><IDREF_array id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("joints_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, jointCount);
  dae_w_lit(w, "\">");

  for (i = 0; i < jointCount; i++) {
    if (i > 0)
      dae_w_ch(w, ' ');
    if (!dae_w_node_id_ref(st, joints[i]))
      return false;
  }

  dae_w_lit(w, "</IDREF_array><technique_common><accessor source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("joints_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, jointCount);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"JOINT\" type=\"IDREF\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_skin_bind_source(DAEExpState * __restrict st,
                           AkSkin      * __restrict skin,
                           uint32_t                 skinIdx) {
  DAEExpWriter *w;
  size_t        jointCount;
  size_t        i;

  if (!skin || skin->nJoints == 0)
    return false;

  w          = &st->w;
  jointCount = skin->nJoints;

  dae_w_lit(w, "<source id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("bind_poses"));
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("bind_poses_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, jointCount * 16u);
  dae_w_lit(w, "\">");

  for (i = 0; i < jointCount; i++) {
    if (i > 0)
      dae_w_ch(w, ' ');

    if (skin->invBindPoses) {
      dae_w_matrix4x4_dae(w, skin->invBindPoses[i]);
    } else {
      dae_w_identity4x4(w);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("bind_poses_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, jointCount);
  dae_w_lit(w, "\" stride=\"16\"><param name=\"TRANSFORM\" type=\"float4x4\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_skin_weight_source_csr(DAEExpState   * __restrict st,
                                 AkSkin        * __restrict skin,
                                 uint32_t                   skinIdx) {
  DAEExpWriter *w;
  size_t        totalWeights;
  size_t        i;
  uint32_t      primIdx;

  if (!skin || !skin->weights || skin->nPrims == 0)
    return false;

  totalWeights = 0;
  for (primIdx = 0; primIdx < skin->nPrims; primIdx++) {
    AkBoneWeights *weights;

    weights = skin->weights[primIdx];
    if (!weights || !weights->weights)
      return false;
    if (totalWeights > (size_t)-1 - weights->nWeights)
      return false;
    totalWeights += weights->nWeights;
  }

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights"));
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, totalWeights);
  dae_w_lit(w, "\">");

  totalWeights = 0;
  for (primIdx = 0; primIdx < skin->nPrims; primIdx++) {
    AkBoneWeights *weights;

    weights = skin->weights[primIdx];
    for (i = 0; i < weights->nWeights; i++, totalWeights++) {
      if (totalWeights > 0)
        dae_w_ch(w, ' ');
      dae_w_float(w, weights->weights[i].weight);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, totalWeights);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"WEIGHT\" type=\"float\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
bool
dae_write_skin_weight_source_flat(DAEExpState * __restrict st,
                                  const float * __restrict flatWeights,
                                  size_t                   vertexCount,
                                  uint32_t                 skinIdx) {
  DAEExpWriter *w;
  size_t        i;
  size_t        c;

  if (!flatWeights || vertexCount == 0)
    return false;

  w = &st->w;
  dae_w_lit(w, "<source id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights"));
  dae_w_lit(w, "\"><float_array id=\"");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, vertexCount * 4u);
  dae_w_lit(w, "\">");

  for (i = 0; i < vertexCount; i++) {
    for (c = 0; c < 4u; c++) {
      if (i > 0 || c > 0)
        dae_w_ch(w, ' ');
      dae_w_float(w, flatWeights[i * 4u + c]);
    }
  }

  dae_w_lit(w, "</float_array><technique_common><accessor source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights_array"));
  dae_w_lit(w, "\" count=\"");
  dae_w_uint(w, vertexCount * 4u);
  dae_w_lit(w, "\" stride=\"1\"><param name=\"WEIGHT\" type=\"float\"/>"
               "</accessor></technique_common></source>");

  return w->result == AK_OK;
}

static
void
dae_write_skin_vertex_weights_header(DAEExpState * __restrict st,
                                     size_t                   vertexCount,
                                     uint32_t                 skinIdx) {
  DAEExpWriter *w;

  w = &st->w;
  dae_w_lit(w, "<vertex_weights count=\"");
  dae_w_uint(w, vertexCount);
  dae_w_lit(w, "\"><input semantic=\"JOINT\" source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME(joints));
  dae_w_lit(w, "\" offset=\"0\"/><input semantic=\"WEIGHT\" source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("weights"));
  dae_w_lit(w, "\" offset=\"1\"/>");
}

static
void
dae_write_skin_vertex_weights_csr(DAEExpState   * __restrict st,
                                  AkSkin        * __restrict skin) {
  DAEExpWriter *w;
  size_t        i;
  size_t        k;
  size_t        weightIdx;
  uint32_t      primIdx;

  w = &st->w;
  dae_w_lit(w, "<vcount>");
  weightIdx = 0;
  for (primIdx = 0; primIdx < skin->nPrims; primIdx++) {
    AkBoneWeights *weights;

    weights = skin->weights[primIdx];
    for (i = 0; i < weights->nVertex; i++, weightIdx++) {
      if (weightIdx > 0)
        dae_w_ch(w, ' ');
      dae_w_uint(w, weights->counts[i]);
    }
  }
  dae_w_lit(w, "</vcount><v>");

  weightIdx = 0;
  for (primIdx = 0; primIdx < skin->nPrims; primIdx++) {
    AkBoneWeights *weights;

    weights = skin->weights[primIdx];
    for (i = 0; i < weights->nVertex; i++) {
      AkBoneWeight *items;
      uint32_t      count;

      count = weights->counts[i];
      items = weights->weights + weights->indexes[i];
      for (k = 0; k < count; k++, weightIdx++) {
        if (primIdx > 0 || i > 0 || k > 0)
          dae_w_ch(w, ' ');
        dae_w_uint(w, items[k].joint);
        dae_w_ch(w, ' ');
        dae_w_uint(w, weightIdx);
      }
    }
  }

  dae_w_lit(w, "</v></vertex_weights>");
}

static
void
dae_write_skin_vertex_weights_flat(DAEExpState    * __restrict st,
                                   const uint16_t * __restrict flatJoints,
                                   size_t                      vertexCount) {
  DAEExpWriter *w;
  size_t        i;
  size_t        c;

  w = &st->w;
  dae_w_lit(w, "<vcount>");
  for (i = 0; i < vertexCount; i++) {
    if (i > 0)
      dae_w_ch(w, ' ');
    dae_w_uint(w, 4u);
  }
  dae_w_lit(w, "</vcount><v>");

  for (i = 0; i < vertexCount; i++) {
    for (c = 0; c < 4u; c++) {
      if (i > 0 || c > 0)
        dae_w_ch(w, ' ');
      dae_w_uint(w, flatJoints[i * 4u + c]);
      dae_w_ch(w, ' ');
      dae_w_uint(w, i * 4u + c);
    }
  }

  dae_w_lit(w, "</v></vertex_weights>");
}

static
bool
dae_write_skin_weights(DAEExpState     * __restrict st,
                       AkSkin          * __restrict skin,
                       AkMesh          * __restrict mesh,
                       uint32_t                     skinIdx) {
  AkMeshPrimitive *prim;
  uint16_t      *flatJoints;
  float         *flatWeights;
  void          *flatData;
  size_t         vertexCount;
  size_t         vertexOffset;
  size_t         jointBytes;
  size_t         weightBytes;
  uint32_t       primIdx;
  bool           ok;

  if (!skin || !mesh || !mesh->primitive)
    return false;

  if (skin->weights && skin->nPrims > 0) {
    vertexCount = 0;
    for (primIdx = 0, prim = mesh->primitive;
         prim;
         primIdx++, prim = prim->next) {
      AkBoneWeights *weights;

      if (primIdx >= skin->nPrims)
        return false;
      weights = skin->weights[primIdx];
      if (!weights || vertexCount > (size_t)-1 - weights->nVertex)
        return false;
      vertexCount += weights->nVertex;
    }

    if (!dae_write_skin_weight_source_csr(st, skin, skinIdx))
      return false;
    dae_write_skin_vertex_weights_header(st, vertexCount, skinIdx);
    dae_write_skin_vertex_weights_csr(st, skin);
    return st->w.result == AK_OK;
  }

  vertexCount = 0;
  for (prim = mesh->primitive; prim; prim = prim->next) {
    if (!prim->pos || !prim->pos->accessor)
      return false;
    if (vertexCount > (size_t)-1 - prim->pos->accessor->count)
      return false;
    vertexCount += prim->pos->accessor->count;
  }

  if (vertexCount == 0
      || vertexCount > (size_t)-1 / (sizeof(uint16_t) * 4u)
      || vertexCount > (size_t)-1 / (sizeof(float) * 4u))
    return false;

  jointBytes  = vertexCount * sizeof(uint16_t) * 4u;
  weightBytes = vertexCount * sizeof(float) * 4u;
  if (jointBytes > (size_t)-1 - weightBytes)
    return false;

  flatData = dae_scratch(st, jointBytes + weightBytes);
  if (!flatData)
    return false;

  flatJoints  = flatData;
  flatWeights = (float *)((char *)flatData + jointBytes);
  ok           = true;
  vertexOffset = 0;
  primIdx      = 0;
  for (prim = mesh->primitive; prim; prim = prim->next, primIdx++) {
    size_t primVertexCount;

    primVertexCount = prim->pos->accessor->count;
    if (ak_skinFillWeights(skin,
                           prim,
                           primIdx,
                           4u,
                           flatJoints + vertexOffset * 4u,
                           flatWeights + vertexOffset * 4u)
        != primVertexCount) {
      ok = false;
      break;
    }
    vertexOffset += primVertexCount;
  }

  if (ok)
    ok = dae_write_skin_weight_source_flat(st, flatWeights, vertexCount, skinIdx);
  if (ok) {
    dae_write_skin_vertex_weights_header(st, vertexCount, skinIdx);
    dae_write_skin_vertex_weights_flat(st, flatJoints, vertexCount);
    ok = st->w.result == AK_OK;
  }

  return ok;
}

AK_HIDE
bool
dae_write_skin_controller(DAEExpState * __restrict st,
                          AkSkin      * __restrict skin,
                          uint32_t                 skinIdx) {
  DAEExpWriter     *w;
  AkInstanceSkin   *skinner;
  AkGeometry       *geom;
  AkMesh           *mesh;
  AkMorph          *morph;
  uint32_t          geomIdx;
  uint32_t          morphIdx;

  skinner = rb_find(st->skinInstances, skin);
  geom    = rb_find(st->skinGeometries, skin);
  geomIdx = geom ? dae_map_index(st->geometries, geom) : UINT32_MAX;
  morph   = rb_find(st->skinMorphs, skin);
  morphIdx = morph ? dae_map_index(st->morphs, morph)
                   : UINT32_MAX;
  mesh    = dae_skin_base_mesh(geom);
  if (!skin
      || geomIdx == UINT32_MAX
      || (morph && morphIdx == UINT32_MAX)
      || !mesh)
    return false;

  w = &st->w;
  dae_w_lit(w, "<controller id=\"");
  dae_w_skin_id(w, skinIdx);
  dae_w_lit(w, "\"><skin source=\"#");
  if (morph)
    dae_w_morph_id(w, morphIdx);
  else
    dae_w_geom_id(w, geomIdx);
  dae_w_lit(w, "\"><bind_shape_matrix>");
  dae_w_matrix4x4_dae(w, skin->bindShapeMatrix);
  dae_w_lit(w, "</bind_shape_matrix>");

  if (!dae_write_skin_joint_source(st, skin, skinner, skinIdx)
      || !dae_write_skin_bind_source(st, skin, skinIdx)
      || !dae_write_skin_weights(st, skin, mesh, skinIdx)) {
    if (w->result == AK_OK)
      w->result = AK_EINVAL;
    return false;
  }

  dae_w_lit(w, "<joints><input semantic=\"JOINT\" source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME(joints));
  dae_w_lit(w, "\"/><input semantic=\"INV_BIND_MATRIX\" source=\"#");
  dae_w_skin_source_id(w, skinIdx, DAE_EXP_NAME_LIT("bind_poses"));
  dae_w_lit(w, "\"/></joints></skin></controller>");

  return w->result == AK_OK;
}
