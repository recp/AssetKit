/*
 * Skin, morph, and animation playback sample.
 *
 * This does not try to be a full renderer. It shows the data extraction calls
 * an engine would use before sending buffers and baked matrices to GPU code.
 */

#include <cglm/cglm.h>
#include <cglm/struct.h>

#include "sample_common.h"

#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_MAX_JOINTS 4u
#define SAMPLE_MAX_MORPH_BYTES (16u * 1024u * 1024u)

static uint32_t
sample_vertex_count(AkMeshPrimitive *prim) {
  return prim && prim->pos && prim->pos->accessor ? prim->pos->accessor->count : 0u;
}

static void
sample_preview_skin(AkInstanceGeometry *inst, AkMesh *mesh) {
  AkSkin *skin;
  AkMeshPrimitive *prim;
  uint32_t vertex_count;
  uint16_t *joint_ids;
  float *weights;
  size_t filled;
  uint32_t j;

  skin = inst && inst->skinner ? inst->skinner->skin : NULL;
  prim = mesh ? mesh->primitive : NULL;
  vertex_count = sample_vertex_count(prim);
  if (!skin || !prim || !vertex_count)
    return;

  joint_ids = calloc((size_t)vertex_count * SAMPLE_MAX_JOINTS, sizeof(*joint_ids));
  weights = calloc((size_t)vertex_count * SAMPLE_MAX_JOINTS, sizeof(*weights));
  if (!joint_ids || !weights) {
    free(joint_ids);
    free(weights);
    return;
  }

  filled = ak_skinFillWeights(skin, prim, 0u, SAMPLE_MAX_JOINTS, joint_ids, weights);
  printf("skin_playback: joints=%zu vertices=%zu max_joints=%u first_vertex=",
         skin->nJoints,
         filled,
         SAMPLE_MAX_JOINTS);
  for (j = 0; j < SAMPLE_MAX_JOINTS; j++)
    printf(" (%u, %.3g)", joint_ids[j], weights[j]);
  printf("\n");

  if (skin->invBindPoses && skin->nJoints) {
    mat4 inv_bind;
    vec4 origin = {0.0f, 0.0f, 0.0f, 1.0f};
    vec4 bind_origin;

    memcpy(inv_bind, skin->invBindPoses[0], sizeof(inv_bind));
    glm_mat4_mulv(inv_bind, origin, bind_origin);
    printf("skin_matrix_preview: joint0=%s inv_bind_origin=(%.3g %.3g %.3g)\n",
           skin->joints && skin->joints[0] ? sample_or_unnamed(skin->joints[0]->name) : "(none)",
           bind_origin[0],
           bind_origin[1],
           bind_origin[2]);
  }

  free(joint_ids);
  free(weights);
}

static void
sample_preview_morph(AkGeometry *geom, AkInstanceGeometry *inst, AkMesh *mesh) {
  AkInputSemantic inputs[2] = {AK_INPUT_POSITION, AK_INPUT_NORMAL};
  AkMorph *morph;
  AkMorphInspectView *view;
  void *buffer;

  morph = inst && inst->morpher ? inst->morpher->morph : NULL;
  if (!morph || !geom || !mesh)
    return;

  if (ak_morphInspect(geom, morph, inputs, 2u, false, true) != AK_OK || !morph->inspectResult) {
    printf("morph_playback: inspect failed\n");
    return;
  }

  view = morph->inspectResult;
  if (ak_morphInspectPrepareLayout(view, AK_MORPH_NATURAL) != AK_OK) {
    printf("morph_playback: layout failed\n");
    return;
  }

  printf("morph_playback: targets=%u bytes=%zu layout=%s animated_weight0=%.3g\n",
         morph->targetCount,
         view->interleaveTotalBufferSize,
         sample_morph_layout_name(view->layout),
         morph->targetCount ? 0.5f + 0.5f * sinf(1.0f) : 0.0f);

  if (!view->interleaveTotalBufferSize || view->interleaveTotalBufferSize > SAMPLE_MAX_MORPH_BYTES)
    return;

  buffer = malloc(view->interleaveTotalBufferSize);
  if (!buffer)
    return;

  printf("morph_interleave: result=%d\n",
         ak_morphInterleave(geom, morph, AK_MORPH_NATURAL, buffer));
  free(buffer);
}

static int
sample_preview_deformers(AkNode *node, unsigned depth) {
  AkNode *child;
  AkInstanceGeometry *inst;
  AkInstanceNode *node_ref;

  if (depth > 32u)
    return 0;

  for (; node; node = node->next) {
    for (inst = node->geometry; inst; inst = (AkInstanceGeometry *)inst->base.next) {
      AkGeometry *geom = ak_instanceObject(&inst->base);
      AkMesh *mesh = sample_mesh_from_geometry(geom);

      if (!mesh || (!inst->skinner && !inst->morpher))
        continue;

      printf("deformer_node: %s geometry=%s mesh=%s\n",
             sample_or_unnamed(node->name),
             geom ? sample_or_unnamed(geom->name) : "(none)",
             sample_or_unnamed(mesh->name));
      sample_preview_skin(inst, mesh);
      sample_preview_morph(geom, inst, mesh);
      return 1;
    }

    for (child = node->chld; child; child = child->next) {
      if (sample_preview_deformers(child, depth + 1u))
        return 1;
    }

    for (node_ref = node->node; node_ref; node_ref = node_ref->next) {
      AkNode *target = ak_instanceNodeTarget(node_ref);
      if (target && sample_preview_deformers(target, depth + 1u))
        return 1;
    }
  }

  return 0;
}

static int
sample_preview_animation_node(AkDoc *doc, AkNode *node, unsigned depth) {
  AkNode *child;
  AkInstanceNode *node_ref;

  if (depth > 32u)
    return 0;

  for (; node; node = node->next) {
    AkBakedAnimation *baked = ak_nodeBakeAnimation(doc, node);

    if (baked && baked->count) {
      uint32_t mid = baked->count / 2u;
      mat4 m;
      vec4 origin = {0.0f, 0.0f, 0.0f, 1.0f};
      vec4 p;

      memcpy(m, baked->matrices + (size_t)mid * 16u, sizeof(m));
      glm_mat4_mulv(m, origin, p);
      printf("animation_playback: node=%s frames=%u sample_time=%.3g origin=(%.3g %.3g %.3g)\n",
             sample_or_unnamed(node->name),
             baked->count,
             baked->times[mid],
             p[0],
             p[1],
             p[2]);
      ak_free(baked);
      return 1;
    }

    if (baked)
      ak_free(baked);

    for (child = node->chld; child; child = child->next) {
      if (sample_preview_animation_node(doc, child, depth + 1u))
        return 1;
    }

    for (node_ref = node->node; node_ref; node_ref = node_ref->next) {
      AkNode *target = ak_instanceNodeTarget(node_ref);
      if (target && sample_preview_animation_node(doc, target, depth + 1u))
        return 1;
    }
  }

  return 0;
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  AkScene *scene;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  scene = doc->scene ? doc->scene : doc->lib.scenes.first;
  printf("libraries: animations=%zu skins=%u morphs=%u\n",
         ak_animationsCount(doc),
         doc->lib.skins.count,
         doc->lib.morphs.count);

  if (!scene || !scene->node) {
    printf("scene: none\n");
  } else {
    if (!sample_preview_deformers(scene->node->chld, 0u))
      printf("deformer_node: none\n");
    if (!sample_preview_animation_node(doc, scene->node->chld, 0u))
      printf("animation_playback: none\n");
  }

  ak_free(doc);
  return 0;
}
