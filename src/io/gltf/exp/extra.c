/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "extra.h"
#include "../strpool.h"
#include "../../../string_fast.h"

#include <string.h>

static
AkTreeNode*
gltf_extra_child(const AkTreeNode * __restrict node,
                 const char       * __restrict name,
                 size_t                        nameLen) {
  AkTreeNode *child;

  if (!node)
    return NULL;

  for (child = node->chld; child; child = child->next) {
    if (ak_str_eq_cstr_fast(child->name, name, nameLen))
      return child;
  }

  return NULL;
}

static
const char*
gltf_extra_type(const AkTreeNode * __restrict node) {
  AkTreeNodeAttr *attr;

  if (!node)
    return NULL;

  for (attr = node->attribs; attr; attr = attr->next) {
    if (ak_str_eq_cstr_fast(attr->name, _s_gltf_type, _s_gltf_type_len))
      return attr->val;
  }

  return NULL;
}

static
bool
gltf_extra_type_eq(const AkTreeNode * __restrict node,
                   const char       * __restrict type) {
  const char *nodeType;

  nodeType = gltf_extra_type(node);
  return nodeType && strcmp(nodeType, type) == 0;
}

AkTreeNode*
gltf_extra_extensions_node(AkTreeNode * __restrict extra) {
  return gltf_extra_child(extra,
                          _s_gltf_extensions,
                          _s_gltf_extensions_len);
}

AkTreeNode*
gltf_extra_root_extensions_node(GLTFExpState * __restrict st) {
  AkTree *root;

  root = st && st->doc ? ak_extra(st->doc) : NULL;
  return gltf_extra_extensions_node(root);
}

static
AkTreeNode*
gltf_extra_root_required(GLTFExpState * __restrict st) {
  AkTree *root;

  root = st && st->doc ? ak_extra(st->doc) : NULL;
  return gltf_extra_child(root,
                          _s_gltf_extensionsRequired,
                          _s_gltf_extensionsRequired_len);
}

static
bool
gltf_extra_value_eq(const AkTreeNode * __restrict node,
                    const char       * __restrict name,
                    size_t                        nameLen) {
  return node
         && node->val
         && ak_str_eq_cstr_fast(node->val, name, nameLen);
}

static
bool
gltf_extra_array_contains(const AkTreeNode * __restrict array,
                          const char       * __restrict name,
                          size_t                        nameLen) {
  AkTreeNode *child;

  for (child = array ? array->chld : NULL; child; child = child->next) {
    if (gltf_extra_value_eq(child, name, nameLen))
      return true;
  }

  return false;
}

static
bool
gltf_extra_json_number(const char * __restrict val, size_t len) {
  size_t i;

  if (!val || len == 0)
    return false;

  i = 0;
  if (val[i] == '-') {
    if (++i == len)
      return false;
  }

  if (val[i] == '0') {
    i++;
  } else if (val[i] >= '1' && val[i] <= '9') {
    do {
      i++;
    } while (i < len && val[i] >= '0' && val[i] <= '9');
  } else {
    return false;
  }

  if (i < len && val[i] == '.') {
    if (++i == len || val[i] < '0' || val[i] > '9')
      return false;
    do {
      i++;
    } while (i < len && val[i] >= '0' && val[i] <= '9');
  }

  if (i < len && (val[i] == 'e' || val[i] == 'E')) {
    if (++i == len)
      return false;
    if (val[i] == '+' || val[i] == '-') {
      if (++i == len)
        return false;
    }
    if (val[i] < '0' || val[i] > '9')
      return false;
    do {
      i++;
    } while (i < len && val[i] >= '0' && val[i] <= '9');
  }

  return i == len;
}

static
bool
gltf_extra_literal(const char * __restrict val,
                   size_t                  len,
                   const char * __restrict literal,
                   size_t                  literalLen) {
  return len == literalLen && memcmp(val, literal, literalLen) == 0;
}

static
bool
gltf_extra_reserved_root_child(AkTreeNode * __restrict node) {
  const char *name;

  name = node ? node->name : NULL;
  return name
         && (ak_str_eq_cstr_fast(name,
                                 _s_gltf_extensions,
                                 _s_gltf_extensions_len)
             || ak_str_eq_cstr_fast(name,
                                    _s_gltf_extensionsRequired,
                                    _s_gltf_extensionsRequired_len));
}

bool
gltf_extra_has_extensions(AkTreeNode               * __restrict extra,
                          GLTFExtraExtensionSkipFn              skip,
                          void                     * __restrict userdata) {
  AkTreeNode *extensions;
  AkTreeNode *child;

  extensions = gltf_extra_extensions_node(extra);
  for (child = extensions ? extensions->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (skip && skip(child->name, nameLen, userdata))
      continue;

    return true;
  }

  return false;
}

void
gltf_write_extra_json_value(GLTFExpWriter * __restrict w,
                            AkTreeNode    * __restrict node);

static
void
gltf_write_extra_object(GLTFExpWriter * __restrict w,
                        AkTreeNode    * __restrict node) {
  AkTreeNode *child;
  bool        comma;

  gltf_w_ch(w, '{');
  comma = false;
  for (child = node ? node->chld : NULL; child; child = child->next) {
    if (!child->name)
      continue;
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, child->name, strlen(child->name));
    gltf_write_extra_json_value(w, child);
    comma = true;
  }
  gltf_w_ch(w, '}');
}

static
void
gltf_write_extra_array(GLTFExpWriter * __restrict w,
                       AkTreeNode    * __restrict node) {
  AkTreeNode *child;
  bool        comma;

  gltf_w_ch(w, '[');
  comma = false;
  for (child = node ? node->chld : NULL; child; child = child->next) {
    if (comma)
      gltf_w_ch(w, ',');
    gltf_write_extra_json_value(w, child);
    comma = true;
  }
  gltf_w_ch(w, ']');
}

static
void
gltf_write_extra_leaf(GLTFExpWriter * __restrict w,
                      AkTreeNode    * __restrict node) {
  const char *val;
  size_t      len;

  val = node ? node->val : NULL;
  if (!val) {
    gltf_w_raw(w, "null", 4);
    return;
  }

  len = strlen(val);
  if (gltf_extra_literal(val, len, "true", 4)
      || gltf_extra_literal(val, len, "false", 5)
      || gltf_extra_literal(val, len, "null", 4)
      || gltf_extra_json_number(val, len)) {
    gltf_w_raw(w, val, len);
    return;
  }

  gltf_w_qstr_len(w, val, len);
}

bool
gltf_extra_has_json_extras(AkTreeNode * __restrict node) {
  AkTreeNode *child;

  if (!node)
    return false;

  if (gltf_extra_type_eq(node, "array"))
    return true;

  if (!node->chld)
    return node->val != NULL;

  for (child = node->chld; child; child = child->next) {
    if (child->name && !gltf_extra_reserved_root_child(child))
      return true;
  }

  return false;
}

void
gltf_write_extra_json_extras(GLTFExpWriter * __restrict w,
                             AkTreeNode    * __restrict node) {
  AkTreeNode *child;
  bool        comma;

  if (!node || gltf_extra_type_eq(node, "array") || !node->chld) {
    gltf_write_extra_json_value(w, node);
    return;
  }

  gltf_w_ch(w, '{');
  comma = false;
  for (child = node->chld; child; child = child->next) {
    if (!child->name || gltf_extra_reserved_root_child(child))
      continue;
    if (comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, child->name, strlen(child->name));
    gltf_write_extra_json_value(w, child);
    comma = true;
  }
  gltf_w_ch(w, '}');
}

void
gltf_write_extra_extension_entries(GLTFExpWriter           * __restrict w,
                                   AkTreeNode              * __restrict extra,
                                   GLTFExtraExtensionSkipFn              skip,
                                   void                    * __restrict userdata,
                                   bool                    * __restrict comma) {
  AkTreeNode *extensions;
  AkTreeNode *child;

  extensions = gltf_extra_extensions_node(extra);
  for (child = extensions ? extensions->chld : NULL; child; child = child->next) {
    size_t nameLen;

    if (!child->name)
      continue;

    nameLen = strlen(child->name);
    if (skip && skip(child->name, nameLen, userdata))
      continue;

    if (*comma)
      gltf_w_ch(w, ',');
    gltf_w_key(w, child->name, nameLen);
    gltf_write_extra_json_value(w, child);
    *comma = true;
  }
}

bool
gltf_write_extra_extensions_member(GLTFExpWriter           * __restrict w,
                                   bool                    * __restrict outerComma,
                                   AkTreeNode              * __restrict extra,
                                   GLTFExtraExtensionSkipFn              skip,
                                   void                    * __restrict userdata) {
  bool comma;

  if (!gltf_extra_has_extensions(extra, skip, userdata))
    return false;

  if (*outerComma)
    gltf_w_ch(w, ',');
  gltf_w_key(w, _s_gltf_extensions, _s_gltf_extensions_len);
  gltf_w_ch(w, '{');
  comma = false;
  gltf_write_extra_extension_entries(w, extra, skip, userdata, &comma);
  gltf_w_ch(w, '}');
  *outerComma = true;

  return true;
}

void
gltf_write_extra_json_value(GLTFExpWriter * __restrict w,
                            AkTreeNode    * __restrict node) {
  if (!node) {
    gltf_w_raw(w, "null", 4);
    return;
  }

  if (gltf_extra_type_eq(node, "array")) {
    gltf_write_extra_array(w, node);
  } else if (gltf_extra_type_eq(node, "object") || node->chld) {
    gltf_write_extra_object(w, node);
  } else {
    gltf_write_extra_leaf(w, node);
  }
}

bool
gltf_extra_has_root_extension(GLTFExpState * __restrict st,
                              const char   * __restrict name,
                              size_t                    nameLen) {
  return gltf_extra_child(gltf_extra_root_extensions_node(st), name, nameLen) != NULL;
}

bool
gltf_extra_root_extension_required(GLTFExpState * __restrict st,
                                   const char   * __restrict name,
                                   size_t                    nameLen) {
  return gltf_extra_array_contains(gltf_extra_root_required(st), name, nameLen);
}

void
gltf_write_extra_root_extension(GLTFExpWriter * __restrict w,
                                GLTFExpState  * __restrict st,
                                const char    * __restrict name,
                                size_t                     nameLen) {
  AkTreeNode *extension;

  extension = gltf_extra_child(gltf_extra_root_extensions_node(st), name, nameLen);
  if (!extension)
    return;

  gltf_w_key(w, name, nameLen);
  gltf_write_extra_json_value(w, extension);
}
