/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_asset.h"
#include "aio_libxml.h"
#include "aio_types.h"
#include "aio_memory.h"
#include "aio_utils.h"
#include "../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_asset(xmlNode * __restrict xml_node, ...) {
  xmlNode         * curr_node;
  xmlNode         * curr_state_node;

  aio_docinf     ** doc_inf;
  aio_assetinf   ** ast_inf;
  int               load_as;

  doc_inf = NULL;
  ast_inf = NULL;

  va_list args;
  va_start(args, xml_node);
  load_as = va_arg(args, int);
  if (load_as == _AIO_ASSET_LOAD_AS_DOCINF) {
    doc_inf = va_arg(args, aio_docinf**);
    *doc_inf = aio_malloc(sizeof(aio_docinf));
  } else if (load_as == _AIO_ASSET_LOAD_AS_ASTINF) {
    ast_inf = va_arg(args, aio_assetinf**);
    *ast_inf = aio_malloc(sizeof(**ast_inf));
  } else {
    va_end(args);
    return -1;
  }

  va_end(args);

  curr_node = xml_node->children;

  do {
    if (curr_node->type == XML_ELEMENT_NODE) {
      const char * node_name;
      node_name = (const char *)curr_node->name;

      /* contributor */
      if (AIO_IS_EQ_CASE(node_name, "contributor")) {
        aio_contributor * contributor;

        contributor = aio_malloc(sizeof(*contributor));

        curr_state_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;

          if (curr_node->type == XML_ELEMENT_NODE) {
            const char * node_content;

            node_content = aio_xml_node_content(curr_node);

            if (AIO_IS_EQ_CASE(node_name, "author"))
              contributor->author = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "author_email"))
              contributor->author_email = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "author_website"))
              contributor->author_website = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "authoring_tool"))
              contributor->authoring_tool = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "comments"))
              contributor->comments = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "copyright"))
              contributor->copyright = aio_strdup(node_content);
            else if (AIO_IS_EQ_CASE(node_name, "source_data"))
              contributor->source_data = aio_strdup(node_content);
            else {
              /* TODO: DEGUB Log */
            } /* if elm */
          } /* if type */

          curr_node = curr_node->next;
        } /* while */

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->contributor = contributor;
        else
          (*ast_inf)->contributor = contributor;

        node_name = NULL;
        curr_node = curr_state_node;

      } else if (AIO_IS_EQ_CASE(node_name, "coverage")
                 /* Dont include coverage to doc info */
                 && load_as == _AIO_ASSET_LOAD_AS_ASTINF) {

        aio_coverage * coverage;

        coverage = aio_malloc(sizeof(*coverage));

        curr_state_node = curr_node;
        curr_node = curr_node->children;
        while (curr_node) {
          node_name = (const char *)curr_node->name;

          if (curr_node->type == XML_ELEMENT_NODE) {
            if (AIO_IS_EQ_CASE(node_name, "geographic_location")) {
              xmlNode * coverage_curr;

              coverage_curr = curr_node;

              curr_node = curr_node->children;
              while (curr_node) {
                node_name = (const char *)curr_node->name;
                if (curr_node->type == XML_ELEMENT_NODE) {
                  const char * node_content;

                  node_content = aio_xml_node_content(curr_node);

                  if (AIO_IS_EQ_CASE(node_name, "longitude")) {
                    coverage->geo_loc.lng = strtod(node_content, NULL);
                  } else if (AIO_IS_EQ_CASE(node_name, "latitude")) {
                    coverage->geo_loc.lat = strtod(node_content, NULL);
                  } else if (AIO_IS_EQ_CASE(node_name, "altitude")) {
                    const xmlAttr * curr_attr;
                    const char    * attr_name;

                    aio_altitude_mode mode_enum;

                    curr_attr = curr_node->properties;
                    while (curr_attr) {
                      attr_name = (const char *)curr_attr->name;
                      if (curr_attr->type == XML_ATTRIBUTE_NODE) {
                        if (AIO_IS_EQ_CASE(attr_name, "mode")) {
                          const char * mode_str;
                          mode_str = aio_xml_node_content((xmlNode *)curr_attr);

                          if (AIO_IS_EQ_CASE(mode_str, "relativeToGround"))
                            mode_enum = AIO_ALTITUDE_RELATIVETOGROUND;
                          else if (AIO_IS_EQ_CASE(mode_str, "absolute"))
                            mode_enum = AIO_ALTITUDE_ABSOLUTE;

                          /* single attr */
                          break;
                        }
                      } /* if elm */

                      curr_attr = curr_attr->next;
                    } /* while */

                    coverage->geo_loc.alt.val = strtod(node_content, NULL);
                    coverage->geo_loc.alt.mode = mode_enum;

                  } /* else if altitute */
                } /* if elm */

                curr_node = curr_node->next;
              } /* while */

              node_name = NULL;
              curr_node = coverage_curr;
            }
          }

          curr_node = curr_node->next;
        } /* while */

        (*ast_inf)->coverage = coverage;

        node_name = NULL;
        curr_node = curr_state_node;

      } else if (AIO_IS_EQ_CASE(node_name, "created")) {
        const char * node_content;
        time_t       created_time;

        node_content = aio_xml_node_content(curr_node);
        created_time = aio_parse_date(node_content, NULL);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->created = created_time;
        else
          (*ast_inf)->created = created_time;

      } else if (AIO_IS_EQ_CASE(node_name, "modified")) {
        const char * node_content;
        time_t       modified_time;

        node_content = aio_xml_node_content(curr_node);
        modified_time = aio_parse_date(node_content, NULL);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->modified = modified_time;
        else
          (*ast_inf)->modified = modified_time;

      } else if (AIO_IS_EQ_CASE(node_name, "keywords")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->keywords = aio_strdup(node_content);
        else
          (*ast_inf)->keywords = aio_strdup(node_content);

      } else if (AIO_IS_EQ_CASE(node_name, "revision")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->revision = strtod(node_content, NULL);
        else
          (*ast_inf)->revision = strtod(node_content, NULL);

      } else if (AIO_IS_EQ_CASE(node_name, "subject")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->subject = aio_strdup(node_content);
        else
          (*ast_inf)->subject = aio_strdup(node_content);

      } else if (AIO_IS_EQ_CASE(node_name, "title")) {
        const char * node_content;

        node_content = aio_xml_node_content(curr_node);

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->title = aio_strdup(node_content);
        else
          (*ast_inf)->title = aio_strdup(node_content);

      } else if (AIO_IS_EQ_CASE(node_name, "unit")) {
        aio_unit      * ast_unit;
        const xmlAttr * curr_attr;
        const char    * attr_name;

        ast_unit = aio_malloc(sizeof(*ast_unit));
        curr_attr = curr_node->properties;

        while (curr_attr) {
          attr_name = (const char *)curr_attr->name;
          if (curr_attr->type == XML_ATTRIBUTE_NODE) {
            if (AIO_IS_EQ_CASE(attr_name, "name")) {
              const char * unit_name;
              unit_name = aio_xml_node_content((xmlNode *)curr_attr);

              ast_unit->name = aio_strdup(unit_name);
            } else if (AIO_IS_EQ_CASE(attr_name, "meter")) {
              const char * meter_val;
              meter_val = aio_xml_node_content((xmlNode *)curr_attr);

              ast_unit->dist = strtod(meter_val, NULL);
            }
          }

          curr_attr = curr_attr->next;
        } /* while */

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->unit = ast_unit;
        else
          (*ast_inf)->unit = ast_unit;

      } else if (AIO_IS_EQ_CASE(node_name, "up_axis")) {
        const char * node_content;
        aio_upaxis   upaxis;

        node_content = aio_xml_node_content(curr_node);

        if (AIO_IS_EQ_CASE(node_content, "Z_UP"))
          upaxis = AIO_UP_AXIS_Z;
        else if (AIO_IS_EQ_CASE(node_content, "X_UP"))
          upaxis = AIO_UP_AXIS_X;
        else
          /* Acording to COLLADA Specs 1.5 Y_UP is the default */
          upaxis = AIO_UP_AXIS_Y;

        if (load_as == _AIO_ASSET_LOAD_AS_DOCINF)
          (*doc_inf)->upaxis = upaxis;
        else
          (*ast_inf)->upaxis = upaxis;

      } else if (AIO_IS_EQ_CASE(node_name, "extra")) {
        _AIO_TREE_LOAD_TO(curr_node->children,
                          (*ast_inf)->extra,
                          NULL);
      } else {
        /* TODO: DEGUB Log */
      } /* if elm */
    } /* if type */

    if (!curr_node->next) {
      curr_node = curr_node->children;
      continue;
    }

    curr_node = curr_node->next;
  } while (curr_node);

  return 0;
}
