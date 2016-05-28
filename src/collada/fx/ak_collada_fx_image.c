/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_collada_fx_image.h"
#include "../ak_collada_asset.h"
#include "ak_collada_fx_enums.h"

static
AkResult _assetkit_hide
ak_dae_fxImage_initFrom(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkInitFrom ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_format(AkHeap * __restrict heap,
                      void * __restrict memParent,
                      xmlTextReaderPtr reader,
                      AkImageFormat ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_create2d(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkImage2d ** __restrict dest);

static
AkResult _assetkit_hide
ak_dae_fxImage_create3d(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkImage3d ** __restrict dest);

static
int _assetkit_hide
ak_dae_fxImage_createCube(AkHeap * __restrict heap,
                          void * __restrict memParent,
                          xmlTextReaderPtr reader,
                          AkImageCube ** __restrict dest);


AkResult _assetkit_hide
ak_dae_fxImage(AkHeap * __restrict heap,
               void * __restrict memParent,
               xmlTextReaderPtr reader,
               AkImage ** __restrict dest) {
  AkImage *img;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  img = ak_heap_calloc(heap, memParent, sizeof(*img), false);

  _xml_readAttr(img, img->id, _s_dae_id);
  _xml_readAttr(img, img->sid, _s_dae_sid);
  _xml_readAttr(img, img->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_image);

    if (_xml_eqElm(_s_dae_asset)) {
      AkAssetInf *assetInf;
      AkResult     ret;

      assetInf = NULL;
      ret = ak_dae_assetInf(heap, img, reader, &assetInf);
      if (ret == AK_OK)
        img->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_renderable)) {
      xmlChar *attrValStr;
      attrValStr =
      xmlTextReaderGetAttribute(reader,
                                (const xmlChar *)_s_dae_renderable);
      if (attrValStr) {
        if (xmlStrcasecmp(attrValStr, (const xmlChar *)_s_dae_true))
          img->renderableShare = true;
        else
          img->renderableShare = false;
      }
    } else if (_xml_eqElm(_s_dae_init_from)) {
      AkInitFrom *initFrom;
      AkResult      ret;

      ret = ak_dae_fxImage_initFrom(heap, img, reader, &initFrom);
      if (ret == AK_OK)
        img->initFrom = initFrom;
    } else if (_xml_eqElm(_s_dae_create_2d)) {
      AkImage2d *image2d;
      AkResult    ret;

      ret = ak_dae_fxImage_create2d(heap, img, reader, &image2d);
      if (ret == AK_OK)
        img->image2d = image2d;
    } else if (_xml_eqElm(_s_dae_create_3d)) {
      AkImage3d *image3d;
      AkResult    ret;

      ret = ak_dae_fxImage_create3d(heap, img, reader, &image3d);
      if (ret == AK_OK)
        img->image3d = image3d;
    } else if (_xml_eqElm(_s_dae_create_cube)) {
      AkImageCube *imageCube;
      AkResult ret;

      ret = ak_dae_fxImage_createCube(heap, img, reader, &imageCube);
      if (ret == AK_OK)
        img->cube = imageCube;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree    *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap, img, nodePtr, &tree, NULL);
      img->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = img;

  return AK_OK;
}

AkResult _assetkit_hide
ak_dae_fxInstanceImage(AkHeap * __restrict heap,
                       void * __restrict memParent,
                       xmlTextReaderPtr reader,
                       AkInstanceImage ** __restrict dest) {
  AkInstanceImage *instanceImage;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  instanceImage = ak_heap_calloc(heap,
                                 memParent,
                                 sizeof(*instanceImage),
                                 false);

  _xml_readAttr(instanceImage, instanceImage->url, _s_dae_url);
  _xml_readAttr(instanceImage, instanceImage->sid, _s_dae_sid);
  _xml_readAttr(instanceImage, instanceImage->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_instance_image);

    if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      AkTree   *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      ak_tree_fromXmlNode(heap,
                          instanceImage,
                          nodePtr,
                          &tree,
                          NULL);
      instanceImage->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = instanceImage;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_initFrom(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkInitFrom ** __restrict dest) {
  AkInitFrom *initFrom;
  char          *attrValStr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  initFrom = ak_heap_calloc(heap, memParent, sizeof(*initFrom), false);

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

  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_face);
  if (attrValStr) {
    AkEnum attrVal;
    attrVal = ak_dae_fxEnumFace(attrValStr);
    if (attrVal != -1)
      initFrom->face = attrVal;

    xmlFree(attrValStr);
  }

  do {
    _xml_beginElement(_s_dae_init_from);

    if (_xml_eqElm(_s_dae_ref)) {
      _xml_readText(initFrom, initFrom->ref);
    } else if (_xml_eqElm(_s_dae_hex)) {
      AkHexData *hex;
      hex = ak_heap_calloc(heap, initFrom, sizeof(*hex), false);

      _xml_readAttr(hex, hex->format, _s_dae_format);

      if (hex->format) {
        _xml_readText(hex, hex->val);
        initFrom->hex = hex;
      } else {
        ak_free(hex);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = initFrom;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_format(AkHeap * __restrict heap,
                      void * __restrict memParent,
                      xmlTextReaderPtr reader,
                      AkImageFormat ** __restrict dest) {

  AkImageFormat *format;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  format = ak_heap_calloc(heap, memParent, sizeof(*format), false);

  do {
    _xml_beginElement(_s_dae_format);

    if (_xml_eqElm(_s_dae_hint)) {
      char *attrValStr;

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                        (const xmlChar *)_s_dae_channels);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumChannel(attrValStr);
        if (attrVal != -1)
          format->hint.channel = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                                     (const xmlChar *)_s_dae_range);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->hint.range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                        (const xmlChar *)_s_dae_precision);
      if (attrValStr) {
        AkEnum attrVal;
        attrVal = ak_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->hint.range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      _xml_readAttr(format, format->hint.space, _s_dae_space);
    } else if (_xml_eqElm(_s_dae_exact)) {
      _xml_readText(format, format->exact);
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = format;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create2d(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkImage2d ** __restrict dest) {

  AkImage2d   *image2d;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  image2d = ak_heap_calloc(heap, memParent, sizeof(*image2d), false);

  do {
    _xml_beginElement(_s_dae_create_2d);

    if (_xml_eqElm(_s_dae_size_exact)) {
      AkSizeExact *sizeExact;

      sizeExact = ak_heap_calloc(heap, image2d, sizeof(*sizeExact), false);

      _xml_readAttrUsingFn(sizeExact->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(sizeExact->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->sizeExact = sizeExact;
    } else if (_xml_eqElm(_s_dae_size_ratio)) {
      AkSizeRatio *sizeRatio;

      sizeRatio = ak_heap_calloc(heap, image2d, sizeof(*sizeRatio), false);

      _xml_readAttrUsingFn(sizeRatio->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(sizeRatio->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->sizeRatio = sizeRatio;
    } else if (_xml_eqElm(_s_dae_mips)) {
      AkMips *mips;

      mips = ak_heap_calloc(heap, image2d, sizeof(*mips), false);

      _xml_readAttrUsingFn(mips->levels,
                           _s_dae_width,
                           (AkUInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(mips->autoGenerate,
                           _s_dae_height,
                           strtol, NULL, 10);

      image2d->mips = mips;
    } else if (_xml_eqElm(_s_dae_unnormalized)) {
      _xml_readText(image2d, image2d->unnormalized);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(image2d->arrayLen,
                           _s_dae_length,
                           strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(heap, image2d, reader, &imageFormat);
      if (ret == AK_OK)
        image2d->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(heap, image2d, reader, &initFrom);
      if (ret == AK_OK)
        image2d->initFrom = initFrom;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = image2d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_create3d(AkHeap * __restrict heap,
                        void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        AkImage3d ** __restrict dest) {
  AkImage3d   *image3d;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  image3d = ak_heap_calloc(heap, memParent, sizeof(*image3d), false);

  do {
    _xml_beginElement(_s_dae_create_3d);

    if (_xml_eqElm(_s_dae_size)) {
      _xml_readAttrUsingFn(image3d->size.width,
                           _s_dae_width,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.height,
                           _s_dae_height,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.depth,
                           _s_dae_depth,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_mips)) {
      _xml_readAttrUsingFn(image3d->mips.levels,
                           _s_dae_levels,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->mips.autoGenerate,
                           _s_dae_auto_generate,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(image3d->arrayLen,
                           _s_dae_length,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;

      ret = ak_dae_fxImage_format(heap, image3d, reader, &imageFormat);
      if (ret == AK_OK)
        image3d->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;

      ret = ak_dae_fxImage_initFrom(heap, image3d, reader, &initFrom);
      if (ret == AK_OK)
        image3d->initFrom = initFrom;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = image3d;

  return AK_OK;
}

static
AkResult _assetkit_hide
ak_dae_fxImage_createCube(AkHeap * __restrict heap,
                          void * __restrict memParent,
                          xmlTextReaderPtr reader,
                          AkImageCube ** __restrict dest) {
  AkImageCube *imageCube;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  imageCube = ak_heap_calloc(heap, memParent, sizeof(*imageCube), false);

  do {
    _xml_beginElement(_s_dae_create_cube);

    if (_xml_eqElm(_s_dae_size)) {
      _xml_readAttrUsingFn(imageCube->width,
                           _s_dae_width,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_mips)) {
      _xml_readAttrUsingFn(imageCube->mips.levels,
                           _s_dae_levels,
                           (AkInt)strtol, NULL, 10);

      _xml_readAttrUsingFn(imageCube->mips.autoGenerate,
                           _s_dae_auto_generate,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(imageCube->arrayLen,
                           _s_dae_length,
                           (AkInt)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      AkImageFormat *imageFormat;
      AkResult ret;
      
      ret = ak_dae_fxImage_format(heap, imageCube, reader, &imageFormat);
      if (ret == AK_OK)
        imageCube->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      AkInitFrom *initFrom;
      AkResult ret;
      
      ret = ak_dae_fxImage_initFrom(heap, imageCube, reader, &initFrom);
      if (ret == AK_OK)
        imageCube->initFrom = initFrom;
    } else {
      _xml_skipElement;
    }
    
    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = imageCube;
  
  return AK_OK;
}
