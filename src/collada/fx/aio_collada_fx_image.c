/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_fx_image.h"

#include "../aio_collada_asset.h"
#include "../../aio_libxml.h"
#include "../../aio_types.h"
#include "../../aio_memory.h"
#include "../../aio_utils.h"
#include "../../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

static
int _assetio_hide
aio_load_collada_image_init_from(xmlNode * __restrict xml_node,
                                 aio_init_from ** __restrict dest);

static
int _assetio_hide
aio_load_collada_image_format(xmlNode * __restrict xml_node,
                              aio_image_format ** __restrict dest);

static
int _assetio_hide
aio_load_collada_image_2d(xmlNode * __restrict xml_node,
                          aio_image2d ** __restrict dest);

static
int _assetio_hide
aio_load_collada_image_3d(xmlNode * __restrict xml_node,
                          aio_image3d ** __restrict dest);

static
int _assetio_hide
aio_load_collada_image_cube(xmlNode * __restrict xml_node,
                            aio_image_cube ** __restrict dest);


int _assetio_hide
aio_load_collada_image(xmlNode * __restrict xml_node,
                       aio_image ** __restrict dest) {

  xmlNode   * curr_node;
  xmlAttr   * curr_attr;
  aio_image * image;

  curr_node = xml_node;
  image = aio_malloc(sizeof(*image));
  memset(image, '\0', sizeof(*image));

  curr_attr = curr_node->properties;

  /* parse attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "id"))
        image->id = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "sid"))
        image->sid = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "name"))
        image->name = aio_strdup(attr_val);

    }

    curr_attr = curr_attr->next;
  }

  curr_attr = NULL;

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "asset")) {

        _AIO_ASSET_LOAD_TO(curr_node,
                           image->inf);

      } else if (AIO_IS_EQ_CASE(node_name, "renderable")) {
        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "share")) {
              if (AIO_IS_EQ_CASE(attr_val, "true"))
                image->renderable.share = true;
              else
                image->renderable.share = false;

              break;
            }
            
          }
          
          curr_attr = curr_attr->next;
        }

        curr_attr = NULL;

      } else if (AIO_IS_EQ_CASE(node_name, "init_from")) {
        aio_init_from * init_from;
        int             ret;

        ret = aio_load_collada_image_init_from(curr_node, &init_from);
        if (ret == 0)
          image->init_from = init_from;

      } else if (AIO_IS_EQ_CASE(node_name, "create_2d")) {
        aio_image2d * image2d;
        int           ret;

        ret = aio_load_collada_image_2d(curr_node, &image2d);
        if (ret == 0)
          image->image2d = image2d;

      } else if (AIO_IS_EQ_CASE(node_name, "create_3d")) {
        aio_image3d * image3d;
        int           ret;

        ret = aio_load_collada_image_3d(curr_node, &image3d);
        if (ret == 0)
          image->image3d = image3d;

      } else if (AIO_IS_EQ_CASE(node_name, "create_cube")) {
        aio_image_cube * image_cube;
        int              ret;

        ret = aio_load_collada_image_cube(curr_node, &image_cube);
        if (ret == 0)
          image->cube = image_cube;

      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          image->extra,
                          NULL);
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = image;
  
  return 0;
}

static
int _assetio_hide
aio_load_collada_image_init_from(xmlNode * __restrict xml_node,
                                 aio_init_from ** __restrict dest) {

  xmlNode       * curr_node;
  xmlAttr       * curr_attr;
  aio_init_from * init_from;

  curr_node = xml_node;
  init_from = aio_malloc(sizeof(*init_from));
  memset(init_from, '\0', sizeof(*init_from));

  curr_attr = curr_node->properties;

  /* parse attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "mips_generate"))
        init_from->mips_generate = (int)strtol(attr_val, NULL, 10);
      else if (AIO_IS_EQ_CASE(attr_name, "array_index"))
        init_from->array_index = (int)strtol(attr_val, NULL, 10);
      else if (AIO_IS_EQ_CASE(attr_name, "mip_index"))
        init_from->mip_index = (aio_uint)strtol(attr_val, NULL, 10);
      else if (AIO_IS_EQ_CASE(attr_name, "depth"))
        init_from->depth = (aio_uint)strtol(attr_val, NULL, 10);
      else if (AIO_IS_EQ_CASE(attr_name, "face")) {
        if (AIO_IS_EQ_CASE(attr_val, "POSITIVE_X"))
          init_from->face = AIO_FACE_POSITIVE_X;
        else if (AIO_IS_EQ_CASE(attr_val, "NEGATIVE_X"))
          init_from->face = AIO_FACE_NEGATIVE_X;
        else if (AIO_IS_EQ_CASE(attr_val, "POSITIVE_Y"))
          init_from->face = AIO_FACE_POSITIVE_Y;
        else if (AIO_IS_EQ_CASE(attr_val, "NEGATIVE_Y"))
          init_from->face = AIO_FACE_NEGATIVE_Y;
        else if (AIO_IS_EQ_CASE(attr_val, "POSITIVE_Z"))
          init_from->face = AIO_FACE_POSITIVE_Z;
        else if (AIO_IS_EQ_CASE(attr_val, "NEGATIVE_Z"))
          init_from->face = AIO_FACE_NEGATIVE_Z;
      }
    }

    curr_attr = curr_attr->next;
  }
  
  curr_attr = NULL;

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "ref")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);
        init_from->ref = aio_strdup(node_content);

      } else if (AIO_IS_EQ_CASE(node_name, "hex")) {
        aio_hex_data * hex;
        const char   * node_content;

        hex = aio_malloc(sizeof(*hex));
        memset(hex, '\0', sizeof(*hex));

        node_content = aio_xml_node_content(curr_node);

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "format")) {
              hex->format = aio_strdup(attr_val);
              break;
            }

          }

          curr_attr = curr_attr->next;
        }

        curr_attr = NULL;

        /* hex data should specify format */
        if (!hex->format) {
          aio_free(hex);
          goto err;
        }

        hex->val = aio_strdup(node_content);
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = init_from;

  return 0;

err:
  return -1;
}

static
int _assetio_hide
aio_load_collada_image_format(xmlNode * __restrict xml_node,
                              aio_image_format ** __restrict dest) {

  xmlNode          * curr_node;
  aio_image_format * format;

  format = aio_malloc(sizeof(*format));
  memset(format, '\0', sizeof(*format));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "hint")) {
        xmlAttr        * curr_attr;
        aio_size_exact * size_exact;

        size_exact = aio_malloc(sizeof(*size_exact));
        memset(size_exact, '\0', sizeof(*size_exact));

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "channels")) {

              if (AIO_IS_EQ_CASE(attr_val, "RGB"))
                format->hint.channel = AIO_FORMAT_CHANNEL_RGB;
              else if (AIO_IS_EQ_CASE(attr_val, "RGBA"))
                format->hint.channel = AIO_FORMAT_CHANNEL_RGBA;
              else if (AIO_IS_EQ_CASE(attr_val, "RGBE"))
                format->hint.channel = AIO_FORMAT_CHANNEL_RGBE;
              else if (AIO_IS_EQ_CASE(attr_val, "L"))
                format->hint.channel = AIO_FORMAT_CHANNEL_L;
              else if (AIO_IS_EQ_CASE(attr_val, "LA"))
                format->hint.channel = AIO_FORMAT_CHANNEL_LA;
              else if (AIO_IS_EQ_CASE(attr_val, "D"))
                format->hint.channel = AIO_FORMAT_CHANNEL_D;

            } else if (AIO_IS_EQ_CASE(attr_name, "range")) {

              if (AIO_IS_EQ_CASE(attr_val, "SNORM"))
                format->hint.channel = AIO_FORMAT_RANGE_SNORM;
              else if (AIO_IS_EQ_CASE(attr_val, "UNORM"))
                format->hint.channel = AIO_FORMAT_RANGE_UNORM;
              else if (AIO_IS_EQ_CASE(attr_val, "SINT"))
                format->hint.channel = AIO_FORMAT_RANGE_SINT;
              else if (AIO_IS_EQ_CASE(attr_val, "UINT"))
                format->hint.channel = AIO_FORMAT_RANGE_UINT;
              else if (AIO_IS_EQ_CASE(attr_val, "FLOAT"))
                format->hint.channel = AIO_FORMAT_RANGE_FLOAT;

            } else if (AIO_IS_EQ_CASE(attr_name, "precision")) {

              if (AIO_IS_EQ_CASE(attr_val, "DEFAULT"))
                format->hint.channel = AIO_FORMAT_PRECISION_DEFAULT;
              else if (AIO_IS_EQ_CASE(attr_val, "LOW"))
                format->hint.channel = AIO_FORMAT_PRECISION_LOW;
              else if (AIO_IS_EQ_CASE(attr_val, "MID"))
                format->hint.channel = AIO_FORMAT_PRECISION_MID;
              else if (AIO_IS_EQ_CASE(attr_val, "HIGH"))
                format->hint.channel = AIO_FORMAT_PRECISION_HIGHT;
              else if (AIO_IS_EQ_CASE(attr_val, "MAX"))
                format->hint.channel = AIO_FORMAT_PRECISION_MAX;

            } else if (AIO_IS_EQ_CASE(attr_name, "space")) {
              format->hint.space = aio_strdup(attr_val);
            }
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "exact")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);
        format->exact = aio_strdup(node_content);
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = format;
  
  return 0;
}

static
int _assetio_hide
aio_load_collada_image_2d(xmlNode * __restrict xml_node,
                          aio_image2d ** __restrict dest) {

  xmlNode     * curr_node;
  aio_image2d * image2d;

  image2d = aio_malloc(sizeof(*image2d));
  memset(image2d, '\0', sizeof(*image2d));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "size_exact")) {
        xmlAttr        * curr_attr;
        aio_size_exact * size_exact;

        size_exact = aio_malloc(sizeof(*size_exact));
        memset(size_exact, '\0', sizeof(*size_exact));

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "width"))
              size_exact->width = strtof(attr_val, NULL);
            else if (AIO_IS_EQ_CASE(attr_name, "height"))
              size_exact->height = strtof(attr_val, NULL);
          }

          curr_attr = curr_attr->next;
        }

        image2d->size_exact = size_exact;

      } else if (AIO_IS_EQ_CASE(node_name, "size_ratio")) {
        xmlAttr        * curr_attr;
        aio_size_ratio * size_ratio;

        size_ratio = aio_malloc(sizeof(*size_ratio));
        memset(size_ratio, '\0', sizeof(*size_ratio));

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "width"))
              size_ratio->height = strtof(attr_val, NULL);
            else if (AIO_IS_EQ_CASE(attr_name, "height"))
              size_ratio->width = strtof(attr_val, NULL);
          }
          
          curr_attr = curr_attr->next;
        }

        image2d->size_ratio = size_ratio;
      } else if (AIO_IS_EQ_CASE(node_name, "mips")) {
        xmlAttr    * curr_attr;
        aio_mips   * mips;

        mips = aio_malloc(sizeof(*mips));
        memset(mips, '\0', sizeof(*mips));

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "levels"))
              mips->levels = (aio_uint)strtol(attr_val, NULL, 10);
            else if (AIO_IS_EQ_CASE(attr_name, "auto_generate"))
              mips->auto_generate = (int)strtol(attr_val, NULL, 10);
          }

          curr_attr = curr_attr->next;
        }

        image2d->mips = mips;
      } else if (AIO_IS_EQ_CASE(node_name, "unnormalized")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);
        image2d->unnormalized = aio_strdup(node_content);
      } else if (AIO_IS_EQ_CASE(node_name, "array")) {
        xmlAttr    * curr_attr;

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "length")) {
              image2d->array_len = strtol(attr_val, NULL, 10);
              break;
            }
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "format")) {
        aio_image_format * image_format;
        int                ret;

        ret = aio_load_collada_image_format(curr_node, &image_format);
        if (ret == 0)
          image2d->format = image_format;

      } else if (AIO_IS_EQ_CASE(node_name, "init_from")) {
        aio_init_from * init_from;
        int             ret;

        ret = aio_load_collada_image_init_from(curr_node, &init_from);
        if (ret == 0)
          image2d->init_from = init_from;
      }
    }

    curr_node = curr_node->next;
  }
  
  *dest = image2d;
  
  return 0;
}

static
int _assetio_hide
aio_load_collada_image_3d(xmlNode * __restrict xml_node,
                          aio_image3d ** __restrict dest) {

  xmlNode     * curr_node;
  aio_image3d * image3d;

  image3d = aio_malloc(sizeof(*image3d));
  memset(image3d, '\0', sizeof(*image3d));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "size")) {
        xmlAttr * curr_attr;
        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "width"))
              image3d->size.width = (aio_int)strtol(attr_val, NULL, 10);
            else if (AIO_IS_EQ_CASE(attr_name, "height"))
              image3d->size.height = (aio_int)strtol(attr_val, NULL, 10);
            else if (AIO_IS_EQ_CASE(attr_name, "depth"))
              image3d->size.depth = (aio_int)strtol(attr_val, NULL, 10);
          }

          curr_attr = curr_attr->next;
        }

      } else if (AIO_IS_EQ_CASE(node_name, "mips")) {
        xmlAttr * curr_attr;

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "levels"))
              image3d->mips.levels = (int)strtol(attr_val, NULL, 10);
            else if (AIO_IS_EQ_CASE(attr_name, "auto_generate"))
              image3d->mips.auto_generate = (int)strtol(attr_val, NULL, 10);
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "array")) {
        xmlAttr * curr_attr;

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "length")) {
              image3d->array_len = strtol(attr_val, NULL, 10);
              break;
            }
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "format")) {
        aio_image_format * image_format;
        int                ret;

        ret = aio_load_collada_image_format(curr_node, &image_format);
        if (ret == 0)
          image3d->format = image_format;

      } else if (AIO_IS_EQ_CASE(node_name, "init_from")) {
        aio_init_from * init_from;
        int             ret;
        
        ret = aio_load_collada_image_init_from(curr_node, &init_from);
        if (ret == 0)
          image3d->init_from = init_from;
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = image3d;
  
  return 0;
}

static
int _assetio_hide
aio_load_collada_image_cube(xmlNode * __restrict xml_node,
                            aio_image_cube ** __restrict dest) {

  xmlNode        * curr_node;
  aio_image_cube * image_cube;

  image_cube = aio_malloc(sizeof(*image_cube));
  memset(image_cube, '\0', sizeof(*image_cube));

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "size")) {
        xmlAttr * curr_attr;
        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "width"))
              image_cube->size.width = strtol(attr_val, NULL, 10);
          }

          curr_attr = curr_attr->next;
        }

      } else if (AIO_IS_EQ_CASE(node_name, "mips")) {
        xmlAttr * curr_attr;

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "levels"))
              image_cube->mips.levels = (int)strtol(attr_val, NULL, 10);
            else if (AIO_IS_EQ_CASE(attr_name, "auto_generate"))
              image_cube->mips.auto_generate = (int)strtol(attr_val, NULL, 10);
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "array")) {
        xmlAttr * curr_attr;

        curr_attr = curr_node->properties;

        while (curr_attr) {
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            const char * attr_name;
            const char * attr_val;

            attr_name = (const char *)curr_attr->name;
            attr_val = aio_xml_node_content((xmlNode *)curr_attr);

            if (AIO_IS_EQ_CASE(attr_name, "length")) {
              image_cube->array_len = strtol(attr_val, NULL, 10);
              break;
            }
          }

          curr_attr = curr_attr->next;
        }
      } else if (AIO_IS_EQ_CASE(node_name, "format")) {
        aio_image_format * image_format;
        int                ret;

        ret = aio_load_collada_image_format(curr_node, &image_format);
        if (ret == 0)
          image_cube->format = image_format;

      } else if (AIO_IS_EQ_CASE(node_name, "init_from")) {
        aio_init_from * init_from;
        int             ret;

        ret = aio_load_collada_image_init_from(curr_node, &init_from);
        if (ret == 0)
          image_cube->init_from = init_from;
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = image_cube;
  
  return 0;
}

int _assetio_hide
aio_load_collada_image_instance(xmlNode * __restrict xml_node,
                                aio_image_instance ** __restrict dest) {
  xmlNode            * curr_node;
  xmlAttr            * curr_attr;
  aio_image_instance * image_instance;

  curr_node = xml_node;
  image_instance = aio_malloc(sizeof(*image_instance));
  memset(image_instance, '\0', sizeof(*image_instance));

  curr_attr = curr_node->properties;

  /* parse attributes */
  while (curr_attr) {
    if (curr_attr->type == XML_ATTRIBUTE_NODE) {
      const char * attr_name;
      const char * attr_val;

      attr_name = (const char *)curr_attr->name;
      attr_val = aio_xml_node_content((xmlNode *)curr_attr);

      if (AIO_IS_EQ_CASE(attr_name, "url"))
        image_instance->url = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "sid"))
        image_instance->sid = aio_strdup(attr_val);
      else if (AIO_IS_EQ_CASE(attr_name, "name"))
        image_instance->name = aio_strdup(attr_val);

    }

    curr_attr = curr_attr->next;
  }

  curr_attr = NULL;

  /* parse childrens */
  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          image_instance->extra,
                          NULL);
      }
    }
    
    curr_node = curr_node->next;
  }
  
  *dest = image_instance;
  
  return 0;
}
