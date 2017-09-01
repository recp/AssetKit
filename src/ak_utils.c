/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <math.h>

char *
strptime(const char * __restrict buf,
         const char * __restrict fmt,
         struct tm * __restrict tm);

AkResult
ak_readfile(const char * __restrict file,
            const char * __restrict modes,
            char ** __restrict dest) {
  FILE      * infile;
  size_t      blksize;
  size_t      fsize;
  size_t      fcontents_size;
  size_t      total_read;
  size_t      nread;
  struct stat infile_st;
  int         infile_no;
  
  infile    = fopen(file, modes);
  if (!infile)
    goto err;

  infile_no = fileno(infile);

  if (fstat(infile_no, &infile_st) != 0)
    goto err;

#ifndef _MSC_VER
  blksize = infile_st.st_blksize;
#else
  blksize = 512;
#endif

  fsize          = infile_st.st_size;
  fcontents_size = sizeof(char) * fsize;

  *dest = malloc(fcontents_size + 1);
  assert(*dest && "malloc failed");

  memset(*dest + fcontents_size, '\0', 1);

  total_read = 0;

  do {
    if ((fcontents_size - total_read) < blksize)
      blksize = fcontents_size - total_read;

    nread = fread(*dest + total_read,
                  sizeof(**dest),
                  blksize,
                  infile);

    total_read += nread;
  } while (nread > 0 && total_read < fsize);

  fclose(infile);

  return AK_OK;
err:
  *dest = NULL;
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
  return (int)floor(log10(number)) + 1;
}
