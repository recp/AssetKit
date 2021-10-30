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

#include "common.h"

#ifndef AK_WINAPI
#  include <sys/mman.h>
#else
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <io.h>
#endif

AK_EXPORT
void*
ak_mmap_rdonly(int fd, size_t size) {
  void *mapped;
  
  mapped = NULL;

#ifndef AK_WINAPI
  mapped = mmap(0, size, PROT_READ, MAP_SHARED, fd, 0);
  if (!mapped || mapped == MAP_FAILED)
    return NULL;

  madvise(mapped, size, MADV_SEQUENTIAL);
#else
  HANDLE hmap;
  if (!((hmap = CreateFileMapping((HANDLE)_get_osfhandle(fd), 0, PAGE_READONLY, 0, 0, 0))
       && (mapped = MapViewOfFileEx(hmap, FILE_MAP_READ, 0, 0, size, 0))))
    return NULL;

  CloseHandle(hmap);
#endif
  
  return mapped;
}

AK_EXPORT
void
ak_unmap(void *file, size_t size) {
#ifndef AK_WINAPI
  munmap(file, size);
#else
  UnmapViewOfFile(file);
#endif
}

AK_EXPORT
void
ak_mmap_attach(void * __restrict obj, void * __restrict mapped, size_t sized) {
  AkHeap          *heap;
  AkHeapNode      *hnode;
  AkMemoryMapNode **mmapNode, *mmapNodeNew;

  heap     = ak_heap_getheap(obj);
  hnode    = ak__alignof(obj);
  mmapNode = (AkMemoryMapNode **)ak_heap_ext_add(heap, hnode, AK_HEAP_NODE_FLAGS_MMAP);

  if (mmapNode) {
    mmapNodeNew         = ak_heap_calloc(heap, obj, sizeof(*mmapNodeNew));
    mmapNodeNew->mapped = mapped;
    mmapNodeNew->sized  = sized;
    
    if (*mmapNode) {
      mmapNodeNew->next = *mmapNode;
      (*mmapNode)->prev = mmapNodeNew;
    }
    *mmapNode = mmapNodeNew;
  }
}

AK_EXPORT
void
ak_unmmap_attached(void * __restrict obj) {
  AkHeapNode      *hnode;
  AkMemoryMapNode **mmapNode, *it;
  void            *tofree;

  hnode = ak__alignof(obj);
  if ((mmapNode = ak_heap_ext_get(hnode, AK_HEAP_NODE_FLAGS_MMAP))
      && (it = *mmapNode)) {
    while (it) {
      ak_unmap(it->mapped, it->sized);
      tofree = it;
      it = it->next;
      ak_free(tofree);
    }
    
    *mmapNode = NULL;
  }
}
