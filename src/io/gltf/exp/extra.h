/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 */

#ifndef assetkit_gltf_exp_extra_h
#define assetkit_gltf_exp_extra_h

#include "common.h"
#include "writer.h"

#include <stddef.h>
#include <stdbool.h>

typedef bool (*GLTFExtraExtensionSkipFn)(const char * __restrict name,
                                         size_t                  nameLen,
                                         void * __restrict       userdata);

void
gltf_write_extra_json_value(GLTFExpWriter * __restrict w,
                            AkTreeNode    * __restrict node);

bool
gltf_extra_has_json_extras(AkTreeNode * __restrict node);

void
gltf_write_extra_json_extras(GLTFExpWriter * __restrict w,
                             AkTreeNode    * __restrict node);

AkTreeNode*
gltf_extra_extensions_node(AkTreeNode * __restrict extra);

bool
gltf_extra_has_extensions(AkTreeNode               * __restrict extra,
                          GLTFExtraExtensionSkipFn              skip,
                          void                     * __restrict userdata);

void
gltf_write_extra_extension_entries(GLTFExpWriter           * __restrict w,
                                   AkTreeNode              * __restrict extra,
                                   GLTFExtraExtensionSkipFn              skip,
                                   void                    * __restrict userdata,
                                   bool                    * __restrict comma);

bool
gltf_write_extra_extensions_member(GLTFExpWriter           * __restrict w,
                                   bool                    * __restrict outerComma,
                                   AkTreeNode              * __restrict extra,
                                   GLTFExtraExtensionSkipFn              skip,
                                   void                    * __restrict userdata);

AkTreeNode*
gltf_extra_root_extensions_node(GLTFExpState * __restrict st);

bool
gltf_extra_has_root_extension(GLTFExpState * __restrict st,
                              const char   * __restrict name,
                              size_t                    nameLen);

bool
gltf_extra_root_extension_required(GLTFExpState * __restrict st,
                                   const char   * __restrict name,
                                   size_t                    nameLen);

void
gltf_write_extra_root_extension(GLTFExpWriter * __restrict w,
                                GLTFExpState  * __restrict st,
                                const char    * __restrict name,
                                size_t                     nameLen);

#endif /* assetkit_gltf_exp_extra_h */
