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

#include "wobj.h"
#include "common.h"
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
  
  meshObj     = ak_objAlloc(heap, geom, sizeof(AkMesh), AK_GEOMETRY_MESH, true);
  geom->gdata = meshObj;
  mesh        = ak_objGet(meshObj);
  mesh->geom  = geom;
  
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
  AkDataContext *dc_pos, *dc_tex, *dc_nor;
  AkDataContext *dc_indv, *dc_indt, *dc_indn;
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

  /* create default mesh */
  mesh = ak_allocMesh(heap, lib_geom);

  /* vertex data  */
  dc_pos  = ak_data_new(doc, 128, sizeof(vec4),    ak_cmp_vec4);
  dc_tex  = ak_data_new(doc, 128, sizeof(vec3),    ak_cmp_vec3);
  dc_nor  = ak_data_new(doc, 128, sizeof(vec3),    ak_cmp_vec3);

  /* vertex indices */
  dc_indv = ak_data_new(doc, 128, sizeof(vec3), ak_cmp_vec3);

#define skip_spaces \
  do { \
    while (c != '\0' && AK_ARRAY_SPACE_CHECK) c = *++p; \
    if (c == '\0') \
      continue; /* to break loop */ \
  } while (0)

  /* parse .obj */
  do {
    /* skip spaces */
    skip_spaces;

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
          ak_strtof_line(p, 0, 4, v);
          ak_data_append(dc_pos, v);
          break;
        }
        case 'f': {
          if ((c = *(p += 2)) == '\0')
            goto err;

          do {
            /* found single or indices group */
            skip_spaces;
            
            
          } while (p
                   && p[0] != '\0'
                   && (c = *++p) != '\0'
                   && !AK_ARRAY_NLINE_CHECK);
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
