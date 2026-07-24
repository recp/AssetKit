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

#include "../../../../include/ak/assetkit.h"
#include "../common.h"
#include "dae14.h"
#include "../bugfix/texture.h"
#include "../../../string_fast.h"

static
AkNewParam*
dae14_find_newparam_by_sid(AkNewParam *param, const char *sid) {
  while (param) {
    const char *paramSid;

    paramSid = ak_sid_get(param);
    if (paramSid && strcmp(paramSid, sid) == 0)
      return param;

    param = param->next;
  }

  return NULL;
}

static
AkNewParam*
dae14_find_newparam(void *parent, const char *sid) {
  if (!sid)
    return NULL;

  while (parent) {
    switch (ak_typeid(parent)) {
      case AKT_PROFILE: {
        AkNewParam *param;

        param = dae14_find_newparam_by_sid(((AkProfile *)parent)->newparam, sid);
        if (param)
          return param;
        break;
      }
      case AKT_EFFECT: {
        AkNewParam *param;

        param = dae14_find_newparam_by_sid(((AkEffect *)parent)->newparam, sid);
        if (param)
          return param;
        break;
      }
      default:
        break;
    }

    parent = ak_mem_parent(parent);
  }

  return NULL;
}

static
AkImage*
dae14_find_image_by_id_n(AkDoc * __restrict doc,
                         const char * __restrict id,
                         size_t                  idLen) {
  AkImage *image;

  if (!doc || !id || idLen == 0)
    return NULL;

  for (image = doc->lib.images.first; image; image = image->next) {
    const char *imageId;

    imageId = ak_getId(image);
    if (ak_str_eq_cstr_fast(imageId, id, idLen))
      return image;
  }

  return NULL;
}

static
AkImage*
dae14_find_image_for_surface_source(DAEState * __restrict dst,
                                    const char * __restrict sid) {
  static const char suffix[] = "-surface";
  AkDoc            *doc;
  AkImage          *image;
  size_t            sidLen;
  size_t            suffixLen;

  if (!dst || !sid || !*sid)
    return NULL;

  doc    = dst->doc;
  sidLen = strlen(sid);
  if ((image = dae14_find_image_by_id_n(doc, sid, sidLen)))
    return image;
  if ((image = dae_bugfix_texture_image_by_ref(dst, sid)))
    return image;

  suffixLen = sizeof(suffix) - 1;
  if (sidLen > suffixLen
      && ak_str_eq_fast(sid + sidLen - suffixLen,
                        suffixLen,
                        suffix,
                        suffixLen)) {
    char imageRef[PATH_MAX];

    if (sidLen - suffixLen >= sizeof(imageRef))
      return NULL;

    memcpy(imageRef, sid, sidLen - suffixLen);
    imageRef[sidLen - suffixLen] = '\0';
    if ((image = dae14_find_image_by_id_n(doc,
                                          imageRef,
                                          sidLen - suffixLen)))
      return image;
    if ((image = dae_bugfix_texture_image_by_ref(dst, imageRef)))
      return image;
  }

  return NULL;
}

static
AkInstanceBase*
dae14_make_image_instance(DAEState * __restrict dst,
                          void     * __restrict parent,
                          const char * __restrict imageId) {
  AkInstanceBase *instanceImage;

  if (!imageId)
    return NULL;

  instanceImage = ak_heap_calloc(dst->heap, parent, sizeof(*instanceImage));
  if (!instanceImage)
    return NULL;

  ak_url_init_with_id(dst->heap->allocator,
                      instanceImage,
                      (char *)imageId,
                      &instanceImage->url);

  return instanceImage;
}

AK_HIDE
void
dae14_loadjobs_add(DAEState   * __restrict  dst,
                   void       *  __restrict parent,
                   void       * __restrict  value,
                   AkDae14LoadJobType       type) {
  AkDae14LoadJob *job, *last;

  if (!dst)
    return;

  job = ak_heap_calloc(dst->heap,
                       NULL,
                       sizeof(*job));
  job->parent = parent;
  job->type   = type;
  job->value  = value;

  last = dst->jobs14;
  if (last)
    last->prev = job;

  job->next   = dst->jobs14;
  dst->jobs14 = job;
}

AK_HIDE
void
dae14_loadjobs_finish(DAEState * __restrict dst) {
  AkDae14LoadJob *job;

  job = dst->jobs14;
  while (job) {
    switch (job->type) {
      case AK_DAE14_LOADJOB_SURFACE: {
        AkNewParam     *surfaceParam;
        AkDae14Surface *surface;

        surfaceParam = dae14_find_newparam(job->parent, job->value);
        if (surfaceParam && surfaceParam->val) {
          AkSampler      *sampler;
          AkInstanceBase *instanceImage;

          surface = surfaceParam->val->value;
          if (!surface)
            break;

          /* surface may already migrated to 1.5+ */
          if (!surface->instanceImage) {
            AkImage *image;
            const char *imageId;

            sampler = job->parent;

            /* convert initFrom to instance_image */
            instanceImage = ak_heap_calloc(dst->heap,
                                           sampler,
                                           sizeof(*instanceImage));
            if (surface->extra) {
              instanceImage->extra = surface->extra;
              surface->extra       = NULL;
              ak_mem_setp(instanceImage->extra,
                          instanceImage);
            }

            rb_insert(dst->instanceMap, sampler, instanceImage);
            if (surface->initFrom && surface->initFrom->image) {
              image = dae14_find_image_by_id_n(dst->doc,
                                               surface->initFrom->image,
                                               strlen(surface->initFrom->image));
              if (!image)
                image = dae_bugfix_texture_image_by_ref(
                  dst,
                  surface->initFrom->image);
              imageId = image ? ak_getId(image) : surface->initFrom->image;
              ak_url_init_with_id(dst->heap->allocator,
                                  instanceImage,
                                  (char *)imageId,
                                  &instanceImage->url);
              if (image)
                dae_bugfix_texture_image_path(dst, image);
            }

            /* TODO: */
//            sampler->instanceImage = instanceImage;
            surface->instanceImage = instanceImage;

            /* convert other params to update/new image */
            image = ak_instanceObject(instanceImage);
            if (image) {
              if (surface->initFrom && image->source) {
                image->source->face       = surface->initFrom->face;
                image->source->mipIndex   = surface->initFrom->mip;
                image->source->arrayIndex = surface->initFrom->slice;
              }

              image->renderable = surface->initAsTarget;
            }
          }
        } else {
          AkSampler       *sampler;
          AkInstanceBase  *instanceImage;
          AkImage         *image;

          image = dae14_find_image_for_surface_source(dst, job->value);
          if (!image)
            break;

          sampler       = job->parent;
          dae_bugfix_texture_image_path(dst, image);
          instanceImage = dae14_make_image_instance(dst,
                                                     sampler,
                                                     ak_getId(image));
          if (instanceImage)
            rb_insert(dst->instanceMap, sampler, instanceImage);
        }

        break;
      }
    }

    job = job->next;
  }

  /* cleanup */
  job = dst->jobs14;
  while (job) {
    AkDae14LoadJob *tofree;
    
    tofree = job;
    switch (job->type) {
      case AK_DAE14_LOADJOB_SURFACE: {
        AkNewParam     *surfaceParam;
        AkDae14Surface *surface;

        surfaceParam = dae14_find_newparam(job->parent, job->value);
        if (surfaceParam && surfaceParam->val) {
          void *parentOfParam;

          parentOfParam = ak_mem_parent(surfaceParam);

          surface = surfaceParam->val->value;
          if (!surface)
            break;
          ak_free(surface);

          if (surfaceParam->prev)
            surfaceParam->prev->next = surfaceParam->next;

          if (surfaceParam->next)
            surfaceParam->next->prev = surfaceParam->prev;

          switch (ak_typeid(parentOfParam)) {
            case AKT_EFFECT: {
              AkEffect *effect;
              effect = parentOfParam;
              if (effect->newparam == surfaceParam)
                effect->newparam = surfaceParam->next;
              break;
            }
            case AKT_PROFILE: {
              AkProfile *profile;
              profile = parentOfParam;
              if (profile->newparam == surfaceParam)
                profile->newparam = surfaceParam->next;
              break;
            }

            default: break;
          }

          ak_free(surfaceParam);
        }

        break;
      }
    }

    job = job->next;

    ak_free(tofree->value);
    ak_free(tofree);
  }

  dst->jobs14 = NULL;
}
