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

#include "3mf.h"
#include "internal.h"
#include "vendor/bambu/bambu.h"
#include "../../common/buffer.h"
#include "../../common/text_number.h"
#include "../../common/util.h"
#include "../../common/zip.h"
#include "../../../mem/common.h"
#include "../../../mat/internal.h"
#include "../../../string_fast.h"
#include "../../../strpool.h"
#include "../../../thread.h"
#include "../../../id.h"
#include "../../../../include/ak/path.h"

#include <stdlib.h>
#include <string.h>

#define AK_3MF_XMLA(XML, NAME) xmla_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)
#define AK_3MF_XMLA_LOCAL(XML, NAME)                                          \
  ak_3mf_xmla_local_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)
#define AK_3MF_STRDUP_PATH_ATTR_LOCAL(DOC, XML, NAME, PARENT)                 \
  ak_3mf_strdup_path_attr_local_sz((DOC),                                     \
                                   (XML),                                     \
                                   _s_ak_##NAME,                              \
                                   _s_ak_##NAME##_len,                        \
                                   (PARENT))
#define AK_3MF_TAG(XML, NAME)                                                 \
  ak_3mf_tag_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)
#define AK_3MF_TAG8(XML, NAME)                                                \
  ak_3mf_tag_packed((XML), _s_ak_##NAME##_u64_exact, _s_ak_##NAME##_len)
#define AK_3MF_CHILD(XML, NAME)                                               \
  ak_3mf_child_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)
#define AK_3MF_CHILD8(XML, NAME)                                              \
  ak_3mf_child_packed((XML), _s_ak_##NAME##_u64_exact, _s_ak_##NAME##_len)
#define AK_3MF_COUNT_CHILDREN(XML, NAME)                                      \
  ak_3mf_count_children_sz((XML), _s_ak_##NAME, _s_ak_##NAME##_len)
#define AK_3MF_COUNT_CHILDREN8(XML, NAME)                                     \
  ak_3mf_count_children_packed((XML),                                         \
                               _s_ak_##NAME##_u64_exact,                     \
                               _s_ak_##NAME##_len)
#define AK_3MF_ENTRY_NAME_EQ(NAME, NAME_LEN, PATH)                            \
  ak_3mf_entry_name_eq_sz((NAME), (NAME_LEN), (PATH), sizeof(PATH) - 1u)

static const char AK_3MF_CONTENT_TYPES_PART[] = "[Content_Types].xml";
static const char AK_3MF_ROOT_RELS_PART[]     = "_rels/.rels";
static const char AK_3MF_CT_RELS[]            = "application/vnd.openxmlformats-package.relationships+xml";
static const char AK_3MF_CT_MODEL[]           = "application/vnd.ms-package.3dmanufacturing-3dmodel+xml";

typedef struct AK3MFPackageInflateJob {
  AK3MFFastPreparedModel *preparedModel;
  const char             *path;
  void     *data;
  size_t    capacity;
  size_t    expectedSize;
  size_t    writtenSize;
  size_t    entryIndex;
  AkResult  result;
  bool      prepareModel;
  bool      preferVendorPaint;
} AK3MFPackageInflateJob;

typedef struct AK3MFPackageImportState {
  AkDoc                   *doc;
  AkPrintDocument         *print;
  AK3MFImportState        *importState;
  AkZipArchive            *package;
  AK3MFPackageInflateJob  *jobs;
  const char              *filepath;
  const char              *modelPath;
  xml_t                   *contentTypesRoot;
  xml_t                   *rootRelsRoot;
  size_t                   jobCount;
  size_t                   jobCapacity;
  size_t                   modelJobCount;
  size_t                   totalInflateBytes;
  AkResult                 result;
} AK3MFPackageImportState;

typedef IOBuffer AK3MFXMLBuffer;

static
bool
ak_3mf_slice_eq_cstr(const char * __restrict slice,
                     size_t                  sliceLen,
                     const char * __restrict str) {
  size_t len;

  if (!slice || !str)
    return false;

  len = strlen(str);
  return len == sliceLen && memcmp(slice, str, len) == 0;
}

bool
ak_3mf_slice_eq_lit_ci(const char * __restrict slice,
                       size_t                  sliceLen,
                       const char * __restrict lit,
                       size_t                  litLen) {
  size_t i;

  if (!slice || !lit || sliceLen != litLen)
    return false;

  for (i = 0u; i < litLen; i++) {
    char a;
    char b;

    a = slice[i];
    b = lit[i];
    if (a >= 'A' && a <= 'Z')
      a = (char)(a + ('a' - 'A'));
    if (b >= 'A' && b <= 'Z')
      b = (char)(b + ('a' - 'A'));
    if (a != b)
      return false;
  }

  return true;
}

bool
ak_3mf_cstr_starts_lit(const char * __restrict str,
                       const char * __restrict lit,
                       size_t                  litLen) {
  return str && lit && strncmp(str, lit, litLen) == 0;
}

static
bool
ak_3mf_cstr_ends_lit(const char * __restrict str,
                     const char * __restrict lit,
                     size_t                  litLen) {
  size_t len;

  if (!str || !lit)
    return false;

  len = strlen(str);
  return len >= litLen && memcmp(str + len - litLen, lit, litLen) == 0;
}

static
bool
ak_3mf_text_char(char c) {
  return (c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z')
         || (c >= '0' && c <= '9');
}

static
bool
ak_3mf_token_eq_ci(const char * __restrict token,
                   size_t                  tokenLen,
                   const char * __restrict lit,
                   size_t                  litLen) {
  return ak_3mf_slice_eq_lit_ci(token, tokenLen, lit, litLen);
}

static
bool
ak_3mf_token_starts_ci(const char * __restrict token,
                       size_t                  tokenLen,
                       const char * __restrict lit,
                       size_t                  litLen) {
  if (!token || !lit || tokenLen < litLen)
    return false;
  return ak_3mf_slice_eq_lit_ci(token, litLen, lit, litLen);
}

static
bool
ak_3mf_text_has_token_ci(const char * __restrict text,
                         size_t                  len,
                         const char * __restrict lit,
                         size_t                  litLen) {
  size_t i;

  if (!text || !lit || litLen == 0u)
    return false;

  i = 0u;
  while (i < len) {
    size_t begin;

    while (i < len && !ak_3mf_text_char(text[i]))
      i++;
    begin = i;
    while (i < len && ak_3mf_text_char(text[i]))
      i++;
    if (i > begin && ak_3mf_token_eq_ci(text + begin, i - begin, lit, litLen))
      return true;
  }

  return false;
}

static
bool
ak_3mf_text_has_token_prefix_ci(const char * __restrict text,
                                size_t                  len,
                                const char * __restrict lit,
                                size_t                  litLen) {
  size_t i;

  if (!text || !lit || litLen == 0u)
    return false;

  i = 0u;
  while (i < len) {
    size_t begin;

    while (i < len && !ak_3mf_text_char(text[i]))
      i++;
    begin = i;
    while (i < len && ak_3mf_text_char(text[i]))
      i++;
    if (i > begin && ak_3mf_token_starts_ci(text + begin, i - begin, lit, litLen))
      return true;
  }

  return false;
}

#define AK_3MF_TEXT_HAS_TOKEN(TEXT, LEN, LIT)                                 \
  ak_3mf_text_has_token_ci((TEXT), (LEN), "" LIT, sizeof("" LIT) - 1u)
#define AK_3MF_TEXT_HAS_TOKEN_PREFIX(TEXT, LEN, LIT)                          \
  ak_3mf_text_has_token_prefix_ci((TEXT),                                     \
                                  (LEN),                                      \
                                  "" LIT,                                    \
                                  sizeof("" LIT) - 1u)

static
const char*
ak_3mf_skip_root_slash(const char * __restrict path,
                       size_t     * __restrict len) {
  if (!path || !len)
    return path;

  while (*len > 0u && (*path == '/' || *path == '\\')) {
    path++;
    (*len)--;
  }

  return path;
}

static
bool
ak_3mf_entry_name_eq_sz(const char * __restrict name,
                        size_t                  nameLen,
                        const char * __restrict path,
                        size_t                  pathLen) {
  if (!name || !path)
    return false;

  name = ak_3mf_skip_root_slash(name, &nameLen);
  path = ak_3mf_skip_root_slash(path, &pathLen);
  return nameLen == pathLen && memcmp(name, path, nameLen) == 0;
}

static
bool
ak_3mf_entry_name_eq(const char * __restrict name,
                     size_t                  nameLen,
                     const char * __restrict path) {
  return path
         ? ak_3mf_entry_name_eq_sz(name, nameLen, path, strlen(path))
         : false;
}

static
xml_attr_t*
ak_3mf_xmla_local_sz(const xml_t * __restrict xml,
                     const char  * __restrict name,
                     size_t                   nameLen) {
  xml_attr_t *attr;

  if (!xml || !name)
    return NULL;

  if ((attr = xmla_sz(xml, name, nameLen)))
    return attr;

  for (attr = xml->attr; attr; attr = attr->next) {
    const char *attrName;
    const char *colon;
    size_t      attrNameLen;

    if (!attr->name || attr->namesize < nameLen)
      continue;

    attrName    = attr->name;
    attrNameLen = attr->namesize;
    colon       = memchr(attrName, ':', attrNameLen);
    if (colon) {
      attrNameLen -= (size_t)(colon - attrName) + 1u;
      attrName     = colon + 1u;
    }

    if (attrNameLen == nameLen && memcmp(attrName, name, nameLen) == 0)
      return attr;
  }

  return NULL;
}

#define AK_3MF_XMLA_LOCAL_LIT(XML, NAME)                                      \
  ak_3mf_xmla_local_sz((XML), "" NAME, sizeof("" NAME) - 1u)

static
const char*
ak_3mf_strdup_path_attr_local_sz(AkDoc       * __restrict doc,
                                 xml_t       * __restrict xml,
                                 const char  * __restrict name,
                                 size_t                   nameLen,
                                 void        * __restrict parent) {
  xml_attr_t *attr;
  AkHeap     *heap;
  const char *src;
  size_t      len;

  if (!doc)
    return NULL;

  attr = ak_3mf_xmla_local_sz(xml, name, nameLen);
  if (!attr || !attr->val || attr->valsize == 0u)
    return NULL;

  src = attr->val;
  len = attr->valsize;
  src = ak_3mf_skip_root_slash(src, &len);
  heap = ak_heap_getheap(doc);
  return heap ? ak_heap_strndup(heap, parent, src, len) : NULL;
}

static
bool
ak_3mf_attr_value_eq_entry(const xml_attr_t * __restrict attr,
                           const char       * __restrict entryName) {
  const char *value;
  size_t      valueLen;

  if (!attr || !attr->val || !entryName)
    return false;

  value    = attr->val;
  valueLen = attr->valsize;
  return ak_3mf_entry_name_eq(value, valueLen, entryName);
}

static
bool
ak_3mf_model_path_eq(const char * __restrict a,
                     const char * __restrict b) {
  size_t aLen;
  size_t bLen;

  if (a == b)
    return true;
  if (!a || !b)
    return false;

  aLen = strlen(a);
  bLen = strlen(b);
  a = ak_3mf_skip_root_slash(a, &aLen);
  b = ak_3mf_skip_root_slash(b, &bLen);
  return aLen == bLen && memcmp(a, b, aLen) == 0;
}

static
char*
ak_3mf_attr_dup_cstr(const xml_attr_t * __restrict attr) {
  char  *out;
  size_t len;

  if (!attr || !attr->val)
    return NULL;

  len = attr->valsize;
  out = malloc(len + 1u);
  if (!out)
    return NULL;

  memcpy(out, attr->val, len);
  out[len] = '\0';
  return out;
}

static
char*
ak_3mf_path_slice_dup_cstr(const char * __restrict src, size_t len) {
  char       *out;

  if (!src)
    return NULL;

  src = ak_3mf_skip_root_slash(src, &len);
  out = malloc(len + 1u);
  if (!out)
    return NULL;

  memcpy(out, src, len);
  out[len] = '\0';
  return out;
}

static
char*
ak_3mf_path_dup_cstr(const char * __restrict path) {
  return path ? ak_3mf_path_slice_dup_cstr(path, strlen(path)) : NULL;
}

static
char*
ak_3mf_attr_dup_path_cstr(const xml_attr_t * __restrict attr) {
  if (!attr || !attr->val || attr->valsize == 0u)
    return NULL;

  return ak_3mf_path_slice_dup_cstr(attr->val, attr->valsize);
}

#define AK_3MF_XML_BUF_INITIAL_CAP 1024u
#define ak_3mf_xml_buf_raw(BUF, DATA, LEN)                                    \
  io_buffer_raw((BUF), (DATA), (LEN), AK_3MF_XML_BUF_INITIAL_CAP)
#define ak_3mf_xml_buf_lit(BUF, LIT)                                          \
  IO_BUFFER_LIT((BUF), LIT, AK_3MF_XML_BUF_INITIAL_CAP)
#define ak_3mf_xml_buf_ch(BUF, CH)                                            \
  io_buffer_ch((BUF), (CH), AK_3MF_XML_BUF_INITIAL_CAP)
#define ak_3mf_xml_buf_terminate(BUF)                                         \
  io_buffer_terminate((BUF), AK_3MF_XML_BUF_INITIAL_CAP)

static
void
ak_3mf_serialize_xml_node(AK3MFXMLBuffer * __restrict buf,
                          xml_t          * __restrict xml,
                          bool                        forceImplicitPrefix) {
  const xml_attr_t *attr;
  const char       *prefix;
  uint16_t          prefixSize;

  if (!io_buffer_ok(buf) || !xml)
    return;

  if (xml->type == XML_STRING) {
    ak_3mf_xml_buf_raw(buf, xml->val, xml->valsize);
    return;
  }
  if (xml->type != XML_ELEMENT || !xml->tag)
    return;

  prefix     = xml->prefix;
  prefixSize = xml->prefixsize;
  if (!prefix && forceImplicitPrefix) {
    prefix     = "i";
    prefixSize = 1u;
  }

  ak_3mf_xml_buf_ch(buf, '<');
  if (prefix && prefixSize > 0u) {
    ak_3mf_xml_buf_raw(buf, prefix, prefixSize);
    ak_3mf_xml_buf_ch(buf, ':');
  }
  ak_3mf_xml_buf_raw(buf, xml->tag, xml->tagsize);

  for (attr = xml->attr; attr; attr = attr->next) {
    char quote;

    if (!attr->name || !attr->val)
      continue;

    quote = attr->valquote ? (char)attr->valquote : '"';
    ak_3mf_xml_buf_ch(buf, ' ');
    ak_3mf_xml_buf_raw(buf, attr->name, attr->namesize);
    ak_3mf_xml_buf_ch(buf, '=');
    ak_3mf_xml_buf_ch(buf, quote);
    ak_3mf_xml_buf_raw(buf, attr->val, attr->valsize);
    ak_3mf_xml_buf_ch(buf, quote);
  }

  if (xml->val) {
    xml_t *child;

    ak_3mf_xml_buf_ch(buf, '>');
    for (child = xml->val; child; child = child->next)
      ak_3mf_serialize_xml_node(buf, child, forceImplicitPrefix);
    ak_3mf_xml_buf_lit(buf, "</");
    if (prefix && prefixSize > 0u) {
      ak_3mf_xml_buf_raw(buf, prefix, prefixSize);
      ak_3mf_xml_buf_ch(buf, ':');
    }
    ak_3mf_xml_buf_raw(buf, xml->tag, xml->tagsize);
    ak_3mf_xml_buf_ch(buf, '>');
  } else {
    ak_3mf_xml_buf_lit(buf, "/>");
  }
}

static
char*
ak_3mf_xml_fragment_dup(xml_t * __restrict xml, bool forceImplicitPrefix) {
  AK3MFXMLBuffer buf;

  io_buffer_init(&buf);
  ak_3mf_serialize_xml_node(&buf, xml, forceImplicitPrefix);
  ak_3mf_xml_buf_terminate(&buf);
  if (!io_buffer_ok(&buf)) {
    free(buf.data);
    return NULL;
  }

  return buf.data;
}

static
bool
ak_3mf_reserve_properties(AK3MFImportState * __restrict st,
                          size_t                         extra) {
  AK3MFPropertyGroup *properties;
  size_t              needed;
  size_t              newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->propertyCount > SIZE_MAX - extra)
    return false;

  needed = st->propertyCount + extra;
  if (needed <= st->propertyCapacity)
    return true;

  newCapacity = st->propertyCapacity ? st->propertyCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  properties = realloc(st->properties, sizeof(*properties) * newCapacity);
  if (!properties)
    return false;

  memset(properties + st->propertyCapacity,
         0,
         sizeof(*properties) * (newCapacity - st->propertyCapacity));
  st->properties        = properties;
  st->propertyCapacity = newCapacity;
  return true;
}

static
bool
ak_3mf_reserve_objects(AK3MFImportState * __restrict st,
                       size_t                         extra) {
  AK3MFObject *objects;
  size_t       needed;
  size_t       newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->objectCount > SIZE_MAX - extra)
    return false;

  needed = st->objectCount + extra;
  if (needed <= st->objectCapacity)
    return true;

  newCapacity = st->objectCapacity ? st->objectCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  objects = realloc(st->objects, sizeof(*objects) * newCapacity);
  if (!objects)
    return false;

  memset(objects + st->objectCapacity,
         0,
         sizeof(*objects) * (newCapacity - st->objectCapacity));
  st->objects        = objects;
  st->objectCapacity = newCapacity;
  return true;
}

static
bool
ak_3mf_reserve_loaded_models(AK3MFImportState * __restrict st,
                             size_t                         extra) {
  const char **paths;
  size_t       needed;
  size_t       newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->loadedModelCount > SIZE_MAX - extra)
    return false;

  needed = st->loadedModelCount + extra;
  if (needed <= st->loadedModelCapacity)
    return true;

  newCapacity = st->loadedModelCapacity ? st->loadedModelCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  paths = realloc(st->loadedModelPaths, sizeof(*paths) * newCapacity);
  if (!paths)
    return false;

  memset(paths + st->loadedModelCapacity,
         0,
         sizeof(*paths) * (newCapacity - st->loadedModelCapacity));
  st->loadedModelPaths    = paths;
  st->loadedModelCapacity = newCapacity;
  return true;
}

static
bool
ak_3mf_tag_local(const xml_t  * __restrict xml,
                 const char ** __restrict outTag,
                 size_t      * __restrict outTagSize) {
  const char *colon;
  const char *xmlTag;
  size_t     xmlTagSize;

  if (!xml || !xml->tag || !outTag || !outTagSize)
    return false;

  xmlTag     = xml->tag;
  xmlTagSize = xml->tagsize;
  colon      = memchr(xmlTag, ':', xmlTagSize);
  if (colon) {
    xmlTagSize -= (size_t)(colon - xmlTag) + 1u;
    xmlTag      = colon + 1u;
  }

  *outTag     = xmlTag;
  *outTagSize = xmlTagSize;
  return true;
}

static
bool
ak_3mf_tag_sz(const xml_t * __restrict xml,
              const char  * __restrict tag,
              size_t                   tagSize) {
  const char *xmlTag;
  size_t      xmlTagSize;

  if (!tag || !ak_3mf_tag_local(xml, &xmlTag, &xmlTagSize))
    return false;

  return tagSize == xmlTagSize && memcmp(xmlTag, tag, tagSize) == 0;
}

static
bool
ak_3mf_tag_packed(const xml_t * __restrict xml,
                  uint64_t                 packed,
                  size_t                   tagSize) {
  const char *xmlTag;
  size_t      xmlTagSize;

  if (!ak_3mf_tag_local(xml, &xmlTag, &xmlTagSize))
    return false;

  return ak_str_eq_packed_fast(xmlTag, xmlTagSize, packed, tagSize);
}

static
xml_t*
ak_3mf_child_sz(xml_t      * __restrict parent,
                const char * __restrict tag,
                size_t                  tagSize) {
  xml_t *child;

  for (child = parent ? parent->val : NULL; child; child = child->next) {
    if (ak_3mf_tag_sz(child, tag, tagSize))
      return child;
  }

  return NULL;
}

static
xml_t*
ak_3mf_child_packed(xml_t    * __restrict parent,
                    uint64_t               packed,
                    size_t                 tagSize) {
  xml_t *child;

  for (child = parent ? parent->val : NULL; child; child = child->next) {
    if (ak_3mf_tag_packed(child, packed, tagSize))
      return child;
  }

  return NULL;
}

static
AkPrintFeatureFlags
ak_3mf_feature_from_text(const char * __restrict text, size_t len) {
  if (!text || len == 0u)
    return 0u;

  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "material")
      || AK_3MF_TEXT_HAS_TOKEN(text, len, "materials"))
    return AK_PRINT_FEATURE_MATERIALS;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "production"))
    return AK_PRINT_FEATURE_PRODUCTION;
  if (AK_3MF_TEXT_HAS_TOKEN_PREFIX(text, len, "slic"))
    return AK_PRINT_FEATURE_SLICE;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "beamlattice")
      || (AK_3MF_TEXT_HAS_TOKEN(text, len, "beam")
          && AK_3MF_TEXT_HAS_TOKEN(text, len, "lattice")))
    return AK_PRINT_FEATURE_BEAM_LATTICE;
  if (AK_3MF_TEXT_HAS_TOKEN_PREFIX(text, len, "boolean"))
    return AK_PRINT_FEATURE_BOOLEAN;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "displacement"))
    return AK_PRINT_FEATURE_DISPLACEMENT;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "volumetric")
      || AK_3MF_TEXT_HAS_TOKEN(text, len, "implicit"))
    return AK_PRINT_FEATURE_VOLUMETRIC;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "secure")
      || AK_3MF_TEXT_HAS_TOKEN(text, len, "protected")
      || AK_3MF_TEXT_HAS_TOKEN(text, len, "signature")
      || AK_3MF_TEXT_HAS_TOKEN(text, len, "encryption"))
    return AK_PRINT_FEATURE_SECURE_CONTENT;
  if (AK_3MF_TEXT_HAS_TOKEN_PREFIX(text, len, "textur"))
    return AK_PRINT_FEATURE_TEXTURES;
  if (AK_3MF_TEXT_HAS_TOKEN(text, len, "thumbnail"))
    return AK_PRINT_FEATURE_THUMBNAIL;

  return 0u;
}

static
AkPrintFeatureFlags
ak_3mf_feature_from_cstr(const char * __restrict text) {
  return text ? ak_3mf_feature_from_text(text, strlen(text)) : 0u;
}

static
void
ak_3mf_mark_required_feature(AkPrintDocument    * __restrict print,
                             AkPrintFeatureFlags             feature) {
  static const AkPrintFeatureFlags semanticFeatures =
    AK_PRINT_FEATURE_CORE
    | AK_PRINT_FEATURE_MATERIALS
    | AK_PRINT_FEATURE_PACKAGE
    | AK_PRINT_FEATURE_PRODUCTION
    | AK_PRINT_FEATURE_SLICE
    | AK_PRINT_FEATURE_BEAM_LATTICE
    | AK_PRINT_FEATURE_BOOLEAN
    | AK_PRINT_FEATURE_DISPLACEMENT
    | AK_PRINT_FEATURE_VOLUMETRIC;

  if (!print)
    return;

  if (feature == 0u) {
    print->features |= AK_PRINT_FEATURE_UNKNOWN;
    print->requiredFeatures |= AK_PRINT_FEATURE_UNKNOWN;
    print->unknownExtensionCount++;
    ak_printSetUnsupportedFeature(print, AK_PRINT_FEATURE_UNKNOWN);
    print->validationFlags |= AK_PRINT_VALIDATION_LOSSY_IMPORT;
    return;
  }

  print->features |= feature;
  print->requiredFeatures |= feature;
  if ((feature & ~semanticFeatures) != 0u) {
    ak_printSetUnsupportedFeature(print, feature & ~semanticFeatures);
    print->validationFlags |= AK_PRINT_VALIDATION_LOSSY_IMPORT;
  }
}

static
void
ak_3mf_add_production_item(AK3MFImportState        * __restrict st,
                           AkPrintProductionItemType            type,
                           xml_t                  * __restrict xml,
                           uint32_t                            objectId,
                           uint32_t                            parentObjectId) {
  xml_attr_t *uuidAttr;
  xml_attr_t *pathAttr;
  xml_attr_t *partNumberAttr;
  xml_attr_t *modelResolutionAttr;
  char       *path;
  char       *uuid;
  char       *partNumber;
  char       *modelResolution;

  if (!st || !st->doc || !st->print || !xml)
    return;

  uuidAttr            = AK_3MF_XMLA_LOCAL(xml, UUID);
  pathAttr            = AK_3MF_XMLA_LOCAL(xml, path);
  partNumberAttr      = AK_3MF_XMLA_LOCAL(xml, partnumber);
  modelResolutionAttr = AK_3MF_XMLA_LOCAL(xml, modelresolution);

  if ((!uuidAttr || !uuidAttr->val)
      && (!pathAttr || !pathAttr->val)
      && (!partNumberAttr || !partNumberAttr->val)
      && (!modelResolutionAttr || !modelResolutionAttr->val))
    return;

  uuid            = ak_3mf_attr_dup_cstr(uuidAttr);
  path            = ak_3mf_attr_dup_path_cstr(pathAttr);
  if (!path) {
    switch (type) {
      case AK_PRINT_PRODUCTION_ITEM:
        path = ak_3mf_path_dup_cstr(st->rootModelPath);
        break;
      case AK_PRINT_PRODUCTION_OBJECT:
      case AK_PRINT_PRODUCTION_COMPONENT:
        path = ak_3mf_path_dup_cstr(st->currentModelPath);
        break;
      default:
        break;
    }
  }
  partNumber      = ak_3mf_attr_dup_cstr(partNumberAttr);
  modelResolution = ak_3mf_attr_dup_cstr(modelResolutionAttr);
  if (!uuid && !path && !partNumber && !modelResolution)
    return;

  (void)ak_printAddProductionItem(st->doc,
                                  type,
                                  uuid,
                                  path,
                                  partNumber,
                                  modelResolution,
                                  objectId,
                                  parentObjectId);
  free(uuid);
  free(path);
  free(partNumber);
  free(modelResolution);
}

static
const xml_attr_t*
ak_3mf_xmlns_for_prefix(const xml_t * __restrict xml,
                        const char  * __restrict prefix,
                        size_t                   prefixLen) {
  const xml_attr_t *attr;

  if (!xml || !prefix)
    return NULL;

  for (attr = xml->attr; attr; attr = attr->next) {
    if (!attr->name || attr->namesize != prefixLen + 6u)
      continue;
    if (memcmp(attr->name, "xmlns:", 6u) == 0
        && memcmp(attr->name + 6u, prefix, prefixLen) == 0)
      return attr;
  }

  return NULL;
}

static
void
ak_3mf_mark_model_extensions(AkPrintDocument * __restrict print,
                             const xml_t     * __restrict root) {
  const xml_attr_t *attr;
  const xml_attr_t *required;
  const char       *it;
  const char       *end;

  if (!print || !root)
    return;

  for (attr = root->attr; attr; attr = attr->next) {
    AkPrintFeatureFlags feature;

    if (!attr->name || !attr->val || attr->namesize < 5u)
      continue;
    if (attr->namesize == 5u && memcmp(attr->name, "xmlns", 5u) == 0)
      continue;
    if (attr->namesize <= 6u || memcmp(attr->name, "xmlns:", 6u) != 0)
      continue;

    feature = ak_3mf_feature_from_text(attr->val, attr->valsize);
    if (feature)
      print->features |= feature;
  }

  required = AK_3MF_XMLA(root, requiredextensions);
  if (!required || !required->val || required->valsize == 0u)
    return;

  it  = required->val;
  end = required->val + required->valsize;
  while (it < end) {
    const char       *token;
    size_t            tokenLen;
    const xml_attr_t *xmlnsAttr;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    token = it;
    while (it < end && *it != ' ' && *it != '\t' && *it != '\n' && *it != '\r')
      it++;
    tokenLen = (size_t)(it - token);
    if (tokenLen == 0u)
      continue;

    xmlnsAttr = ak_3mf_xmlns_for_prefix(root, token, tokenLen);
    if (xmlnsAttr && xmlnsAttr->val)
      ak_3mf_mark_required_feature(print,
                                   ak_3mf_feature_from_text(xmlnsAttr->val,
                                                            xmlnsAttr->valsize));
    else
      ak_3mf_mark_required_feature(print, 0u);
  }
}

static
double
ak_3mf_unit_scale(const xml_attr_t * __restrict unitAttr,
                  const char      ** __restrict unitName) {
  if (!unitAttr || !unitAttr->val) {
    *unitName = "millimeter";
    return 0.001;
  }

  if (unitAttr->valsize == 6 && memcmp(unitAttr->val, "micron", 6) == 0) {
    *unitName = "micron";
    return 0.000001;
  }
  if (unitAttr->valsize == 10 && memcmp(unitAttr->val, "millimeter", 10) == 0) {
    *unitName = "millimeter";
    return 0.001;
  }
  if (unitAttr->valsize == 10 && memcmp(unitAttr->val, "centimeter", 10) == 0) {
    *unitName = "centimeter";
    return 0.01;
  }
  if (unitAttr->valsize == 4 && memcmp(unitAttr->val, "inch", 4) == 0) {
    *unitName = "inch";
    return 0.0254;
  }
  if (unitAttr->valsize == 4 && memcmp(unitAttr->val, "foot", 4) == 0) {
    *unitName = "foot";
    return 0.3048;
  }
  if (unitAttr->valsize == 5 && memcmp(unitAttr->val, "meter", 5) == 0) {
    *unitName = "meter";
    return 1.0;
  }

  *unitName = "millimeter";
  return 0.001;
}

static
char*
ak_3mf_strdup_attr_path(const xml_attr_t * __restrict attr) {
  const char *src;
  size_t      len;
  char       *dst;

  if (!attr || !attr->val || attr->valsize == 0)
    return NULL;

  src = attr->val;
  len = attr->valsize;
  while (len > 0 && (*src == '/' || *src == '\\')) {
    src++;
    len--;
  }

  dst = malloc(len + 1u);
  if (!dst)
    return NULL;

  memcpy(dst, src, len);
  dst[len] = '\0';
  return dst;
}

static
char*
ak_3mf_model_path_from_rels(AkZipArchive * __restrict package,
                            const char   * __restrict filepath) {
  static const char defaultPath[] = "3D/3dmodel.model";
  void            *relsData;
  size_t           relsSize;
  xml_doc_t       *xdoc;
  xml_t           *root;
  xml_t           *rel;
  char            *target;

  target   = NULL;
  relsData = NULL;
  relsSize = 0;
  if (package) {
    if (ak_zip_archive_extract_file(package,
                                    AK_3MF_ROOT_RELS_PART,
                                    &relsData,
                                    &relsSize) != AK_OK)
      goto fallback;
  } else if (ak_zip_extract_file(filepath,
                                 AK_3MF_ROOT_RELS_PART,
                                 &relsData,
                                 &relsSize) != AK_OK) {
    goto fallback;
  }

  xdoc = xml_parse(relsData, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root) {
    if (xdoc)
      xml_free(xdoc);
    free(relsData);
    goto fallback;
  }

  root = xdoc->root;
  for (rel = root->val; rel; rel = rel->next) {
    xml_attr_t *type;

    if (!AK_3MF_TAG(rel, Relationship))
      continue;

    type = AK_3MF_XMLA(rel, Type);
    if (type
        && (!type->val
            || !AK_3MF_TEXT_HAS_TOKEN(type->val, type->valsize, "3dmodel")))
      continue;

    target = ak_3mf_strdup_attr_path(AK_3MF_XMLA(rel, Target));
    if (target)
      break;
  }

  xml_free(xdoc);
  free(relsData);
  if (target)
    return target;

fallback:
  target = malloc(sizeof(defaultPath));
  if (target)
    memcpy(target, defaultPath, sizeof(defaultPath));
  return target;
}

static
const char*
ak_3mf_entry_extension(const char * __restrict entryName) {
  const char *dot;
  const char *slash;
  const char *it;

  if (!entryName)
    return NULL;

  dot   = NULL;
  slash = entryName;
  for (it = entryName; *it; it++) {
    if (*it == '/' || *it == '\\') {
      slash = it + 1;
      dot   = NULL;
    } else if (*it == '.') {
      dot = it + 1;
    }
  }

  return dot && dot > slash ? dot : NULL;
}

static
bool
ak_3mf_entry_is_2d_part(const char * __restrict entryName) {
  size_t len;

  if (!entryName)
    return false;

  len       = strlen(entryName);
  entryName = ak_3mf_skip_root_slash(entryName, &len);
  return len > 3u
         && entryName[0] == '2'
         && entryName[1] == 'D'
         && (entryName[2] == '/' || entryName[2] == '\\');
}

static
const char*
ak_3mf_content_type_dup(AkDoc       * __restrict doc,
                        xml_t       * __restrict contentTypesRoot,
                        const char  * __restrict entryName) {
  AkHeap  *heap;
  xml_t   *child;
  const char *ext;

  if (!doc || !entryName)
    return NULL;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return NULL;

  if (contentTypesRoot) {
    for (child = contentTypesRoot->val; child; child = child->next) {
      xml_attr_t *partName;
      xml_attr_t *contentType;

      if (!AK_3MF_TAG8(child, Override))
        continue;

      partName = AK_3MF_XMLA(child, PartName);
      if (!ak_3mf_attr_value_eq_entry(partName, entryName))
        continue;

      contentType = AK_3MF_XMLA(child, ContentType);
      if (contentType && contentType->val)
        return ak_heap_strndup(heap, doc, contentType->val, contentType->valsize);
    }

    ext = ak_3mf_entry_extension(entryName);
    if (ext) {
      for (child = contentTypesRoot->val; child; child = child->next) {
        xml_attr_t *extension;
        xml_attr_t *contentType;

        if (!AK_3MF_TAG8(child, Default))
          continue;

        extension = AK_3MF_XMLA(child, Extension);
        if (!extension
            || !extension->val
            || !ak_3mf_slice_eq_cstr(extension->val, extension->valsize, ext))
          continue;

        contentType = AK_3MF_XMLA(child, ContentType);
        if (contentType && contentType->val)
          return ak_heap_strndup(heap, doc, contentType->val, contentType->valsize);
      }
    }
  }

  ext = ak_3mf_entry_extension(entryName);
  if (ext) {
    if (strcmp(ext, "rels") == 0)
      return AK_3MF_CT_RELS;
    if (strcmp(ext, "model") == 0)
      return AK_3MF_CT_MODEL;
    if (strcmp(ext, "png") == 0)
      return "image/png";
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0)
      return "image/jpeg";
  }

  return "application/octet-stream";
}

static
const char*
ak_3mf_root_relationship_type_dup(AkDoc       * __restrict doc,
                                  xml_t       * __restrict relsRoot,
                                  const char  * __restrict entryName) {
  AkHeap *heap;
  xml_t  *rel;

  if (!doc || !relsRoot || !entryName)
    return NULL;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return NULL;

  for (rel = relsRoot->val; rel; rel = rel->next) {
    xml_attr_t *target;
    xml_attr_t *type;

    if (!AK_3MF_TAG(rel, Relationship))
      continue;

    target = AK_3MF_XMLA(rel, Target);
    if (!ak_3mf_attr_value_eq_entry(target, entryName))
      continue;

    type = AK_3MF_XMLA(rel, Type);
    if (type && type->val)
      return ak_heap_strndup(heap, doc, type->val, type->valsize);
  }

  return NULL;
}

static
xml_t*
ak_3mf_root_relationship_for_entry(xml_t       * __restrict relsRoot,
                                   const char  * __restrict entryName) {
  xml_t *rel;

  if (!relsRoot || !entryName)
    return NULL;

  for (rel = relsRoot->val; rel; rel = rel->next) {
    xml_attr_t *target;

    if (!AK_3MF_TAG(rel, Relationship))
      continue;

    target = AK_3MF_XMLA(rel, Target);
    if (ak_3mf_attr_value_eq_entry(target, entryName))
      return rel;
  }

  return NULL;
}

static
AkPrintPackagePartType
ak_3mf_package_part_type(const char * __restrict entryName,
                         const char * __restrict contentType,
                         const char * __restrict relationshipType) {
  size_t entryLen;
  size_t contentTypeLen;
  size_t relationshipTypeLen;

  entryLen            = entryName ? strlen(entryName) : 0u;
  contentTypeLen      = contentType ? strlen(contentType) : 0u;
  relationshipTypeLen = relationshipType ? strlen(relationshipType) : 0u;

  if (ak_3mf_cstr_ends_lit(entryName, ".rels", sizeof(".rels") - 1u)
      || AK_3MF_TEXT_HAS_TOKEN(contentType, contentTypeLen, "relationships"))
    return AK_PRINT_PACKAGE_PART_RELATIONSHIPS;
  if (AK_3MF_TEXT_HAS_TOKEN(relationshipType, relationshipTypeLen, "thumbnail")
      || AK_3MF_TEXT_HAS_TOKEN(entryName, entryLen, "thumbnail"))
    return AK_PRINT_PACKAGE_PART_THUMBNAIL;
  if (ak_3mf_entry_is_2d_part(entryName))
    return AK_PRINT_PACKAGE_PART_SLICE;
  if (ak_3mf_cstr_ends_lit(entryName, ".gcode", sizeof(".gcode") - 1u)
      || AK_3MF_TEXT_HAS_TOKEN(contentType, contentTypeLen, "gcode"))
    return AK_PRINT_PACKAGE_PART_GCODE;
  if (AK_3MF_TEXT_HAS_TOKEN(contentType, contentTypeLen, "printticket")
      || ak_3mf_cstr_starts_lit(entryName, "Metadata/", sizeof("Metadata/") - 1u))
    return AK_PRINT_PACKAGE_PART_METADATA;
  if (AK_3MF_TEXT_HAS_TOKEN(contentType, contentTypeLen, "3dmodel"))
    return AK_PRINT_PACKAGE_PART_MODEL;
  if (AK_3MF_TEXT_HAS_TOKEN_PREFIX(entryName, entryLen, "slic"))
    return AK_PRINT_PACKAGE_PART_SLICE;
  if (ak_3mf_cstr_starts_lit(contentType, "image/", sizeof("image/") - 1u)
      || ak_3mf_cstr_starts_lit(entryName, "Textures/", sizeof("Textures/") - 1u))
    return AK_PRINT_PACKAGE_PART_TEXTURE;

  return AK_PRINT_PACKAGE_PART_OTHER;
}

static
void
ak_3mf_mark_package_part_features(AkPrintDocument       * __restrict print,
                                  AkPrintPackagePartType             type,
                                  const char           * __restrict name,
                                  const char           * __restrict contentType,
                                  const char           * __restrict relationshipType) {
  AkPrintFeatureFlags features;

  if (!print)
    return;

  features = ak_3mf_feature_from_cstr(name)
             | ak_3mf_feature_from_cstr(contentType)
             | ak_3mf_feature_from_cstr(relationshipType);

  switch (type) {
    case AK_PRINT_PACKAGE_PART_MODEL:
      features |= AK_PRINT_FEATURE_CORE;
      break;
    case AK_PRINT_PACKAGE_PART_THUMBNAIL:
      features |= AK_PRINT_FEATURE_THUMBNAIL;
      break;
    case AK_PRINT_PACKAGE_PART_TEXTURE:
      features |= AK_PRINT_FEATURE_TEXTURES;
      break;
    case AK_PRINT_PACKAGE_PART_SLICE:
    case AK_PRINT_PACKAGE_PART_GCODE:
      features |= AK_PRINT_FEATURE_SLICE;
      break;
    default:
      break;
  }

  print->features |= features;
}

#define AK_3MF_PACKAGE_INFLATE_PARALLEL_MIN_JOBS  2u
#define AK_3MF_PACKAGE_INFLATE_PARALLEL_MIN_BYTES (8u * 1024u * 1024u)
#define AK_3MF_PACKAGE_INFLATE_MAX_THREADS        8u
#define AK_3MF_PACKAGE_PREPARE_MIN_MODEL_JOBS     1u

typedef struct AK3MFPackageInflateWorker {
  AkZipArchive           *package;
  AK3MFPackageInflateJob *jobs;
  size_t                  jobCount;
  size_t                  startIndex;
  size_t                  stride;
  AkResult                result;
} AK3MFPackageInflateWorker;

static
bool
ak_3mf_package_add_inflate_job(AK3MFPackageImportState * __restrict st,
                               size_t                               entryIndex,
                               const char             * __restrict path,
                               void                   * __restrict data,
                               size_t                              expectedSize,
                               bool                                prepareModel) {
  AK3MFPackageInflateJob *jobs;
  AK3MFPackageInflateJob *job;
  size_t                  newCapacity;

  if (!st || !data)
    return false;

  if (st->jobCount == st->jobCapacity) {
    newCapacity = st->jobCapacity ? st->jobCapacity << 1u : 16u;
    if (newCapacity <= st->jobCapacity)
      return false;

    jobs = realloc(st->jobs, sizeof(*jobs) * newCapacity);
    if (!jobs)
      return false;

    st->jobs        = jobs;
    st->jobCapacity = newCapacity;
  }

  job               = &st->jobs[st->jobCount++];
  job->preparedModel = NULL;
  job->path         = path;
  job->data         = data;
  job->capacity     = expectedSize + 1u;
  job->expectedSize = expectedSize;
  job->writtenSize  = 0u;
  job->entryIndex   = entryIndex;
  job->result       = AK_OK;
  job->prepareModel = prepareModel;
  job->preferVendorPaint = prepareModel
                           && st->importState
                           && st->importState->bambuColorCount > 0u;
  if (prepareModel)
    st->modelJobCount++;
  st->totalInflateBytes += expectedSize;
  return true;
}

static
AkResult
ak_3mf_package_inflate_jobs_serial(AkZipArchive           * __restrict package,
                                   AK3MFPackageInflateJob * __restrict jobs,
                                   size_t                              jobCount) {
  size_t i;

  if (!package || (!jobs && jobCount > 0u))
    return AK_ERR;

  for (i = 0u; i < jobCount; i++) {
    AK3MFPackageInflateJob *job;
    AkResult                result;

    job = &jobs[i];
    result = ak_zip_archive_extract_index_to(package,
                                             job->entryIndex,
                                             NULL,
                                             job->data,
                                             job->capacity,
                                             &job->writtenSize);
    job->result = result;
    if (result != AK_OK)
      return result;
    if (job->writtenSize != job->expectedSize)
      return AK_EBADF;
    if (job->prepareModel)
      job->preparedModel =
        ak_3mf_fast_prepare_model_part((const char *)job->data,
                                       job->writtenSize,
                                       job->preferVendorPaint);
  }

  return AK_OK;
}

static
void
ak_3mf_package_inflate_worker(void *userdata) {
  AK3MFPackageInflateWorker *worker;
  AkZipDecompressor         *decompressor;
  size_t                     i;

  worker = userdata;
  if (!worker || !worker->package || !worker->jobs || worker->stride == 0u) {
    if (worker)
      worker->result = AK_ERR;
    return;
  }

  decompressor = ak_zip_decompressor_new();
  if (!decompressor) {
    worker->result = AK_ERR;
    return;
  }

  worker->result = AK_OK;
  for (i = worker->startIndex; i < worker->jobCount; i += worker->stride) {
    AK3MFPackageInflateJob *job;
    AkResult                result;

    job = &worker->jobs[i];
    result = ak_zip_archive_extract_index_to(worker->package,
                                             job->entryIndex,
                                             decompressor,
                                             job->data,
                                             job->capacity,
                                             &job->writtenSize);
    job->result = result;
    if (result != AK_OK) {
      worker->result = result;
      break;
    }
    if (job->writtenSize != job->expectedSize) {
      worker->result = AK_EBADF;
      break;
    }
    if (job->prepareModel)
      job->preparedModel =
        ak_3mf_fast_prepare_model_part((const char *)job->data,
                                       job->writtenSize,
                                       job->preferVendorPaint);
  }

  ak_zip_decompressor_free(decompressor);
}

static
uint32_t
ak_3mf_package_inflate_thread_count(size_t jobCount, size_t totalBytes) {
  uint32_t cpuCount;
  uint32_t threadCount;

  if (jobCount < AK_3MF_PACKAGE_INFLATE_PARALLEL_MIN_JOBS
      || totalBytes < AK_3MF_PACKAGE_INFLATE_PARALLEL_MIN_BYTES)
    return 1u;

  cpuCount = ak_thread_cpu_count();
  if (cpuCount < 2u)
    return 1u;

  threadCount = cpuCount;
  if (threadCount > AK_3MF_PACKAGE_INFLATE_MAX_THREADS)
    threadCount = AK_3MF_PACKAGE_INFLATE_MAX_THREADS;
  if ((size_t)threadCount > jobCount)
    threadCount = (uint32_t)jobCount;

  return threadCount > 1u ? threadCount : 1u;
}

static
AkResult
ak_3mf_package_inflate_jobs(AK3MFPackageImportState * __restrict st) {
  AK3MFPackageInflateWorker *workers;
  AkThreadTask              *tasks;
  uint32_t                   threadCount;
  uint32_t                   i;
  AkResult                   result;

  if (!st || !st->package)
    return AK_ERR;
  if (st->jobCount == 0u)
    return AK_OK;

  if (st->modelJobCount < AK_3MF_PACKAGE_PREPARE_MIN_MODEL_JOBS) {
    size_t jobIndex;

    for (jobIndex = 0u; jobIndex < st->jobCount; jobIndex++)
      st->jobs[jobIndex].prepareModel = false;
  }

  threadCount = ak_3mf_package_inflate_thread_count(st->jobCount,
                                                    st->totalInflateBytes);
  if (threadCount <= 1u)
    return ak_3mf_package_inflate_jobs_serial(st->package,
                                             st->jobs,
                                             st->jobCount);

  workers = AK_ALLOCA(sizeof(*workers) * threadCount);
  tasks   = AK_ALLOCA(sizeof(*tasks) * threadCount);
  memset(workers, 0, sizeof(*workers) * threadCount);

  for (i = 0u; i < threadCount; i++) {
    workers[i].package    = st->package;
    workers[i].jobs       = st->jobs;
    workers[i].jobCount   = st->jobCount;
    workers[i].startIndex = i;
    workers[i].stride     = threadCount;
    workers[i].result     = AK_OK;
    tasks[i].func         = ak_3mf_package_inflate_worker;
    tasks[i].userdata     = &workers[i];
  }

  if (!ak_thread_run_tasks(tasks, threadCount))
    return ak_3mf_package_inflate_jobs_serial(st->package,
                                             st->jobs,
                                             st->jobCount);

  result = workers[0].result;
  for (i = 1u; i < threadCount; i++) {
    if (workers[i].result != AK_OK) {
      result = workers[i].result;
      break;
    }
  }

  return result;
}

static
bool
ak_3mf_reserve_prepared_models(AK3MFImportState * __restrict st,
                               size_t                         extra) {
  AK3MFPreparedModelEntry *entries;
  size_t                   needed;
  size_t                   newCapacity;

  if (!st)
    return false;
  if (extra == 0u)
    return true;
  if (st->preparedModelCount > SIZE_MAX - extra)
    return false;

  needed = st->preparedModelCount + extra;
  if (needed <= st->preparedModelCapacity)
    return true;

  newCapacity = st->preparedModelCapacity ? st->preparedModelCapacity * 2u : 8u;
  while (newCapacity < needed) {
    if (newCapacity > SIZE_MAX / 2u)
      return false;
    newCapacity *= 2u;
  }

  entries = realloc(st->preparedModels, sizeof(*entries) * newCapacity);
  if (!entries)
    return false;

  st->preparedModels        = entries;
  st->preparedModelCapacity = newCapacity;
  return true;
}

static
bool
ak_3mf_add_prepared_model(AK3MFImportState       * __restrict st,
                          const char             * __restrict path,
                          AK3MFFastPreparedModel * __restrict prepared) {
  AK3MFPreparedModelEntry *entry;

  if (!st || !path || !prepared)
    return false;
  if (!ak_3mf_reserve_prepared_models(st, 1u))
    return false;

  entry        = &st->preparedModels[st->preparedModelCount++];
  entry->path  = path;
  entry->model = prepared;
  return true;
}

static
AK3MFFastPreparedModel*
ak_3mf_find_prepared_model(AK3MFImportState * __restrict st,
                           const char       * __restrict modelPath) {
  size_t i;

  if (!st || !modelPath)
    return NULL;

  for (i = 0u; i < st->preparedModelCount; i++) {
    if (ak_3mf_model_path_eq(st->preparedModels[i].path, modelPath))
      return st->preparedModels[i].model;
  }

  return NULL;
}

static
void
ak_3mf_package_store_prepared_models(AK3MFPackageImportState * __restrict st) {
  size_t i;

  if (!st || !st->jobs)
    return;

  for (i = 0u; i < st->jobCount; i++) {
    AK3MFPackageInflateJob *job;

    job = &st->jobs[i];
    if (!job->preparedModel)
      continue;
    if (st->importState
        && ak_3mf_add_prepared_model(st->importState,
                                     job->path,
                                     job->preparedModel)) {
      job->preparedModel = NULL;
      continue;
    }
    ak_3mf_fast_prepared_model_free(job->preparedModel);
    job->preparedModel = NULL;
  }
}

static
bool
ak_3mf_import_package_part_visitor(const AkZipEntryInfo * __restrict info,
                                   void                 * __restrict userdata) {
  AK3MFPackageImportState *st;
  AkPrintPackagePartType   type;
  AkPrintPackagePart      *part;
  xml_t                   *relationship;
  const char              *contentType;
  const char              *relationshipType;
  char                    *relationshipId;
  char                    *relationshipTargetMode;
  char                    *entryName;
  void                    *entryData;
  size_t                   entrySize;
  AkResult                 result;

  st = userdata;
  if (!st || !info || !info->name)
    return false;

  if (info->nameLen == 0u || info->name[info->nameLen - 1u] == '/')
    return true;
  if (AK_3MF_ENTRY_NAME_EQ(info->name, info->nameLen, AK_3MF_CONTENT_TYPES_PART)
      || AK_3MF_ENTRY_NAME_EQ(info->name, info->nameLen, AK_3MF_ROOT_RELS_PART)
      || ak_3mf_entry_name_eq(info->name, info->nameLen, st->modelPath))
    return true;

  entryName = malloc(info->nameLen + 1u);
  if (!entryName) {
    st->result = AK_ERR;
    return false;
  }
  memcpy(entryName, info->name, info->nameLen);
  entryName[info->nameLen] = '\0';

  contentType      = ak_3mf_content_type_dup(st->doc, st->contentTypesRoot, entryName);
  relationshipType = ak_3mf_root_relationship_type_dup(st->doc, st->rootRelsRoot, entryName);
  relationship     = ak_3mf_root_relationship_for_entry(st->rootRelsRoot, entryName);
  type             = ak_3mf_package_part_type(entryName, contentType, relationshipType);

  entryData = NULL;
  entrySize = 0u;
  part = ak_printAddPackagePart(st->doc,
                                type,
                                entryName,
                                contentType,
                                relationshipType);
  if (!part) {
    free(entryName);
    st->result = AK_ERR;
    return false;
  }

  if (st->package) {
    AkHeap *heap;

    heap      = ak_heap_getheap(st->doc);
    entrySize = info->uncompressedSize;
    entryData = heap ? ak_heap_alloc(heap, part, entrySize + 1u) : NULL;
    if (!entryData) {
      free(entryName);
      st->result = AK_ERR;
      return false;
    }
    part->data = entryData;
    part->size = entrySize;
    result = ak_3mf_package_add_inflate_job(st,
                                            info->index,
                                            part->name,
                                            entryData,
                                            entrySize,
                                            type == AK_PRINT_PACKAGE_PART_MODEL)
             ? AK_OK
             : AK_ERR;
  } else {
    result = ak_zip_extract_file(st->filepath, entryName, &entryData, &entrySize);
  }
  if (result != AK_OK) {
    free(entryName);
    st->result = result;
    return false;
  }

  if (!st->package) {
    if (!ak_printSetPackagePartData(st->doc, part, entryData, entrySize)) {
      free(entryData);
      free(entryName);
      st->result = AK_ERR;
      return false;
    }
    free(entryData);
    entryData = NULL;
  }
  relationshipId = relationship
                   ? ak_3mf_attr_dup_cstr(AK_3MF_XMLA(relationship, Id))
                   : NULL;
  relationshipTargetMode = relationship
                           ? ak_3mf_attr_dup_cstr(
                               AK_3MF_XMLA(relationship, TargetMode))
                           : NULL;
  if (relationshipId || relationshipTargetMode) {
    if (!ak_printSetPackagePartRelationship(st->doc,
                                            part,
                                            relationshipId,
                                            relationshipTargetMode)) {
      free(relationshipTargetMode);
      free(relationshipId);
      if (!st->package)
        free(entryData);
      free(entryName);
      st->result = AK_ERR;
      return false;
    }
  }
  free(relationshipTargetMode);
  free(relationshipId);
  ak_3mf_mark_package_part_features(st->print, type, entryName, contentType, relationshipType);

  if (!st->package)
    free(entryData);
  free(entryName);
  return true;
}

static
AkResult
ak_3mf_import_package_parts(AK3MFImportState * __restrict importState,
                            AkDoc            * __restrict doc,
                            AkPrintDocument  * __restrict print,
                            AkZipArchive     * __restrict package,
                            const char       * __restrict filepath,
                            const char       * __restrict modelPath,
                            xml_t            * __restrict contentTypesRoot,
                            xml_t            * __restrict rootRelsRoot) {
  AK3MFPackageImportState st;
  AkResult                result;

  memset(&st, 0, sizeof(st));
  st.doc              = doc;
  st.print            = print;
  st.importState      = importState;
  st.package          = package;
  st.filepath         = filepath;
  st.modelPath        = modelPath;
  st.contentTypesRoot = contentTypesRoot;
  st.rootRelsRoot     = rootRelsRoot;
  st.result           = AK_OK;

  result = package
           ? ak_zip_archive_visit_entries(package,
                                          ak_3mf_import_package_part_visitor,
                                          &st)
           : ak_zip_visit_entries(filepath, ak_3mf_import_package_part_visitor, &st);
  if (result != AK_OK) {
    free(st.jobs);
    return result;
  }
  if (st.result == AK_OK && package)
    st.result = ak_3mf_package_inflate_jobs(&st);
  if (st.result == AK_OK)
    ak_3mf_package_store_prepared_models(&st);
  else {
    size_t i;

    for (i = 0u; i < st.jobCount; i++) {
      if (st.jobs[i].preparedModel)
        ak_3mf_fast_prepared_model_free(st.jobs[i].preparedModel);
    }
  }
  free(st.jobs);
  return st.result;
}

static
bool
ak_3mf_cached_package_part_data(AK3MFImportState * __restrict st,
                                const char       * __restrict modelPath,
                                const void      ** __restrict data,
                                size_t           * __restrict size) {
  AkPrintPackagePart *part;

  if (!st || !st->print || !modelPath || !data || !size)
    return false;

  for (part = st->print->parts; part; part = part->next) {
    if (!part->name || !part->data || part->size == 0u)
      continue;
    if (!ak_3mf_model_path_eq(part->name, modelPath))
      continue;

    *data = part->data;
    *size = part->size;
    return true;
  }

  return false;
}

static
AkDoc*
ak_3mf_doc_new(const char * __restrict filepath,
               xml_t      * __restrict root) {
  AkHeap *heap;
  AkDoc  *doc;
  const char *unitName;
  double      unitScale;

  heap = ak_heap_new(NULL, NULL, NULL);
  if (!heap)
    return NULL;

  doc = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!doc)
    return NULL;

  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = ak_heap_strdup(heap, doc->inf, filepath);
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = false;
  doc->inf->ftype     = AK_FILE_TYPE_3MF;
  doc->coordSys       = AK_ZUP;

  if (doc->inf->dir)
    doc->inf->dirlen = strlen(doc->inf->dir);

  unitScale = ak_3mf_unit_scale(root ? AK_3MF_XMLA(root, unit) : NULL, &unitName);

  doc->inf->base.coordSys   = AK_ZUP;
  doc->inf->base.unit       = ak_heap_calloc(heap, doc->inf, sizeof(*doc->inf->base.unit));
  doc->inf->base.unit->dist = unitScale;
  doc->inf->base.unit->name = ak_heap_strdup(heap, doc->inf->base.unit, unitName);
  doc->unit                 = doc->inf->base.unit;

  return doc;
}

static
AkScene*
ak_3mf_scene_new(AkDoc * __restrict doc) {
  AkHeap  *heap;
  AkScene *scene;
  AkNode  *root;

  heap  = ak_heap_getheap(doc);
  scene = ak_heap_calloc(heap, doc, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  if (!scene || !root)
    return NULL;

  ak_setypeid(root, AKT_NODE);
  root->visible = true;

  scene->node = root;
  doc->scene  = scene;
  AK_LIB_PREPEND(doc->lib.scenes, scene, next);

  return scene;
}

static
AkNode*
ak_3mf_node_new(AkDoc       * __restrict doc,
                AkNode      * __restrict parent,
                const char  * __restrict name) {
  AkHeap *heap;
  AkNode *node;

  heap = ak_heap_getheap(doc);
  node = ak_heap_calloc(heap, doc, sizeof(*node));
  if (!node)
    return NULL;

  ak_setypeid(node, AKT_NODE);
  node->visible = true;
  if (name)
    node->name = ak_heap_strdup(heap, node, name);

  AK_LIB_PREPEND(doc->lib.nodes, node, docNext);
  if (parent)
    ak_addSubNode(parent, node, false);

  return node;
}

static
size_t
ak_3mf_count_children_sz(const xml_t * __restrict parent,
                         const char  * __restrict tag,
                         size_t                   tagSize) {
  const xml_t *child;
  size_t       count;

  count = 0;
  if (!parent)
    return 0;

  for (child = parent->val; child; child = child->next) {
    if (ak_3mf_tag_sz(child, tag, tagSize))
      count++;
  }

  return count;
}

static
size_t
ak_3mf_count_children_packed(const xml_t * __restrict parent,
                             uint64_t                 packed,
                             size_t                   tagSize) {
  const xml_t *child;
  size_t       count;

  count = 0;
  if (!parent)
    return 0;

  for (child = parent->val; child; child = child->next) {
    if (ak_3mf_tag_packed(child, packed, tagSize))
      count++;
  }

  return count;
}

static
uint8_t
ak_3mf_hex_digit(char c) {
  if (c >= '0' && c <= '9')
    return (uint8_t)(c - '0');
  if (c >= 'a' && c <= 'f')
    return (uint8_t)(10 + c - 'a');
  if (c >= 'A' && c <= 'F')
    return (uint8_t)(10 + c - 'A');
  return 0xffu;
}

AK_HIDE
bool
ak_3mf_parse_color_slice(const char * __restrict p,
                         size_t                  len,
                         uint8_t                 rgba[4]) {
  uint32_t    i;

  rgba[0] = 255u;
  rgba[1] = 255u;
  rgba[2] = 255u;
  rgba[3] = 255u;

  if (!p || len == 0u)
    return false;

  if (len > 0 && *p == '#') {
    p++;
    len--;
  }

  if (len != 6u && len != 8u)
    return false;

  for (i = 0; i < len / 2u; i++) {
    uint8_t hi;
    uint8_t lo;

    hi = ak_3mf_hex_digit(p[i * 2u + 0u]);
    lo = ak_3mf_hex_digit(p[i * 2u + 1u]);
    if (hi == 0xffu || lo == 0xffu)
      return false;

    rgba[i] = (uint8_t)((hi << 4u) | lo);
  }

  return true;
}

static
bool
ak_3mf_parse_color_attr(const xml_attr_t * __restrict attr,
                        uint8_t                       rgba[4]) {
  if (!attr || !attr->val || attr->valsize == 0)
    return false;

  return ak_3mf_parse_color_slice(attr->val, attr->valsize, rgba);
}

AK_HIDE
AkMaterialInput*
ak_3mf_material_color_input(AkHeap     * __restrict heap,
                            void       * __restrict parent,
                            uint8_t                  rgba[4]) {
  AkMaterialInput *input;

  input = ak_heap_calloc(heap, parent, sizeof(*input));
  if (!input)
    return NULL;

  input->semantic    = _s_ak_baseColor;
  input->source      = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType   = AK_MATERIAL_VALUE_COLOR;
  input->channels    = AK_TEXTURE_CHANNEL_RGBA;
  input->colorSpace  = AK_TEXTURE_COLORSPACE_SRGB;
  input->color.rgba.R = rgba[0] / 255.0f;
  input->color.rgba.G = rgba[1] / 255.0f;
  input->color.rgba.B = rgba[2] / 255.0f;
  input->color.rgba.A = rgba[3] / 255.0f;

  return input;
}

AK_HIDE
AkMaterialInput*
ak_3mf_material_scalar_input(AkHeap      * __restrict heap,
                             void        * __restrict parent,
                             const char  * __restrict semantic,
                             float                    value) {
  AkMaterialInput *input;

  input = ak_heap_calloc(heap, parent, sizeof(*input));
  if (!input)
    return NULL;

  input->semantic   = semantic;
  input->source     = AK_MATERIAL_INPUT_CONSTANT;
  input->valueType  = AK_MATERIAL_VALUE_FLOAT;
  input->value[0]   = value;
  input->colorSpace = AK_TEXTURE_COLORSPACE_LINEAR;

  return input;
}

typedef enum AK3MFPropertyGroupKind {
  AK_3MF_PROPERTY_GROUP_UNSUPPORTED = 0,
  AK_3MF_PROPERTY_GROUP_BASE,
  AK_3MF_PROPERTY_GROUP_COLOR,
  AK_3MF_PROPERTY_GROUP_TEXTURE2D,
  AK_3MF_PROPERTY_GROUP_COMPOSITE,
  AK_3MF_PROPERTY_GROUP_MULTI
} AK3MFPropertyGroupKind;

static
bool
ak_3mf_property_group_kind(xml_t                          * __restrict xml,
                           AK3MFPropertyGroupKind         * __restrict kind,
                           AkMaterialPropertySetType      * __restrict setType) {
  if (!xml || !kind || !setType)
    return false;

  if (AK_3MF_TAG(xml, basematerials)) {
    *kind    = AK_3MF_PROPERTY_GROUP_BASE;
    *setType = AK_MATERIAL_PROPERTY_BASE;
    return true;
  }
  if (AK_3MF_TAG(xml, colorgroup)) {
    *kind    = AK_3MF_PROPERTY_GROUP_COLOR;
    *setType = AK_MATERIAL_PROPERTY_COLOR;
    return true;
  }
  if (AK_3MF_TAG(xml, texture2dgroup)) {
    *kind    = AK_3MF_PROPERTY_GROUP_TEXTURE2D;
    *setType = AK_MATERIAL_PROPERTY_TEXTURE2D;
    return true;
  }
  if (AK_3MF_TAG(xml, compositematerials)) {
    *kind    = AK_3MF_PROPERTY_GROUP_COMPOSITE;
    *setType = AK_MATERIAL_PROPERTY_COMPOSITE;
    return true;
  }
  if (AK_3MF_TAG(xml, multiproperties)) {
    *kind    = AK_3MF_PROPERTY_GROUP_MULTI;
    *setType = AK_MATERIAL_PROPERTY_MULTI;
    return true;
  }

  return false;
}

static
size_t
ak_3mf_count_property_group_children(xml_t                  * __restrict xml,
                                     AK3MFPropertyGroupKind              kind) {
  switch (kind) {
    case AK_3MF_PROPERTY_GROUP_BASE:
      return AK_3MF_COUNT_CHILDREN(xml, base);
    case AK_3MF_PROPERTY_GROUP_COLOR:
      return AK_3MF_COUNT_CHILDREN8(xml, color);
    case AK_3MF_PROPERTY_GROUP_TEXTURE2D:
      return AK_3MF_COUNT_CHILDREN(xml, tex2coord);
    case AK_3MF_PROPERTY_GROUP_COMPOSITE:
      return AK_3MF_COUNT_CHILDREN(xml, composite);
    case AK_3MF_PROPERTY_GROUP_MULTI:
      return AK_3MF_COUNT_CHILDREN8(xml, multi);
    default:
      return 0u;
  }
}

static
size_t
ak_3mf_count_property_groups(xml_t * __restrict resourcesXml) {
  xml_t *xml;
  size_t count;
  AK3MFPropertyGroupKind kind;
  AkMaterialPropertySetType setType;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (xml = resourcesXml->val; xml; xml = xml->next) {
    if (ak_3mf_property_group_kind(xml, &kind, &setType))
      count++;
  }

  return count;
}

static
AK3MFPropertyGroup*
ak_3mf_find_property_group(AK3MFImportState * __restrict st,
                           const char       * __restrict path,
                           uint32_t                      id) {
  size_t i;

  if (!st)
    return NULL;

  for (i = 0; i < st->propertyCount; i++) {
    if (st->properties[i].id == id
        && ak_3mf_model_path_eq(st->properties[i].path, path))
      return &st->properties[i];
  }

  return NULL;
}

static
void
ak_3mf_property_set_color(AkHeap             * __restrict heap,
                          AkMaterialProperty * __restrict prop,
                          void               * __restrict parent,
                          const uint8_t                    rgba[4]) {
  uint8_t color[4];

  if (!prop || !rgba)
    return;

  memcpy(color, rgba, sizeof(color));
  prop->displayColor.rgba.R = color[0] / 255.0f;
  prop->displayColor.rgba.G = color[1] / 255.0f;
  prop->displayColor.rgba.B = color[2] / 255.0f;
  prop->displayColor.rgba.A = color[3] / 255.0f;
  prop->baseColor           = ak_3mf_material_color_input(heap, parent, color);
  prop->metallic            = ak_3mf_material_scalar_input(heap,
                                                           parent,
                                                           _s_ak_metallic,
                                                           0.0f);
  prop->roughness           = ak_3mf_material_scalar_input(heap,
                                                           parent,
                                                           _s_ak_roughness,
                                                           1.0f);
}

static
bool
ak_3mf_property_color(AK3MFImportState * __restrict st,
                      const char       * __restrict path,
                      uint32_t                      id,
                      uint32_t                      index,
                      uint8_t                       rgba[4],
                      bool               * __restrict hasAlpha) {
  AK3MFPropertyGroup *group;

  group = ak_3mf_find_property_group(st, path, id);
  if (!group || !group->hasColors || !group->colors || index >= group->count)
    return false;

  memcpy(rgba, group->colors + (size_t)index * 4u, 4u);
  if (hasAlpha && rgba[3] < 255u)
    *hasAlpha = true;
  return true;
}

static
bool
ak_3mf_property_has_color(AK3MFImportState * __restrict st,
                          const char       * __restrict path,
                          uint32_t                      id,
                          uint32_t                      index) {
  AK3MFPropertyGroup *group;

  group = ak_3mf_find_property_group(st, path, id);
  return group
         && group->hasColors
         && group->colors
         && index < group->count;
}

static
bool
ak_3mf_property_texcoord(AK3MFImportState * __restrict st,
                         const char       * __restrict path,
                         uint32_t                      id,
                         uint32_t                      index,
                         float                         uv[2]) {
  AK3MFPropertyGroup *group;

  group = ak_3mf_find_property_group(st, path, id);
  if (!group || !group->hasTexcoords || !group->texcoords || index >= group->count)
    return false;

  memcpy(uv, group->texcoords + (size_t)index * 2u, sizeof(float) * 2u);
  return true;
}

static
bool
ak_3mf_property_has_texcoord(AK3MFImportState * __restrict st,
                             const char       * __restrict path,
                             uint32_t                      id,
                             uint32_t                      index) {
  AK3MFPropertyGroup *group;

  group = ak_3mf_find_property_group(st, path, id);
  return group
         && group->hasTexcoords
         && group->texcoords
         && index < group->count;
}

static
size_t
ak_3mf_attr_u32_list_count(const xml_attr_t * __restrict attr) {
  const char *it;
  const char *end;
  size_t      count;

  if (!attr || !attr->val || attr->valsize == 0u)
    return 0u;

  it    = attr->val;
  end   = attr->val + attr->valsize;
  count = 0u;
  while (it < end) {
    const char *tok;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    tok = it;
    while (it < end && *it >= '0' && *it <= '9')
      it++;
    if (it > tok)
      count++;
    while (it < end && !(*it >= '0' && *it <= '9'))
      it++;
  }

  return count;
}

static
size_t
ak_3mf_attr_u32_list(const xml_attr_t * __restrict attr,
                     uint32_t         * __restrict out,
                     size_t                         cap) {
  const char *it;
  const char *end;
  size_t      count;

  if (!attr || !attr->val || !out || cap == 0u)
    return 0u;

  it    = attr->val;
  end   = attr->val + attr->valsize;
  count = 0u;
  while (it < end && count < cap) {
    const char *tok;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    tok = it;
    while (it < end && *it >= '0' && *it <= '9')
      it++;
    if (it > tok) {
      if (!ak_str_parse_u32_slice_fast(tok, it, out + count))
        out[count] = 0u;
      count++;
    }
    while (it < end && !(*it >= '0' && *it <= '9'))
      it++;
  }

  return count;
}

static
size_t
ak_3mf_attr_float_list_count(const xml_attr_t * __restrict attr) {
  const char *it;
  const char *end;
  size_t      count;

  if (!attr || !attr->val || attr->valsize == 0u)
    return 0u;

  it    = attr->val;
  end   = attr->val + attr->valsize;
  count = 0u;
  while (it < end) {
    const char *next;
    float       ignored;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    if (!ak_str_parse_float_token_fast(it, end, &ignored, &next))
      break;
    count++;
    it = next;
    while (it < end && !((*it >= '0' && *it <= '9') || *it == '-' || *it == '+'
                         || *it == '.'))
      it++;
  }

  return count;
}

static
size_t
ak_3mf_attr_float_list(const xml_attr_t * __restrict attr,
                       float            * __restrict out,
                       size_t                         cap) {
  const char *it;
  const char *end;
  size_t      count;

  if (!attr || !attr->val || !out || cap == 0u)
    return 0u;

  it    = attr->val;
  end   = attr->val + attr->valsize;
  count = 0u;
  while (it < end && count < cap) {
    const char *next;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    if (!ak_str_parse_float_token_fast(it, end, out + count, &next))
      break;
    count++;
    it = next;
    while (it < end && !((*it >= '0' && *it <= '9') || *it == '-' || *it == '+'
                         || *it == '.'))
      it++;
  }

  return count;
}

static
void
ak_3mf_mix_property_colors(AK3MFPropertyGroup * __restrict baseGroup,
                           const uint32_t     * __restrict indices,
                           size_t                          indexCount,
                           const float        * __restrict values,
                           size_t                          valueCount,
                           uint8_t                         rgba[4],
                           bool               * __restrict hasColor) {
  double accum[4] = {0.0, 0.0, 0.0, 0.0};
  double sum;
  bool   equalWeights;
  size_t i;

  rgba[0] = rgba[1] = rgba[2] = rgba[3] = 255u;
  if (hasColor)
    *hasColor = false;
  if (!baseGroup || !baseGroup->hasColors || !baseGroup->colors
      || !indices || indexCount == 0u)
    return;

  sum = 0.0;
  for (i = 0u; i < indexCount; i++) {
    if (i < valueCount && values[i] > 0.0f)
      sum += values[i];
  }
  equalWeights = sum <= 0.0;
  if (equalWeights)
    sum = (double)indexCount;

  for (i = 0u; i < indexCount; i++) {
    const uint8_t *src;
    double weight;

    if (indices[i] >= baseGroup->count)
      continue;
    weight = i < valueCount ? values[i] : 0.0f;
    if (weight < 0.0)
      weight = 0.0;
    if (equalWeights)
      weight = 1.0;
    weight /= sum;

    src = baseGroup->colors + (size_t)indices[i] * 4u;
    accum[0] += (double)src[0] * weight;
    accum[1] += (double)src[1] * weight;
    accum[2] += (double)src[2] * weight;
    accum[3] += (double)src[3] * weight;
    if (hasColor)
      *hasColor = true;
  }

  rgba[0] = (uint8_t)(accum[0] < 0.0 ? 0.0 : accum[0] > 255.0 ? 255.0 : accum[0] + 0.5);
  rgba[1] = (uint8_t)(accum[1] < 0.0 ? 0.0 : accum[1] > 255.0 ? 255.0 : accum[1] + 0.5);
  rgba[2] = (uint8_t)(accum[2] < 0.0 ? 0.0 : accum[2] > 255.0 ? 255.0 : accum[2] + 0.5);
  rgba[3] = (uint8_t)(accum[3] < 0.0 ? 0.0 : accum[3] > 255.0 ? 255.0 : accum[3] + 0.5);
}

static
void
ak_3mf_blend_layer_color(uint8_t accum[4],
                         const uint8_t layer[4],
                         bool multiply) {
  uint32_t a;
  uint32_t invA;
  uint32_t r;
  uint32_t g;
  uint32_t b;

  a    = layer[3];
  invA = 255u - a;
  if (multiply) {
    r = ((uint32_t)accum[0] * layer[0] + 127u) / 255u;
    g = ((uint32_t)accum[1] * layer[1] + 127u) / 255u;
    b = ((uint32_t)accum[2] * layer[2] + 127u) / 255u;
  } else {
    r = layer[0];
    g = layer[1];
    b = layer[2];
  }

  accum[0] = (uint8_t)((r * a + (uint32_t)accum[0] * invA + 127u) / 255u);
  accum[1] = (uint8_t)((g * a + (uint32_t)accum[1] * invA + 127u) / 255u);
  accum[2] = (uint8_t)((b * a + (uint32_t)accum[2] * invA + 127u) / 255u);
  if (a > accum[3])
    accum[3] = (uint8_t)a;
}

static
bool
ak_3mf_blend_method_is_multiply(const xml_attr_t * __restrict attr,
                                size_t                         methodIndex) {
  const char *it;
  const char *end;
  size_t      index;

  if (!attr || !attr->val || attr->valsize == 0u)
    return false;

  it    = attr->val;
  end   = attr->val + attr->valsize;
  index = 0u;
  while (it < end) {
    const char *tok;
    size_t      len;

    while (it < end && (*it == ' ' || *it == '\t' || *it == '\n' || *it == '\r'))
      it++;
    tok = it;
    while (it < end && *it != ' ' && *it != '\t' && *it != '\n' && *it != '\r')
      it++;
    len = (size_t)(it - tok);
    if (len > 0u) {
      if (index == methodIndex)
        return len == 8u && memcmp(tok, "multiply", 8u) == 0;
      index++;
    }
  }

  return false;
}

static
size_t
ak_3mf_parse_property_groups(AK3MFImportState * __restrict st,
                             xml_t            * __restrict resourcesXml) {
  xml_t  *xml;
  size_t  added;

  if (!st || !resourcesXml)
    return 0;

  if (!ak_3mf_reserve_properties(st, ak_3mf_count_property_groups(resourcesXml)))
    return 0;

  added = 0;
  for (xml = resourcesXml->val; xml; xml = xml->next) {
    AK3MFPropertyGroup *group;
    AkMaterialPropertySet *set;
    AkHeap              *heap;
    AK3MFPropertyGroupKind kind;
    AkMaterialPropertySetType setType;
    xml_t              *child;
    size_t              propertyCount;
    size_t              i;
    uint32_t           *indices;
    float              *values;
    size_t              indexCount;
    size_t              valueCount;
    AK3MFPropertyGroup *baseGroup;
    xml_attr_t         *blendMethodsAttr;
    bool                groupHasColor;

    if (!ak_3mf_property_group_kind(xml, &kind, &setType))
      continue;

    propertyCount = ak_3mf_count_property_group_children(xml, kind);
    if (propertyCount == 0 || propertyCount > UINT32_MAX)
      continue;

    group         = &st->properties[st->propertyCount];
    group->path   = st->currentModelPath;
    group->id     = xmla_u32(AK_3MF_XMLA(xml, id), (uint32_t)(st->propertyCount + 1u));
    group->count  = (uint32_t)propertyCount;

    heap            = ak_heap_getheap(st->doc);
    set             = ak_heap_calloc(heap, st->doc, sizeof(*set));
    if (!set)
      continue;
    set->id         = group->id;
    set->count      = group->count;
    set->type       = setType;
    set->properties = ak_heap_calloc(heap,
                                     set,
                                     sizeof(*set->properties) * propertyCount);
    if (!set->properties)
      continue;
    if (kind != AK_3MF_PROPERTY_GROUP_TEXTURE2D) {
      group->colors = calloc(propertyCount, 4u);
      if (!group->colors)
        continue;
    } else {
      group->texcoords = calloc(propertyCount, sizeof(float) * 2u);
      if (!group->texcoords)
        continue;
    }
    set->next       = st->doc->materialProperties.sets;
    st->doc->materialProperties.sets = set;
    st->doc->materialProperties.count++;
    group->set      = set;
    group->hasColors = false;
    if (st->print) {
      st->print->features |= AK_PRINT_FEATURE_MATERIALS;
      if (kind == AK_3MF_PROPERTY_GROUP_TEXTURE2D)
        st->print->features |= AK_PRINT_FEATURE_TEXTURES;
      st->print->materialGroupCount++;
      st->print->materialPropertyCount += (uint32_t)propertyCount;
    }

    indices          = NULL;
    values           = NULL;
    indexCount       = 0u;
    valueCount       = 0u;
    baseGroup        = NULL;
    blendMethodsAttr = NULL;

    if (kind == AK_3MF_PROPERTY_GROUP_COMPOSITE) {
      xml_attr_t *matIndicesAttr;

      matIndicesAttr = AK_3MF_XMLA(xml, matindices);
      indexCount     = ak_3mf_attr_u32_list_count(matIndicesAttr);
      if (indexCount > 0u) {
        indices = malloc(sizeof(*indices) * indexCount);
        if (indices)
          indexCount = ak_3mf_attr_u32_list(matIndicesAttr, indices, indexCount);
      }
      baseGroup = ak_3mf_find_property_group(st,
                                             st->currentModelPath,
                                             xmla_u32(AK_3MF_XMLA(xml, matid), 0u));
      group->hasColors = baseGroup && baseGroup->hasColors && baseGroup->colors && indices;
    } else if (kind == AK_3MF_PROPERTY_GROUP_MULTI) {
      xml_attr_t *pidsAttr;

      pidsAttr         = AK_3MF_XMLA(xml, pids);
      indexCount       = ak_3mf_attr_u32_list_count(pidsAttr);
      blendMethodsAttr = AK_3MF_XMLA(xml, blendmethods);
      if (indexCount > 0u) {
        indices = malloc(sizeof(*indices) * indexCount);
        if (indices)
          indexCount = ak_3mf_attr_u32_list(pidsAttr, indices, indexCount);
      }
      group->hasColors = indices != NULL && indexCount > 0u;
    }

    groupHasColor = false;
    i = 0;
    for (child = xml->val; child; child = child->next) {
      AkMaterialProperty *prop;
      uint8_t             rgba[4];
      bool                previewColor;

      switch (kind) {
        case AK_3MF_PROPERTY_GROUP_BASE:
          if (!AK_3MF_TAG8(child, base))
            continue;
          break;
        case AK_3MF_PROPERTY_GROUP_COLOR:
          if (!AK_3MF_TAG8(child, color))
            continue;
          break;
        case AK_3MF_PROPERTY_GROUP_TEXTURE2D:
          if (!AK_3MF_TAG(child, tex2coord))
            continue;
          break;
        case AK_3MF_PROPERTY_GROUP_COMPOSITE:
          if (!AK_3MF_TAG(child, composite))
            continue;
          break;
        case AK_3MF_PROPERTY_GROUP_MULTI:
          if (!AK_3MF_TAG8(child, multi))
            continue;
          break;
        default:
          continue;
      }

      prop = &set->properties[i];
      prop->materialIndex = (uint32_t)i;
      if (kind == AK_3MF_PROPERTY_GROUP_TEXTURE2D) {
        group->texcoords[i * 2u + 0u] = xmla_float(AK_3MF_XMLA_LOCAL(child, u), 0.0f);
        group->texcoords[i * 2u + 1u] = xmla_float(AK_3MF_XMLA_LOCAL(child, v), 0.0f);
        group->hasTexcoords = true;
        i++;
        continue;
      }

      rgba[0]      = 255u;
      rgba[1]      = 255u;
      rgba[2]      = 255u;
      rgba[3]      = 255u;
      previewColor = false;

      if (kind == AK_3MF_PROPERTY_GROUP_BASE
          || kind == AK_3MF_PROPERTY_GROUP_COLOR) {
        previewColor = ak_3mf_parse_color_attr(
          kind == AK_3MF_PROPERTY_GROUP_BASE
            ? AK_3MF_XMLA(child, displaycolor)
            : AK_3MF_XMLA(child, color),
          rgba);
        if (!previewColor) {
          rgba[0] = 255u;
          rgba[1] = 255u;
          rgba[2] = 255u;
          rgba[3] = 255u;
          previewColor = true;
        }
      } else if (kind == AK_3MF_PROPERTY_GROUP_COMPOSITE) {
        xml_attr_t *valuesAttr;

        valuesAttr = AK_3MF_XMLA(child, values);
        valueCount = ak_3mf_attr_float_list_count(valuesAttr);
        free(values);
        values = NULL;
        if (valueCount > 0u) {
          values = malloc(sizeof(*values) * valueCount);
          if (values)
            valueCount = ak_3mf_attr_float_list(valuesAttr, values, valueCount);
        }
        ak_3mf_mix_property_colors(baseGroup,
                                   indices,
                                   indexCount,
                                   values,
                                   valueCount,
                                   rgba,
                                   &previewColor);
      } else if (kind == AK_3MF_PROPERTY_GROUP_MULTI) {
        xml_attr_t *pindicesAttr;
        uint32_t   stackIndices[16];
        uint32_t  *pindices;
        size_t     pindexCount;
        size_t     layer;
        bool       firstLayer;

        pindicesAttr = AK_3MF_XMLA(child, pindices);
        pindexCount  = ak_3mf_attr_u32_list_count(pindicesAttr);
        pindices     = stackIndices;
        if (pindexCount > AK_ARRAY_LEN(stackIndices))
          pindices = malloc(sizeof(*pindices) * pindexCount);
        if (pindices && pindexCount > 0u)
          pindexCount = ak_3mf_attr_u32_list(pindicesAttr, pindices, pindexCount);

        firstLayer = true;
        for (layer = 0u; indices && layer < indexCount; layer++) {
          AK3MFPropertyGroup *layerGroup;
          uint8_t             layerColor[4];
          uint32_t            propertyIndex;

          layerGroup = ak_3mf_find_property_group(st,
                                                  st->currentModelPath,
                                                  indices[layer]);
          propertyIndex = layer < pindexCount ? pindices[layer] : 0u;
          if (!layerGroup
              || !layerGroup->hasColors
              || !layerGroup->colors
              || propertyIndex >= layerGroup->count)
            continue;

          memcpy(layerColor,
                 layerGroup->colors + (size_t)propertyIndex * 4u,
                 sizeof(layerColor));
          if (firstLayer) {
            memcpy(rgba, layerColor, sizeof(rgba));
            firstLayer = false;
          } else {
            ak_3mf_blend_layer_color(
              rgba,
              layerColor,
              ak_3mf_blend_method_is_multiply(blendMethodsAttr, layer - 1u));
          }
          previewColor = true;
        }

        if (pindices != stackIndices)
          free(pindices);
      }

      if (!previewColor) {
        i++;
        continue;
      }

      groupHasColor = true;
      if (rgba[3] < 255u)
        group->hasAlpha = true;

      memcpy(group->colors + i * 4u, rgba, 4u);
      ak_3mf_property_set_color(heap, prop, set, rgba);
      if (kind == AK_3MF_PROPERTY_GROUP_BASE)
        prop->name = xmla_strdup(AK_3MF_XMLA(child, name), heap, set);
      i++;
    }
    free(indices);
    free(values);
    group->hasColors = groupHasColor;
    st->propertyCount++;
    added++;
  }

  return added;
}

static
uint32_t
ak_3mf_clamp_count_u32(size_t count);

static
bool
ak_3mf_parse_transform_attr(const xml_attr_t * __restrict attr,
                            float                         matrix[16]);

static
uint32_t
ak_3mf_optional_attr_flag(const xml_attr_t * __restrict attr,
                          uint32_t                      flag) {
  return attr && attr->val ? flag : 0u;
}

static
bool
ak_3mf_attr_bool(const xml_attr_t * __restrict attr, bool fallback) {
  if (!attr || !attr->val || attr->valsize == 0u)
    return fallback;

  if ((attr->valsize == 4u && memcmp(attr->val, "true", 4u) == 0)
      || (attr->valsize == 1u && attr->val[0] == '1'))
    return true;
  if ((attr->valsize == 5u && memcmp(attr->val, "false", 5u) == 0)
      || (attr->valsize == 1u && attr->val[0] == '0'))
    return false;

  return fallback;
}

static
uint32_t
ak_3mf_volumetric_element_flags(xml_t * __restrict xml,
                                const xml_attr_t ** __restrict transformAttr,
                                const xml_attr_t ** __restrict minFeatureAttr,
                                const xml_attr_t ** __restrict fallbackAttr) {
  const xml_attr_t *transform;
  const xml_attr_t *minFeature;
  const xml_attr_t *fallback;

  transform  = AK_3MF_XMLA_LOCAL(xml, transform);
  minFeature = AK_3MF_XMLA_LOCAL_LIT(xml, "minfeaturesize");
  fallback   = AK_3MF_XMLA_LOCAL_LIT(xml, "fallbackvalue");

  if (transformAttr)
    *transformAttr = transform;
  if (minFeatureAttr)
    *minFeatureAttr = minFeature;
  if (fallbackAttr)
    *fallbackAttr = fallback;

  return ak_3mf_optional_attr_flag(transform,
                                   AK_PRINT_VOLUMETRIC_ELEMENT_HAS_TRANSFORM)
         | ak_3mf_optional_attr_flag(minFeature,
                                     AK_PRINT_VOLUMETRIC_ELEMENT_HAS_MIN_FEATURE_SIZE)
         | ak_3mf_optional_attr_flag(fallback,
                                     AK_PRINT_VOLUMETRIC_ELEMENT_HAS_FALLBACK_VALUE);
}

static
void
ak_3mf_parse_image3d(AK3MFImportState * __restrict st,
                     xml_t            * __restrict xml) {
  AkPrintImage3D *image;
  xml_t          *stackXml;
  xml_t          *sheetXml;
  char           *name;

  if (!st || !xml)
    return;

  stackXml = AK_3MF_CHILD(xml, imagestack);
  if (!stackXml)
    return;

  name = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL(xml, name));
  image = ak_printAddImage3D(st->doc,
                             st->currentModelPath,
                             xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
                             name,
                             xmla_u32(AK_3MF_XMLA_LOCAL_LIT(stackXml, "rowcount"), 0u),
                             xmla_u32(AK_3MF_XMLA_LOCAL_LIT(stackXml, "columncount"), 0u),
                             xmla_u32(AK_3MF_XMLA_LOCAL_LIT(stackXml, "sheetcount"), 0u));
  free(name);
  if (!image)
    return;

  for (sheetXml = stackXml->val; sheetXml; sheetXml = sheetXml->next) {
    char *path;

    if (!AK_3MF_TAG(sheetXml, imagesheet))
      continue;

    path = ak_3mf_attr_dup_path_cstr(AK_3MF_XMLA_LOCAL(sheetXml, path));
    (void)ak_printAddImageSheet(st->doc, image, path);
    free(path);
  }
}

static
void
ak_3mf_parse_function_from_image3d(AK3MFImportState * __restrict st,
                                   xml_t            * __restrict xml) {
  xml_attr_t *valueOffsetAttr;
  xml_attr_t *valueScaleAttr;
  xml_attr_t *filterAttr;
  xml_attr_t *tileUAttr;
  xml_attr_t *tileVAttr;
  xml_attr_t *tileWAttr;
  char       *displayName;
  char       *filter;
  char       *tileStyleU;
  char       *tileStyleV;
  char       *tileStyleW;
  uint32_t    flags;

  if (!st || !xml)
    return;

  valueOffsetAttr = AK_3MF_XMLA_LOCAL_LIT(xml, "valueoffset");
  valueScaleAttr  = AK_3MF_XMLA_LOCAL_LIT(xml, "valuescale");
  filterAttr      = AK_3MF_XMLA_LOCAL_LIT(xml, "filter");
  tileUAttr       = AK_3MF_XMLA_LOCAL_LIT(xml, "tilestyleu");
  tileVAttr       = AK_3MF_XMLA_LOCAL_LIT(xml, "tilestylev");
  tileWAttr       = AK_3MF_XMLA_LOCAL_LIT(xml, "tilestylew");
  displayName     = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL(xml, displayname));
  filter          = ak_3mf_attr_dup_cstr(filterAttr);
  tileStyleU      = ak_3mf_attr_dup_cstr(tileUAttr);
  tileStyleV      = ak_3mf_attr_dup_cstr(tileVAttr);
  tileStyleW      = ak_3mf_attr_dup_cstr(tileWAttr);
  flags           = ak_3mf_optional_attr_flag(valueOffsetAttr,
                                             AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_OFFSET)
                    | ak_3mf_optional_attr_flag(valueScaleAttr,
                                                AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_VALUE_SCALE)
                    | ak_3mf_optional_attr_flag(filterAttr,
                                                AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_FILTER)
                    | ak_3mf_optional_attr_flag(tileUAttr,
                                                AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_U)
                    | ak_3mf_optional_attr_flag(tileVAttr,
                                                AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_V)
                    | ak_3mf_optional_attr_flag(tileWAttr,
                                                AK_PRINT_FUNCTION_FROM_IMAGE3D_HAS_TILESTYLE_W);

  (void)ak_printAddFunctionFromImage3D(
    st->doc,
    st->currentModelPath,
    xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
    displayName,
    xmla_u32(AK_3MF_XMLA_LOCAL_LIT(xml, "image3did"), 0u),
    xmla_float(valueOffsetAttr, 0.0f),
    xmla_float(valueScaleAttr, 1.0f),
    filter,
    tileStyleU,
    tileStyleV,
    tileStyleW,
    flags);

  free(displayName);
  free(filter);
  free(tileStyleU);
  free(tileStyleV);
  free(tileStyleW);
}

static
void
ak_3mf_parse_implicit_function(AK3MFImportState * __restrict st,
                               xml_t            * __restrict xml) {
  char     *displayName;
  char     *fragment;
  uint32_t  flags;

  if (!st || !xml)
    return;

  displayName = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL(xml, displayname));
  fragment    = ak_3mf_xml_fragment_dup(xml, true);
  flags       = fragment ? AK_PRINT_IMPLICIT_FUNCTION_HAS_XML : 0u;

  (void)ak_printAddImplicitFunction(st->doc,
                                    st->currentModelPath,
                                    xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
                                    displayName,
                                    fragment,
                                    flags);
  free(displayName);
  free(fragment);
}

static
void
ak_3mf_parse_volumetric_element(AK3MFImportState           * __restrict st,
                                AkPrintVolumeData          * __restrict volume,
                                xml_t                      * __restrict xml,
                                AkPrintVolumetricElementType            type) {
  const xml_attr_t *transformAttr;
  const xml_attr_t *minFeatureAttr;
  const xml_attr_t *fallbackAttr;
  xml_attr_t       *requiredAttr;
  char             *channel;
  char             *name;
  float             matrix[16];
  uint32_t          flags;

  if (!st || !volume || !xml)
    return;

  flags = ak_3mf_volumetric_element_flags(xml,
                                          &transformAttr,
                                          &minFeatureAttr,
                                          &fallbackAttr);
  if (!ak_3mf_parse_transform_attr(transformAttr, matrix))
    return;

  requiredAttr = AK_3MF_XMLA_LOCAL_LIT(xml, "required");
  if (type == AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY
      && ak_3mf_attr_bool(requiredAttr, false)) {
    flags |= AK_PRINT_VOLUMETRIC_ELEMENT_REQUIRED;
  }

  channel = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(xml, "channel"));
  name    = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL(xml, name));
  (void)ak_printAddVolumetricElement(
    st->doc,
    volume,
    type,
    xmla_u32(AK_3MF_XMLA_LOCAL_LIT(xml, "functionid"), 0u),
    channel,
    name,
    matrix,
    xmla_float(minFeatureAttr, 0.0f),
    xmla_float(fallbackAttr, 0.0f),
    flags);
  free(channel);
  free(name);
}

static
void
ak_3mf_parse_volume_data(AK3MFImportState * __restrict st,
                         xml_t            * __restrict xml) {
  AkPrintVolumeData *volume;
  xml_t             *compositeXml;
  xml_t             *childXml;
  xml_attr_t        *baseMaterialAttr;
  uint32_t           flags;

  if (!st || !xml)
    return;

  compositeXml     = AK_3MF_CHILD(xml, composite);
  baseMaterialAttr = AK_3MF_XMLA_LOCAL_LIT(compositeXml, "basematerialid");
  flags            = ak_3mf_optional_attr_flag(baseMaterialAttr,
                                               AK_PRINT_VOLUME_DATA_HAS_BASE_MATERIAL_ID);
  volume = ak_printAddVolumeData(st->doc,
                                 st->currentModelPath,
                                 xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
                                 xmla_u32(baseMaterialAttr, 0u),
                                 flags);
  if (!volume)
    return;

  for (childXml = compositeXml ? compositeXml->val : NULL;
       childXml;
       childXml = childXml->next) {
    if (AK_3MF_TAG(childXml, materialmapping)) {
      ak_3mf_parse_volumetric_element(st,
                                      volume,
                                      childXml,
                                      AK_PRINT_VOLUMETRIC_ELEMENT_MATERIAL_MAPPING);
    }
  }

  for (childXml = xml->val; childXml; childXml = childXml->next) {
    if (AK_3MF_TAG8(childXml, color)) {
      ak_3mf_parse_volumetric_element(st,
                                      volume,
                                      childXml,
                                      AK_PRINT_VOLUMETRIC_ELEMENT_COLOR);
    } else if (AK_3MF_TAG8(childXml, property)) {
      ak_3mf_parse_volumetric_element(st,
                                      volume,
                                      childXml,
                                      AK_PRINT_VOLUMETRIC_ELEMENT_PROPERTY);
    }
  }
}

static
void
ak_3mf_parse_volumetric_resources(AK3MFImportState * __restrict st,
                                  xml_t            * __restrict resourcesXml) {
  xml_t *xml;

  if (!st || !resourcesXml)
    return;

  for (xml = resourcesXml->val; xml; xml = xml->next) {
    if (AK_3MF_TAG8(xml, image3d))
      ak_3mf_parse_image3d(st, xml);
    else if (AK_3MF_TAG(xml, functionfromimage3d))
      ak_3mf_parse_function_from_image3d(st, xml);
    else if (AK_3MF_TAG(xml, implicitfunction))
      ak_3mf_parse_implicit_function(st, xml);
    else if (AK_3MF_TAG(xml, volumedata))
      ak_3mf_parse_volume_data(st, xml);
  }
}

static
void
ak_3mf_parse_volumetric_mesh(AK3MFImportState * __restrict st,
                             xml_t            * __restrict meshXml,
                             uint32_t                      objectId) {
  xml_attr_t *volumeAttr;

  if (!st || !meshXml || !AK_3MF_TAG8(meshXml, mesh))
    return;

  volumeAttr = AK_3MF_XMLA_LOCAL_LIT(meshXml, "volumeid");
  if (!volumeAttr || !volumeAttr->val)
    return;

  (void)ak_printAddVolumetricMesh(st->doc,
                                  st->currentModelPath,
                                  objectId,
                                  xmla_u32(volumeAttr, 0u),
                                  AK_PRINT_VOLUMETRIC_MESH_HAS_VOLUME_ID);
}

static
bool
ak_3mf_parse_level_set(AK3MFImportState * __restrict st,
                       AK3MFObject      * __restrict object,
                       xml_t            * __restrict levelSetXml) {
  xml_attr_t *transformAttr;
  xml_attr_t *minFeatureAttr;
  xml_attr_t *fallbackAttr;
  xml_attr_t *meshBoxAttr;
  xml_attr_t *volumeAttr;
  char       *channel;
  float       matrix[16];
  uint32_t    flags;

  if (!st || !object || !levelSetXml)
    return false;

  transformAttr  = AK_3MF_XMLA_LOCAL(levelSetXml, transform);
  minFeatureAttr = AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "minfeaturesize");
  fallbackAttr   = AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "fallbackvalue");
  meshBoxAttr    = AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "meshbboxonly");
  volumeAttr     = AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "volumeid");
  if (!ak_3mf_parse_transform_attr(transformAttr, matrix))
    return false;

  flags = ak_3mf_optional_attr_flag(transformAttr,
                                    AK_PRINT_LEVEL_SET_HAS_TRANSFORM)
          | ak_3mf_optional_attr_flag(minFeatureAttr,
                                      AK_PRINT_LEVEL_SET_HAS_MIN_FEATURE_SIZE)
          | ak_3mf_optional_attr_flag(fallbackAttr,
                                      AK_PRINT_LEVEL_SET_HAS_FALLBACK_VALUE)
          | ak_3mf_optional_attr_flag(volumeAttr,
                                      AK_PRINT_LEVEL_SET_HAS_VOLUME_ID);
  if (ak_3mf_attr_bool(meshBoxAttr, false))
    flags |= AK_PRINT_LEVEL_SET_HAS_MESH_BBOX_ONLY;

  channel = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "channel"));
  if (!ak_printAddLevelSet(st->doc,
                           st->currentModelPath,
                           object->id,
                           xmla_u32(AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "functionid"), 0u),
                           channel,
                           xmla_u32(AK_3MF_XMLA_LOCAL_LIT(levelSetXml, "meshid"), 0u),
                           xmla_u32(volumeAttr, 0u),
                           matrix,
                           xmla_float(minFeatureAttr, 0.0f),
                           xmla_float(fallbackAttr, 0.0f),
                           flags)) {
    free(channel);
    return false;
  }

  free(channel);
  return true;
}

static
void
ak_3mf_parse_beam_lattice_beams(AK3MFImportState     * __restrict st,
                                AkPrintBeamLattice   * __restrict lattice,
                                xml_t                * __restrict beamsXml) {
  xml_t *beamXml;

  if (!st || !lattice || !beamsXml)
    return;

  for (beamXml = beamsXml->val; beamXml; beamXml = beamXml->next) {
    xml_attr_t *r1Attr;
    xml_attr_t *r2Attr;
    xml_attr_t *p1Attr;
    xml_attr_t *p2Attr;
    xml_attr_t *pidAttr;
    xml_attr_t *cap1Attr;
    xml_attr_t *cap2Attr;
    char       *cap1;
    char       *cap2;
    uint32_t    flags;
    float       r1;
    float       r2;

    if (!AK_3MF_TAG8(beamXml, beam))
      continue;

    r1Attr   = AK_3MF_XMLA_LOCAL_LIT(beamXml, "r1");
    r2Attr   = AK_3MF_XMLA_LOCAL_LIT(beamXml, "r2");
    p1Attr   = AK_3MF_XMLA_LOCAL(beamXml, p1);
    p2Attr   = AK_3MF_XMLA_LOCAL(beamXml, p2);
    pidAttr  = AK_3MF_XMLA_LOCAL(beamXml, pid);
    cap1Attr = AK_3MF_XMLA_LOCAL_LIT(beamXml, "cap1");
    cap2Attr = AK_3MF_XMLA_LOCAL_LIT(beamXml, "cap2");
    flags    = ak_3mf_optional_attr_flag(r1Attr, AK_PRINT_BEAM_HAS_R1)
               | ak_3mf_optional_attr_flag(r2Attr, AK_PRINT_BEAM_HAS_R2)
               | ak_3mf_optional_attr_flag(p1Attr, AK_PRINT_BEAM_HAS_P1)
               | ak_3mf_optional_attr_flag(p2Attr, AK_PRINT_BEAM_HAS_P2)
               | ak_3mf_optional_attr_flag(pidAttr, AK_PRINT_BEAM_HAS_PID)
               | ak_3mf_optional_attr_flag(cap1Attr, AK_PRINT_BEAM_HAS_CAP1)
               | ak_3mf_optional_attr_flag(cap2Attr, AK_PRINT_BEAM_HAS_CAP2);
    r1       = xmla_float(r1Attr, lattice->radius);
    r2       = xmla_float(r2Attr, r1);
    cap1     = ak_3mf_attr_dup_cstr(cap1Attr);
    cap2     = ak_3mf_attr_dup_cstr(cap2Attr);

    (void)ak_printAddBeam(st->doc,
                          lattice,
                          xmla_u32(AK_3MF_XMLA_LOCAL(beamXml, v1), 0u),
                          xmla_u32(AK_3MF_XMLA_LOCAL(beamXml, v2), 0u),
                          r1,
                          r2,
                          xmla_u32(p1Attr, UINT32_MAX),
                          xmla_u32(p2Attr, UINT32_MAX),
                          xmla_u32(pidAttr, 0u),
                          cap1,
                          cap2,
                          flags);
    free(cap1);
    free(cap2);
  }
}

static
void
ak_3mf_parse_beam_lattice_balls(AK3MFImportState     * __restrict st,
                                AkPrintBeamLattice   * __restrict lattice,
                                xml_t                * __restrict ballsXml) {
  xml_t *ballXml;

  if (!st || !lattice || !ballsXml)
    return;

  for (ballXml = ballsXml->val; ballXml; ballXml = ballXml->next) {
    xml_attr_t *rAttr;
    xml_attr_t *pAttr;
    xml_attr_t *pidAttr;
    uint32_t    flags;

    if (!AK_3MF_TAG8(ballXml, ball))
      continue;

    rAttr  = AK_3MF_XMLA_LOCAL_LIT(ballXml, "r");
    pAttr  = AK_3MF_XMLA_LOCAL_LIT(ballXml, "p");
    pidAttr = AK_3MF_XMLA_LOCAL(ballXml, pid);
    flags  = ak_3mf_optional_attr_flag(rAttr, AK_PRINT_BEAM_BALL_HAS_RADIUS)
             | ak_3mf_optional_attr_flag(pAttr, AK_PRINT_BEAM_BALL_HAS_P)
             | ak_3mf_optional_attr_flag(pidAttr, AK_PRINT_BEAM_BALL_HAS_PID);

    (void)ak_printAddBeamBall(st->doc,
                              lattice,
                              xmla_u32(AK_3MF_XMLA_LOCAL_LIT(ballXml, "vindex"), 0u),
                              xmla_float(rAttr, lattice->ballRadius),
                              xmla_u32(pAttr, UINT32_MAX),
                              xmla_u32(pidAttr, 0u),
                              flags);
  }
}

static
void
ak_3mf_parse_beam_lattice_sets(AK3MFImportState     * __restrict st,
                               AkPrintBeamLattice   * __restrict lattice,
                               xml_t                * __restrict beamSetsXml) {
  xml_t *setXml;

  if (!st || !lattice || !beamSetsXml)
    return;

  for (setXml = beamSetsXml->val; setXml; setXml = setXml->next) {
    char    *name;
    char    *identifier;
    size_t   refCount;
    size_t   ballRefCount;

    if (!AK_3MF_TAG8(setXml, beamset))
      continue;

    name         = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL(setXml, name));
    identifier   = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(setXml, "identifier"));
    refCount     = AK_3MF_COUNT_CHILDREN8(setXml, ref);
    ballRefCount = AK_3MF_COUNT_CHILDREN8(setXml, ballref);
    (void)ak_printAddBeamSet(st->doc,
                             lattice,
                             name,
                             identifier,
                             ak_3mf_clamp_count_u32(refCount),
                             ak_3mf_clamp_count_u32(ballRefCount));
    free(name);
    free(identifier);
  }
}

static
void
ak_3mf_parse_beam_lattice(AK3MFImportState * __restrict st,
                          xml_t            * __restrict meshXml,
                          uint32_t                      objectId) {
  AkPrintBeamLattice *lattice;
  xml_t              *beamLatticeXml;
  xml_attr_t         *ballRadiusAttr;
  xml_attr_t         *clippingMeshAttr;
  xml_attr_t         *representationMeshAttr;
  xml_attr_t         *pidAttr;
  xml_attr_t         *pindexAttr;
  char               *clippingMode;
  char               *cap;
  char               *ballMode;
  uint32_t            flags;

  if (!st || !st->doc || !st->print || !meshXml)
    return;

  beamLatticeXml = AK_3MF_CHILD(meshXml, beamlattice);
  if (!beamLatticeXml)
    return;

  ballRadiusAttr         = AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "ballradius");
  clippingMeshAttr       = AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "clippingmesh");
  representationMeshAttr = AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "representationmesh");
  pidAttr                = AK_3MF_XMLA_LOCAL(beamLatticeXml, pid);
  pindexAttr             = AK_3MF_XMLA_LOCAL(beamLatticeXml, pindex);
  clippingMode           = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "clippingmode"));
  cap                    = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "cap"));
  ballMode               = ak_3mf_attr_dup_cstr(AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "ballmode"));
  flags                  = ak_3mf_optional_attr_flag(ballRadiusAttr,
                                                     AK_PRINT_BEAM_LATTICE_HAS_BALL_RADIUS)
                           | ak_3mf_optional_attr_flag(clippingMeshAttr,
                                                       AK_PRINT_BEAM_LATTICE_HAS_CLIPPING_MESH)
                           | ak_3mf_optional_attr_flag(representationMeshAttr,
                                                       AK_PRINT_BEAM_LATTICE_HAS_REPRESENTATION_MESH)
                           | ak_3mf_optional_attr_flag(pidAttr,
                                                       AK_PRINT_BEAM_LATTICE_HAS_PID)
                           | ak_3mf_optional_attr_flag(pindexAttr,
                                                       AK_PRINT_BEAM_LATTICE_HAS_PINDEX);

  lattice = ak_printAddBeamLattice(
    st->doc,
    st->currentModelPath,
    objectId,
    xmla_float(AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "minlength"), 0.0f),
    xmla_float(AK_3MF_XMLA_LOCAL_LIT(beamLatticeXml, "radius"), 0.0f),
    clippingMode,
    cap,
    ballMode,
    xmla_float(ballRadiusAttr, 0.0f),
    xmla_u32(clippingMeshAttr, 0u),
    xmla_u32(representationMeshAttr, 0u),
    xmla_u32(pidAttr, 0u),
    xmla_u32(pindexAttr, UINT32_MAX),
    flags);
  free(clippingMode);
  free(cap);
  free(ballMode);
  if (!lattice)
    return;

  ak_3mf_parse_beam_lattice_beams(st,
                                  lattice,
                                  AK_3MF_CHILD8(beamLatticeXml, beams));
  ak_3mf_parse_beam_lattice_balls(st,
                                  lattice,
                                  AK_3MF_CHILD8(beamLatticeXml, balls));
  ak_3mf_parse_beam_lattice_sets(st,
                                 lattice,
                                 AK_3MF_CHILD8(beamLatticeXml, beamsets));
}

static
void
ak_3mf_parse_displacement_mesh(AK3MFImportState * __restrict st,
                               xml_t            * __restrict meshXml,
                               uint32_t                      objectId);

AK_INLINE
bool
ak_3mf_parse_triangle_vertices(xml_t  * __restrict triangleXml,
                               size_t              vertexCount,
                               AkUInt              v[3]) {
  v[0] = xmla_u32(AK_3MF_XMLA_LOCAL(triangleXml, v1), 0u);
  v[1] = xmla_u32(AK_3MF_XMLA_LOCAL(triangleXml, v2), 0u);
  v[2] = xmla_u32(AK_3MF_XMLA_LOCAL(triangleXml, v3), 0u);

  return v[0] < vertexCount && v[1] < vertexCount && v[2] < vertexCount;
}

static
AkGeometry*
ak_3mf_parse_mesh(AK3MFImportState * __restrict st,
                  xml_t            * __restrict objXml,
                  xml_t            * __restrict meshXml) {
  AkDoc           *doc;
  AkHeap          *heap;
  xml_t           *verticesXml;
  xml_t           *trianglesXml;
  xml_t           *beamLatticeXml;
  xml_t           *vertexXml;
  xml_t           *triangleXml;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  AkMeshPrimitive *prim;
  AkBuffer        *posBuff;
  AkAccessor      *posAcc;
  AkIndexArray    *indices;
  uint8_t         *colors;
  float           *texcoords;
  float           *srcPositions;
  float           *positions;
  size_t           vertexCount;
  size_t           outputVertexCount;
  size_t           triangleCount;
  size_t           i;
  AkUInt           maxIndex;
  uint32_t         objectId;
  uint32_t         defaultPid;
  uint32_t         defaultPIndex;
  bool             hasColor;
  bool             hasTexcoord;
  bool             hasAlpha;
  bool             expandVertices;

  if (!st || !st->doc || !meshXml)
    return NULL;

  doc          = st->doc;
  verticesXml  = AK_3MF_CHILD8(meshXml, vertices);
  trianglesXml = AK_3MF_CHILD(meshXml, triangles);
  beamLatticeXml = AK_3MF_CHILD(meshXml, beamlattice);
  vertexCount  = AK_3MF_COUNT_CHILDREN8(verticesXml, vertex);
  triangleCount = AK_3MF_COUNT_CHILDREN8(trianglesXml, triangle);
  if (vertexCount == 0
      || (triangleCount == 0 && !beamLatticeXml)
      || vertexCount > UINT32_MAX
      || triangleCount > SIZE_MAX / 3u
      || triangleCount * 3u > UINT32_MAX)
    return NULL;

  srcPositions = malloc(vertexCount * sizeof(float) * 3u);
  if (!srcPositions)
    return NULL;

  i = 0;
  for (vertexXml = verticesXml->val; vertexXml; vertexXml = vertexXml->next) {
    if (!AK_3MF_TAG8(vertexXml, vertex))
      continue;

    srcPositions[i * 3u + 0u] = xmla_float(AK_3MF_XMLA_LOCAL(vertexXml, x), 0.0f);
    srcPositions[i * 3u + 1u] = xmla_float(AK_3MF_XMLA_LOCAL(vertexXml, y), 0.0f);
    srcPositions[i * 3u + 2u] = xmla_float(AK_3MF_XMLA_LOCAL(vertexXml, z), 0.0f);
    i++;
  }

  objectId      = xmla_u32(AK_3MF_XMLA(objXml, id), 0u);
  defaultPid    = xmla_u32(AK_3MF_XMLA(objXml, pid), 0u);
  defaultPIndex = xmla_u32(AK_3MF_XMLA(objXml, pindex), UINT32_MAX);
  hasColor      = false;
  hasTexcoord   = false;
  hasAlpha      = false;
  if (st->propertyCount > 0u) {
    for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
         triangleXml;
         triangleXml = triangleXml->next) {
      uint32_t pid;
      uint32_t p1;

      if (!AK_3MF_TAG8(triangleXml, triangle))
        continue;

      pid = xmla_u32(AK_3MF_XMLA(triangleXml, pid), defaultPid);
      p1  = xmla_u32(AK_3MF_XMLA(triangleXml, p1), defaultPIndex);
      if (pid != 0u
          && p1 != UINT32_MAX) {
        if (ak_3mf_property_has_color(st, st->currentModelPath, pid, p1))
          hasColor = true;
        if (ak_3mf_property_has_texcoord(st, st->currentModelPath, pid, p1))
          hasTexcoord = true;
        if (hasColor && hasTexcoord)
          break;
      }
    }
    if (!hasColor
        && defaultPid != 0u
        && defaultPIndex != UINT32_MAX
        && ak_3mf_property_has_color(st,
                                     st->currentModelPath,
                                     defaultPid,
                                     defaultPIndex))
      hasColor = true;
    if (!hasTexcoord
        && defaultPid != 0u
        && defaultPIndex != UINT32_MAX
        && ak_3mf_property_has_texcoord(st,
                                        st->currentModelPath,
                                        defaultPid,
                                        defaultPIndex))
      hasTexcoord = true;
  }

  heap = ak_heap_getheap(doc);
  mesh = ak_allocMeshEx(heap, doc, &geom, true);
  if (!mesh || !geom) {
    free(srcPositions);
    return NULL;
  }

  expandVertices    = hasColor || hasTexcoord;
  outputVertexCount = expandVertices ? triangleCount * 3u : vertexCount;

  posBuff         = ak_heap_calloc(heap, doc, sizeof(*posBuff));
  if (!posBuff) {
    free(srcPositions);
    return NULL;
  }

  posBuff->length = outputVertexCount * sizeof(float) * 3u;
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  positions       = posBuff->data;
  if (!posBuff->data) {
    free(srcPositions);
    return NULL;
  }

  colors = NULL;
  if (hasColor) {
    colors = ak_heap_alloc(heap, posBuff, outputVertexCount * 4u);
    if (!colors) {
      free(srcPositions);
      return NULL;
    }
  }
  texcoords = NULL;
  if (hasTexcoord) {
    texcoords = ak_heap_alloc(heap, posBuff, outputVertexCount * sizeof(float) * 2u);
    if (!texcoords) {
      free(srcPositions);
      return NULL;
    }
  }
  if (!expandVertices) {
    memcpy(positions, srcPositions, posBuff->length);
  }

  AK_LIB_PREPEND(doc->lib.buffers, posBuff, next);

  posAcc = io_acc(heap,
                  doc,
                  AK_COMPONENT_SIZE_VEC3,
                  AKT_FLOAT,
                  (uint32_t)outputVertexCount,
                  posBuff);
  if (!posAcc)
    return NULL;
  AK_LIB_PREPEND(doc->lib.accessors, posAcc, next);

  tri            = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*tri));
  tri->mode      = AK_TRIANGLES;
  tri->base.type = AK_PRIMITIVE_TRIANGLES;
  prim           = (AkMeshPrimitive *)tri;
  prim->mesh     = mesh;
  prim->nPolygons = (uint32_t)triangleCount;
  prim->indexStride = 1u;
  mesh->primitive = prim;
  mesh->primitiveCount = 1u;

  prim->pos = io_input(heap,
                       prim,
                       posAcc,
                       AK_INPUT_POSITION,
                       _s_POSITION,
                       0u);

  maxIndex = (AkUInt)(outputVertexCount - 1u);
  indices  = NULL;
  if (!expandVertices) {
    indices = ak_indexArrayAlloc(heap,
                                 prim,
                                 triangleCount * 3u,
                                 ak_indexComponentTypeForMax(maxIndex));
    if (!indices) {
      free(srcPositions);
      return NULL;
    }

    indices->max = maxIndex;
  }

  i = 0;
  if (expandVertices) {
    for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
         triangleXml;
         triangleXml = triangleXml->next) {
      AkUInt  v[3];
      uint32_t pid;
      uint32_t p[3];
      uint32_t j;

      if (!AK_3MF_TAG8(triangleXml, triangle))
        continue;
      if (!ak_3mf_parse_triangle_vertices(triangleXml, vertexCount, v)) {
        free(srcPositions);
        return NULL;
      }

      pid  = xmla_u32(AK_3MF_XMLA(triangleXml, pid), defaultPid);
      p[0] = xmla_u32(AK_3MF_XMLA(triangleXml, p1), defaultPIndex);
      p[1] = xmla_u32(AK_3MF_XMLA(triangleXml, p2), p[0]);
      p[2] = xmla_u32(AK_3MF_XMLA(triangleXml, p3), p[0]);

      for (j = 0; j < 3u; j++) {
        uint8_t rgba[4] = {255u, 255u, 255u, 255u};
        float   uv[2] = {0.0f, 0.0f};

        memcpy(positions + i * 3u,
               srcPositions + (size_t)v[j] * 3u,
               sizeof(float) * 3u);

        if (pid != 0u && p[j] != UINT32_MAX) {
          if (colors)
            (void)ak_3mf_property_color(st,
                                        st->currentModelPath,
                                        pid,
                                        p[j],
                                        rgba,
                                        &hasAlpha);
          if (texcoords)
            (void)ak_3mf_property_texcoord(st,
                                           st->currentModelPath,
                                           pid,
                                           p[j],
                                           uv);
        }
        if (colors)
          memcpy(colors + i * 4u, rgba, 4u);
        if (texcoords)
          memcpy(texcoords + i * 2u, uv, sizeof(uv));
        i++;
      }
    }
  } else {
    switch (indices->componentType) {
      case AKT_UBYTE: {
        uint8_t *dst;

        dst = (uint8_t *)indices->items;
        for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
             triangleXml;
             triangleXml = triangleXml->next) {
          AkUInt v[3];

          if (!AK_3MF_TAG8(triangleXml, triangle))
            continue;
          if (!ak_3mf_parse_triangle_vertices(triangleXml, vertexCount, v)) {
            free(srcPositions);
            return NULL;
          }

          dst[i++] = (uint8_t)v[0];
          dst[i++] = (uint8_t)v[1];
          dst[i++] = (uint8_t)v[2];
        }
        break;
      }
      case AKT_USHORT: {
        uint16_t *dst;

        dst = (uint16_t *)indices->items;
        for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
             triangleXml;
             triangleXml = triangleXml->next) {
          AkUInt v[3];

          if (!AK_3MF_TAG8(triangleXml, triangle))
            continue;
          if (!ak_3mf_parse_triangle_vertices(triangleXml, vertexCount, v)) {
            free(srcPositions);
            return NULL;
          }

          dst[i++] = (uint16_t)v[0];
          dst[i++] = (uint16_t)v[1];
          dst[i++] = (uint16_t)v[2];
        }
        break;
      }
      case AKT_UINT: {
        uint32_t *dst;

        dst = (uint32_t *)indices->items;
        for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
             triangleXml;
             triangleXml = triangleXml->next) {
          AkUInt v[3];

          if (!AK_3MF_TAG8(triangleXml, triangle))
            continue;
          if (!ak_3mf_parse_triangle_vertices(triangleXml, vertexCount, v)) {
            free(srcPositions);
            return NULL;
          }

          dst[i++] = v[0];
          dst[i++] = v[1];
          dst[i++] = v[2];
        }
        break;
      }
      default:
        free(srcPositions);
        return NULL;
    }
  }

  free(srcPositions);

  if (hasColor) {
    AkBuffer   *colorBuff;
    AkAccessor *colorAcc;

    colorBuff         = ak_heap_calloc(heap, doc, sizeof(*colorBuff));
    colorBuff->length = outputVertexCount * 4u;
    colorBuff->data   = colors;
    AK_LIB_PREPEND(doc->lib.buffers, colorBuff, next);

    colorAcc = io_acc(heap,
                      doc,
                      AK_COMPONENT_SIZE_VEC4,
                      AKT_UBYTE,
                      (uint32_t)outputVertexCount,
                      colorBuff);
    if (!colorAcc)
      return NULL;
    colorAcc->bytesPerComponent = sizeof(uint8_t);
    colorAcc->byteStride        = 4u;
    colorAcc->fillByteSize      = 4u;
    AK_LIB_PREPEND(doc->lib.accessors, colorAcc, next);

    io_input(heap, prim, colorAcc, AK_INPUT_COLOR, _s_COLOR, 0u);
    prim->material = ak_materialDefaultVertexColorAlpha(doc, hasAlpha);
  }

  if (hasTexcoord) {
    AkBuffer   *texBuff;
    AkAccessor *texAcc;

    texBuff         = ak_heap_calloc(heap, doc, sizeof(*texBuff));
    texBuff->length = outputVertexCount * sizeof(float) * 2u;
    texBuff->data   = texcoords;
    AK_LIB_PREPEND(doc->lib.buffers, texBuff, next);

    texAcc = io_acc(heap,
                    doc,
                    AK_COMPONENT_SIZE_VEC2,
                    AKT_FLOAT,
                    (uint32_t)outputVertexCount,
                    texBuff);
    if (!texAcc)
      return NULL;
    AK_LIB_PREPEND(doc->lib.accessors, texAcc, next);
    io_input(heap, prim, texAcc, AK_INPUT_TEXCOORD, _s_TEXCOORD, 0u);
  }

  if (!hasColor) {
    prim->material = ak_3mf_bambu_orca_material_for_object(st, objectId);
  }
  if (!expandVertices) {
    prim->indices = indices;
  }

  ak_3mf_parse_beam_lattice(st,
                            meshXml,
                            objectId);
  ak_3mf_parse_volumetric_mesh(st,
                               meshXml,
                               objectId);
  if (AK_3MF_TAG(meshXml, displacementmesh))
    ak_3mf_parse_displacement_mesh(st,
                                   meshXml,
                                   objectId);

  AK_LIB_PREPEND(doc->lib.geometries, geom, next);
  return geom;
}

static
size_t
ak_3mf_count_resource_objects(xml_t * __restrict resourcesXml) {
  xml_t *objXml;
  size_t count;

  count = 0;
  if (!resourcesXml)
    return 0;

  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    if (AK_3MF_TAG8(objXml, object)
        && (AK_3MF_CHILD8(objXml, mesh)
            || AK_3MF_CHILD(objXml, components)
            || AK_3MF_CHILD(objXml, displacementmesh)
            || AK_3MF_CHILD(objXml, booleanshape)
            || AK_3MF_CHILD8(objXml, levelset)))
      count++;
  }

  return count;
}

static
AK3MFObject*
ak_3mf_find_object(AK3MFObject * __restrict objects,
                   size_t                   objectCount,
                   const char              *path,
                   uint32_t                 id) {
  size_t i;

  for (i = 0; i < objectCount; i++) {
    if (objects[i].id == id && ak_3mf_model_path_eq(objects[i].path, path))
      return &objects[i];
  }

  return NULL;
}

static
bool
ak_3mf_load_model_part(AK3MFImportState * __restrict st,
                       const char       * __restrict modelPath);

static
void
ak_3mf_parse_displacement_mesh(AK3MFImportState * __restrict st,
                               xml_t            * __restrict meshXml,
                               uint32_t                      objectId);

static
uint32_t
ak_3mf_clamp_count_u32(size_t count) {
  return count > UINT32_MAX ? UINT32_MAX : (uint32_t)count;
}

static
AkPrintBooleanOperation
ak_3mf_boolean_operation_from_attr(const xml_attr_t * __restrict attr) {
  if (!attr || !attr->val || attr->valsize == 0u)
    return AK_PRINT_BOOLEAN_OPERATION_UNION;

  if (ak_3mf_slice_eq_cstr(attr->val, attr->valsize, "union"))
    return AK_PRINT_BOOLEAN_OPERATION_UNION;
  if (ak_3mf_slice_eq_cstr(attr->val, attr->valsize, "difference"))
    return AK_PRINT_BOOLEAN_OPERATION_DIFFERENCE;
  if (ak_3mf_slice_eq_cstr(attr->val, attr->valsize, "intersection"))
    return AK_PRINT_BOOLEAN_OPERATION_INTERSECTION;

  return AK_PRINT_BOOLEAN_OPERATION_UNKNOWN;
}

static
void
ak_3mf_parse_boolean_operands(AK3MFImportState      * __restrict st,
                              AkPrintBooleanShape   * __restrict shape,
                              xml_t                 * __restrict shapeXml) {
  xml_t *operandXml;

  if (!st || !shape || !shapeXml)
    return;

  for (operandXml = shapeXml->val; operandXml; operandXml = operandXml->next) {
    xml_attr_t *objectIdAttr;
    xml_attr_t *pathAttr;
    xml_attr_t *transformAttr;
    float       matrix[16];
    char       *path;
    uint32_t    flags;
    uint32_t    objectId;

    if (!AK_3MF_TAG8(operandXml, boolean))
      continue;

    objectIdAttr  = AK_3MF_XMLA_LOCAL(operandXml, objectid);
    pathAttr      = AK_3MF_XMLA_LOCAL(operandXml, path);
    transformAttr = AK_3MF_XMLA_LOCAL(operandXml, transform);
    objectId      = xmla_u32(objectIdAttr, 0u);
    if (objectId == 0u)
      continue;
    if (!ak_3mf_parse_transform_attr(transformAttr, matrix))
      continue;

    path  = ak_3mf_attr_dup_path_cstr(pathAttr);
    flags = ak_3mf_optional_attr_flag(transformAttr,
                                      AK_PRINT_BOOLEAN_OPERAND_HAS_TRANSFORM);

    (void)ak_printAddBooleanOperand(st->doc,
                                    shape,
                                    path,
                                    objectId,
                                    matrix,
                                    flags);
    if (path && !ak_3mf_model_path_eq(path, st->currentModelPath))
      (void)ak_3mf_load_model_part(st, path);
    free(path);
  }
}

static
bool
ak_3mf_parse_boolean_shape(AK3MFImportState * __restrict st,
                           AK3MFObject      * __restrict object,
                           xml_t            * __restrict shapeXml) {
  AkPrintBooleanShape   *shape;
  xml_attr_t            *baseIdAttr;
  xml_attr_t            *basePathAttr;
  xml_attr_t            *operationAttr;
  xml_attr_t            *transformAttr;
  AkPrintBooleanOperation operation;
  float                  matrix[16];
  char                  *basePath;
  uint32_t               flags;
  uint32_t               baseObjectId;

  if (!st || !st->doc || !object || !shapeXml)
    return false;

  baseIdAttr    = AK_3MF_XMLA_LOCAL(shapeXml, objectid);
  basePathAttr  = AK_3MF_XMLA_LOCAL(shapeXml, path);
  operationAttr = AK_3MF_XMLA_LOCAL_LIT(shapeXml, "operation");
  transformAttr = AK_3MF_XMLA_LOCAL(shapeXml, transform);
  baseObjectId  = xmla_u32(baseIdAttr, 0u);
  if (baseObjectId == 0u)
    return false;
  if (!ak_3mf_parse_transform_attr(transformAttr, matrix))
    return false;

  operation = ak_3mf_boolean_operation_from_attr(operationAttr);
  basePath  = ak_3mf_attr_dup_path_cstr(basePathAttr);
  flags     = ak_3mf_optional_attr_flag(transformAttr,
                                        AK_PRINT_BOOLEAN_SHAPE_HAS_TRANSFORM);

  shape = ak_printAddBooleanShape(st->doc,
                                  st->currentModelPath,
                                  basePath,
                                  object->id,
                                  baseObjectId,
                                  operation,
                                  matrix,
                                  flags);
  if (basePath && !ak_3mf_model_path_eq(basePath, st->currentModelPath))
    (void)ak_3mf_load_model_part(st, basePath);
  free(basePath);
  if (!shape)
    return false;

  ak_3mf_parse_boolean_operands(st, shape, shapeXml);
  return true;
}

static
void
ak_3mf_parse_displacement2d(AK3MFImportState * __restrict st,
                            xml_t            * __restrict xml) {
  xml_attr_t *channelAttr;
  xml_attr_t *tileUAttr;
  xml_attr_t *tileVAttr;
  xml_attr_t *filterAttr;
  char       *imagePath;
  char       *channel;
  char       *tileStyleU;
  char       *tileStyleV;
  char       *filter;
  uint32_t    flags;

  if (!st || !xml)
    return;

  channelAttr = AK_3MF_XMLA_LOCAL_LIT(xml, "channel");
  tileUAttr   = AK_3MF_XMLA_LOCAL_LIT(xml, "tilestyleu");
  tileVAttr   = AK_3MF_XMLA_LOCAL_LIT(xml, "tilestylev");
  filterAttr  = AK_3MF_XMLA_LOCAL_LIT(xml, "filter");
  imagePath   = ak_3mf_attr_dup_path_cstr(AK_3MF_XMLA_LOCAL(xml, path));
  channel     = ak_3mf_attr_dup_cstr(channelAttr);
  tileStyleU  = ak_3mf_attr_dup_cstr(tileUAttr);
  tileStyleV  = ak_3mf_attr_dup_cstr(tileVAttr);
  filter      = ak_3mf_attr_dup_cstr(filterAttr);
  flags       = ak_3mf_optional_attr_flag(channelAttr,
                                          AK_PRINT_DISPLACEMENT_2D_HAS_CHANNEL)
                | ak_3mf_optional_attr_flag(tileUAttr,
                                            AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_U)
                | ak_3mf_optional_attr_flag(tileVAttr,
                                            AK_PRINT_DISPLACEMENT_2D_HAS_TILESTYLE_V)
                | ak_3mf_optional_attr_flag(filterAttr,
                                            AK_PRINT_DISPLACEMENT_2D_HAS_FILTER);

  (void)ak_printAddDisplacement2D(st->doc,
                                  st->currentModelPath,
                                  xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
                                  imagePath,
                                  channel,
                                  tileStyleU,
                                  tileStyleV,
                                  filter,
                                  flags);
  free(imagePath);
  free(channel);
  free(tileStyleU);
  free(tileStyleV);
  free(filter);
}

static
void
ak_3mf_parse_norm_vector_group(AK3MFImportState * __restrict st,
                               xml_t            * __restrict xml) {
  AkPrintNormVectorGroup *group;
  xml_t                  *vectorXml;

  if (!st || !xml)
    return;

  group = ak_printAddNormVectorGroup(st->doc,
                                     st->currentModelPath,
                                     xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u));
  if (!group)
    return;

  for (vectorXml = xml->val; vectorXml; vectorXml = vectorXml->next) {
    if (!AK_3MF_TAG(vectorXml, normvector))
      continue;

    (void)ak_printAddNormVector(st->doc,
                                group,
                                xmla_float(AK_3MF_XMLA_LOCAL(vectorXml, x), 0.0f),
                                xmla_float(AK_3MF_XMLA_LOCAL(vectorXml, y), 0.0f),
                                xmla_float(AK_3MF_XMLA_LOCAL(vectorXml, z), 1.0f));
  }
}

static
void
ak_3mf_parse_disp2d_group(AK3MFImportState * __restrict st,
                          xml_t            * __restrict xml) {
  AkPrintDisp2DGroup *group;
  xml_t              *coordXml;
  xml_attr_t         *offsetAttr;

  if (!st || !xml)
    return;

  offsetAttr = AK_3MF_XMLA_LOCAL_LIT(xml, "offset");
  group = ak_printAddDisp2DGroup(st->doc,
                                 st->currentModelPath,
                                 xmla_u32(AK_3MF_XMLA_LOCAL(xml, id), 0u),
                                 xmla_u32(AK_3MF_XMLA_LOCAL_LIT(xml, "dispid"), 0u),
                                 xmla_u32(AK_3MF_XMLA_LOCAL_LIT(xml, "nid"), 0u),
                                 xmla_float(AK_3MF_XMLA_LOCAL_LIT(xml, "height"), 0.0f),
                                 xmla_float(offsetAttr, 0.0f),
                                 ak_3mf_optional_attr_flag(offsetAttr,
                                                           AK_PRINT_DISP2D_GROUP_HAS_OFFSET));
  if (!group)
    return;

  for (coordXml = xml->val; coordXml; coordXml = coordXml->next) {
    xml_attr_t *factorAttr;

    if (!AK_3MF_TAG(coordXml, disp2dcoord))
      continue;

    factorAttr = AK_3MF_XMLA_LOCAL_LIT(coordXml, "f");
    (void)ak_printAddDisp2DCoord(st->doc,
                                 group,
                                 xmla_float(AK_3MF_XMLA_LOCAL_LIT(coordXml, "u"), 0.0f),
                                 xmla_float(AK_3MF_XMLA_LOCAL_LIT(coordXml, "v"), 0.0f),
                                 xmla_u32(AK_3MF_XMLA_LOCAL_LIT(coordXml, "n"), 0u),
                                 xmla_float(factorAttr, 1.0f),
                                 ak_3mf_optional_attr_flag(factorAttr,
                                                           AK_PRINT_DISP2D_COORD_HAS_FACTOR));
  }
}

static
void
ak_3mf_parse_displacement_resources(AK3MFImportState * __restrict st,
                                    xml_t            * __restrict resourcesXml) {
  xml_t *xml;

  if (!st || !resourcesXml)
    return;

  for (xml = resourcesXml->val; xml; xml = xml->next) {
    if (AK_3MF_TAG(xml, displacement2d))
      ak_3mf_parse_displacement2d(st, xml);
    else if (AK_3MF_TAG(xml, normvectorgroup))
      ak_3mf_parse_norm_vector_group(st, xml);
    else if (AK_3MF_TAG(xml, disp2dgroup))
      ak_3mf_parse_disp2d_group(st, xml);
  }
}

static
void
ak_3mf_parse_displacement_mesh(AK3MFImportState * __restrict st,
                               xml_t            * __restrict meshXml,
                               uint32_t                      objectId) {
  AkPrintDisplacementMesh *mesh;
  xml_t                   *trianglesXml;
  xml_t                   *triangleXml;
  xml_attr_t              *defaultGroupAttr;
  uint32_t                 flags;

  if (!st || !meshXml)
    return;

  trianglesXml     = AK_3MF_CHILD(meshXml, triangles);
  defaultGroupAttr = AK_3MF_XMLA_LOCAL_LIT(trianglesXml, "did");
  flags            = ak_3mf_optional_attr_flag(defaultGroupAttr,
                                               AK_PRINT_DISPLACEMENT_MESH_HAS_DEFAULT_GROUP);
  mesh = ak_printAddDisplacementMesh(st->doc,
                                     st->currentModelPath,
                                     objectId,
                                     xmla_u32(defaultGroupAttr, 0u),
                                     flags);
  if (!mesh)
    return;

  for (triangleXml = trianglesXml ? trianglesXml->val : NULL;
       triangleXml;
       triangleXml = triangleXml->next) {
    xml_attr_t *groupAttr;
    xml_attr_t *d1Attr;
    xml_attr_t *d2Attr;
    xml_attr_t *d3Attr;
    uint32_t    triangleFlags;
    uint32_t    d1;

    if (!AK_3MF_TAG8(triangleXml, triangle))
      continue;

    groupAttr = AK_3MF_XMLA_LOCAL_LIT(triangleXml, "did");
    d1Attr    = AK_3MF_XMLA_LOCAL_LIT(triangleXml, "d1");
    d2Attr    = AK_3MF_XMLA_LOCAL_LIT(triangleXml, "d2");
    d3Attr    = AK_3MF_XMLA_LOCAL_LIT(triangleXml, "d3");
    d1        = xmla_u32(d1Attr, UINT32_MAX);
    triangleFlags = ak_3mf_optional_attr_flag(groupAttr,
                                              AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_GROUP)
                    | ak_3mf_optional_attr_flag(d1Attr,
                                                AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D1)
                    | ak_3mf_optional_attr_flag(d2Attr,
                                                AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D2)
                    | ak_3mf_optional_attr_flag(d3Attr,
                                                AK_PRINT_DISPLACEMENT_TRIANGLE_HAS_D3);

    (void)ak_printAddDisplacementTriangle(st->doc,
                                          mesh,
                                          xmla_u32(groupAttr, 0u),
                                          d1,
                                          xmla_u32(d2Attr, d1),
                                          xmla_u32(d3Attr, d1),
                                          triangleFlags);
  }
}

static
void
ak_3mf_count_slice_geometry(xml_t    * __restrict sliceXml,
                            uint32_t * __restrict vertexCount,
                            uint32_t * __restrict polygonCount,
                            uint32_t * __restrict segmentCount) {
  xml_t  *verticesXml;
  xml_t  *polygonXml;
  size_t  vertices;
  size_t  polygons;
  size_t  segments;

  vertices = 0u;
  polygons = 0u;
  segments = 0u;

  if (sliceXml) {
    verticesXml = AK_3MF_CHILD8(sliceXml, vertices);
    vertices    = AK_3MF_COUNT_CHILDREN8(verticesXml, vertex);

    for (polygonXml = sliceXml->val; polygonXml; polygonXml = polygonXml->next) {
      if (!AK_3MF_TAG8(polygonXml, polygon))
        continue;

      polygons++;
      segments += AK_3MF_COUNT_CHILDREN8(polygonXml, segment);
    }
  }

  if (vertexCount)
    *vertexCount = ak_3mf_clamp_count_u32(vertices);
  if (polygonCount)
    *polygonCount = ak_3mf_clamp_count_u32(polygons);
  if (segmentCount)
    *segmentCount = ak_3mf_clamp_count_u32(segments);
}

static
size_t
ak_3mf_parse_slice_stacks(AK3MFImportState * __restrict st,
                          xml_t            * __restrict resourcesXml) {
  xml_t  *stackXml;
  size_t  added;

  if (!st || !st->doc || !st->print || !resourcesXml)
    return 0u;

  added = 0u;
  for (stackXml = resourcesXml->val; stackXml; stackXml = stackXml->next) {
    AkPrintSliceStack *stack;
    xml_t             *childXml;
    uint32_t           stackId;
    float              zBottom;

    if (!AK_3MF_TAG(stackXml, slicestack))
      continue;

    stackId = xmla_u32(AK_3MF_XMLA_LOCAL(stackXml, id), 0u);
    zBottom = xmla_float(AK_3MF_XMLA_LOCAL_LIT(stackXml, "zbottom"), 0.0f);
    stack   = ak_printAddSliceStack(st->doc,
                                    st->currentModelPath,
                                    stackId,
                                    zBottom);
    if (!stack)
      continue;

    added++;
    for (childXml = stackXml->val; childXml; childXml = childXml->next) {
      if (AK_3MF_TAG8(childXml, sliceref)) {
        xml_attr_t *pathAttr;
        char       *path;
        uint32_t    refStackId;
        float       zTop;

        pathAttr   = AK_3MF_XMLA_LOCAL_LIT(childXml, "slicepath");
        path       = ak_3mf_attr_dup_path_cstr(pathAttr);
        refStackId = xmla_u32(AK_3MF_XMLA_LOCAL_LIT(childXml, "slicestackid"), 0u);
        zTop       = xmla_float(AK_3MF_XMLA_LOCAL_LIT(childXml, "ztop"), 0.0f);
        if (ak_printAddSliceRef(st->doc,
                                path ? path : st->currentModelPath,
                                refStackId,
                                zTop)) {
          stack->sliceRefCount++;
          added++;
        }
        if (path && !ak_3mf_model_path_eq(path, st->currentModelPath))
          (void)ak_3mf_load_model_part(st, path);
        free(path);
      } else if (AK_3MF_TAG8(childXml, slice)) {
        uint32_t vertexCount;
        uint32_t polygonCount;
        uint32_t segmentCount;
        float    zTop;

        ak_3mf_count_slice_geometry(childXml,
                                    &vertexCount,
                                    &polygonCount,
                                    &segmentCount);
        zTop = xmla_float(AK_3MF_XMLA_LOCAL_LIT(childXml, "ztop"), 0.0f);
        if (ak_printAddSlice(st->doc,
                             st->currentModelPath,
                             stackId,
                             zTop,
                             vertexCount,
                             polygonCount,
                             segmentCount)) {
          stack->sliceCount++;
          added++;
        }
      }
    }
  }

  return added;
}

static
void
ak_3mf_parse_slice_object(AK3MFImportState * __restrict st,
                          xml_t            * __restrict objXml,
                          uint32_t                      objectId) {
  xml_attr_t *stackAttr;
  xml_attr_t *pathAttr;
  xml_attr_t *meshResolutionAttr;
  char       *slicePath;
  char       *meshResolution;
  uint32_t    sliceStackId;

  if (!st || !st->doc || !st->print || !objXml)
    return;

  stackAttr          = AK_3MF_XMLA_LOCAL_LIT(objXml, "slicestackid");
  pathAttr           = AK_3MF_XMLA_LOCAL_LIT(objXml, "slicepath");
  meshResolutionAttr = AK_3MF_XMLA_LOCAL_LIT(objXml, "meshresolution");

  if ((!stackAttr || !stackAttr->val)
      && (!pathAttr || !pathAttr->val)
      && (!meshResolutionAttr || !meshResolutionAttr->val))
    return;

  slicePath      = ak_3mf_attr_dup_path_cstr(pathAttr);
  meshResolution = ak_3mf_attr_dup_cstr(meshResolutionAttr);
  sliceStackId   = xmla_u32(stackAttr, 0u);

  (void)ak_printAddSliceObject(st->doc,
                               st->currentModelPath,
                               slicePath,
                               meshResolution,
                               objectId,
                               sliceStackId);
  if (slicePath && !ak_3mf_model_path_eq(slicePath, st->currentModelPath))
    (void)ak_3mf_load_model_part(st, slicePath);

  free(slicePath);
  free(meshResolution);
}

static
bool
ak_3mf_parse_components(AK3MFImportState * __restrict st,
                        AK3MFObject      * __restrict object,
                        xml_t            * __restrict componentsXml) {
  xml_t   *componentXml;
  size_t   componentCount;
  size_t   i;

  if (!st || !object || !componentsXml)
    return false;

  componentCount = AK_3MF_COUNT_CHILDREN(componentsXml, component);
  if (componentCount == 0 || componentCount > UINT32_MAX)
    return false;

  object->components = ak_heap_calloc(ak_heap_getheap(st->doc),
                                      st->doc,
                                      sizeof(*object->components) * componentCount);
  if (!object->components)
    return false;

  object->componentCount = (uint32_t)componentCount;
  object->kind           = AK_3MF_OBJECT_COMPONENTS;

  i = 0;
  for (componentXml = componentsXml->val;
       componentXml;
       componentXml = componentXml->next) {
    AK3MFComponent *component;

    if (!AK_3MF_TAG(componentXml, component))
      continue;

    component           = &object->components[i++];
    component->objectId = xmla_u32(AK_3MF_XMLA(componentXml, objectid), 0u);
    component->path     = AK_3MF_STRDUP_PATH_ATTR_LOCAL(st->doc,
                                                        componentXml,
                                                        path,
                                                        object->components);
    ak_3mf_add_production_item(st,
                               AK_PRINT_PRODUCTION_COMPONENT,
                               componentXml,
                               component->objectId,
                               object->id);
    if (!ak_3mf_parse_transform_attr(AK_3MF_XMLA(componentXml, transform),
                                     component->matrix))
      return false;
  }

  return true;
}

static
void
ak_3mf_parse_production_alternatives(AK3MFImportState * __restrict st,
                                     xml_t            * __restrict objectXml,
                                     uint32_t                       parentObjectId) {
  xml_t *alternativesXml;
  xml_t *alternativeXml;

  if (!st || !objectXml)
    return;

  alternativesXml = AK_3MF_CHILD(objectXml, alternatives);
  if (!alternativesXml)
    return;

  for (alternativeXml = alternativesXml->val;
       alternativeXml;
       alternativeXml = alternativeXml->next) {
    if (!AK_3MF_TAG(alternativeXml, alternative))
      continue;

    ak_3mf_add_production_item(st,
                               AK_PRINT_PRODUCTION_ALTERNATIVE,
                               alternativeXml,
                               xmla_u32(AK_3MF_XMLA_LOCAL(alternativeXml,
                                                           objectid),
                                        0u),
                               parentObjectId);
  }
}

static
size_t
ak_3mf_parse_resources(AK3MFImportState * __restrict st,
                       xml_t            * __restrict resourcesXml) {
  xml_t  *objXml;
  size_t  startCount;

  if (!st || !resourcesXml)
    return 0;

  if (!ak_3mf_reserve_objects(st, ak_3mf_count_resource_objects(resourcesXml)))
    return 0;

  startCount = st->objectCount;
  for (objXml = resourcesXml->val; objXml; objXml = objXml->next) {
    AK3MFObject *object;
    xml_t      *meshXml;
    xml_t      *componentsXml;
    xml_t      *displacementMeshXml;
    xml_t      *booleanShapeXml;
    xml_t      *levelSetXml;

    if (!AK_3MF_TAG8(objXml, object))
      continue;
    meshXml = AK_3MF_CHILD8(objXml, mesh);
    componentsXml = AK_3MF_CHILD(objXml, components);
    displacementMeshXml = AK_3MF_CHILD(objXml, displacementmesh);
    booleanShapeXml = AK_3MF_CHILD(objXml, booleanshape);
    levelSetXml = AK_3MF_CHILD8(objXml, levelset);
    if (!meshXml
        && !componentsXml
        && !displacementMeshXml
        && !booleanShapeXml
        && !levelSetXml)
      continue;

    if (st->objectCount >= st->objectCapacity)
      break;

    object         = &st->objects[st->objectCount];
    object->path   = st->currentModelPath;
    object->id     = xmla_u32(AK_3MF_XMLA(objXml, id), (uint32_t)(st->objectCount + 1u));
    object->pid    = xmla_u32(AK_3MF_XMLA(objXml, pid), 0u);
    object->pindex = xmla_u32(AK_3MF_XMLA(objXml, pindex), UINT32_MAX);
    object->name   = xmla_strdup(AK_3MF_XMLA(objXml, name),
                                  ak_heap_getheap(st->doc),
                                  st->doc);
    ak_3mf_add_production_item(st,
                               AK_PRINT_PRODUCTION_OBJECT,
                               objXml,
                               object->id,
                               0u);
    ak_3mf_parse_production_alternatives(st, objXml, object->id);

    if (meshXml || displacementMeshXml) {
      object->geom = ak_3mf_parse_mesh(st,
                                       objXml,
                                       displacementMeshXml ? displacementMeshXml : meshXml);
      if (!object->geom)
        continue;
      object->kind = displacementMeshXml
                     ? AK_3MF_OBJECT_DISPLACEMENT
                     : AK_3MF_OBJECT_MESH;
      if (st->print)
        st->print->meshObjectCount++;
      st->objectCount++;
      ak_3mf_parse_slice_object(st, objXml, object->id);
    } else if (componentsXml && ak_3mf_parse_components(st, object, componentsXml)) {
      if (st->print)
        st->print->componentObjectCount++;
      st->objectCount++;
      ak_3mf_parse_slice_object(st, objXml, object->id);
    } else if (booleanShapeXml && ak_3mf_parse_boolean_shape(st,
                                                              object,
                                                              booleanShapeXml)) {
      object->kind = AK_3MF_OBJECT_BOOLEAN;
      st->objectCount++;
    } else if (levelSetXml && ak_3mf_parse_level_set(st, object, levelSetXml)) {
      object->kind = AK_3MF_OBJECT_LEVELSET;
      st->objectCount++;
    }
  }

  return st->objectCount - startCount;
}

static
bool
ak_3mf_model_part_loaded(AK3MFImportState * __restrict st,
                         const char       * __restrict modelPath) {
  size_t i;

  if (!st || !modelPath)
    return false;

  for (i = 0; i < st->loadedModelCount; i++) {
    if (ak_3mf_model_path_eq(st->loadedModelPaths[i], modelPath))
      return true;
  }

  for (i = 0; i < st->objectCount; i++) {
    if (ak_3mf_model_path_eq(st->objects[i].path, modelPath))
      return true;
  }
  for (i = 0; i < st->propertyCount; i++) {
    if (ak_3mf_model_path_eq(st->properties[i].path, modelPath))
      return true;
  }

  return false;
}

static
bool
ak_3mf_mark_model_part_loaded(AK3MFImportState * __restrict st,
                              const char       * __restrict modelPath) {
  if (!st || !modelPath)
    return false;
  if (ak_3mf_model_part_loaded(st, modelPath))
    return true;
  if (!ak_3mf_reserve_loaded_models(st, 1u))
    return false;

  st->loadedModelPaths[st->loadedModelCount++] = modelPath;
  return true;
}

static
const char*
ak_3mf_heap_strdup_model_path(AkDoc       * __restrict doc,
                              const char  * __restrict modelPath) {
  AkHeap     *heap;
  const char *src;
  size_t      len;

  if (!doc || !modelPath)
    return NULL;

  heap = ak_heap_getheap(doc);
  if (!heap)
    return NULL;

  src = modelPath;
  len = strlen(modelPath);
  src = ak_3mf_skip_root_slash(src, &len);
  return ak_heap_strndup(heap, doc, src, len);
}

static
bool
ak_3mf_load_model_part(AK3MFImportState * __restrict st,
                       const char       * __restrict modelPath) {
  void        *modelData;
  size_t       modelSize;
  xml_doc_t   *xdoc;
  xml_t       *root;
  xml_t       *resourcesXml;
  const char  *savedModelPath;
  const char  *storedModelPath;
  const void  *cachedModelData;
  AK3MFFastPreparedModel *preparedModel;
  size_t       added;
  AkResult     result;
  AK3MFFastLoadResult fastResult;
  bool         borrowedModelData;

  if (!st || !st->packagePath || !modelPath)
    return false;
  if (ak_3mf_model_path_eq(modelPath, st->currentModelPath)
      || ak_3mf_model_part_loaded(st, modelPath))
    return true;

  modelData = NULL;
  modelSize = 0u;
  xdoc      = NULL;
  cachedModelData = NULL;
  borrowedModelData = ak_3mf_cached_package_part_data(st,
                                                      modelPath,
                                                      &cachedModelData,
                                                      &modelSize);
  if (borrowedModelData)
    modelData = (void *)cachedModelData;
  if (!borrowedModelData) {
    result = st->package
             ? ak_zip_archive_extract_file(st->package,
                                           modelPath,
                                           &modelData,
                                           &modelSize)
             : ak_zip_extract_file(st->packagePath,
                                   modelPath,
                                   &modelData,
                                   &modelSize);
    if (result != AK_OK)
      return false;
  }

  preparedModel = ak_3mf_find_prepared_model(st, modelPath);
  fastResult = preparedModel
               ? ak_3mf_fast_commit_prepared_model_part(st,
                                                        modelPath,
                                                        preparedModel)
               : AK_3MF_FAST_LOAD_UNSUPPORTED;
  if (fastResult == AK_3MF_FAST_LOAD_UNSUPPORTED) {
    fastResult = ak_3mf_fast_load_mesh_model_part(st,
                                                  modelPath,
                                                  modelData,
                                                  modelSize);
  }
  if (fastResult != AK_3MF_FAST_LOAD_UNSUPPORTED) {
    if (!borrowedModelData)
      free(modelData);
    return fastResult == AK_3MF_FAST_LOAD_LOADED;
  }

  if (borrowedModelData) {
    modelData = NULL;
    modelSize = 0u;
    result = st->package
             ? ak_zip_archive_extract_file(st->package,
                                           modelPath,
                                           &modelData,
                                           &modelSize)
             : ak_zip_extract_file(st->packagePath,
                                   modelPath,
                                   &modelData,
                                   &modelSize);
    if (result != AK_OK)
      return false;
    borrowedModelData = false;
  }

  xdoc = xml_parse(modelData, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root || !AK_3MF_TAG8(xdoc->root, model)) {
    if (xdoc)
      xml_free(xdoc);
    free(modelData);
    return false;
  }

  storedModelPath = ak_3mf_heap_strdup_model_path(st->doc, modelPath);
  if (!storedModelPath) {
    xml_free(xdoc);
    free(modelData);
    return false;
  }

  savedModelPath       = st->currentModelPath;
  st->currentModelPath = storedModelPath;
  (void)ak_3mf_mark_model_part_loaded(st, storedModelPath);
  root                 = xdoc->root;
  resourcesXml         = AK_3MF_CHILD(root, resources);

  if (st->print)
    ak_3mf_mark_model_extensions(st->print, root);

  (void)ak_3mf_parse_property_groups(st, resourcesXml);
  (void)ak_3mf_parse_displacement_resources(st, resourcesXml);
  (void)ak_3mf_parse_volumetric_resources(st, resourcesXml);
  (void)ak_3mf_parse_slice_stacks(st, resourcesXml);
  added = ak_3mf_parse_resources(st, resourcesXml);
  if (st->print)
    st->print->objectCount = (uint32_t)st->objectCount;

  st->currentModelPath = savedModelPath;
  xml_free(xdoc);
  free(modelData);
  return added > 0u || ak_3mf_model_part_loaded(st, storedModelPath);
}

static
bool
ak_3mf_parse_transform_attr(const xml_attr_t * __restrict attr,
                            float                         matrix[16]) {
  const char *it;
  const char *end;
  const char *next;
  float       values[12];
  size_t      i;

  matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
  matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
  matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;

  if (!attr || !attr->val || attr->valsize == 0)
    return true;

  it  = attr->val;
  end = attr->val + attr->valsize;
  for (i = 0; i < 12u; i++) {
    it = ak_str_skip_sep_fast((char *)it, (char *)end, false);
    if (!ak_str_parse_float_token_fast(it, end, &values[i], &next))
      return false;
    it = next;
  }

  matrix[0]  = values[0];
  matrix[1]  = values[1];
  matrix[2]  = values[2];
  matrix[4]  = values[3];
  matrix[5]  = values[4];
  matrix[6]  = values[5];
  matrix[8]  = values[6];
  matrix[9]  = values[7];
  matrix[10] = values[8];
  matrix[12] = values[9];
  matrix[13] = values[10];
  matrix[14] = values[11];

  return true;
}

static
void
ak_3mf_identity_matrix(float matrix[16]) {
  matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f; matrix[3] = 0.0f;
  matrix[4] = 0.0f; matrix[5] = 1.0f; matrix[6] = 0.0f; matrix[7] = 0.0f;
  matrix[8] = 0.0f; matrix[9] = 0.0f; matrix[10] = 1.0f; matrix[11] = 0.0f;
  matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

static AK_NOINLINE
void
ak_3mf_object_name(char * __restrict dst, uint32_t objectId) {
  char *p;

  memcpy(dst, "3MF Object ", sizeof("3MF Object ") - 1u);
  p  = dst + sizeof("3MF Object ") - 1u;
  p  = ak_io_text_format_uint64(p, objectId);
  *p = '\0';
}

static
bool
ak_3mf_attach_object_node(AK3MFImportState * __restrict st,
                          AkNode           * __restrict parent,
                          AK3MFObject      * __restrict object,
                          const float                   matrix[16],
                          const char       * __restrict fallbackName,
                          uint32_t                      depth) {
  AkNode   *node;
  char      name[64];
  uint32_t  i;

  if (!st || !parent || !object)
    return false;
  if (depth > 512u)
    return false;

  if (object->name) {
    fallbackName = object->name;
  } else if (!fallbackName) {
    ak_3mf_object_name(name, object->id);
    fallbackName = name;
  }

  node = ak_3mf_node_new(st->doc, parent, fallbackName);
  if (!node)
    return false;
  ak_nodeSetTransformMatrix(node, matrix);

  if (object->kind == AK_3MF_OBJECT_MESH
      || object->kind == AK_3MF_OBJECT_DISPLACEMENT) {
    return object->geom && ak_nodeAttachGeometry(node, object->geom);
  }

  if (object->kind != AK_3MF_OBJECT_COMPONENTS)
    return true;

  {
    AK3MFComponent *components;
    const char     *objectPath;
    uint32_t        componentCount;

    components     = object->components;
    componentCount = object->componentCount;
    objectPath     = object->path;
    if (!components)
      return true;

    for (i = 0; i < componentCount; i++) {
      AK3MFObject *child;
      const char  *childPath;
      const char  *componentPath;
      uint32_t     componentObjectId;
      float        componentMatrix[16];

      componentPath     = components[i].path;
      componentObjectId = components[i].objectId;
      memcpy(componentMatrix, components[i].matrix, sizeof(componentMatrix));
      childPath = componentPath ? componentPath : objectPath;
      child     = ak_3mf_find_object(st->objects,
                                     st->objectCount,
                                     childPath,
                                     componentObjectId);
      if (!child && componentPath) {
        (void)ak_3mf_load_model_part(st, componentPath);
        childPath = componentPath;
        child = ak_3mf_find_object(st->objects,
                                   st->objectCount,
                                   childPath,
                                   componentObjectId);
      }
      if (!child)
        continue;

      if (!ak_3mf_attach_object_node(st,
                                     node,
                                     child,
                                     componentMatrix,
                                     NULL,
                                     depth + 1u))
        return false;
    }
  }

  return true;
}

static
bool
ak_3mf_attach_build_items(AK3MFImportState * __restrict st,
                          AkScene          * __restrict scene,
                          xml_t            * __restrict buildXml) {
  xml_t   *itemXml;
  size_t   attached;

  attached = 0;
  if (!st || !buildXml)
    return false;

  ak_3mf_add_production_item(st,
                             AK_PRINT_PRODUCTION_BUILD,
                             buildXml,
                             0u,
                             0u);

  for (itemXml = buildXml->val; itemXml; itemXml = itemXml->next) {
    AK3MFObject *object;
    const char  *path;
    uint32_t     objectId;
    float        matrix[16];
    char         name[48];

    if (!AK_3MF_TAG8(itemXml, item))
      continue;

    path = AK_3MF_STRDUP_PATH_ATTR_LOCAL(st->doc, itemXml, path, st->doc);
    objectId = xmla_u32(AK_3MF_XMLA(itemXml, objectid), 0u);
    if (path && !ak_3mf_load_model_part(st, path))
      continue;

    object = ak_3mf_find_object(st->objects,
                                st->objectCount,
                                path ? path : st->rootModelPath,
                                objectId);
    if (!object)
      continue;
    ak_3mf_add_production_item(st,
                               AK_PRINT_PRODUCTION_ITEM,
                               itemXml,
                               objectId,
                               0u);

    ak_3mf_object_name(name, objectId);
    if (!ak_3mf_parse_transform_attr(AK_3MF_XMLA(itemXml, transform), matrix))
      return false;
    if (!ak_3mf_attach_object_node(st, scene->node, object, matrix, name, 0u))
      return false;

    attached++;
  }

  if (st->print)
    st->print->buildItemCount += (uint32_t)attached;

  return attached > 0;
}

static
bool
ak_3mf_attach_resource_fallback(AK3MFImportState * __restrict st,
                                AkScene          * __restrict scene) {
  size_t i;
  float  matrix[16];

  ak_3mf_identity_matrix(matrix);
  for (i = 0; i < st->objectCount; i++) {
    char    name[48];

    ak_3mf_object_name(name, st->objects[i].id);
    if (!ak_3mf_attach_object_node(st,
                                   scene->node,
                                   &st->objects[i],
                                   matrix,
                                   name,
                                   0u))
      return false;
  }

  return true;
}

AK_HIDE
AkResult
imp_3mf(AkDoc ** __restrict dest, const char * __restrict filepath) {
  char        *modelPath;
  void        *modelData;
  void        *contentTypesData;
  void        *rootRelsData;
  size_t       modelSize;
  size_t       contentTypesSize;
  size_t       rootRelsSize;
  xml_doc_t   *xdoc;
  xml_doc_t   *contentTypesDoc;
  xml_doc_t   *rootRelsDoc;
  xml_t       *root;
  xml_t       *resourcesXml;
  xml_t       *buildXml;
  AkDoc       *doc;
  AkScene     *scene;
  AkZipArchive *package;
  AK3MFImportState st;
  AkResult     result;
  size_t       i;

  if (!dest || !filepath)
    return AK_ERR;

  *dest     = NULL;
  modelPath = NULL;
  modelData = NULL;
  contentTypesData = NULL;
  rootRelsData = NULL;
  modelSize = 0;
  contentTypesSize = 0;
  rootRelsSize = 0;
  xdoc      = NULL;
  contentTypesDoc = NULL;
  rootRelsDoc = NULL;
  doc       = NULL;
  package   = NULL;
  memset(&st, 0, sizeof(st));

  result = ak_zip_open(filepath, &package);
  if (result != AK_OK)
    goto cleanup;

  modelPath = ak_3mf_model_path_from_rels(package, filepath);
  if (!modelPath)
    goto cleanup_err;

  result = ak_zip_archive_extract_file(package, modelPath, &modelData, &modelSize);
  if (result != AK_OK)
    goto cleanup;

  if (ak_zip_archive_extract_file(package,
                                  AK_3MF_CONTENT_TYPES_PART,
                                  &contentTypesData,
                                  &contentTypesSize) == AK_OK) {
    contentTypesDoc = xml_parse(contentTypesData, XML_PREFIXES | XML_READONLY);
  }
  if (ak_zip_archive_extract_file(package,
                                  AK_3MF_ROOT_RELS_PART,
                                  &rootRelsData,
                                  &rootRelsSize) == AK_OK) {
    rootRelsDoc = xml_parse(rootRelsData, XML_PREFIXES | XML_READONLY);
  }

  xdoc = xml_parse(modelData, XML_PREFIXES | XML_READONLY);
  if (!xdoc || !xdoc->root) {
    result = AK_EBADF;
    goto cleanup;
  }

  root = xdoc->root;
  if (!AK_3MF_TAG8(root, model)) {
    result = AK_EBADF;
    goto cleanup;
  }

  doc = ak_3mf_doc_new(filepath, root);
  if (!doc) {
    result = AK_ERR;
    goto cleanup;
  }
  st.doc = doc;
  st.package          = package;
  st.packagePath      = filepath;
  st.rootModelPath    = modelPath;
  st.currentModelPath = modelPath;
  (void)ak_3mf_mark_model_part_loaded(&st, modelPath);
  st.print = ak_printDocumentEnsure(doc);
  ak_3mf_bambu_orca_parse_metadata(&st);
  if (st.print) {
    st.print->features |= AK_PRINT_FEATURE_CORE;
    ak_3mf_mark_model_extensions(st.print, root);
    (void)ak_printAddPackagePart(
      doc,
      AK_PRINT_PACKAGE_PART_MODEL,
      modelPath,
      "application/vnd.ms-package.3dmanufacturing-3dmodel+xml",
      "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel");
    result = ak_3mf_import_package_parts(&st,
                                         doc,
                                         st.print,
                                         package,
                                         filepath,
                                         modelPath,
                                         contentTypesDoc ? contentTypesDoc->root : NULL,
                                         rootRelsDoc ? rootRelsDoc->root : NULL);
    if (result != AK_OK)
      goto cleanup;
  }

  scene = ak_3mf_scene_new(doc);
  if (!scene) {
    result = AK_ERR;
    goto cleanup;
  }

  resourcesXml   = AK_3MF_CHILD(root, resources);
  buildXml       = AK_3MF_CHILD8(root, build);
  (void)ak_3mf_parse_property_groups(&st, resourcesXml);
  (void)ak_3mf_parse_displacement_resources(&st, resourcesXml);
  (void)ak_3mf_parse_volumetric_resources(&st, resourcesXml);
  (void)ak_3mf_parse_slice_stacks(&st, resourcesXml);
  (void)ak_3mf_parse_resources(&st, resourcesXml);
  if (st.print)
    st.print->objectCount = (uint32_t)st.objectCount;
  if (buildXml && ak_3mf_attach_build_items(&st, scene, buildXml)) {
    if (st.print)
      st.print->objectCount = (uint32_t)st.objectCount;
  } else if (st.objectCount > 0) {
    if (!ak_3mf_attach_resource_fallback(&st, scene)) {
      result = AK_ERR;
      goto cleanup;
    }
  }

  *dest  = doc;
  result = AK_OK;

cleanup:
  if (st.properties) {
    for (i = 0; i < st.propertyCount; i++) {
      free(st.properties[i].colors);
      free(st.properties[i].texcoords);
    }
  }
  free(st.properties);
  free(st.objects);
  free(st.loadedModelPaths);
  if (st.preparedModels) {
    for (i = 0; i < st.preparedModelCount; i++)
      ak_3mf_fast_prepared_model_free(st.preparedModels[i].model);
  }
  free(st.preparedModels);
  free(st.bambuParts);
  free(st.bambuColors);
  free(st.bambuMaterials);
  if (xdoc)
    xml_free(xdoc);
  if (contentTypesDoc)
    xml_free(contentTypesDoc);
  if (rootRelsDoc)
    xml_free(rootRelsDoc);
  free(modelData);
  free(contentTypesData);
  free(rootRelsData);
  free(modelPath);
  ak_zip_close(package);
  return result;

cleanup_err:
  result = AK_ERR;
  goto cleanup;
}
