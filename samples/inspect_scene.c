/*
 * Scene inspection sample.
 *
 * Prints scene roots, nodes, camera/light libraries, and basic animation
 * channels. This is intentionally read-only and uses public AssetKit structs.
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdio.h>

static uint32_t
sample_count_instances(const AkInstanceBase *base) {
  uint32_t count;

  count = 0;
  for (; base; base = base->next)
    count++;

  return count;
}

static uint32_t
sample_count_instance_nodes(const AkInstanceNode *inst) {
  uint32_t count;

  count = 0;
  for (; inst; inst = inst->next)
    count++;

  return count;
}

static void
sample_print_indent(unsigned depth) {
  while (depth--)
    printf("  ");
}

static void
sample_print_node_tree(AkNode *node, unsigned depth) {
  AkNode *child;
  AkInstanceGeometry *geom;
  AkInstanceBase *base;
  AkInstanceNode *nodeRef;
  uint32_t geometryCount;

  if (depth > 32u) {
    sample_print_indent(depth);
    printf("... depth limit reached\n");
    return;
  }

  for (; node; node = node->next) {
    geometryCount = 0;
    for (geom = node->geometry; geom; geom = (AkInstanceGeometry *)geom->base.next)
      geometryCount++;

    sample_print_indent(depth);
    printf("node: %s visible=%s geometries=%u cameras=%u lights=%u node_refs=%u",
           sample_or_unnamed(node->name),
           node->visible ? "yes" : "no",
           geometryCount,
           sample_count_instances(node->camera),
           sample_count_instances(node->light),
           sample_count_instance_nodes(node->node));
    if (node->gpuInstancing)
      printf(" gpu_instances=%u", node->gpuInstancing->count);
    printf("\n");

    for (geom = node->geometry; geom; geom = (AkInstanceGeometry *)geom->base.next) {
      AkGeometry *targetGeom;
      AkMesh *mesh;

      targetGeom = ak_instanceObject(&geom->base);
      mesh       = sample_mesh_from_geometry(targetGeom);

      sample_print_indent(depth + 1u);
      printf("geometry_instance: name=%s geometry=%s mesh=%s primitives=%u bindings=%s morph=%s skin=%s\n",
             sample_or_unnamed(geom->base.name),
             targetGeom ? sample_or_unnamed(targetGeom->name) : "(none)",
             mesh ? sample_or_unnamed(mesh->name) : "(none)",
             mesh ? mesh->primitiveCount : 0u,
             geom->objectBindings ? "yes" : "no",
             geom->morpher ? "yes" : "no",
             geom->skinner ? "yes" : "no");
    }

    for (base = node->camera; base; base = base->next) {
      AkCamera *camera;
      const AkProjection *proj;

      camera = ak_instanceObject(base);
      proj   = camera && camera->optics ? camera->optics->proj : NULL;
      sample_print_indent(depth + 1u);
      printf("camera_instance: name=%s camera=%s projection=%s\n",
             sample_or_unnamed(base->name),
             camera ? sample_or_unnamed(camera->name) : "(none)",
             proj ? sample_projection_type_name(proj->type) : "none");
    }

    for (base = node->light; base; base = base->next) {
      AkLight *light;
      AkLightBase *lightData;

      light     = ak_instanceObject(base);
      lightData = light ? light->data : NULL;
      sample_print_indent(depth + 1u);
      printf("light_instance: name=%s light=%s type=%s\n",
             sample_or_unnamed(base->name),
             light ? sample_or_unnamed(light->name) : "(none)",
             lightData ? sample_light_type_name(lightData->type) : "none");
    }

    for (child = node->chld; child; child = child->next)
      sample_print_node_tree(child, depth + 1u);

    for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
      AkNode *target;

      target = ak_instanceNodeTarget(nodeRef);
      sample_print_indent(depth + 1u);
      printf("node_instance: name=%s target=%s\n",
             sample_or_unnamed(nodeRef->name),
             target ? sample_or_unnamed(target->name) : "(none)");
      if (target)
        sample_print_node_tree(target, depth + 1u);
    }
  }
}

static void
sample_print_cameras(AkDoc *doc) {
  const AkCamera *cam;
  uint64_t index;

  index = 0;
  for (cam = doc->lib.cameras.first; cam; cam = cam->next, index++) {
    const AkProjection *proj;

    proj = cam->optics ? cam->optics->proj : NULL;
    printf("camera %" PRIu64 ": name=%s projection=%s\n",
           index,
           sample_or_unnamed(cam->name),
           proj ? sample_projection_type_name(proj->type) : "none");
    if (!proj)
      continue;

    if (proj->type == AK_PROJECTION_PERSPECTIVE) {
      const AkPerspective *persp = (const AkPerspective *)proj;
      printf("  perspective: xfov=%.6g yfov=%.6g aspect=%.6g znear=%.6g zfar=%.6g\n",
             persp->xfov,
             persp->yfov,
             persp->aspectRatio,
             persp->znear,
             persp->zfar);
    } else if (proj->type == AK_PROJECTION_ORTHOGRAPHIC) {
      const AkOrthographic *ortho = (const AkOrthographic *)proj;
      printf("  orthographic: xmag=%.6g ymag=%.6g aspect=%.6g znear=%.6g zfar=%.6g\n",
             ortho->xmag,
             ortho->ymag,
             ortho->aspectRatio,
             ortho->znear,
             ortho->zfar);
    }
  }
}

static void
sample_print_lights(AkDoc *doc) {
  const AkLight *light;
  uint64_t index;

  index = 0;
  for (light = doc->lib.lights.first; light; light = light->next, index++) {
    const AkLightBase *base;

    base = light->data;
    printf("light %" PRIu64 ": name=%s type=%s\n",
           index,
           sample_or_unnamed(light->name),
           base ? sample_light_type_name(base->type) : "none");
    if (!base)
      continue;

    printf("  color=(%.6g, %.6g, %.6g, %.6g) intensity=%.6g range=%.6g direction=(%.6g, %.6g, %.6g)\n",
           base->color.rgba.R,
           base->color.rgba.G,
           base->color.rgba.B,
           base->color.rgba.A,
           base->intensity,
           base->range,
           base->direction[0],
           base->direction[1],
           base->direction[2]);
    if (base->type == AK_LIGHT_TYPE_SPOT) {
      const AkSpotLight *spot = (const AkSpotLight *)base;
      printf("  spot: inner=%.6g outer=%.6g falloff=%.6g attenuation=(%.6g, %.6g, %.6g)\n",
             spot->innerConeAngle,
             spot->outerConeAngle,
             spot->coneFalloffExponent,
             spot->attenuation.constant,
             spot->attenuation.linear,
             spot->attenuation.quadratic);
    } else if (base->type == AK_LIGHT_TYPE_POINT) {
      const AkPointLight *point = (const AkPointLight *)base;
      printf("  point: attenuation=(%.6g, %.6g, %.6g)\n",
             point->attenuation.constant,
             point->attenuation.linear,
             point->attenuation.quadratic);
    }
  }
}

static void
sample_print_animation(AkAnimation *anim, unsigned depth, uint64_t *index) {
  AkChannel *channel;
  AkAnimSampler *sampler;
  uint32_t channelCount;
  uint32_t samplerCount;

  for (; anim; anim = anim->next) {
    channelCount = 0;
    for (channel = anim->channel; channel; channel = channel->next)
      channelCount++;

    samplerCount = 0;
    for (sampler = anim->sampler; sampler; sampler = (AkAnimSampler *)sampler->base.next)
      samplerCount++;

    sample_print_indent(depth);
    printf("animation %" PRIu64 ": name=%s channels=%u samplers=%u\n",
           (*index)++,
           sample_or_unnamed(anim->name),
           channelCount,
           samplerCount);

    for (channel = anim->channel; channel; channel = channel->next) {
      sample_print_indent(depth + 1u);
      printf("channel: target=%s target_type=%s source=%s partial=%s resolved=%s\n",
             channel->target ? channel->target : "(none)",
             sample_animation_target_name(channel->targetType),
             channel->source.url ? channel->source.url : "(none)",
             ak_channelTargetIsPartial(channel) ? "yes" : "no",
             channel->resolvedTarget && channel->resolvedTarget->target ? "yes" : "no");
    }

    for (sampler = anim->sampler; sampler; sampler = (AkAnimSampler *)sampler->base.next) {
      sample_print_indent(depth + 1u);
      printf("sampler: interpolation=%s inputs=%s/%s/%s\n",
             sample_interpolation_name(sampler->uniInterpolation),
             sampler->inputInput ? "time" : "none",
             sampler->outputInput ? "value" : "none",
             sampler->interpInput ? "interp" : "none");
    }

    if (anim->animation)
      sample_print_animation(anim->animation, depth + 1u, index);
  }
}

static void
sample_print_scenes(AkDoc *doc) {
  const AkScene *scene;
  uint64_t index;

  index = 0;
  for (scene = doc->lib.scenes.first; scene; scene = scene->next, index++) {
    const AkSceneCamera *sceneCamera;
    const AkSceneLight *sceneLight;

    printf("scene %" PRIu64 ": name=%s active=%s unique_cameras=%u camera_uses=%u unique_lights=%u light_uses=%u\n",
           index,
           sample_or_unnamed(scene->name),
           scene == doc->scene ? "yes" : "no",
           scene->cameras.count,
           scene->cameras.useCount,
           scene->lights.count,
           scene->lights.useCount);

    for (sceneCamera = scene->cameras.first; sceneCamera; sceneCamera = sceneCamera->next) {
      printf("  scene_camera: %s uses=%u first_instance=%s\n",
             sceneCamera->camera ? sample_or_unnamed(sceneCamera->camera->name) : "(none)",
             sceneCamera->useCount,
             sceneCamera->firstInstance ? sample_or_unnamed(sceneCamera->firstInstance->name) : "(none)");
    }

    for (sceneLight = scene->lights.first; sceneLight; sceneLight = sceneLight->next) {
      printf("  scene_light: %s uses=%u first_instance=%s\n",
             sceneLight->light ? sample_or_unnamed(sceneLight->light->name) : "(none)",
             sceneLight->useCount,
             sceneLight->firstInstance ? sample_or_unnamed(sceneLight->firstInstance->name) : "(none)");
    }

    sample_print_node_tree(scene->node ? scene->node->chld : NULL, 1u);
  }
}

int
main(int argc, char **argv) {
  AkDoc *doc;
  uint64_t animationIndex;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  printf("summary: scenes=%u nodes=%u cameras=%u lights=%u animations=%u total_animations=%zu\n",
         doc->lib.scenes.count,
         doc->lib.nodes.count,
         doc->lib.cameras.count,
         doc->lib.lights.count,
         doc->lib.animations.count,
         ak_animationsCount(doc));

  sample_print_cameras(doc);
  sample_print_lights(doc);
  sample_print_scenes(doc);

  animationIndex = 0;
  sample_print_animation(doc->lib.animations.first, 0u, &animationIndex);

  ak_free(doc);

  return 0;
}
