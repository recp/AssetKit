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

#include "cam.h"
#include "asset.h"
#include "techn.h"

AK_HIDE
void*
dae_cam(DAEState * __restrict dst,
        xml_t    * __restrict xml,
        void     * __restrict memp) {
  AkHeap      *heap;
  AkCamera    *cam;
  AkTechnique *tq;

  heap      = dst->heap;
  cam       = ak_heap_calloc(heap, memp, sizeof(*cam));
  ak_setypeid(cam, AKT_CAMERA);
  cam->name = DAE_XMLA_STRDUP8(xml, heap, name, cam);

  xmla_setid(xml, heap, cam);
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, cam, NULL);
    } else if (DAE_XML_TAG_EQ8(xml, optics)) {
      AkOptics *optics;
      xml_t    *xoptics;

      optics  = ak_heap_calloc(heap, cam, sizeof(*optics));
      xoptics = xml->val;

      while (xoptics) {
        if (DAE_XML_TAG_EQ(xoptics, techniquec)) {
          xml_t *xtech, *xtechv;

          xtech = xoptics->val;

          while (xtech) {
            if (DAE_XML_TAG_EQ(xtech, perspective)) {
              AkPerspective *persp;

              persp  = ak_heap_calloc(heap, optics, sizeof(*persp));
              xtechv = xtech->val;

              while (xtechv) {
                if (DAE_XML_TAG_EQ8(xtechv, xfov)) {
                  sid_seta(xtechv, heap, persp, &persp->xfov);
                  persp->xfov = glm_rad(xml_float(xtechv, 0.0f));
                } else if (DAE_XML_TAG_EQ8(xtechv, yfov)) {
                  sid_seta(xtechv, heap, persp, &persp->yfov);
                  persp->yfov = glm_rad(xml_float(xtechv, 0.0f));
                } else if (DAE_XML_TAG_EQ(xtechv, aspect_ratio)) {
                  sid_seta(xtechv, heap, persp, &persp->aspectRatio);
                  persp->aspectRatio = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ8(xtechv, znear)) {
                  sid_seta(xtechv, heap, persp, &persp->znear);
                  persp->znear = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ8(xtechv, zfar)) {
                  sid_seta(xtechv, heap, persp, &persp->zfar);
                  persp->zfar = xml_float(xtechv, 0.0f);
                }
                xtechv = xtechv->next;
              }

              persp->base.type = AK_PROJECTION_PERSPECTIVE;
              if (!persp->aspectRatio && persp->yfov && persp->xfov) {
                persp->aspectRatio = persp->xfov / persp->yfov;
              } else if (!persp->yfov && persp->aspectRatio && persp->xfov) {
                persp->yfov = persp->xfov / persp->aspectRatio;
              } else if (!persp->xfov && persp->aspectRatio && persp->yfov) {
                persp->xfov = persp->yfov * persp->aspectRatio;
              }

              optics->proj = &persp->base;
            } else if (DAE_XML_TAG_EQ(xtech, orthographic)) {
              AkOrthographic *ortho;

              ortho = ak_heap_calloc(heap, optics, sizeof(*ortho));
              xtechv = xtech->val;

              while (xtechv) {
                if (DAE_XML_TAG_EQ8(xtechv, xmag)) {
                  sid_seta(xtechv, heap, ortho, &ortho->xmag);
                  ortho->xmag = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ8(xtechv, ymag)) {
                  sid_seta(xtechv, heap, ortho, &ortho->ymag);
                  ortho->ymag = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ(xtechv, aspect_ratio)) {
                  sid_seta(xtechv, heap, ortho, &ortho->aspectRatio);
                  ortho->aspectRatio = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ8(xtechv, znear)) {
                  sid_seta(xtechv, heap, ortho, &ortho->znear);
                  ortho->znear = xml_float(xtechv, 0.0f);
                } else if (DAE_XML_TAG_EQ8(xtechv, zfar)) {
                  sid_seta(xtechv, heap, ortho, &ortho->zfar);
                  ortho->zfar = xml_float(xtechv, 0.0f);
                }
                xtechv = xtechv->next;
              }

              ortho->base.type = AK_PROJECTION_ORTHOGRAPHIC;
              if (!ortho->aspectRatio && ortho->ymag && ortho->xmag) {
                ortho->aspectRatio = ortho->xmag / ortho->ymag;
              } else if (!ortho->ymag && ortho->aspectRatio && ortho->xmag) {
                ortho->ymag = ortho->xmag / ortho->aspectRatio;
              } else if (!ortho->xmag && ortho->aspectRatio && ortho->ymag) {
                ortho->xmag = ortho->ymag * ortho->aspectRatio;
              }
              
              optics->proj = &ortho->base;
            }
            xtech = xtech->next;
          }
        } else if (DAE_XML_TAG_EQ(xoptics, technique)) {
          tq       = dae_techn(xoptics, heap, optics);
          tq->next = (AkTechnique *)optics->reserved;
          optics->reserved = tq;
        }
        xoptics = xoptics->next;
      }

      cam->optics = optics;
    } else if (DAE_XML_TAG_EQ8(xml, imager)) {
      AkImager *imager;
      xml_t    *ximager;

      imager  = ak_heap_calloc(heap, cam, sizeof(*imager));
      ximager = xml->val;

      while (ximager) {
        if (DAE_XML_TAG_EQ(ximager, technique)) {
          tq       = dae_techn(ximager, heap, imager);
          tq->next = (AkTechnique *)imager->reserved;
          imager->reserved = tq;
        } else if (DAE_XML_TAG_EQ8(ximager, extra)) {
          imager->extra = tree_fromxml(heap, imager, xml);
        }
        ximager = ximager->next;
      }

      cam->imager = imager;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      cam->extra = tree_fromxml(heap, cam, xml);
    }

    xml = xml->next;
  }

  return cam;
}
