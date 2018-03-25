/*
 * Copyright (c), Recep Aslantas.
 *
 * MIT License (MIT), http://opensource.org/licenses/MIT
 * Full license can be found in the LICENSE file
 */

#include "ak_curl.h"
#include "../../include/ak/path.h"

#include <string.h>
#include <stdio.h>
#include <curl/curl.h>

size_t
ak_curl_write(void  *ptr,
              size_t size,
              size_t nmemb,
              FILE  *stream);

size_t
ak_curl_write(void  *ptr,
              size_t size,
              size_t nmemb,
              FILE  *stream) {
  return fwrite(ptr, size, nmemb, stream);
}

char *
ak_curl_dwn(const char *url) {
  char    *localurl;
  CURL    *curl;
  FILE    *file;

  localurl = NULL;
  curl     = curl_easy_init();
  if (curl) {
    localurl = ak_path_tmpfilepath();
    file = fopen(localurl, "wb");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ak_curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    (void)curl_easy_perform(curl);

    curl_easy_cleanup(curl);
    fclose(file);
  }

  return localurl;
}
