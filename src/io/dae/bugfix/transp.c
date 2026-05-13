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

#include "transp.h"
#include <string.h>

static
char
dae_ascii_tolower(char c) {
  if (c >= 'A' && c <= 'Z')
    return (char)(c + ('a' - 'A'));

  return c;
}

static
bool
dae_contains_nocase(const char * __restrict str,
                    const char * __restrict needle) {
  const char *s, *n, *p;

  if (!str || !needle || !needle[0])
    return false;

  for (p = str; *p; p++) {
    s = p;
    n = needle;

    while (*s
           && *n
           && dae_ascii_tolower(*s) == dae_ascii_tolower(*n)) {
      s++;
      n++;
    }

    if (!*n)
      return true;
  }

  return false;
}

static
const char*
dae_parse_next_uint(const char * __restrict p,
                    int        * __restrict out) {
  int  value;
  bool found;

  while (*p && (*p < '0' || *p > '9'))
    p++;

  value = 0;
  found = false;
  while (*p >= '0' && *p <= '9') {
    value = value * 10 + (*p - '0');
    p++;
    found = true;
  }

  if (!found)
    return NULL;

  *out = value;
  return p;
}

AK_HIDE
void
dae_bugfix_transp(AkTransparent * __restrict transp) {
  AkContributor *contr;
  const char    *tool;

  if (!(contr = ak_getAssetInfo(transp, offsetof(AkAssetInf, contributor)))
      || !(tool = (const char *)contr->authoringTool))
    return;

  /* fix old SketchUp transparency bug */
  if (dae_contains_nocase(tool, _s_dae_sketchup)) {
    const char *p;
    int         major, minor, patch;

    major = minor = patch = 0;
    if ((p = dae_parse_next_uint(tool, &major))) {
      if ((p = dae_parse_next_uint(p, &minor)))
        dae_parse_next_uint(p, &patch);

      /* don't flip >= 7.1.1 */
      if (major <= 7 && minor < 2 && patch < 1) {
        transp->amount = 1.0f - transp->amount;
      }
    }
  } /* _s_dae_sketchup */
}
