/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "wobj.h"
#include "../../id.h"
#include "../../../include/ak/path.h"

AkResult _assetkit_hide
wobj_obj(AkDoc     ** __restrict dest,
         const char * __restrict filepath) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkAssetInf *inf;
  char       *objstr;
  AkWOBJState ostVal, *ost;
  AkResult    ret;
  size_t      objstrSize;

  if ((ret = ak_readfile(filepath, "rb", &objstr, &objstrSize)) != AK_OK)
    return ret;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  doc->inf            = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name      = filepath;
  doc->inf->dir       = ak_path_dir(heap, doc, filepath);
  doc->inf->flipImage = true;
  doc->inf->ftype     = AK_FILE_TYPE_WAVEFRONT;
  doc->coordSys       = AK_YUP; /* Default */

  return ret;
}
