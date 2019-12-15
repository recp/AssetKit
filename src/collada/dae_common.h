/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef __libassetkit__dae_common__h_
#define __libassetkit__dae_common__h_

#include "../../include/ak/assetkit.h"
#include "../../include/ak/url.h"
#include "../common.h"
#include "../memory_common.h"
#include "../utils.h"
#include "../tree.h"
#include "../xml.h"
#include "dae_strpool.h"

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

typedef struct AkURLQueue {
  AkURL *url;
  struct AkURLQueue *next;
} AkURLQueue;

typedef AK_ALIGN(16) struct DAEState {
  AkHeap          *heap;
  void            *jobs14;
  AkDoc           *doc;
  AkURLQueue      *urlQueue;
  FListItem       *accessors;
  FListItem       *instCtlrs;
  FListItem       *inputs;
  FListItem       *toRadiansSampelers;
  RBTree          *meshInfo;
  RBTree          *inputmap;
  RBTree          *texmap;
  RBTree          *instanceMap;
  AkCOLLADAVersion version;
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

#endif /* __libassetkit__dae_common__h_ */
