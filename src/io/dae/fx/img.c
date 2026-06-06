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

#include "img.h"
#include "../core/asset.h"
#include "../1.4/image.h"
#include "../core/enum.h"

static
AkImageSource*
dae_imageSource(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp);

static
AkImageFormat*
dae_imageFormat(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp);

static
AkImage2d*
dae_create2d(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

static
AkImage3d*
dae_create3d(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp);

static
AkImageCube*
dae_createCube(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp);

AK_HIDE
void*
dae_image(DAEState * __restrict dst,
          xml_t    * __restrict xml,
          void     * __restrict memp) {
  AkHeap     *heap;
  AkImage    *img;
  xml_attr_t *att;

  if (dst->version < AK_COLLADA_VERSION_150) {
    dae14_fxMigrateImg(dst, xml, memp);
    return NULL;
  }

  heap = dst->heap;
  img  = ak_heap_calloc(heap, memp, sizeof(*img));

  xmla_setid(xml, heap, img);
  sid_set(xml, heap, img);
  
  img->name = DAE_XMLA_STRDUP8(xml, heap, name, img);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ8(xml, asset)) {
      (void)dae_asset(dst, xml, img, NULL);
    } else if (DAE_XML_TAG_EQ(xml, renderable)) {
      img->renderable = (att = DAE_XMLA8(xml, share))
                          && att->val
                          && strcasecmp(att->val, _s_dae_true) == 0;
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      img->source = dae_imageSource(dst, xml, img);
    } else if (DAE_XML_TAG_EQ(xml, create_2d)) {
      AkImage2d *image2d;
      if ((image2d = dae_create2d(dst, xml, img)))
          img->image = &image2d->base;
    } else if (DAE_XML_TAG_EQ(xml, create_3d)) {
      AkImage3d *image3d;
      if ((image3d = dae_create3d(dst, xml, img)))
          img->image = &image3d->base;
    } else if (DAE_XML_TAG_EQ(xml, create_cube)) {
      AkImageCube *imageCube;
      if ((imageCube = dae_createCube(dst, xml, img)))
          img->image = &imageCube->base;
    } else if (DAE_XML_TAG_EQ8(xml, extra)) {
      img->extra = tree_fromxml(heap, img, xml);
      if (img->extra)
        ak_extra_set(img, img->extra);
    }
    xml = xml->next;
  }

  return img;
}

AK_HIDE
AkInstanceBase*
dae_instImage(DAEState * __restrict dst,
              xml_t    * __restrict xml,
              void     * __restrict memp) {
  AkHeap         *heap;
  AkInstanceBase *instImg;

  heap          = dst->heap;
  instImg       = ak_heap_calloc(heap, memp, sizeof(*instImg));
  instImg->name = DAE_XMLA_STRDUP8(xml, heap, name, instImg);
  
  sid_set(xml, heap, instImg);
  DAE_URL_SET(dst, xml, url, instImg, &instImg->url);

  return instImg;
}

static
AkImageSource*
dae_imageSource(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp) {
  AkHeap        *heap;
  AkImageSource *source;
  AkHexData     *hex;
  xml_attr_t    *att;
  
  heap   = dst->heap;
  source = ak_heap_calloc(heap, memp, sizeof(*source));
  
  source->generateMips = xmla_u32(DAE_XMLA(xml, mips_generate), 0);
  source->arrayIndex   = xmla_u32(DAE_XMLA(xml, array_index), 0);
  source->mipIndex     = xmla_u32(DAE_XMLA(xml, mip_index), 0);
  source->depth        = xmla_u32(DAE_XMLA8(xml, depth), 0);

  if ((att = DAE_XMLA4(xml, face)) && att->val)
    source->face = dae_face(att);

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ4(xml, ref)) {
      source->uri  = xml_strdup(xml, heap, source);
      source->type = AK_IMAGE_SOURCE_URI;
    } else if (DAE_XML_TAG_EQ4(xml, hex)) {
      hex         = ak_heap_calloc(heap, source, sizeof(*hex));
      hex->format = xmla_strdup(DAE_XMLA8(xml, format), heap, hex);

      if (hex->format) {
        hex->hexval = xml_strdup(xml, heap, source);
        source->hex  = hex;
        source->type = AK_IMAGE_SOURCE_HEX;
      } else {
        ak_free(hex);
      }
    }
    xml = xml->next;
  }
  
  return source;
}

static
AkImageFormat*
dae_imageFormat(DAEState * __restrict dst,
                xml_t    * __restrict xml,
                void     * __restrict memp) {
  AkHeap        *heap;
  AkImageFormat *format;
  xml_attr_t    *att;

  heap   = dst->heap;
  format = ak_heap_calloc(heap, memp, sizeof(*format));

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ4(xml, hint)) {
      if ((att = DAE_XMLA8(xml, channels)) && att->val)
        format->channel = dae_enumChannel(att->val, att->valsize);
      
      if ((att = DAE_XMLA8(xml, range)) && att->val)
        format->range = dae_range(att->val, att->valsize);
      
      if ((att = DAE_XMLA(xml, precision)) && att->val)
        format->precision = dae_precision(att->val, att->valsize);
      
      if ((att = DAE_XMLA8(xml, space)) && att->val)
        format->space = xmla_strdup(att, heap, format);
    } else if (DAE_XML_TAG_EQ8(xml, exact)) {
      format->exact = xml_strdup(xml, heap, format);
    }
    xml = xml->next;
  }

  return format;
}

static
AkImage2d*
dae_create2d(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkHeap    *heap;
  AkImage2d *img;

  heap = dst->heap;
  img  = ak_heap_calloc(heap, memp, sizeof(*img));

  img->base.type = AK_IMAGE_TYPE_2D;

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ(xml, size_exact)) {
      AkSizeExact *sizeExact;
      
      sizeExact = ak_heap_calloc(heap, img, sizeof(*sizeExact));
      
      sizeExact->width  = xmla_u32(DAE_XMLA8(xml, width), 0);
      sizeExact->height = xmla_u32(DAE_XMLA8(xml, height), 0);
      img->sizeExact    = sizeExact;
    } else if (DAE_XML_TAG_EQ(xml, size_ratio)) {
      AkSizeRatio *sizeRatio;
      
      sizeRatio = ak_heap_calloc(heap, img, sizeof(*sizeRatio));
      
      sizeRatio->width  = xmla_float(DAE_XMLA8(xml, width), 0);
      sizeRatio->height = xmla_float(DAE_XMLA8(xml, height), 0);
      img->sizeRatio    = sizeRatio;
    } else if (DAE_XML_TAG_EQ4(xml, mips)) {
      AkMips *mips;
      
      mips = ak_heap_calloc(heap, img, sizeof(*mips));
      
      mips->levels       = xmla_u32(DAE_XMLA8(xml, levels), 0);
      mips->autoGenerate = xmla_u32(DAE_XMLA(xml, auto_generate), 0);
      img->mips          = mips;
    } else if (DAE_XML_TAG_EQ(xml, unnormalized)) {
      img->unnormalized = xml_strdup(xml, heap, img);
    } else if (DAE_XML_TAG_EQ8(xml, array)) {
      img->base.arrayLen = xmla_u32(DAE_XMLA8(xml, length), 0);
    } else if (DAE_XML_TAG_EQ8(xml, format)) {
      img->base.format = dae_imageFormat(dst, xml, img);
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      img->base.source = dae_imageSource(dst, xml, img);
    }
    xml = xml->next;
  }

  return img;
}

static
AkImage3d*
dae_create3d(DAEState * __restrict dst,
             xml_t    * __restrict xml,
             void     * __restrict memp) {
  AkHeap    *heap;
  AkImage3d *img;

  heap = dst->heap;
  img  = ak_heap_calloc(heap, memp, sizeof(*img));
  
  img->base.type = AK_IMAGE_TYPE_3D;

  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ4(xml, size)) {
      img->size.width  = xmla_u32(DAE_XMLA8(xml, width),  0);
      img->size.height = xmla_u32(DAE_XMLA8(xml, height), 0);
      img->size.depth  = xmla_u32(DAE_XMLA8(xml, depth),  0);
    } else if (DAE_XML_TAG_EQ4(xml, mips)) {
      img->mips.levels       = xmla_u32(DAE_XMLA8(xml, levels), 0);
      img->mips.autoGenerate = xmla_u32(DAE_XMLA(xml, auto_generate), 0);
    } else if (DAE_XML_TAG_EQ8(xml, array)) {
      img->base.arrayLen = xmla_u32(DAE_XMLA8(xml, length), 0);
    } else if (DAE_XML_TAG_EQ8(xml, format)) {
      img->base.format = dae_imageFormat(dst, xml, img);
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      img->base.source = dae_imageSource(dst, xml, img);
    }
    xml = xml->next;
  }

  return img;
}

static
AkImageCube*
dae_createCube(DAEState * __restrict dst,
               xml_t    * __restrict xml,
               void     * __restrict memp) {
  AkHeap      *heap;
  AkImageCube *img;
  
  heap = dst->heap;
  img  = ak_heap_calloc(heap, memp, sizeof(*img));
  
  img->base.type = AK_IMAGE_TYPE_CUBE;
  
  xml = xml->val;
  while (xml) {
    if (DAE_XML_TAG_EQ4(xml, size)) {
      img->width = xmla_u32(DAE_XMLA8(xml, width),  0);
    } else if (DAE_XML_TAG_EQ4(xml, mips)) {
      img->mips.levels       = xmla_u32(DAE_XMLA8(xml, levels), 0);
      img->mips.autoGenerate = xmla_u32(DAE_XMLA(xml, auto_generate), 0);
    } else if (DAE_XML_TAG_EQ8(xml, array)) {
      img->base.arrayLen = xmla_u32(DAE_XMLA8(xml, length), 0);
    } else if (DAE_XML_TAG_EQ8(xml, format)) {
      img->base.format = dae_imageFormat(dst, xml, img);
    } else if (DAE_XML_TAG_EQ(xml, init_from)) {
      img->base.source = dae_imageSource(dst, xml, img);
    }
    xml = xml->next;
  }

  return img;
}
