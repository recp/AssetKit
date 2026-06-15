/*
 * Skin and morph inspection sample.
 *
 * Shows the runtime path most engines need: fixed-N skin weights for one
 * primitive and an interleaved morph-target buffer layout.
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SAMPLE_MAX_JOINTS 4u
#define SAMPLE_MAX_MORPH_BYTES (16u * 1024u * 1024u)

static void
sample_print_indent(unsigned depth) {
  while (depth--)
    printf("  ");
}

static void
sample_print_weights(const char *label,
                     const AkFloatArray *weights,
                     uint32_t targetCount,
                     unsigned depth) {
  uint32_t count;
  uint32_t i;

  sample_print_indent(depth);
  printf("%s:", label);
  if (!weights || !weights->count) {
    printf(" none\n");
    return;
  }

  count = weights->count < targetCount ? (uint32_t)weights->count : targetCount;
  if (count > 8u)
    count = 8u;
  for (i = 0; i < count; i++)
    printf(" %.6g", weights->items[i]);
  if (weights->count > count)
    printf(" ...");
  printf("\n");
}

static uint32_t
sample_primitive_vertex_count(const AkMeshPrimitive *prim) {
  if (prim && prim->pos && prim->pos->accessor)
    return prim->pos->accessor->count;
  return 0;
}

static void
sample_print_skin_preview(AkSkin *skin,
                          AkMeshPrimitive *prim,
                          uint32_t primIndex,
                          unsigned depth) {
  uint16_t *indices;
  float *weights;
  size_t vertexCount;
  size_t affectedByRoot;
  size_t previewCount;
  size_t v;

  vertexCount = sample_primitive_vertex_count(prim);
  if (!vertexCount) {
    sample_print_indent(depth);
    printf("skin_preview: no position accessor\n");
    return;
  }

  indices = calloc(vertexCount * SAMPLE_MAX_JOINTS, sizeof(*indices));
  weights = calloc(vertexCount * SAMPLE_MAX_JOINTS, sizeof(*weights));
  if (!indices || !weights) {
    sample_print_indent(depth);
    printf("skin_preview: allocation failed\n");
    free(indices);
    free(weights);
    return;
  }

  vertexCount = ak_skinFillWeights(skin,
                                   prim,
                                   primIndex,
                                   SAMPLE_MAX_JOINTS,
                                   indices,
                                   weights);
  affectedByRoot = skin && skin->nJoints
                   ? ak_skinVerticesForJoint(skin, prim, primIndex, 0u, NULL, 0u)
                   : 0u;

  sample_print_indent(depth);
  printf("skin_preview: vertices=%zu max_joints=%u affected_by_joint0=%zu\n",
         vertexCount,
         SAMPLE_MAX_JOINTS,
         affectedByRoot);

  previewCount = vertexCount < 3u ? vertexCount : 3u;
  for (v = 0; v < previewCount; v++) {
    size_t base;
    uint32_t j;

    base = v * SAMPLE_MAX_JOINTS;
    sample_print_indent(depth + 1u);
    printf("vertex %zu:", v);
    for (j = 0; j < SAMPLE_MAX_JOINTS; j++)
      printf(" (%u, %.6g)", indices[base + j], weights[base + j]);
    printf("\n");
  }

  free(indices);
  free(weights);
}

static void
sample_print_skin(AkSkin *skin,
                  AkMesh *mesh,
                  unsigned depth) {
  AkMeshPrimitive *prim;
  uint32_t primIndex;

  sample_print_indent(depth);
  if (!skin) {
    printf("skin: none\n");
    return;
  }

  printf("skin: joints=%zu primitives=%u max_authored_joints=%u skeleton=%s\n",
         skin->nJoints,
         skin->nPrims,
         skin->nMaxJoints,
         skin->skeleton ? sample_or_unnamed(skin->skeleton->name) : "(none)");

  if (skin->joints && skin->nJoints) {
    size_t i;
    size_t previewCount;

    previewCount = skin->nJoints < 6u ? skin->nJoints : 6u;
    sample_print_indent(depth + 1u);
    printf("joints:");
    for (i = 0; i < previewCount; i++)
      printf(" %s", skin->joints[i] ? sample_or_unnamed(skin->joints[i]->name) : "(none)");
    if (previewCount < skin->nJoints)
      printf(" ...");
    printf("\n");
  }

  primIndex = 0;
  for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next, primIndex++) {
    sample_print_skin_preview(skin, prim, primIndex, depth + 1u);
    break;
  }
}

static void
sample_print_morph(AkGeometry *baseGeometry,
                   AkMesh *baseMesh,
                   AkInstanceMorph *inst,
                   unsigned depth) {
  AkInputSemantic desiredInputs[2] = {AK_INPUT_POSITION, AK_INPUT_NORMAL};
  AkMorph *morph;
  AkFloatArray *weights;
  AkMorphInspectView *view;
  AkResult inspectResult;
  AkResult layoutResult;
  uint32_t i;

  sample_print_indent(depth);
  if (!inst || !inst->morph) {
    printf("morph: none\n");
    return;
  }

  morph = inst->morph;
  weights = inst->overrideWeights
            ? inst->overrideWeights
            : (morph->defaultWeights ? morph->defaultWeights
                                     : (baseMesh ? baseMesh->weights : NULL));

  printf("morph: targets=%u method=%s presets=%u override=%s\n",
         morph->targetCount,
         sample_morph_method_name(morph->method),
         morph->presetCount,
         inst->overrideWeights ? "yes" : "no");

  if (morph->targetNames && morph->targetCount) {
    uint32_t previewCount;

    previewCount = morph->targetCount < 8u ? morph->targetCount : 8u;
    sample_print_indent(depth + 1u);
    printf("targets:");
    for (i = 0; i < previewCount; i++)
      printf(" %s", sample_or_unnamed(morph->targetNames[i]));
    if (previewCount < morph->targetCount)
      printf(" ...");
    printf("\n");
  }

  sample_print_weights("weights", weights, morph->targetCount, depth + 1u);

  for (i = 0; i < morph->presetCount; i++) {
    const AkMorphPreset *preset;

    preset = &morph->presets[i];
    sample_print_indent(depth + 1u);
    printf("preset %u: %s", i, sample_or_unnamed(preset->name));
    if (preset->weights && preset->weights->count) {
      uint32_t j;
      uint32_t previewCount;

      previewCount = preset->weights->count < 6u ? (uint32_t)preset->weights->count : 6u;
      printf(" weights:");
      for (j = 0; j < previewCount; j++)
        printf(" %.6g", preset->weights->items[j]);
      if (previewCount < preset->weights->count)
        printf(" ...");
    }
    printf("\n");
  }

  inspectResult = ak_morphInspect(baseGeometry,
                                  morph,
                                  desiredInputs,
                                  2u,
                                  false,
                                  true);
  view = morph->inspectResult;
  sample_print_indent(depth + 1u);
  if (inspectResult != AK_OK || !view) {
    printf("morph_buffer: inspect failed result=%d\n", inspectResult);
    return;
  }

  layoutResult = ak_morphInspectPrepareLayout(view, AK_MORPH_NATURAL);
  printf("morph_buffer: targets=%u bytes=%zu layout=%s include_base=%s\n",
         view->nTargets,
         view->interleaveTotalBufferSize,
         sample_morph_layout_name(view->layout),
         view->includeBaseShape ? "yes" : "no");
  if (layoutResult != AK_OK) {
    sample_print_indent(depth + 1u);
    printf("morph_layout: prepare failed result=%d\n", layoutResult);
    return;
  }

  if (view->interleaveTotalBufferSize
      && view->interleaveTotalBufferSize <= SAMPLE_MAX_MORPH_BYTES) {
    void *buffer;
    AkResult interleaveResult;

    buffer = malloc(view->interleaveTotalBufferSize);
    if (!buffer) {
      sample_print_indent(depth + 1u);
      printf("morph_interleave: allocation failed\n");
      return;
    }

    interleaveResult = ak_morphInterleave(baseGeometry,
                                          morph,
                                          AK_MORPH_NATURAL,
                                          buffer);
    sample_print_indent(depth + 1u);
    printf("morph_interleave: result=%d layout=%s\n",
           interleaveResult,
           sample_morph_layout_name(view->layout));
    free(buffer);
  }
}

static void
sample_print_deformer_instance(AkNode *node,
                               AkInstanceGeometry *inst,
                               unsigned depth,
                               uint64_t *instanceCount) {
  AkGeometry *geom;
  AkMesh *mesh;
  AkSkin *skin;
  AkMorph *morph;

  geom = ak_instanceObject(&inst->base);
  mesh = sample_mesh_from_geometry(geom);
  skin = inst->skinner ? inst->skinner->skin : NULL;
  morph = inst->morpher ? inst->morpher->morph : NULL;

  if (!skin && !morph)
    return;

  (*instanceCount)++;
  sample_print_indent(depth);
  printf("deformer_instance: node=%s geometry=%s mesh=%s skin=%s morph=%s\n",
         sample_or_unnamed(node->name),
         geom ? sample_or_unnamed(geom->name) : "(none)",
         mesh ? sample_or_unnamed(mesh->name) : "(none)",
         skin ? "yes" : "no",
         morph ? "yes" : "no");

  sample_print_skin(skin, mesh, depth + 1u);
  sample_print_morph(geom, mesh, inst->morpher, depth + 1u);
}

static void
sample_walk_nodes(AkNode *node,
                  unsigned depth,
                  uint64_t *instanceCount) {
  AkNode *child;
  AkInstanceGeometry *geom;
  AkInstanceNode *nodeRef;

  if (depth > 32u) {
    sample_print_indent(depth);
    printf("... depth limit reached\n");
    return;
  }

  for (; node; node = node->next) {
    for (geom = node->geometry; geom; geom = (AkInstanceGeometry *)geom->base.next)
      sample_print_deformer_instance(node, geom, depth, instanceCount);

    for (child = node->chld; child; child = child->next)
      sample_walk_nodes(child, depth + 1u, instanceCount);

    for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
      AkNode *target;

      target = ak_instanceNodeTarget(nodeRef);
      if (target)
        sample_walk_nodes(target, depth + 1u, instanceCount);
    }
  }
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  AkScene *scene;
  uint64_t instanceCount;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  printf("deformer_libraries: skins=%u morphs=%u\n",
         doc->lib.skins.count,
         doc->lib.morphs.count);

  instanceCount = 0;
  for (scene = doc->lib.scenes.first; scene; scene = scene->next) {
    printf("scene: %s active=%s\n",
           sample_or_unnamed(scene->name),
           scene == doc->scene ? "yes" : "no");
    sample_walk_nodes(scene->node ? scene->node->chld : NULL, 1u, &instanceCount);
  }

  printf("deformer_instances=%" PRIu64 "\n", instanceCount);
  ak_free(doc);

  return 0;
}
