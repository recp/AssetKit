/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "../../../include/assetkit.h"
#include "../dae_common.h"
#include "dae14.h"

void
_assetkit_hide
ak_dae14_loadjobs_add(AkXmlState * __restrict  xst,
                      void       *  __restrict parent,
                      void       * __restrict  value,
                      AkDae14LoadJobType       type) {
  AkDae14LoadJob *job, *last;

  if (!xst)
    return;

  job = ak_heap_calloc(xst->heap,
                       NULL,
                       sizeof(*job));
  job->parent = parent;
  job->type   = type;
  job->value  = value;

  last = xst->jobs14;
  if (last)
    last->prev = job;

  job->next   = xst->jobs14;
  xst->jobs14 = job;
}

void
_assetkit_hide
ak_dae14_loadjobs_finish(AkXmlState * __restrict xst) {
  AkDae14LoadJob *job;

  job = xst->jobs14;
  while (job) {
    switch (job->type) {
      case AK_DAE14_LOADJOB_SURFACE: {
        AkNewParam     *surfaceParam;
        AkDae14Surface *surface;
        AkContext       ctx;

        memset(&ctx, 0, sizeof(ctx));
        ctx.doc = xst->doc;

        surfaceParam = ak_sid_resolve(&ctx, job->value);
        if (surfaceParam) {
          AkSampler      *sampler;
          AkInstanceBase *instanceImage;

          surface = surfaceParam->val->value;

          /* surface may already migrated to 1.5+ */
          if (!surface->instanceImage) {
            AkImage *image;

            sampler = job->parent;

            /* convert initFrom to instance_image */
            instanceImage = ak_heap_calloc(xst->heap,
                                           sampler,
                                           sizeof(*instanceImage));
            if (surface->extra) {
              instanceImage->extra = surface->extra;
              surface->extra       = NULL;
              ak_mem_setp(instanceImage->extra,
                          instanceImage);
            }

            ak_url_init_with_id(xst->heap->allocator,
                                instanceImage,
                                (char *)surface->initFrom->image,
                                &instanceImage->url);

            sampler->instanceImage = instanceImage;
            surface->instanceImage = instanceImage;

            /* convert other params to update/new image */
            image = ak_instanceObject(instanceImage);
            if (image) {
              if (surface->initFrom) {
                image->initFrom->face       = surface->initFrom->face;
                image->initFrom->mipIndex   = surface->initFrom->mip;
                image->initFrom->arrayIndex = surface->initFrom->slice;
              }

              image->renderable = surface->initAsTarget;
            }
          }
        }

        break;
      }
    }

    job = job->next;
  }

  /* cleanup */
  job = xst->jobs14;
  while (job) {
    AkDae14LoadJob *tofree;
    
    tofree = job;
    switch (job->type) {
      case AK_DAE14_LOADJOB_SURFACE: {
        AkNewParam     *surfaceParam;
        AkDae14Surface *surface;
        AkContext       ctx;

        memset(&ctx, 0, sizeof(ctx));
        ctx.doc = xst->doc;

        surfaceParam = ak_sid_resolve(&ctx, job->value);
        if (surfaceParam) {
          void *parentOfParam;

          parentOfParam = ak_mem_parent(surfaceParam);

          surface = surfaceParam->val->value;
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

  xst->jobs14 = NULL;
}
