/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "mesh_util.h"
#include "../common.h"
#include "../id.h"
#include "../accessor.h"
#include "../memory_common.h"
#include <assert.h>

int
ak_mesh_src_usg(AkHeap   *heap,
                AkMesh   *mesh,
                AkSource *src) {
  AkMeshPrimitive *primi;
  AkInput         *input;
  AkDoc           *doc;
  AkSource        *src_it;
  int              count;

  doc    = ak_heap_data(heap);
  primi  = mesh->primitive;
  count  = 0;

  /* inputs */
  while (primi) {
    input = primi->input;

    while (input) {
      if (input->source.doc == doc) {
        src_it = ak_getObjectByUrl(&input->source);
        if (src_it && src_it == src)
          count++;
      }

      input = input->next;
    }

    primi = primi->next;
  }

  return count;
}

AkSource *
ak_mesh_src(AkHeap   *heap,
            AkMesh   *mesh,
            AkSource *src,
            int       max) {
  AkSource    *newsrc;
  AkAccessor  *newacc, *oldacc;
  AkDataParam *dp, *last_dp;
  const char  *newid;
  int          usg;

  usg = ak_mesh_src_usg(heap, mesh, src);

  /* source needs be duplicated */
  if (ak_refc(src) <= usg
      && usg <= max
      && usg > 0)
    return src;

  newsrc = ak_heap_calloc(heap,
                          ak_objFrom(mesh),
                          sizeof(*src));

  newid = ak_id_gen(heap,
                    newsrc,
                    ak_getId(newsrc));
  ak_setId(newsrc, newid);

  oldacc = src->tcommon;

  /* duplicate accesor */
  newacc = ak_heap_alloc(heap, newsrc, sizeof(*newacc));
  memcpy(newacc, oldacc, sizeof(*newacc));

  dp      = oldacc->param;
  last_dp = NULL;

  /* duplicate params */
  while (dp) {
    AkDataParam *dpi;

    dpi = ak_heap_calloc(heap, newacc, sizeof(*dpi));
    memcpy(dpi, dpi, sizeof(*dp));

    if (dp->name)
      dpi->name = ak_heap_strdup(heap, dpi, dp->name);

    if (dp->semantic)
      dpi->semantic = ak_heap_strdup(heap, dpi, dp->semantic);

    if (last_dp)
      last_dp->next = dpi;
    else
      newacc->param = dpi;
    last_dp = dpi;

    dp = dp->next;
  }

  newsrc->tcommon = newacc;

  return newsrc;
}

AkSource*
ak_mesh_src_for(AkHeap          *heap,
                AkMesh          *mesh,
                AkMeshPrimitive *prim,
                AkInputSemantic  semantic) {
  AkBuffer    *buff;
  AkSource    *src,   *possrc;
  AkAccessor  *acc,   *posacc;
  char        *srcid, *buffid, *url;
  size_t       c;

  if (!prim->pos
      || !(possrc  = ak_getObjectByUrl(&prim->pos->source))
      || !(posacc  = possrc->tcommon))
    return NULL;

  c = posacc->count;

  /* TODO: find existing src and join data into one */
  src   = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*src));
  srcid = (char *)ak_id_gen(heap, src, NULL);
  ak_setId(src, srcid);

  acc = ak_heap_calloc(heap, src, sizeof(*acc));

  /* set params */
  ak_accessor_setparams(acc, semantic);

  acc->stride     = acc->bound;
  acc->count      = c;
  acc->itemTypeId = AKT_FLOAT;
  acc->type       = ak_typeDesc(acc->itemTypeId);
  src->tcommon    = acc;

  buff = ak_heap_calloc(heap, src, sizeof(*buff));
  buff->data = ak_heap_alloc(heap,
                             buff,
                             c * acc->stride * sizeof(float));


  buff->name   = NULL;
  buff->length = c * acc->stride * sizeof(float);

  buffid = (char *)ak_id_gen(heap, buff->data, NULL);
  ak_setId(buff, buffid);

  /* update accessor source url */
  url = ak_url_string(heap->allocator, buffid);
  ak_url_init(acc, url, &acc->source);
  heap->allocator->free(url);

  src->buffer = buff;

  return src;
}

AkSource*
ak_mesh_src_for_ext(AkHeap          *heap,
                    AkMesh          *mesh,
                    AkMeshPrimitive *prim,
                    char            *srcid,
                    AkInputSemantic  semantic,
                    size_t           count) {
  AkBuffer    *buff;
  AkSource    *src,    *possrc;
  AkAccessor  *acc,    *posacc;
  char        *buffid, *url;

  if (!prim->pos
      || !(possrc  = ak_getObjectByUrl(&prim->pos->source))
      || !(posacc  = possrc->tcommon))
    return NULL;

  if (count == 0)
    count = posacc->count;

  /* TODO: find existing src and join data into one */
  src = ak_heap_calloc(heap, ak_objFrom(mesh), sizeof(*src));
  if (srcid)
    ak_setId(src, srcid);

  acc = ak_heap_calloc(heap, src, sizeof(*acc));

  /* set params */
  ak_accessor_setparams(acc, semantic);

  acc->stride     = acc->bound;
  acc->count      = count;
  acc->itemTypeId = AKT_FLOAT;
  acc->type       = ak_typeDesc(acc->itemTypeId);
  acc->byteStride = acc->stride * acc->type->size;
  acc->byteLength = acc->count * acc->byteStride;
  src->tcommon    = acc;

  buff = ak_heap_calloc(heap, src, sizeof(*buff));
  buff->data = ak_heap_alloc(heap,
                             buff,
                             count * acc->stride * sizeof(float));


  buff->name   = NULL;
  buff->length = count * acc->stride * sizeof(float);



  buffid = (char *)ak_id_gen(heap, buff->data, NULL);
  ak_setId(buff, buffid);

  /* update accessor source url */
  url = ak_url_string(heap->allocator, buffid);
  ak_url_init(acc, url, &acc->source);
  heap->allocator->free(url);

  src->buffer = buff;

  /* retain obj */
  ak_retain(src);

  return src;
}

uint32_t
ak_mesh_arr_stride(AkMesh *mesh, AkURL *arrayURL) {
  AkMeshPrimitive *primi;
  AkInput         *input;
  AkSource        *src;
  AkAccessor      *acc;
  AkMap           *map;
  AkMapItem       *mapi;
  uint32_t         stride;

  primi  = mesh->primitive;
  stride = 0;
  map    = ak_map_new(NULL);

  /* per-primitive inputs */
  while (primi) {
    input = primi->input;

    while (input) {
      src = ak_getObjectByUrl(&input->source);
      acc = src->tcommon;

      if (strcmp(acc->source.url, arrayURL->url) == 0
          && acc->source.doc == arrayURL->doc)
        ak_map_addptr(map, src);

      input = input->next;
    }

    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    src = ak_getId(mapi);
    acc = src->tcommon;
    if (acc)
      stride += acc->bound;
    mapi = mapi->next;
  }

  ak_map_destroy(map);

  return stride;
}

size_t
ak_mesh_intr_count(AkMesh *mesh) {
  AkMeshPrimitive *primi;
  AkInput         *input;
  AkSource        *src;
  size_t           count;
  size_t           icount;

  primi = mesh->primitive;
  count = 0;

  while (primi) {
    input  = primi->input;
    icount = primi->indices->count / primi->indexStride;

    while (input) {
      src = ak_getObjectByUrl(&input->source);
      if (src && src->tcommon)
        count += src->tcommon->bound * icount;

      input = input->next;
    }

    primi = primi->next;
  }

  return count;
}

void
ak_accessor_rebound(AkHeap     *heap,
                    AkAccessor *acc,
                    uint32_t    offset) {
  AkDataParam *dpi, *dpu;
  AkDataParam *bound,  *last_bound;
  AkDataParam *ubound, *last_ubound;
  uint32_t     uboundc, required, i;

  uboundc     = 0;
  ubound      = NULL;
  last_ubound = NULL;
  last_bound  = NULL;
  bound       = NULL;
  dpi         = acc->param;

  /* save unbound params to reduce mem allocs */
  while (dpi) {
    if (!dpi->name) {
      if (last_ubound)
        last_ubound->next = dpi;
      else
        ubound = dpi;
      last_ubound = dpi;

      uboundc++;
    } else {
      if (last_bound)
        last_bound->next = dpi;
      else
        bound = dpi;
      last_bound = dpi;
    }

    dpi = dpi->next;
  }

  assert(last_bound && offset <= (acc->stride - acc->bound));

  required = uboundc + acc->bound;

  /* add new unbound params */
  if (required < acc->stride) {
    uint32_t missing;

    missing = acc->stride - offset;
    for (i = 0; i < missing; i++) {
      dpu       = ak_heap_calloc(heap, acc, sizeof(*dpu));
      dpu->next = ubound;
      ubound    = dpu;

      if (!last_ubound)
        last_ubound = dpu;
    }
  }

  /* free some unbound params */
  else if (uboundc > required) {
    while (uboundc > required && ubound) {
      AkDataParam *tofree;
      tofree = ubound;
      ubound = ubound->next;
      ak_free(tofree);
    }
  }

  last_bound->next = NULL;
  if (offset == 0) {
    acc->param = bound;
    return;
  }

  if (last_ubound)
    last_ubound->next = NULL;

  /* offset > 0 */
  i          = 0;
  dpu        = ubound;
  acc->param = ubound;

  while (dpu && i++ < offset) {
    dpu = dpu->next;
  }

  assert(dpu);

  last_bound->next = dpu->next;
  dpu->next        = bound;
}

void
ak_meshInspectArray(AkMesh   * __restrict mesh,
                    AkURL    * __restrict arrayURL,
                    uint32_t * __restrict stride,
                    size_t   * __restrict count) {
  AkMeshPrimitive *primi;
  AkInput         *input;
  AkSource        *src;
  AkAccessor      *acc;
  AkMap           *map;
  AkMapItem       *mapi;

  *stride = 0;
  *count  = 0ul;
  primi   = mesh->primitive;
  map     = ak_map_new(NULL);

  /* inputs */
  while (primi) {
    input = primi->input;

    while (input) {
      src = ak_getObjectByUrl(&input->source);
      acc = src->tcommon;

      if (strcmp(acc->source.url, arrayURL->url) == 0
          && acc->source.doc == arrayURL->doc)
        ak_map_addptr(map, src);

      input = input->next;
    }

    primi = primi->next;
  }

  mapi = map->root;
  while (mapi) {
    src = ak_getId(mapi);
    acc = src->tcommon;
    if (acc) {
      *stride += acc->bound;
      *count  += acc->bound * acc->count;
    }
    mapi = mapi->next;
  }

  ak_map_destroy(map);
}

AK_EXPORT
AkObject*
ak_meshArrayOf(AkMesh  * __restrict mesh,
               AkInput * __restrict input) {
  AkSource   *src;
  AkAccessor *acc;

  AK__UNUSED(mesh);

  if (!(src = ak_getObjectByUrl(&input->source)))
    return NULL;

  if (!(acc = src->tcommon))
    return NULL;

  return ak_getObjectByUrl(&acc->source);
}
