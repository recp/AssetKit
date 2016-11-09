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
               AkImage ** __restrict dest) {
  AkImage *img;

  img = ak_heap_calloc(xst->heap, memParent, sizeof(*img), true);

  ak_xml_readid(xst, img);
  img->sid  = ak_xml_attr(xst, img, _s_dae_sid);
  img->name = ak_xml_attr(xst, img, _s_dae_name);

  do {
    if (ak_xml_beginelm(xst, _s_dae_image))
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
      AkResult    ret;

      ret = ak_dae_fxImage_create2d(xst, img, &image2d);
      if (ret == AK_OK)
        img->image2d = image2d;
    } else if (ak_xml_eqelm(xst, _s_dae_create_3d)) {
      AkImage3d *image3d;
      AkResult    ret;

      ret = ak_dae_fxImage_create3d(xst, img, &image3d);
      if (ret == AK_OK)
        img->image3d = image3d;
    } else if (ak_xml_eqelm(xst, _s_dae_create_cube)) {
      AkImageCube *imageCube;
      AkResult ret;

      ret = ak_dae_fxImage_createCube(xst, img, &imageCube);
      if (ret == AK_OK)
        img->cube = imageCube;
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

      ak_xml_skipelm(xst);;
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = img;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxInstanceImage(AkXmlState * __restrict xst,
                       void * __restrict memParent,
                       AkInstanceImage ** __restrict dest) {
  AkInstanceImage *instanceImage;

  instanceImage = ak_heap_calloc(xst->heap,
                                 memParent,
                                 sizeof(*instanceImage),
                                 false);

  instanceImage->sid  = ak_xml_attr(xst, instanceImage, _s_dae_sid);
  instanceImage->name = ak_xml_attr(xst, instanceImage, _s_dae_name);

  ak_url_from_attr(xst->reader,
                   _s_dae_url,
                   instanceImage,
                   &instanceImage->url);

  do {
    if (ak_xml_beginelm(xst, _s_dae_instance_image))
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

      ak_xml_skipelm(xst);;
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = instanceImage;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_initFrom(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkInitFrom ** __restrict dest) {
  AkInitFrom *initFrom;
  char       *attrValStr;

  initFrom = ak_heap_calloc(xst->heap,
                            memParent,
                            sizeof(*initFrom),
                            false);

  _xml_readAttrUsingFn(initFrom->mipsGenerate,
                       _s_dae_mips_generate,
                       (AkBool)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->arrayIndex,
                       _s_dae_array_index,
                       (AkInt)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->mipIndex,
                       _s_dae_mip_index,
                       (AkInt)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->depth,
                       _s_dae_depth,
                       (AkInt)strtol, NULL, 10);

  attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                    (const xmlChar *)_s_dae_face);
  if (attrValStr) {
    AkEnum attrVal;
    attrVal = ak_dae_fxEnumFace(attrValStr);
    if (attrVal != -1)
      initFrom->face = attrVal;

    xmlFree(attrValStr);
  }

  do {
    if (ak_xml_beginelm(xst, _s_dae_init_from))
      break;

    if (ak_xml_eqelm(xst, _s_dae_ref)) {
      initFrom->ref = ak_xml_val(xst, initFrom);
    } else if (ak_xml_eqelm(xst, _s_dae_hex)) {
      AkHexData *hex;
      hex = ak_heap_calloc(xst->heap, initFrom, sizeof(*hex), false);

      hex->format = ak_xml_attr(xst, hex, _s_dae_format);

      if (hex->format) {
        hex->val = ak_xml_val(xst, hex);
        initFrom->hex = hex;
      } else {
        ak_free(hex);
      }
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
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

  format = ak_heap_calloc(xst->heap, memParent, sizeof(*format), false);

  do {
    if (ak_xml_beginelm(xst, _s_dae_format))
      break;

    if (ak_xml_eqelm(xst, _s_dae_hint)) {
      char *attrValStr;

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                        (const xmlChar *)_s_dae_channels);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumChannel(attrValStr);
        if (attrVal != -1)
          format->hint.channel = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                         (const xmlChar *)_s_dae_range);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->hint.range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(xst->reader,
                                        (const xmlChar *)_s_dae_precision);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->hint.range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      format->hint.space = ak_xml_attr(xst, format, _s_dae_space);
    } else if (ak_xml_eqelm(xst, _s_dae_exact)) {
      format->exact = ak_xml_val(xst, format);
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = format;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create2d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage2d ** __restrict dest) {
  AkImage2d *image2d;

  image2d = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*image2d),
                           false);

  do {
    if (ak_xml_beginelm(xst, _s_dae_create_2d))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkSizeExact *sizeExact;

      sizeExact = ak_heap_calloc(xst->heap,
                                 image2d,
                                 sizeof(*sizeExact),
                                 false);

      _xml_readAttrUsingFn(sizeExact->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(sizeExact->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->sizeExact = sizeExact;
    } else if (ak_xml_eqelm(xst, _s_dae_size_ratio)) {
      AkSizeRatio *sizeRatio;

      sizeRatio = ak_heap_calloc(xst->heap,
                                 image2d,
                                 sizeof(*sizeRatio),
                                 false);

      _xml_readAttrUsingFn(sizeRatio->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(sizeRatio->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->sizeRatio = sizeRatio;
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      AkMips *mips;

      mips = ak_heap_calloc(xst->heap,
                            image2d,
                            sizeof(*mips),
                            false);

      _xml_readAttrUsingFn(mips->levels,
                           _s_dae_width,
                           (AkUInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(mips->autoGenerate,
                           _s_dae_height,
                           strtol, NULL, 10);

      image2d->mips = mips;
    } else if (ak_xml_eqelm(xst, _s_dae_unnormalized)) {
      image2d->unnormalized = ak_xml_val(xst, image2d);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      _xml_readAttrUsingFn(image2d->arrayLen,
                           _s_dae_length,
                           strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(xst, image2d, &imageFormat);
      if (ret == AK_OK)
        image2d->format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(xst, image2d, &initFrom);
      if (ret == AK_OK)
        image2d->initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = image2d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create3d(AkXmlState * __restrict xst,
                        void * __restrict memParent,
                        AkImage3d ** __restrict dest) {
  AkImage3d *image3d;

  image3d = ak_heap_calloc(xst->heap,
                           memParent,
                           sizeof(*image3d),
                           false);

  do {
    if (ak_xml_beginelm(xst, _s_dae_create_3d))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size)) {
      _xml_readAttrUsingFn(image3d->size.width,
                           _s_dae_width,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.height,
                           _s_dae_height,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.depth,
                           _s_dae_depth,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      _xml_readAttrUsingFn(image3d->mips.levels,
                           _s_dae_levels,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->mips.autoGenerate,
                           _s_dae_auto_generate,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      _xml_readAttrUsingFn(image3d->arrayLen,
                           _s_dae_length,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(xst, image3d, &imageFormat);
      if (ret == AK_OK)
        image3d->format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(xst, image3d, &initFrom);
      if (ret == AK_OK)
        image3d->initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);;
    }

    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);

  *dest = image3d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_createCube(AkXmlState * __restrict xst,
                          void * __restrict memParent,
                          AkImageCube ** __restrict dest) {
  AkImageCube *imageCube;

  imageCube = ak_heap_calloc(xst->heap,
                             memParent,
                             sizeof(*imageCube),
                             false);

  do {
    if (ak_xml_beginelm(xst, _s_dae_create_cube))
      break;

    if (ak_xml_eqelm(xst, _s_dae_size)) {
      _xml_readAttrUsingFn(imageCube->width,
                           _s_dae_width,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_mips)) {
      _xml_readAttrUsingFn(imageCube->mips.levels,
                           _s_dae_levels,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(imageCube->mips.autoGenerate,
                           _s_dae_auto_generate,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_array)) {
      _xml_readAttrUsingFn(imageCube->arrayLen,
                           _s_dae_length,
                           (AkInt)strtol, NULL, 10);
    } else if (ak_xml_eqelm(xst, _s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;
      
      ret = ak_dae_fxImage_format(xst, imageCube, &imageFormat);
      if (ret == AK_OK)
        imageCube->format = imageFormat;
    } else if (ak_xml_eqelm(xst, _s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;
      
      ret = ak_dae_fxImage_initFrom(xst, imageCube, &initFrom);
      if (ret == AK_OK)
        imageCube->initFrom = initFrom;
    } else {
      ak_xml_skipelm(xst);;
    }
    
    /* end element */
    ak_xml_endelm(xst);;
  } while (xst->nodeRet);
  
  *dest = imageCube;
  
  return AK_OK;
}
