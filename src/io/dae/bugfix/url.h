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

#ifndef dae_bugfix_url_h
#define dae_bugfix_url_h

#include <stdbool.h>
#include <string.h>
#define dae_alloca AK_ALLOCA

static inline
bool
dae_url_has_scheme_sep(const char * __restrict str) {
  const char *p;

  if (!str)
    return false;

  for (p = str; p[0] && p[1] && p[2]; p++) {
    if (p[0] == ':' && p[1] == '/' && p[2] == '/')
      return true;
  }

  return false;
}

/*
 * Industry bug: many DAE exporters (Blender, Maya, some glTF→DAE
 * converters) emit `<... source="some_id"/>` instead of the spec-correct
 * `<... source="#some_id"/>` for internal references. Without the
 * fragment marker AssetKit's url machinery routes these strings through
 * the external-resource fetch path, which never resolves and (worse)
 * NULL-derefs in ak_path_fragment.
 *
 * Policy: AssetKit is lenient toward asset-side mistakes that have an
 * unambiguous fix. When the URL has no '#' AND no path/URL separator
 * (`/`, `\`, scheme `://`), treat it as a bare internal id and prepend
 * the missing '#'.
 *
 * Implementation: macro instead of an inline function so alloca() lives
 * in the caller's stack frame — the normalized buffer must outlive
 * ak_url_init, which strdups the input internally (its '#' branch). No
 * extra heap allocation for the common malformed case; well-formed
 * input is a no-op.
 */
#define DAE_URL_INIT_FIXED(memp_, urlstring_, url_)                      \
  do {                                                                   \
    char *_dbu_str = (urlstring_);                                       \
    if (_dbu_str && _dbu_str[0] != '#'                                   \
        && !strchr(_dbu_str, '/')                                        \
        && !strchr(_dbu_str, '\\')                                       \
        && !dae_url_has_scheme_sep(_dbu_str)) {                          \
      size_t  _dbu_len = strlen(_dbu_str);                               \
      char   *_dbu_norm = dae_alloca(_dbu_len + 2);                      \
      _dbu_norm[0] = '#';                                                \
      memcpy(_dbu_norm + 1, _dbu_str, _dbu_len + 1);                     \
      ak_free(_dbu_str);                                                 \
      _dbu_str = _dbu_norm;                                              \
    }                                                                    \
    ak_url_init((memp_), _dbu_str, (url_));                              \
  } while (0)

#endif /* dae_bugfix_url_h */
