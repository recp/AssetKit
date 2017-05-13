/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#ifndef ak_instance_h
#define ak_instance_h
#ifdef __cplusplus
extern "C" {
#endif

typedef struct AkInstanceListItem {
  struct AkInstanceListItem *prev;
  struct AkInstanceListItem *next;
  AkInstanceBase *instance;
  size_t          index;
} AkInstanceListItem;

typedef struct AkInstanceList {
  AkInstanceListItem *first;
  AkInstanceListItem *last;
  size_t              count;
  size_t              lastindex;
} AkInstanceList;

AK_EXPORT
AkInstanceBase*
ak_instanceMake(AkHeap * __restrict heap,
                void   * __restrict memparent,
                void   * __restrict object);

AK_EXPORT
void
ak_instanceListAdd(AkInstanceList *list,
                   AkInstanceBase *inst);

AK_EXPORT
void
ak_instanceListDel(AkInstanceList *list,
                   AkInstanceListItem *item);

AK_EXPORT
void
ak_instanceListEmpty(AkInstanceList *list);

char*
ak_instanceName(AkInstanceListItem *item);

AK_EXPORT
void *
ak_instanceObject(AkInstanceBase *instance);

AK_EXPORT
AkNode *
ak_instanceObjectNode(AkNode * node);

AK_EXPORT
AkGeometry *
ak_instanceObjectGeom(AkNode * node);

AK_EXPORT
AkGeometry *
ak_instanceObjectGeomId(AkDoc * __restrict doc,
                        const char * id);

#ifdef __cplusplus
}
#endif
#endif /* ak_instance_h */
