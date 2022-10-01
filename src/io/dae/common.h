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

#ifndef dae_common_h
#define dae_common_h

#include "../../../include/ak/assetkit.h"
#include "../../../include/ak/url.h"
#include "../../common.h"
#include "../../utils.h"
#include "../../tree.h"
#include "../../xml.h"
#include "strpool.h"

#include <ds/forward-list-sep.h>
#include <string.h>
#include <xml/xml.h>
#include <xml/attrib.h>

#ifndef AK_INPUT_SEMANTIC_VERTEX 
#  define AK_INPUT_SEMANTIC_VERTEX 100001
#endif

typedef enum AkCOLLADAVersion {
  AK_COLLADA_VERSION_150 = 150,
  AK_COLLADA_VERSION_141 = 141,
  AK_COLLADA_VERSION_140 = 140
} AkCOLLADAVersion;

typedef enum AkControllerType {
  AK_CONTROLLER_MORPH = 1,
  AK_CONTROLLER_SKIN  = 2
} AkControllerType;

typedef struct AkURLQueue {
  AkURL *url;
  struct AkURLQueue *next;
} AkURLQueue;

typedef struct AkDAEVerticesMapItem {
  AkInput         *inp;
  AkMeshPrimitive *prim;
} AkDAEVerticesMapItem;

typedef AK_ALIGN(16) struct DAEState {
  AkHeap          *heap;
  void            *tempmem;
  void            *jobs14;
  AkDoc           *doc;
  AkURLQueue      *urlQueue;
  FListItem       *accessors;
  FListItem       *instCtlrs;
  FListItem       *inputs;
  FListItem       *toRadiansSampelers;
  FListItem       *linkedUserData;
  RBTree          *meshInfo;
  RBTree          *inputmap;
  RBTree          *texmap;
  RBTree          *instanceMap;
  FListItem       *vertMap;
  AkSource        *sources;
  AkCOLLADAVersion version;
  bool             stop;
} DAEState;

typedef struct AkDaeMeshInfo {
  AkInput *pos;
  size_t   nVertex;
} AkDaeMeshInfo;

typedef struct AkDAETextureRef {
  const char *texture;
  const char *texcoord;
} AkDAETextureRef;

typedef struct AkNewParam {
  /* const char * sid; */
  struct AkNewParam *prev;
  struct AkNewParam *next;
  const char        *semantic;
  AkValue           *val;
} AkNewParam;

typedef struct AkController {
  /* const char * id; */
  const char          *name;
  void                *data;
  AkTree              *extra;
  struct AkController *next;
  AkControllerType     type;
} AkController;

typedef struct AkInstanceController {
  AkInstanceBase    base;
  AkURL             geometry;
  struct AkNode   **joints;
  AkBindMaterial   *bindMaterial;
  struct FListItem *reserved;
} AkInstanceController;

typedef struct AkAccessorDAE {
  struct AkDataParam *param;
  AkURL               source;
  size_t              offset;
  uint32_t            stride;
  uint32_t            bound;
} AkAccessorDAE;

typedef struct AkSkinJointsDAE {
  AkInput *joints;
  AkInput *invBindMatrix;
} AkSkinJointsDAE;

typedef struct AkSkinWeightsDAE {
  AkInput     *joints;
  AkInput     *weights;
  AkUIntArray *v;
} AkSkinWeightsDAE;

typedef struct AkSkinDAE {
  AkURL            baseGeom;
  AkSource        *source;
  AkTree          *extra;

  AkSkinJointsDAE  joints;
  AkSkinWeightsDAE weights;
  uint32_t         inputCount;
} AkSkinDAE;

typedef struct AkMorphDAE {
  AkURL     baseGeom;
  AkSource *source;
  AkTree   *extra;
  AkInput  *input;
} AkMorphDAE;

AK_INLINE
void
sid_seta(xml_t  * __restrict xml,
         AkHeap * __restrict heap,
         void   * __restrict memnode,
         void   * __restrict memptr) {
  const char *sid;

  if (!(sid = xmla_strdup_by(xml, heap, _s_dae_sid, memnode)))
    return;

  ak_sid_seta(memnode, memptr, sid);
}

AK_INLINE
void
sid_set(xml_t  * __restrict xml,
        AkHeap * __restrict heap,
        void   * __restrict memnode) {
  const char *sid;
  
  if (!(sid = xmla_strdup_by(xml, heap, _s_dae_sid, memnode)))
    return;
  
  ak_sid_set(memnode, sid);
}

AK_INLINE
void
url_set(DAEState   * __restrict dst,
        xml_t      * __restrict xml,
        const char * __restrict name,
        void       * __restrict memp,
        AkURL      * __restrict url) {
  AkURLQueue *urlQueue;
  xml_attr_t *att;

  if (!(att = xmla(xml, name)) || !att->val) {
    url->reserved = NULL;
    url->url      = NULL;
    return;
  }
  
  ak_url_init(memp, xmla_strdup(att, dst->heap, memp), url);
  
  urlQueue       = dst->heap->allocator->malloc(sizeof(*urlQueue));
  urlQueue->next = dst->urlQueue;
  urlQueue->url  = url;
  dst->urlQueue  = urlQueue;
}


AK_INLINE
void
dae_vertmap_add(DAEState     * __restrict dst,
                AkInput         * __restrict inp,
                AkMeshPrimitive * __restrict prim) {
  AkDAEVerticesMapItem *item;

  if (!inp || !prim) { return; }

  item       = ak_heap_calloc(dst->heap, dst->tempmem, sizeof(*item));
  item->inp  = inp;
  item->prim = prim;

  flist_sp_insert(&dst->vertMap, item);
}

AK_INLINE
AkURL*
url_from(xml_t      * __restrict xml,
         const char * __restrict name,
         void       * __restrict memp) {
  AkHeap     *heap;
  AkURL      *url;
  xml_attr_t *att;

  if (!(att = xmla(xml, name)) || ! att->val)
    return NULL;
  
  heap = ak_heap_getheap(memp);
  url  = ak_heap_calloc(heap, memp, sizeof(*url));
  ak_url_init(memp, xmla_strdup(att, heap, memp), url);
  
  return url;
}

AK_EXPORT
AkGeometry*
ak_baseGeometry(AkURL * __restrict baseurl);

#endif /* dae_common_h */
