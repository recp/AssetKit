/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"

#include "aio_collada_common.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_technique(xmlNode * xml_node,
                           aio_technique ** technique) {
  return -1;
}

int _assetio_hide
aio_load_collada_techniquec(xmlNode * xml_node,
                            aio_technique_common ** techniquec) {
  xmlNode              * curr_node;
  aio_technique_common * techc;

  techc = NULL;

  curr_node = xml_node->children;
  while (curr_node) {
    if (curr_node->type == XML_ELEMENT_NODE) {

      if (techc) {
        techc->next = aio_malloc(sizeof(*techc));
        memset(techc->next, '\0', sizeof(*techc));

        techc = techc->next;
      } else {
        techc = aio_malloc(sizeof(*techc));
        memset(techc, '\0', sizeof(*techc));
        *techniquec = techc;
      }

      const char * node_name;
      node_name = (const char *)curr_node->name;

      /* optics -> perspective */
      if (AIO_IS_EQ_CASE(node_name, "perspective")) {
        xmlNode         * perspective_curr_node;
        aio_perspective * perspective;

        perspective = aio_malloc(sizeof(*perspective));

        perspective_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;
            double       node_val;
            
            node_content = aio_xml_node_content(curr_node);

            if (node_content)
              node_val = strtod(node_content, NULL);
            else
              node_val = 0.0;

            if (AIO_IS_EQ_CASE(node_name, "xfov")) {
              aio_basic_attrd * xfov;
              char            * sid;

              xfov = aio_malloc(sizeof(*xfov));
              xfov->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                xfov->sid = sid;
              else
                xfov->sid = NULL;

              perspective->xfov = xfov;
            } else if (AIO_IS_EQ_CASE(node_name, "yfov")) {
              aio_basic_attrd * yfov;
              char            * sid;

              yfov = aio_malloc(sizeof(*yfov));
              yfov->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                yfov->sid = sid;
              else
                yfov->sid = NULL;
              
              perspective->yfov = yfov;
            } else if (AIO_IS_EQ_CASE(node_name, "aspect_ratio")) {
              aio_basic_attrd * aspect_ratio;
              char            * sid;

              aspect_ratio = aio_malloc(sizeof(*aspect_ratio));
              aspect_ratio->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                aspect_ratio->sid = sid;
              else
                aspect_ratio->sid = NULL;

              perspective->aspect_ratio = aspect_ratio;
            } else if (AIO_IS_EQ_CASE(node_name, "znear")) {
              aio_basic_attrd * znear;
              char            * sid;

              znear = aio_malloc(sizeof(*znear));
              znear->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                znear->sid = sid;
              else
                znear->sid = NULL;

              perspective->znear = znear;
            } else if (AIO_IS_EQ_CASE(node_name, "zfar")) {
              aio_basic_attrd * zfar;
              char            * sid;

              zfar = aio_malloc(sizeof(*zfar));
              zfar->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                zfar->sid = sid;
              else
                zfar->sid = NULL;

              perspective->zfar = zfar;
            }
          }

          curr_node = curr_node->next;
        } /* while */

        techc->technique = perspective;
        techc->technique_type = AIO_TECHNIQUE_COMMON_CAMERA_PERSPECTIVE;

        curr_node = perspective_curr_node;
        node_name = NULL;
      }
      /* optics -> orthographic */
      else if (AIO_IS_EQ_CASE(node_name, "orthographic")) {
        xmlNode          * orthographic_curr_node;
        aio_orthographic * orthographic;

        orthographic = aio_malloc(sizeof(*orthographic));

        orthographic_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;
            double       node_val;

            node_content = aio_xml_node_content(curr_node);

            if (node_content)
              node_val = strtod(node_content, NULL);
            else
              node_val = 0.0;

            if (AIO_IS_EQ_CASE(node_name, "xmag")) {
              aio_basic_attrd * xmag;
              char            * sid;

              xmag = aio_malloc(sizeof(*xmag));
              xmag->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                xmag->sid = sid;
              else
                xmag->sid = NULL;

              orthographic->xmag = xmag;
            } else if (AIO_IS_EQ_CASE(node_name, "ymag")) {
              aio_basic_attrd * ymag;
              char            * sid;

              ymag = aio_malloc(sizeof(*ymag));
              ymag->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                ymag->sid = sid;
              else
                ymag->sid = NULL;

              orthographic->ymag = ymag;
            } else if (AIO_IS_EQ_CASE(node_name, "aspect_ratio")) {
              aio_basic_attrd * aspect_ratio;
              char            * sid;

              aspect_ratio = aio_malloc(sizeof(*aspect_ratio));
              aspect_ratio->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                aspect_ratio->sid = sid;
              else
                aspect_ratio->sid = NULL;

              orthographic->aspect_ratio = aspect_ratio;
            } else if (AIO_IS_EQ_CASE(node_name, "znear")) {
              aio_basic_attrd * znear;
              char            * sid;

              znear = aio_malloc(sizeof(*znear));
              znear->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                znear->sid = sid;
              else
                znear->sid = NULL;

              orthographic->znear = znear;
            } else if (AIO_IS_EQ_CASE(node_name, "zfar")) {
              aio_basic_attrd * zfar;
              char            * sid;

              zfar = aio_malloc(sizeof(*zfar));
              zfar->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                zfar->sid = sid;
              else
                zfar->sid = NULL;
              
              orthographic->zfar = zfar;
            }
          }
          
          curr_node = curr_node->next;
        } /* while */

        techc->technique = orthographic;
        techc->technique_type = AIO_TECHNIQUE_COMMON_CAMERA_ORTHOGRAPHIC;
        
        curr_node = orthographic_curr_node;
        node_name = NULL;

      }

      /* light -> ambient */
      else if (AIO_IS_EQ_CASE(node_name, "ambient")) {
        xmlNode     * ambient_curr_node;
        aio_ambient * ambient;

        ambient = aio_malloc(sizeof(*ambient));

        ambient_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;

            node_content = aio_xml_node_content(curr_node);

            if (AIO_IS_EQ_CASE(node_name, "color")) {
              char  * sid;
              char  * node_content_dup;
              char  * color_comp;
              float * color_vec;
              int     color_comp_idx;

              sid = NULL;
              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                ambient->color.sid = sid;
              else
                ambient->color.sid = NULL;

              color_vec = ambient->color.vec;
              color_comp_idx = 0;
              node_content_dup = strdup(node_content);

              color_comp = strtok(node_content_dup, " ");
              color_vec[color_comp_idx] = strtod(color_comp, NULL);

              while (color_comp && ++color_comp_idx < 4) {
                color_comp = strtok(NULL, " ");

                if (!color_comp) {
                  --color_comp_idx;
                  continue;
                }

                color_vec[color_comp_idx] = strtod(color_comp, NULL);
              }

              /* make alpha channel to 1.0 as default */
              if (color_comp_idx < 3)
                color_vec[3] = 1.0;

              free(node_content_dup);
            }
          }

          curr_node = curr_node->next;
        } /* while */

        techc->technique = ambient;
        techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_AMBIENT;

        curr_node = ambient_curr_node;
        node_name = NULL;
      }

      /* light -> directional */
      else if (AIO_IS_EQ_CASE(node_name, "directional")) {
        xmlNode         * directional_curr_node;
        aio_directional * directional;

        directional = aio_malloc(sizeof(*directional));

        directional_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;

            node_content = aio_xml_node_content(curr_node);

            if (AIO_IS_EQ_CASE(node_name, "color")) {
              char  * sid;
              char  * node_content_dup;
              char  * color_comp;
              float * color_vec;
              int     color_comp_idx;

              sid = NULL;
              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                directional->color.sid = sid;
              else
                directional->color.sid = NULL;

              color_vec = directional->color.vec;
              color_comp_idx = 0;
              node_content_dup = strdup(node_content);

              color_comp = strtok(node_content_dup, " ");
              color_vec[color_comp_idx] = strtod(color_comp, NULL);

              while (color_comp && ++color_comp_idx < 4) {
                color_comp = strtok(NULL, " ");

                if (!color_comp) {
                  --color_comp_idx;
                  continue;
                }

                color_vec[color_comp_idx] = strtod(color_comp, NULL);
              }

              /* make alpha channel to 1.0 as default */
              if (color_comp_idx < 3)
                color_vec[3] = 1.0;

              free(node_content_dup);
            }
          }

          curr_node = curr_node->next;
        } /* while */
        
        techc->technique = directional;
        techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_DIRECTIONAL;
        
        curr_node = directional_curr_node;
        node_name = NULL;
      }

      /* light -> point */
      else if (AIO_IS_EQ_CASE(node_name, "point")) {
        xmlNode   * point_curr_node;
        aio_point * point;

        point = aio_malloc(sizeof(*point));

        point_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;

            node_content = aio_xml_node_content(curr_node);

            if (AIO_IS_EQ_CASE(node_name, "color")) {
              char  * sid;
              char  * node_content_dup;
              char  * color_comp;
              float * color_vec;
              int     color_comp_idx;

              sid = NULL;
              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                point->color.sid = sid;
              else
                point->color.sid = NULL;

              color_vec = point->color.vec;
              color_comp_idx = 0;
              node_content_dup = strdup(node_content);

              color_comp = strtok(node_content_dup, " ");
              color_vec[color_comp_idx] = strtod(color_comp, NULL);

              while (color_comp && ++color_comp_idx < 4) {
                color_comp = strtok(NULL, " ");

                if (!color_comp) {
                  --color_comp_idx;
                  continue;
                }

                color_vec[color_comp_idx] = strtod(color_comp, NULL);
              }

              /* make alpha channel to 1.0 as default */
              if (color_comp_idx < 3)
                color_vec[3] = 1.0;

              free(node_content_dup);
            } else if (AIO_IS_EQ_CASE(node_name, "constant_attenuation")) {
              aio_basic_attrd * constant_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              constant_attenuation = aio_malloc(sizeof(*constant_attenuation));
              constant_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                constant_attenuation->sid = sid;
              else
                constant_attenuation->sid = NULL;

              point->constant_attenuation = constant_attenuation;
            } else if (AIO_IS_EQ_CASE(node_name, "linear_attenuation")) {
              aio_basic_attrd * linear_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              linear_attenuation = aio_malloc(sizeof(*linear_attenuation));
              linear_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                linear_attenuation->sid = sid;
              else
                linear_attenuation->sid = NULL;

              point->linear_attenuation = linear_attenuation;
            } else if (AIO_IS_EQ_CASE(node_name, "quadratic_attenuation")) {
              aio_basic_attrd * quadratic_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              quadratic_attenuation = aio_malloc(sizeof(*quadratic_attenuation));
              quadratic_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                quadratic_attenuation->sid = sid;
              else
                quadratic_attenuation->sid = NULL;

              point->quadratic_attenuation = quadratic_attenuation;
            }
          }

          curr_node = curr_node->next;
        } /* while */

        techc->technique = point;
        techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_POINT;
        
        curr_node = point_curr_node;
        node_name = NULL;
      }

      /* light -> spot */
      else if (AIO_IS_EQ_CASE(node_name, "spot")) {
        xmlNode  * spot_curr_node;
        aio_spot * spot;

        spot = aio_malloc(sizeof(*spot));

        spot_curr_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;
          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;

            node_content = aio_xml_node_content(curr_node);

            if (AIO_IS_EQ_CASE(node_name, "color")) {
              char  * sid;
              char  * node_content_dup;
              char  * color_comp;
              float * color_vec;
              int     color_comp_idx;

              sid = NULL;
              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                spot->color.sid = sid;
              else
                spot->color.sid = NULL;

              color_vec = spot->color.vec;
              color_comp_idx = 0;
              node_content_dup = strdup(node_content);

              color_comp = strtok(node_content_dup, " ");
              color_vec[color_comp_idx] = strtod(color_comp, NULL);

              while (color_comp && ++color_comp_idx < 4) {
                color_comp = strtok(NULL, " ");

                if (!color_comp) {
                  --color_comp_idx;
                  continue;
                }

                color_vec[color_comp_idx] = strtod(color_comp, NULL);
              }

              /* make alpha channel to 1.0 as default */
              if (color_comp_idx < 3)
                color_vec[3] = 1.0;

              free(node_content_dup);
            } else if (AIO_IS_EQ_CASE(node_name, "constant_attenuation")) {
              aio_basic_attrd * constant_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              constant_attenuation = aio_malloc(sizeof(*constant_attenuation));
              constant_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                constant_attenuation->sid = sid;
              else
                constant_attenuation->sid = NULL;

              spot->constant_attenuation = constant_attenuation;
            } else if (AIO_IS_EQ_CASE(node_name, "linear_attenuation")) {
              aio_basic_attrd * linear_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              linear_attenuation = aio_malloc(sizeof(*linear_attenuation));
              linear_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                linear_attenuation->sid = sid;
              else
                linear_attenuation->sid = NULL;

              spot->linear_attenuation = linear_attenuation;
            } else if (AIO_IS_EQ_CASE(node_name, "quadratic_attenuation")) {
              aio_basic_attrd * quadratic_attenuation;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              quadratic_attenuation = aio_malloc(sizeof(*quadratic_attenuation));
              quadratic_attenuation->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                quadratic_attenuation->sid = sid;
              else
                quadratic_attenuation->sid = NULL;
              
              spot->quadratic_attenuation = quadratic_attenuation;
            } else if (AIO_IS_EQ_CASE(node_name, "falloff_angle")) {
              aio_basic_attrd * falloff_angle;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              falloff_angle = aio_malloc(sizeof(*falloff_angle));
              falloff_angle->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                falloff_angle->sid = sid;
              else
                falloff_angle->sid = NULL;

              spot->falloff_angle = falloff_angle;
            } else if (AIO_IS_EQ_CASE(node_name, "falloff_exponent")) {
              aio_basic_attrd * falloff_exponent;
              char            * sid;
              double            node_val;

              node_content = aio_xml_node_content(curr_node);

              if (node_content)
                node_val = strtod(node_content, NULL);
              else
                node_val = 0.0;

              falloff_exponent = aio_malloc(sizeof(*falloff_exponent));
              falloff_exponent->val = node_val;

              aio_xml_collada_read_attr(curr_node, "sid", &sid);

              if (sid)
                falloff_exponent->sid = sid;
              else
                falloff_exponent->sid = NULL;
              
              spot->falloff_angle = falloff_exponent;
            }

          }
          
          curr_node = curr_node->next;
        } /* while */
        
        techc->technique = spot;
        techc->technique_type = AIO_TECHNIQUE_COMMON_LIGHT_SPOT;

        curr_node = spot_curr_node;
        node_name = NULL;
      }

    }

    curr_node = curr_node->next;
  } /* while */

  return 0;
}
