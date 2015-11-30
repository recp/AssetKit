/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "aio_collada_value.h"

#include "aio_collada_asset.h"
#include "aio_collada_technique.h"
#include "../aio_libxml.h"
#include "../aio_types.h"
#include "../aio_memory.h"
#include "../aio_utils.h"
#include "../aio_tree.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <string.h>

int _assetio_hide
aio_load_collada_value(xmlNode * __restrict xml_node,
                       void ** __restrict dest,
                       aio_value_type * __restrict val_type) {

  const char * node_name;
  const char * node_content;
  void       * pval;

  if (xml_node->type != XML_ELEMENT_NODE)
    return 0;

  pval = *dest;
  node_name = (const char *)xml_node->name;

  node_content = aio_xml_node_content(xml_node);

  if (AIO_IS_EQ_CASE(node_name, "string")) {
    char * val;

    val = aio_strdup(node_content);
    *val_type = AIO_VALUE_TYPE_STRING;

    *dest = val;

  } else if (AIO_IS_EQ_CASE(node_name, "bool")) {
    aio_bool * val;

    val = aio_malloc(sizeof(*val));
    *val = (int)strtol(node_content, NULL, 10);

    *dest = val;
    *val_type = AIO_VALUE_TYPE_BOOL;

  } else if (AIO_IS_EQ_CASE(node_name, "bool2")) {
    aio_bool      * val;
    const char    * pstr;
    unsigned long   slen;
    long            idx;
    long            i;

    slen = strlen(node_content);
    pstr = node_content;

    val = aio_calloc(sizeof(*val), 2);

    for (; i < slen - 1; i++) {
      if (*pstr == '\0' || idx > 1)
        break;

      *(val + idx++)= (bool)*pstr;
    }

    *dest = val;
    *val_type = AIO_VALUE_TYPE_BOOL2;

  } else if (AIO_IS_EQ_CASE(node_name, "bool3")) {
    aio_bool      * val;
    const char    * pstr;
    unsigned long   slen;
    long            idx;
    long            i;

    slen = strlen(node_content);
    pstr = node_content;

    val = aio_calloc(sizeof(*val), 3);

    for (; i < slen - 1; i++) {
      if (*pstr == '\0' || idx > 2)
        break;

      *(val + idx++)= (bool)*pstr;
    }

    *val = val;
    *val_type = AIO_VALUE_TYPE_BOOL3;

  } else if (AIO_IS_EQ_CASE(node_name, "bool4")) {
    aio_bool      * val;
    const char    * pstr;
    unsigned long   slen;
    long            idx;
    long            i;

    slen = strlen(node_content);
    pstr = node_content;

    val = aio_calloc(sizeof(*val), 4);

    for (; i < slen - 1; i++) {
      if (*pstr == '\0' || idx > 3)
        break;

      *(val + idx++)= (bool)*pstr;
    }

    *dest = val;
    *val_type = AIO_VALUE_TYPE_BOOL4;

  } else if (AIO_IS_EQ_CASE(node_name, "int")) {
    aio_int * val;

    val = aio_malloc(sizeof(*val));
    *val = (int)strtol(node_content, NULL, 10);

    *dest = val;
    *val_type = AIO_VALUE_TYPE_INT;

  } else if (AIO_IS_EQ_CASE(node_name, "int2")) {
    aio_int    * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 2);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 2) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = (int)strtol(tok, NULL, 10);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_INT2;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "int3")) {
    aio_int    * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 3);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 3) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = (int)strtol(tok, NULL, 10);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_INT3;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "int4")) {
    aio_int    * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 4);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 4) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = (int)strtol(tok, NULL, 10);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_INT4;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "float")) {
    aio_float * val;

    val = aio_malloc(sizeof(*val));
    *val = strtof(node_content, NULL);

    *dest = val;
    *val_type = AIO_VALUE_TYPE_FLOAT;

  } else if (AIO_IS_EQ_CASE(node_name, "float2")) {
    aio_float  * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 2);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 2) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = strtof(tok, NULL);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_FLOAT2;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "float3")) {
    aio_float  * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 3);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 3) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = strtof(tok, NULL);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_FLOAT3;
    free(raw_val);
    
  } else if (AIO_IS_EQ_CASE(node_name, "float4")) {
    aio_float  * val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    if (tok) {
      val = aio_calloc(sizeof(*val), 4);
      val[tok_idx] = strtod(tok, NULL);

      while (tok && tok_idx < 4) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        val[++tok_idx] = strtof(tok, NULL);
      }

      *dest = val;
    }

    *val_type = AIO_VALUE_TYPE_FLOAT4;
    free(raw_val);
    
  } else if (AIO_IS_EQ_CASE(node_name, "float2x2")) {
    aio_float2x2 val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;
    long         m;
    long         n;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    m = 0;
    n = 0;

    if (tok) {
      val[m][n++] = strtod(tok, NULL);

      while (tok && tok_idx < 4) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        if (n > 1) {
          n = 0;
          ++m;
        }

        val[m][n] = strtof(tok, NULL);

        ++n;
      }

      *dest = aio_calloc(sizeof(val), 1);
      memcpy(*dest, val, sizeof(val));
    }

    *val_type = AIO_VALUE_TYPE_FLOAT2x2;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "float3x3")) {
    aio_float3x3 val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;
    long         m;
    long         n;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    m = 0;
    n = 0;

    if (tok) {
      val[m][n++] = strtod(tok, NULL);

      while (tok && tok_idx < 9) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        if (n > 2) {
          n = 0;
          ++m;
        }

        val[m][n] = strtof(tok, NULL);

        ++n;
      }

      *dest = aio_calloc(sizeof(val), 1);
      memcpy(*dest, val, sizeof(val));
    }

    *val_type = AIO_VALUE_TYPE_FLOAT3x3;
    free(raw_val);

  } else if (AIO_IS_EQ_CASE(node_name, "float4x4")) {
    aio_float4x4 val;
    const char * tok;
    char       * raw_val;
    long         tok_idx;
    long         m;
    long         n;

    raw_val = strdup(node_content);
    tok = strtok(raw_val, " ");

    m = 0;
    n = 0;

    if (tok) {
      val[m][n++] = strtod(tok, NULL);

      while (tok && tok_idx < 16) {
        tok = strtok(NULL, " ");

        if (!tok)
          continue;

        if (n > 3) {
          n = 0;
          ++m;
        }

        val[m][n] = strtof(tok, NULL);

        ++n;
      }

      *dest = aio_calloc(sizeof(val), 1);
      memcpy(*dest, val, sizeof(val));
    }

    *val_type = AIO_VALUE_TYPE_FLOAT4x4;
    free(raw_val);

  }

  return 0;
}
