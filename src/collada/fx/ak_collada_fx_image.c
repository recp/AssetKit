/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_image.h"
#include "../core/ak_collada_asset.h"
#include "ak_collada_fx_enums.h"

static
AkResult _assetkit_hide
ak_dae_fxImage_initFrom(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkInitFrom ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_format(AkXmlState * __restrict xst,
                      void * __restrict memParent,
                      AkImageFormat ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_create2d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage2d ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_create3d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage3d ** __restrict dest);

static
int _assetkit_hide
ak_dae_fxImage_createCube(AkXmlState * __restrict xst,
                          void * __restrict memParent,
                          AkImageCube ** __restrict dest);


AkResult _assetkit_hide
ak_dae_fxImage(AkXmlState * __restrict xst,
               void * __restrict memParent,
               void ** __restrict dest) {
  AkImage      *img;
  AkXmlElmState xest;

  img = ak_heap_calloc(xst->heap, memParent, sizeof(*img));

  ak_xml_readid(xst, img);
  ak_xml_readsid(xst, img);

  img->name = ak_xml_attr(xst, img, _s_dae_name);

  ak_xest_init(xest, _s_dae_image)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult     ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(xst, img, &assetInf);
      if (ret == AK_OK)
        img->inf = assetInf;
    } else if (ak_xml_eqelm(xst, _s_dae_renderable)) {
      xmlChar *attrValStr;
      attrValStr =
      xmlTextReaderGetAttribute(xst->reader,
                                (const xmlChar *)_s_dae_renderable);
      if (attrValStr) {
        if (xmlStrcasecmp(attrValStr, (const xmlChar *)_s_dae_true))
          img->renderableShare = true;
        else
          img->renderableShare = false;
      }
    } else if (ak_xml_eqelm(xst, _s_dae_init_from)) {
      AkInitFrom *initFrom;
      AkResult      ret;

      ret = ak_dae_fxImage_initFrom(xst, img, &initFrom);
      if (ret == AK_OK)
        img->initFrom = initFrom;
    } else if (ak_xml_eqelm(xst, _s_dae_create_2d)) {
      AkImage2d *image2d;
      AkResult   ret;

      ret = ak_dae_fxImage_create2d(xst, img, &image2d);
      if (ret == AK_OK)
        img->image = &image2d->base;
    } else if (ak_xml_eqelm(xst, _s_dae_create_3d)) {
      AkImage3d *image3d;
      AkResult    ret;

      ret = ak_dae_fxImage_create3d(xst, img, &image3d);
      if (ret == AK_OK)
        img->image = &image3d->base;
    } else if (ak_xml_eqelm(xst, _s_dae_create_cube)) {
      AkImageCube *imageCube;
      AkResult ret;

      ret = ak_dae_fxImage_createCube(xst, img, &imageCube);
      if (ret == AK_OK)
        img->image = &imageCube->base;
    } else if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          img,
                          nodePtr,
                          &tree,
                          NULL);
      img->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = img;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxInstanceImage(AkXmlState      * __restrict xst,
                       void            * __restrict memParent,
                       AkInstanceBase ** __restrict dest) {
  AkInstanceBase *instanceImage;
  AkXmlElmState   xest;

  instanceImage = ak_heap_calloc(xst->heap,
                                 memParent,
                                 sizeof(*instanceImage));

  ak_xml_readsid(xst, instanceImage);

  instanceImage->name = ak_xml_attr(xst, instanceImage, _s_dae_name);

  ak_xml_attr_url(xst,
                  _s_dae_url,
                  instanceImage,
                  &instanceImage->url);

  ak_xest_init(xest, _s_dae_instance_image)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(xst->reader);
      tree = NULL;

      ak_tree_fromXmlNode(xst->heap,
                          instanceImage,
                          nodePtr,
                          &tree,
                          NULL);
      instanceImage->extra = tree;

      ak_xml_skipelm(xst);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = instanceImage;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_initFrom(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkInitFrom ** __restrict dest) {
  AkInitFrom   *initFrom;
  char         *attrValStr;
  AkXmlElmState xest;

  initFrom = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*initFrom));

  initFrom->mipsGenerate = ak_xml_attrui(xst, _s_dae_mips_generate);
  initFrom->arrayIndex   = ak_xml_attrui(xst, _s_dae_array_index);
  initFrom->mipIndex     = ak_xml_attrui(xst, _s_dae_mip_index);
  initFrom->depth        = ak_xml_attrui(xst, _s_dae_depth);

  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_face);
  if (attrValStr) {
    AkEnum attrVal;
    attrVal = ak_dae_fxEnumFace(attrValStr);
    if (attrVal != -1)
      initFrom->face = attrVal;

    xmlFree(attrValStr);
  }

  ak_xest_init(xest, _s_dae_init_from)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_ref)) {
      initFrom->ref = ak_xml_val(xst, initFrom);
    } else if (ak_xml_eqelm(xst, _s_dae_hex)) {
      AkHexData *hex;
      hex = ak_heap_calloc(xst->heap, initFrom, sizeof(*hex));

      hex->format = ak_xml_attr(xst, hex, _s_dae_format);

      if (hex->format) {
        hex->val = ak_xml_val(xst, hex);
        initFrom->hex = hex;
      } else {
        ak_free(hex);
      }
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = initFrom;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_format(AkXmlState * __restrict xst,
                      void * __restrict memParent,
                      AkImageFormat ** __restrict dest) {
  AkImageFormat *format;
  AkXmlElmState  xest;

  format = ak_heap_calloc(xst->heap, memParent, sizeof(*format));

  ak_xest_init(xest, _s_dae_format)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_hint)) {
      char *attrValStr;

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                        (const xmlChar *)_s_dae_channels);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumChannel(attrValStr);
        if (attrVal != -1)
          format->channel = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)_s_dae_range);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                        (const xmlChar *)_s_dae_precision);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      format->space = ak_xml_attr(xst, format, _s_dae_space);
    } else if (ak_xml_eqelm(xst, _s_dae_exact)) {
      format->exact = ak_xml_val(xst, format);
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = format;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create2d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage2d ** __restrict dest) {
  AkImage2d    *image2d;
  AkXmlElmState xest;

  image2d = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*image2d));

  image2d->base.type = AK_IMAGE_TYPE_2D;
  ak_xest_init(xest, _s_dae_create_2d)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkSizeExact *sizeExact;

      sizeExact = ak_heap_calloc(xst->heap,
                                 image2d,
                                 sizeof(*sizeExact));

      sizeExact->width  = ak_xml_attrf(xst, _s_dae_width);
      sizeExact->height = ak_xml_attrf(xst, _s_dae_height);

      image2d->sizeExact = sizeExact;
    } else if (ak_xml_eqelm(xst, _s_dae_size_ratio)) {
      AkSizeRatio *sizeRatio;

      sizeRatio = ak_heap_calloc(xst->heap,
                                 image2d,
                                 sizeof(*sizeRatio));


      sizeRatio->width  = ak_xml_attrf(xst, _s_dae_width);
      sizeRatio->height = ak_xml_attrf(xst, _s_dae_height);

      image2d->sizeRatio = sizeRatio;
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      AkMips *mips;

      mips = ak_heap_calloc(xst->heap,
                            image2d,
                            sizeof(*mips));

      mips->levels       = ak_xml_attrui(xst, _s_dae_width);
      mips->autoGenerate = ak_xml_attrui(xst, _s_dae_height);

      image2d->mips = mips;
    } else if (ak_xml_eqelm(xst, _s_dae_unnormalized)) {
      image2d->unnormalized = ak_xml_val(xst, image2d);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      image2d->base.arrayLen = ak_xml_attrui(xst, _s_dae_length);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(xst, image2d, &imageFormat);
      if (ret == AK_OK)
        image2d->base.format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(xst, image2d, &initFrom);
      if (ret == AK_OK)
        image2d->base.initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = image2d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create3d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage3d ** __restrict dest) {
  AkImage3d    *image3d;
  AkXmlElmState xest;

  image3d = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*image3d));

  image3d->base.type = AK_IMAGE_TYPE_3D;
  ak_xest_init(xest, _s_dae_create_3d)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size)) {
      image3d->size.width  = ak_xml_attrui(xst, _s_dae_width);
      image3d->size.height = ak_xml_attrui(xst, _s_dae_height);
      image3d->size.depth  = ak_xml_attrui(xst, _s_dae_depth);
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      image3d->mips.levels       = ak_xml_attrui(xst, _s_dae_levels);
      image3d->mips.autoGenerate = ak_xml_attrui(xst, _s_dae_auto_generate);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      image3d->base.arrayLen = ak_xml_attrui(xst, _s_dae_length);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(xst, image3d, &imageFormat);
      if (ret == AK_OK)
        image3d->base.format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(xst, image3d, &initFrom);
      if (ret == AK_OK)
        image3d->base.initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = image3d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_createCube(AkXmlState * __restrict xst,
                          void * __restrict memParent,
                          AkImageCube ** __restrict dest) {
  AkImageCube  *imageCube;
  AkXmlElmState xest;

  imageCube = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*imageCube));

  imageCube->base.type = AK_IMAGE_TYPE_CUBE;
  ak_xest_init(xest, _s_dae_create_cube)

  do {
    if (ak_xml_begin(&xest))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size)) {
      imageCube->width = ak_xml_attrui(xst, _s_dae_width);
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      imageCube->mips.levels       = ak_xml_attrui(xst, _s_dae_levels);
      imageCube->mips.autoGenerate = ak_xml_attrui(xst, _s_dae_auto_generate);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      imageCube->base.arrayLen = ak_xml_attrui(xst, _s_dae_length);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(xst, imageCube, &imageFormat);
      if (ret == AK_OK)
        imageCube->base.format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(xst, imageCube, &initFrom);
      if (ret == AK_OK)
        imageCube->base.initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);
    }

    /* end element */
    if (ak_xml_end(&xest))
      break;
  } while (xst->nodeRet);

  *dest = imageCube;

  return AK_OK;
}
