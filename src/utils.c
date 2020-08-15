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

#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>
#include <errno.h>
#include "../include/ak/options.h"

#include "mem/common.h"

char *
strptime(const char * __restrict buf,
         const char * __restrict fmt,
         struct tm * __restrict tm);

AkResult
ak_readfile(const char * __restrict file,
            void       * __restrict parent,
            void      ** __restrict dest,
            size_t     * __restrict size) {
  FILE      * infile;
  size_t      blksize;
  size_t      fsize;
  size_t      fcontents_size;
  size_t      total_read;
  size_t      nread;
  struct stat infile_st;
  int         infile_no;
  
  infile = fopen(file, "rb");
  if (!infile) {
    fprintf(stderr, "errno: %d: %s", errno, strerror(errno));
    goto err;
  }

  infile_no = fileno(infile);

  if (fstat(infile_no, &infile_st) != 0)
    goto err;

  fsize = infile_st.st_size;

  if (ak_opt_get(AK_OPT_USE_MMAP)
      && (*dest = ak_mmap_rdonly(infile_no, fsize))) {
    *size = fsize;
    if (parent)
      ak_mmap_attach(parent, *dest, fsize);
    return AK_OK;
  }

#ifndef _MSC_VER
  blksize = infile_st.st_blksize;
#else
  blksize = 512;
#endif

  fcontents_size = sizeof(char) * fsize;
  *size          = fcontents_size;

  *dest = malloc(fcontents_size + 1);
  assert(*dest && "malloc failed");

  memset(*(char **)dest + fcontents_size, '\0', 1);

  total_read = 0;

  do {
    if ((fcontents_size - total_read) < blksize)
      blksize = fcontents_size - total_read;

    nread = fread(*(char **)dest + total_read,
                  sizeof(char),
                  blksize,
                  infile);

    total_read += nread;
  } while (nread > 0 && total_read < fsize);

  fclose(infile);

  return AK_OK;
err:
  *dest = NULL;
  *size = 0;
  return AK_ERR;
}

time_t
ak_parse_date(const char * __restrict input,
              const char ** __restrict ret) {
  struct tm    _tm;
  const char * cp;

  memset(&_tm, '\0', sizeof(_tm));

  cp = strptime(input,
                "%Y-%m-%dT%T%Z",
                &_tm);

  if (ret)
    *ret = cp;

  return mktime(&_tm);
}

AK_EXPORT
int
ak_digitsize(size_t number) {
  if (number == 0)
    return 1;
  
  return (int)floor(log10((double)number)) + 1;
}

AK_HIDE
void
ak_releasefile(void *file, size_t size) {
  if (ak_opt_get(AK_OPT_USE_MMAP)) {
    ak_unmap(file, size);
    return;
  }

  free(file);
}
