/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "angle.h"

/*
 COLLADA uses degress for all angles, convert desgress to radians. It may exists
 in these places as far as I know:

 1. Rotate element - fixed in place
 2. Perspective (xfov, yfov) - fixed in place
 3. Light (fallofAngle) - fixed in place
 4. Skew element - fixed in place
 5. Animation data (output, tangents...)!!!! - NEEDS TO BE FIXED?

 */

void _assetkit_hide
dae_cvtAngles(AkAccessor * __restrict acc,
              AkBuffer   * __restrict buff,
              const char * __restrict paramName) {
  AkDataParam *param;
  float       *pbuff;
  size_t       po, i, count, st;

  if (acc->componentType == AKT_FLOAT && (param = acc->param)) {
    po          = 0;
    st          = acc->stride;
    count       = acc->count * st;
    pbuff       = buff->data;

    do {
      if (param->name && strcasecmp(param->name, paramName) == 0) {
        /* TODO: use SIMD */
        for (i = po; i < count; i += st)
          glm_make_rad(pbuff + i);
      }
      po++;
    } while ((param = param->next));
  }
}

/* TODO: This works for BERZIER but HERMITE?? */

void _assetkit_hide
dae_fixAngles(AkXmlState * __restrict xst) {
  FListItem     *item;
  AkAnimSampler *sampler;
  AkDataParam   *param;
  AkAccessor    *acc;
  AkBuffer      *buff;

  item = xst->toRadiansSampelers;
  while (item) {
    sampler = item->data;
    acc     = NULL;
    buff    = NULL;

    if ((acc = sampler->outputInput->accessor)
        && acc->type
        && (buff = ak_getObjectByUrl(&acc->source))) {
      bool foundAngle;

      foundAngle = false;

      if ((param = acc->param)) {
        do {
          if (param->name && strcasecmp(param->name, _s_dae_angle) == 0)
            foundAngle = true;
        } while ((param = param->next));
      }

      if (!foundAngle)
        goto nxt_sampler;

      dae_cvtAngles(acc, buff, _s_dae_angle);

      /* convert in tangents to radians */
      if ((acc = sampler->inTangentInput->accessor)
          && acc->type
          && (buff = ak_getObjectByUrl(&acc->source))) {
        if (acc->param && acc->param->next)
          dae_cvtAngles(acc, buff, acc->param->next->name);
        else if (acc->param) /* 1D tangents */
          dae_cvtAngles(acc, buff, acc->param->name);
      }

      /* convert out tangents to radians */
      if ((acc = sampler->outTangentInput->accessor)
          && acc->type
          && (buff = ak_getObjectByUrl(&acc->source))) {
        if (acc->param && acc->param->next)
          dae_cvtAngles(acc, buff, acc->param->next->name);
        else if (acc->param) /* 1D tangents */
          dae_cvtAngles(acc, buff, acc->param->name);
      }
    }

  nxt_sampler:
    item = item->next;
  }

  flist_sp_destroy(&xst->toRadiansSampelers);
}
