/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_image.h"
#include "../aio_collada_common.h"
#include "../aio_collada_asset.h"
#include "aio_collada_fx_enums.h"

static
int _assetio_hide
aio_dae_fxImage_initFrom(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_init_from ** __restrict dest);

static
int _assetio_hide
aio_dae_fxImage_format(void * __restrict memParent,
                       xmlTextReaderPtr reader,
                       aio_image_format ** __restrict dest);

static
int _assetio_hide
aio_dae_fxImage_create2d(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_image2d ** __restrict dest);

static
int _assetio_hide
aio_dae_fxImage_create3d(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_image3d ** __restrict dest);

static
int _assetio_hide
aio_dae_fxImage_createCube(void * __restrict memParent,
                           xmlTextReaderPtr reader,
                           aio_image_cube ** __restrict dest);


int _assetio_hide
aio_dae_fxImage(void * __restrict memParent,
                xmlTextReaderPtr reader,
                aio_image ** __restrict dest) {
  aio_image *img;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  img = aio_calloc(memParent, sizeof(*img), 1);

  _xml_readAttr(img, img->id, _s_dae_id);
  _xml_readAttr(img, img->sid, _s_dae_sid);
  _xml_readAttr(img, img->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_image);

    if (_xml_eqElm(_s_dae_asset)) {
      aio_assetinf *assetInf;
      int ret;

      assetInf = NULL;
      ret = aio_dae_assetInf(img, reader, &assetInf);
      if (ret == 0)
        img->inf = assetInf;
    } else if (_xml_eqElm(_s_dae_renderable)) {
      xmlChar *attrValStr;
      attrValStr =
      xmlTextReaderGetAttribute(reader,
                                (const xmlChar *)_s_dae_renderable);
      if (attrValStr) {
        if (xmlStrcasecmp(attrValStr, (const xmlChar *)_s_dae_true))
          img->renderable.share = true;
        else
          img->renderable.share = false;
      }
    } else if (_xml_eqElm(_s_dae_init_from)) {
      aio_init_from *init_from;
      int            ret;

      ret = aio_dae_fxImage_initFrom(img, reader, &init_from);
      if (ret == 0)
        img->init_from = init_from;
    } else if (_xml_eqElm(_s_dae_create_2d)) {
      aio_image2d *image2d;
      int          ret;

      ret = aio_dae_fxImage_create2d(img, reader, &image2d);
      if (ret == 0)
        img->image2d = image2d;
    } else if (_xml_eqElm(_s_dae_create_3d)) {
      aio_image3d *image3d;
      int          ret;

      ret = aio_dae_fxImage_create3d(img, reader, &image3d);
      if (ret == 0)
        img->image3d = image3d;
    } else if (_xml_eqElm(_s_dae_create_cube)) {
      aio_image_cube *imageCube;
      int ret;

      ret = aio_dae_fxImage_createCube(img, reader, &imageCube);
      if (ret == 0)
        img->cube = imageCube;
    } else if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(img, nodePtr, &tree, NULL);
      img->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = img;
  
  return 0;
}

int _assetio_hide
aio_dae_fxImageInstance(void * __restrict memParent,
                        xmlTextReaderPtr reader,
                        aio_image_instance ** __restrict dest) {
  aio_image_instance *imgInst;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  imgInst = aio_calloc(memParent, sizeof(*imgInst), 1);

  _xml_readAttr(imgInst, imgInst->url, _s_dae_url);
  _xml_readAttr(imgInst, imgInst->sid, _s_dae_sid);
  _xml_readAttr(imgInst, imgInst->name, _s_dae_name);

  do {
    _xml_beginElement(_s_dae_instance_image);

    if (_xml_eqElm(_s_dae_extra)) {
      xmlNodePtr nodePtr;
      aio_tree  *tree;

      nodePtr = xmlTextReaderExpand(reader);
      tree = NULL;

      aio_tree_fromXmlNode(imgInst, nodePtr, &tree, NULL);
      imgInst->extra = tree;

      _xml_skipElement;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);
  
  *dest = imgInst;
  
  return 0;
}

static
int _assetio_hide
aio_dae_fxImage_initFrom(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_init_from ** __restrict dest) {
  aio_init_from *initFrom;
  char          *attrValStr;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  initFrom = aio_calloc(memParent, sizeof(*initFrom), 1);

  _xml_readAttrUsingFn(initFrom->mips_generate,
                       _s_dae_mips_generate,
                       (aio_bool)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->array_index,
                       _s_dae_array_index,
                       (aio_int)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->mip_index,
                       _s_dae_mip_index,
                       (aio_int)strtol, NULL, 10);

  _xml_readAttrUsingFn(initFrom->depth,
                       _s_dae_depth,
                       (aio_int)strtol, NULL, 10);

  attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                    (const xmlChar *)_s_dae_face);
  if (attrValStr) {
    long attrVal;
    attrVal = aio_dae_fxEnumFace(attrValStr);
    if (attrVal != -1)
      initFrom->face = attrVal;

    xmlFree(attrValStr);
  }

  do {
    _xml_beginElement(_s_dae_init_from);

    if (_xml_eqElm(_s_dae_ref)) {
      _xml_readText(initFrom, initFrom->ref);
    } else if (_xml_eqElm(_s_dae_hex)) {
      aio_hex_data *hex;
      hex = aio_calloc(initFrom, sizeof(*hex), 1);

      _xml_readAttr(hex, hex->format, _s_dae_format);

      if (hex->format) {
        _xml_readText(hex, hex->val);
        initFrom->hex = hex;
      } else {
        aio_free(hex);
      }
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = initFrom;
  
  return 0;
}

static
int _assetio_hide
aio_dae_fxImage_format(void * __restrict memParent,
                       xmlTextReaderPtr reader,
                       aio_image_format ** __restrict dest) {

  aio_image_format *format;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  format = aio_calloc(memParent, sizeof(*format), 1);

  do {
    _xml_beginElement(_s_dae_format);

    if (_xml_eqElm(_s_dae_hint)) {
      char *attrValStr;

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                      (const xmlChar *)_s_dae_channels);
      if (attrValStr) {
        long attrVal;
        attrVal = aio_dae_fxEnumChannel(attrValStr);
        if (attrVal != -1)
          format->hint.channel = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                      (const xmlChar *)_s_dae_range);
      if (attrValStr) {
        long attrVal;
        attrVal = aio_dae_fxEnumRange(attrValStr);
        if (attrVal != -1)
          format->hint.range = attrVal;

        xmlFree(attrValStr);
        attrValStr = NULL;
      }

      attrValStr = (char *)xmlTextReaderGetAttribute(reader,
                                      (const xmlChar *)_s_dae_precision);
      if (attrValStr) {
        long attrVal;
        attrVal = aio_dae_fxEnumRange(attrValStr);
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
  
  return 0;
}

static
int _assetio_hide
aio_dae_fxImage_create2d(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_image2d ** __restrict dest) {

  aio_image2d   *image2d;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  image2d = aio_calloc(memParent, sizeof(*image2d), 1);

  do {
    _xml_beginElement(_s_dae_create_2d);

    if (_xml_eqElm(_s_dae_size_exact)) {
      aio_size_exact *size_exact;

      size_exact = aio_calloc(image2d, sizeof(*size_exact), 1);

      _xml_readAttrUsingFn(size_exact->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(size_exact->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->size_exact = size_exact;
    } else if (_xml_eqElm(_s_dae_size_ratio)) {
      aio_size_ratio *size_ratio;

      size_ratio = aio_calloc(image2d, sizeof(*size_ratio), 1);

      _xml_readAttrUsingFn(size_ratio->width,
                           _s_dae_width,
                           strtof, NULL);

      _xml_readAttrUsingFn(size_ratio->height,
                           _s_dae_height,
                           strtof, NULL);

      image2d->size_ratio = size_ratio;
    } else if (_xml_eqElm(_s_dae_mips)) {
      aio_mips *mips;

      mips = aio_calloc(image2d, sizeof(*mips), 1);

      _xml_readAttrUsingFn(mips->levels,
                           _s_dae_width,
                           (aio_uint)strtol, NULL, 10);

      _xml_readAttrUsingFn(mips->auto_generate,
                           _s_dae_height,
                           strtol, NULL, 10);

      image2d->mips = mips;
    } else if (_xml_eqElm(_s_dae_unnormalized)) {
      _xml_readText(image2d, image2d->unnormalized);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(image2d->array_len,
                           _s_dae_length,
                           strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      aio_image_format *imageFormat;
      int               ret;

      ret = aio_dae_fxImage_format(image2d, reader, &imageFormat);
      if (ret == 0)
        image2d->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      aio_init_from *initFrom;
      int            ret;

      ret = aio_dae_fxImage_initFrom(image2d, reader, &initFrom);
      if (ret == 0)
        image2d->init_from = initFrom;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = image2d;
  
  return 0;
}

static
int _assetio_hide
aio_dae_fxImage_create3d(void * __restrict memParent,
                         xmlTextReaderPtr reader,
                         aio_image3d ** __restrict dest) {
  aio_image3d   *image3d;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  image3d = aio_calloc(memParent, sizeof(*image3d), 1);

  do {
    _xml_beginElement(_s_dae_create_3d);

    if (_xml_eqElm(_s_dae_size)) {
      _xml_readAttrUsingFn(image3d->size.width,
                           _s_dae_width,
                           (aio_int)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.height,
                           _s_dae_height,
                           (aio_int)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->size.depth,
                           _s_dae_depth,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_mips)) {
      _xml_readAttrUsingFn(image3d->mips.levels,
                           _s_dae_levels,
                           (aio_int)strtol, NULL, 10);

      _xml_readAttrUsingFn(image3d->mips.auto_generate,
                           _s_dae_auto_generate,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(image3d->array_len,
                           _s_dae_length,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      aio_image_format *imageFormat;
      int               ret;

      ret = aio_dae_fxImage_format(image3d, reader, &imageFormat);
      if (ret == 0)
        image3d->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      aio_init_from *initFrom;
      int            ret;

      ret = aio_dae_fxImage_initFrom(image3d, reader, &initFrom);
      if (ret == 0)
        image3d->init_from = initFrom;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = image3d;
  
  return 0;
}

static
int _assetio_hide
aio_dae_fxImage_createCube(void * __restrict memParent,
                           xmlTextReaderPtr reader,
                           aio_image_cube ** __restrict dest) {
  aio_image_cube *imageCube;
  const xmlChar *nodeName;
  int            nodeType;
  int            nodeRet;

  imageCube = aio_calloc(memParent, sizeof(*imageCube), 1);

  do {
    _xml_beginElement(_s_dae_create_cube);

    if (_xml_eqElm(_s_dae_size)) {
      _xml_readAttrUsingFn(imageCube->size.width,
                           _s_dae_width,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_mips)) {
      _xml_readAttrUsingFn(imageCube->mips.levels,
                           _s_dae_levels,
                           (aio_int)strtol, NULL, 10);

      _xml_readAttrUsingFn(imageCube->mips.auto_generate,
                           _s_dae_auto_generate,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_array)) {
      _xml_readAttrUsingFn(imageCube->array_len,
                           _s_dae_length,
                           (aio_int)strtol, NULL, 10);
    } else if (_xml_eqElm(_s_dae_format)) {
      aio_image_format *imageFormat;
      int               ret;

      ret = aio_dae_fxImage_format(imageCube, reader, &imageFormat);
      if (ret == 0)
        imageCube->format = imageFormat;
    } else if (_xml_eqElm(_s_dae_size_exact)) {
      aio_init_from *initFrom;
      int            ret;

      ret = aio_dae_fxImage_initFrom(imageCube, reader, &initFrom);
      if (ret == 0)
        imageCube->init_from = initFrom;
    } else {
      _xml_skipElement;
    }

    /* end element */
    _xml_endElement;
  } while (nodeRet);

  *dest = imageCube;
  
  return 0;
}
