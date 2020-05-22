/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "wobj.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"

/*
 https://all3dp.com/1/obj-file-format-3d-printing-cad/
 */

AkMesh*
ak_allocMesh(AkHeap * __restrict heap, void * __restrict memp) {
  AkGeometry *geom;
  AkObject   *meshObj;
  AkMesh     *mesh;
  
  /* create geometries */
  geom              = ak_heap_calloc(heap, memp, sizeof(*geom));
  geom->materialMap = ak_map_new(ak_cmp_str);
  
  /* destroy heap with this object */
  ak_setAttachedHeap(geom, geom->materialMap->heap);
  
  meshObj = ak_objAlloc(heap, geom, sizeof(AkMesh), AK_GEOMETRY_MESH, true);
  geom->gdata          = meshObj;
  mesh                 = ak_objGet(meshObj);
  mesh->geom           = geom;
  
  return mesh;
}

AkResult _assetkit_hide
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkAssetInf    *inf;
  void          *objstr;
  char          *p;
  AkLibrary     *lib_geom, *lib_vscene;
  AkMesh        *mesh;
  AkDataContext *dctx_pos, *dctx_ind;
  AkVisualScene *vscene;
  AkNode        *node;
  float          v[4];
  int32_t        vi[4];
  AkWOBJState    ostVal, *ost;
  size_t         objstrSize;
  AkResult       ret;
  AkUInt         idx;
  char           c;

  if ((ret = ak_readfile(filepath, "rb", &objstr, &objstrSize)) != AK_OK)
    return ret;

  c    = '\0';
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = filepath;
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_WAVEFRONT;
  doc->coordSys       = AK_YUP; /* Default */

  if (!((p = objstr) && !(c = '\0'))) {
    ak_free(doc);
    return AK_ERR;
  }

  /* create libraries */
  lib_geom   = ak_heap_calloc(heap, doc, sizeof(*lib_geom));
  lib_vscene = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));
  
  /* create defaul mesh */
  mesh = ak_allocMesh(heap, lib_geom);

  dctx_pos = ak_data_new(doc, 128, sizeof(vec4), ak_cmp_vec4);
  dctx_ind = ak_data_new(doc, 128, sizeof(vec4), ak_cmp_vec4);

  do {
    /* skip spaces */
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p;
    
    if (p[1] == ' ') {
      switch (c) {
        case '#': {
          /* ignore comments */
          while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
          while ((c = *++p) != '\0' &&  AK_ARRAY_NLINE_CHECK);
          break;
        }
        case 'v': {
          if (*++p == '\0')
            goto err;
          ak_strtof_fast_line(p, 0, 4, v);
          idx = ak_data_append(dctx_pos, v);
          break;
        }
        case 'f': {
          if (*++p == '\0')
            goto err;
          ak_strtoi_fast_line(p, 0, 4, vi);
          idx = ak_data_append(dctx_ind, vi);
        }
        case 'o': {
          
        }
        case 'g': {
          
        }
        default:
          break;
      }
    }
  } while (p && p[0] != '\0' && (c = *++p) != '\0');

  /* Buffer -> Accessor -> Input -> Primitive -> Node */
  *dest = doc;

  return AK_OK;
  
err:
  ak_free(doc);
  return AK_ERR;
}
