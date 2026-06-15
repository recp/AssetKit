/*
 * Animation inspection sample.
 *
 * Shows animation channels, sampler accessors, conflict-free playback sets,
 * and node-local matrix baking for runtimes that do not want to evaluate
 * partial transform channels themselves.
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

static void
sample_print_indent(unsigned depth) {
  while (depth--)
    printf("  ");
}

static size_t
sample_collect_animations(AkAnimation *anim,
                          AkAnimation **items,
                          size_t capacity,
                          size_t count) {
  for (; anim; anim = anim->next) {
    if (count < capacity)
      items[count] = anim;
    count++;
    if (anim->animation)
      count = sample_collect_animations(anim->animation, items, capacity, count);
  }

  return count;
}

static uint32_t
sample_count_channels(const AkAnimation *anim) {
  const AkChannel *channel;
  uint32_t count;

  count = 0;
  for (channel = anim ? anim->channel : NULL; channel; channel = channel->next)
    count++;
  return count;
}

static uint32_t
sample_count_samplers(const AkAnimation *anim) {
  const AkAnimSampler *sampler;
  uint32_t count;

  count = 0;
  for (sampler = anim ? anim->sampler : NULL;
       sampler;
       sampler = (const AkAnimSampler *)sampler->base.next)
    count++;
  return count;
}

static void
sample_print_accessor_brief(const char *label, const AkInput *input) {
  const AkAccessor *acc;

  acc = input ? input->accessor : NULL;
  printf("%s=", label);
  if (!acc) {
    printf("none");
    return;
  }

  printf("count:%u comps:%u type:%s",
         acc->count,
         acc->componentCount,
         sample_component_type_name(acc->componentType));
}

static void
sample_print_animation(AkAnimation *anim, unsigned depth, uint64_t *index) {
  AkChannel *channel;
  AkAnimSampler *sampler;

  for (; anim; anim = anim->next) {
    sample_print_indent(depth);
    printf("animation %" PRIu64 ": name=%s channels=%u samplers=%u nested=%s\n",
           (*index)++,
           sample_or_unnamed(anim->name),
           sample_count_channels(anim),
           sample_count_samplers(anim),
           anim->animation ? "yes" : "no");

    for (channel = anim->channel; channel; channel = channel->next) {
      sample_print_indent(depth + 1u);
      printf("channel: target=%s attr=%s type=%s source=%s partial=%s resolved=%s\n",
             channel->target ? channel->target : "(none)",
             ak_channelTargetAttr(channel) ? ak_channelTargetAttr(channel) : "(whole)",
             sample_animation_target_name(channel->targetType),
             channel->source.url ? channel->source.url : "(none)",
             ak_channelTargetIsPartial(channel) ? "yes" : "no",
             channel->resolvedTarget && channel->resolvedTarget->target ? "yes" : "no");
    }

    for (sampler = anim->sampler; sampler; sampler = (AkAnimSampler *)sampler->base.next) {
      sample_print_indent(depth + 1u);
      printf("sampler: interpolation=%s pre=%d post=%d ",
             sample_interpolation_name(sampler->uniInterpolation),
             (int)sampler->pre,
             (int)sampler->post);
      sample_print_accessor_brief("time", sampler->inputInput);
      printf(" ");
      sample_print_accessor_brief("value", sampler->outputInput);
      printf(" ");
      sample_print_accessor_brief("interp", sampler->interpInput);
      printf("\n");
    }

    if (anim->animation)
      sample_print_animation(anim->animation, depth + 1u, index);
  }
}

static void
sample_print_compatible_set(AkDoc *doc, AkAnimation **items, size_t count) {
  AkContext ctx;
  AkAnimation **compatible;
  size_t compatibleCount;
  size_t i;

  if (!count)
    return;

  compatible = calloc(count + 1u, sizeof(*compatible));
  if (!compatible) {
    printf("compatible_set: allocation failed\n");
    return;
  }

  ctx = AkContextZeroed();
  ctx.doc = doc;
  compatibleCount = ak_animationsCompatibleSetFromDoc(&ctx, doc, items[0], compatible);

  printf("compatible_set primary=%s count=%zu:",
         sample_or_unnamed(items[0]->name),
         compatibleCount);
  for (i = 0; i < compatibleCount; i++)
    printf(" %s", sample_or_unnamed(compatible[i]->name));
  printf("\n");

  free(compatible);
}

static AkBakedAnimation *
sample_find_baked_node(AkDoc *doc, AkNode *node, const char **nodeName, unsigned depth) {
  AkNode *child;
  AkInstanceNode *nodeRef;

  if (depth > 32u)
    return NULL;

  for (; node; node = node->next) {
    AkBakedAnimation *baked;

    baked = ak_nodeBakeAnimation(doc, node);
    if (baked && baked->count > 0) {
      *nodeName = node->name;
      return baked;
    }
    if (baked)
      ak_free(baked);

    for (child = node->chld; child; child = child->next) {
      baked = sample_find_baked_node(doc, child, nodeName, depth + 1u);
      if (baked)
        return baked;
    }

    for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
      AkNode *target;

      target = ak_instanceNodeTarget(nodeRef);
      if (!target)
        continue;

      baked = sample_find_baked_node(doc, target, nodeName, depth + 1u);
      if (baked)
        return baked;
    }
  }

  return NULL;
}

static void
sample_print_baked_preview(AkDoc *doc) {
  AkScene *scene;

  for (scene = doc ? doc->lib.scenes.first : NULL; scene; scene = scene->next) {
    AkBakedAnimation *baked;
    const char *nodeName;
    uint32_t i;
    uint32_t previewCount;

    nodeName = NULL;
    baked = sample_find_baked_node(doc, scene->node ? scene->node->chld : NULL, &nodeName, 0u);
    if (!baked)
      continue;

    printf("baked_node: scene=%s node=%s frames=%u\n",
           sample_or_unnamed(scene->name),
           sample_or_unnamed(nodeName),
           baked->count);

    previewCount = baked->count < 3u ? baked->count : 3u;
    for (i = 0; i < previewCount; i++) {
      const float *m;

      m = baked->matrices + (size_t)i * 16u;
      printf("  frame %u: time=%.6g translation=(%.6g, %.6g, %.6g)\n",
             i,
             baked->times[i],
             m[12],
             m[13],
             m[14]);
    }

    ak_free(baked);
    return;
  }

  printf("baked_node: none\n");
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  AkAnimation **items;
  uint64_t animationIndex;
  size_t count;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  count = ak_animationsCount(doc);
  printf("animations: top_level=%u total=%zu\n",
         doc->lib.animations.count,
         count);

  items = count ? calloc(count, sizeof(*items)) : NULL;
  if (count && !items) {
    fprintf(stderr, "animation list allocation failed\n");
    ak_free(doc);
    return 1;
  }
  sample_collect_animations(doc->lib.animations.first, items, count, 0u);

  animationIndex = 0;
  sample_print_animation(doc->lib.animations.first, 0u, &animationIndex);
  sample_print_compatible_set(doc, items, count);
  sample_print_baked_preview(doc);

  free(items);
  ak_free(doc);

  return 0;
}
