/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#include "extra.h"

static
AkTreeNodeAttr*
gltf_treeAttr(AkHeap     * __restrict heap,
              AkTreeNode * __restrict node,
              const char * __restrict name,
              const char * __restrict val) {
  AkTreeNodeAttr *attr;

  attr       = ak_heap_calloc(heap, node, sizeof(*attr));
  attr->name = ak_heap_strdup(heap, attr, name);
  attr->val  = ak_heap_strdup(heap, attr, val);

  attr->next = node->attribs;
  if (node->attribs)
    node->attribs->prev = attr;

  node->attribs = attr;
  node->attrc++;

  return attr;
}

static
void
gltf_treeAppend(AkTreeNode * __restrict parent,
                AkTreeNode * __restrict child) {
  child->next = parent->chld;
  if (parent->chld)
    parent->chld->prev = child;

  parent->chld  = child;
  child->parent = parent;
  parent->chldc++;
}

static
const char*
gltf_treeType(const json_t * __restrict json) {
  if (!json)
    return "null";

  switch (json->type) {
    case JSON_OBJECT:
      return "object";
    case JSON_ARRAY:
      return "array";
    case JSON_STRING:
      return "value";
    default:
      break;
  }

  return "unknown";
}

static
AkTreeNode*
gltf_treeFromJson(AkHeap      * __restrict heap,
                  void        * __restrict owner,
                  const json_t * __restrict json,
                  const char  * __restrict name,
                  size_t                   nameLen) {
  AkTreeNode *node;
  json_t     *child;

  node = ak_heap_calloc(heap, owner, sizeof(*node));
  if (name && nameLen > 0)
    node->name = ak_heap_strndup(heap, node, name, nameLen);
  else
    node->name = ak_heap_strdup(heap, node, "item");

  gltf_treeAttr(heap, node, "type", gltf_treeType(json));

  if (!json)
    return node;

  if (json->type == JSON_STRING) {
    if (json->value && json->valsize > 0)
      node->val = ak_heap_strndup(heap, node, json->value, json->valsize);
    return node;
  }

  if (json->type != JSON_OBJECT && json->type != JSON_ARRAY)
    return node;

  child = json->value;
  while (child) {
    AkTreeNode *tnode;

    if (json->type == JSON_OBJECT) {
      tnode = gltf_treeFromJson(heap,
                                node,
                                child,
                                child->key,
                                child->keysize);
    } else {
      tnode = gltf_treeFromJson(heap, node, child, "item", 4);
    }

    /* json_parse(..., true) stores children in reverse. Prepend restores
       source order for arrays while object order remains immaterial. */
    gltf_treeAppend(node, tnode);
    child = child->next;
  }

  return node;
}

AK_HIDE
void
gltf_extra(AkGLTFState * __restrict gst,
           void        * __restrict owner,
           const json_t * __restrict jextras,
           const json_t * __restrict jextensions) {
  AkHeap     *heap;
  AkTreeNode *root;
  AkTreeNode *node;

  if (!gst || !owner || (!jextras && !jextensions))
    return;

  heap = gst->heap;
  root = ak_extra(owner);
  if (!root) {
    root       = ak_heap_calloc(heap, owner, sizeof(*root));
    root->name = ak_heap_strdup(heap, root, "extra");
    ak_extra_set(owner, root);
  }

  if (jextensions) {
    node = gltf_treeFromJson(heap,
                             root,
                             jextensions,
                             _s_gltf_extensions,
                             strlen(_s_gltf_extensions));
    gltf_treeAppend(root, node);
  }

  if (jextras) {
    node = gltf_treeFromJson(heap,
                             root,
                             jextras,
                             _s_gltf_extras,
                             strlen(_s_gltf_extras));
    gltf_treeAppend(root, node);
  }
}
