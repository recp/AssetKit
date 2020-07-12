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

/*
 Resources:
   https://all3dp.com/1/obj-file-format-3d-printing-cad/
   http://paulbourke.net/dataformats/obj/
   http://paulbourke.net/dataformats/mtl/
*/

#include "obj.h"
#include "common.h"
#include "group.h"
#include "mtl.h"
#include "postscript.h"
#include "../../id.h"
#include "../../data.h"
#include "../../../include/ak/path.h"

static
void
ak_wobjFreeDupl(RBTree *tree, RBNode *node);

AkResult _assetkit_hide
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap             *heap;
  AkDoc              *doc;
  void               *objstr;
  char               *p, *begin, *end, *m;
  AkLibrary          *lib_geom, *lib_vscene;
  AkVisualScene      *scene;
  WOState             wstVal = {0}, *wst;
  float               v[4];
  size_t              objstrSize;
  AkResult            ret;
  size_t              faceSize;
  char                c;

  if ((ret = ak_readfile(filepath, "rb", &objstr, &objstrSize)) != AK_OK)
    return ret;

  c    = '\0';
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf                = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name          = filepath;
  doc->inf->dir           = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage     = true;
  doc->inf->ftype         = AK_FILE_TYPE_WAVEFRONT;
  doc->inf->base.coordSys = AK_YUP;
  doc->coordSys           = AK_YUP; /* Default */

  if (!((p = objstr) && (c = *p) != '\0')) {
    ak_free(doc);
    return AK_ERR;
  }
  
  /* for fixing skin and morph vertices */
  doc->reserved = rb_newtree_ptr();
  ((RBTree *)doc->reserved)->onFreeNode = ak_wobjFreeDupl;
  
  ak_heap_setdata(heap, doc);
  ak_id_newheap(heap);

  /* libraries */
  doc->lib.geometries = ak_heap_calloc(heap, doc, sizeof(*lib_geom));
  lib_vscene = ak_heap_calloc(heap, doc, sizeof(*lib_vscene));
  
  /* default scene */
  scene                  = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->node            = ak_heap_calloc(heap, doc, sizeof(*scene->node));
  lib_vscene->chld       = &scene->base;
  lib_vscene->count      = 1;
  doc->lib.visualScenes  = lib_vscene;
  doc->scene.visualScene = ak_instanceMake(heap, doc, scene);

  /* parse state */
  memset(&wstVal, 0, sizeof(wstVal));
  wst              = &wstVal;
  wstVal.doc       = doc;
  wstVal.heap      = heap;
  wstVal.tmpParent = ak_heap_alloc(heap, doc, sizeof(void*));
  wstVal.node      = scene->node;
  wstVal.lib_geom  = doc->lib.geometries;

  /* default group */
  wobj_switchGroup(wst);
  wst->obj.isdefault = true;

  /* parse .obj */
  do {
    /* skip spaces */
    SKIP_SPACES

    if (p[1] == ' ' || p[1] == '\t') {
      switch (c) {
        case '#': {
          /* ignore comments */
          while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
          /* while ((c = *++p) != '\0' &&  AK_ARRAY_NLINE_CHECK); */
          break;
        }
        case 'v': {
          if (*++p == '\0')
            goto err;

          /* TODO: handle 4 components */
          ak_strtof_line(p, 0, 3, v);
          ak_data_append(wst->obj.dc_pos, v);
          break;
        }
        case 'f': {
          if ((c = *(p += 2)) == '\0')
            goto err;

          faceSize = 0;

          do {
            int32_t ind;

            /* vertex index */
            SKIP_SPACES
            ind = (int32_t)strtol(p, &p, 10);
            ak_data_append(wst->obj.dc_indv, &ind);

            /* texture index */
            SKIP_SPACES
            if (p && p[0] == '/') {
              ind = (int32_t)strtol(++p, &p, 10);
              ak_data_append(wst->obj.dc_indt, &ind);
            }
            
            /* normal index */
            SKIP_SPACES
            if (p && p[0] == '/') {
              ind = (int32_t)strtol(++p, &p, 10);
              ak_data_append(wst->obj.dc_indn, &ind);
            }

            faceSize += 1;
          } while (p
                   && (c = p[0]) != '\0'
                   && !AK_ARRAY_NLINE_CHECK
                   && (c = *++p) != '\0'
                   && !AK_ARRAY_NLINE_CHECK);
          
          ak_data_append(wst->obj.dc_vcount, &faceSize);
          break;
        }
        case 'o': {
          wobj_switchObject(wst);
          break;
        }
        case 'g': {
          wobj_switchGroup(wst);
          break;
        }
        default:
          break;
      }
    } else if (p[2] == ' ' || p[2] == '\t') {
      if (p[0] == 'v' && p[1] == 'n') {
        if (*(p += 2) == '\0')
          goto err;

        ak_strtof_line(p, 0, 3, v);
        ak_data_append(wst->obj.dc_nor, v);
      } else if (p[0] == 'v' && p[1] == 't') {
        if (*(p += 2) == '\0')
          goto err;

        ak_strtof_line(p, 0, 2, v);
        ak_data_append(wst->obj.dc_tex, v);
      }
    } else if (p[0] == 'm'
               && p[1] == 't'
               && p[2] == 'l'
               && p[3] == 'l'
               && p[4] == 'i'
               && p[5] == 'b'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 6;
      SKIP_SPACES

      begin = ++p;
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
      end = p;

      if (end > begin
          && (m = ak_heap_strndup(heap, wst->doc, begin, end - begin)))
        wst->mtlib = wobj_mtl(wst, m);
    } else if (p[0] == 'u'
               && p[1] == 's'
               && p[2] == 'e'
               && p[3] == 'm'
               && p[4] == 't'
               && p[5] == 'l'
               && (p[6] == ' ' || p[6] == '\t')) {
      p += 6;
      SKIP_SPACES

      begin = ++p;
      while ((c = *++p) != '\0' && !AK_ARRAY_NLINE_CHECK);
      end = p;

      if (end > begin)
        wst->obj.mtlname = ak_heap_strndup(heap, wst->doc, begin, end - begin);
    }
    
    NEXT_LINE
  } while (p && p[0] != '\0'/* && (c = *++p) != '\0'*/);

  wobj_finishObject(wst);
  wobj_finishGroup(wst);
  
  wobj_postscript(wst);

  *dest = doc;

  return AK_OK;

err:
  ak_free(doc);
  return AK_ERR;
}

static
void
ak_wobjFreeDupl(RBTree *tree, RBNode *node) {
  if (node == tree->nullNode)
    return;
  ak_free(node->val);
}
