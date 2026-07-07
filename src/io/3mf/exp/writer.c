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

AK_HIDE
void
ak_3mf_buf_attr(AK3MFBuffer * __restrict buf,
                const char  * __restrict value) {
  const unsigned char *it;

  if (!value)
    return;

  for (it = (const unsigned char *)value; *it; it++) {
    switch (*it) {
      case '&':
        AK_3MF_BUF_LIT(buf, "&amp;");
        break;
      case '<':
        AK_3MF_BUF_LIT(buf, "&lt;");
        break;
      case '"':
        AK_3MF_BUF_LIT(buf, "&quot;");
        break;
      case '\'':
        AK_3MF_BUF_LIT(buf, "&apos;");
        break;
      default:
        ak_3mf_buf_ch(buf, (char)*it);
        break;
    }
  }
}

AK_HIDE
void
ak_3mf_buf_3mf_path_attr(AK3MFBuffer * __restrict buf,
                         const char  * __restrict path) {
  if (!path)
    return;

  if (*path != '/' && *path != '\\')
    ak_3mf_buf_ch(buf, '/');
  ak_3mf_buf_attr(buf, path);
}

AK_HIDE
void
ak_3mf_buf_u32(AK3MFBuffer * __restrict buf, uint32_t value) {
  char  tmp[24];
  char *end;

  end = ak_io_text_format_uint64(tmp, value);
  ak_3mf_buf_raw(buf, tmp, (size_t)(end - tmp));
}

AK_HIDE
void
ak_3mf_buf_float(AK3MFBuffer * __restrict buf, float value) {
  char   tmp[48];
  size_t outLen;

  if (!ak_io_text_format_float6(tmp, sizeof(tmp), value, &outLen)) {
    buf->result = AK_ERR;
    return;
  }

  ak_3mf_buf_raw(buf, tmp, outLen);
}
AK_HIDE
void
ak_3mf_append_transform(AK3MFBuffer * __restrict buf, mat4 world) {
  const float values[12] = {
    world[0][0], world[1][0], world[2][0],
    world[0][1], world[1][1], world[2][1],
    world[0][2], world[1][2], world[2][2],
    world[3][0], world[3][1], world[3][2]
  };
  uint32_t i;

  for (i = 0; i < 12u; i++) {
    if (i > 0)
      ak_3mf_buf_ch(buf, ' ');
    ak_3mf_buf_float(buf, values[i]);
  }
}

AK_HIDE
void
ak_3mf_append_flat_transform(AK3MFBuffer         * __restrict buf,
                             const float          matrix[16]) {
  const float values[12] = {
    matrix[0], matrix[4], matrix[8],
    matrix[1], matrix[5], matrix[9],
    matrix[2], matrix[6], matrix[10],
    matrix[12], matrix[13], matrix[14]
  };
  uint32_t i;

  for (i = 0; i < 12u; i++) {
    if (i > 0)
      ak_3mf_buf_ch(buf, ' ');
    ak_3mf_buf_float(buf, values[i]);
  }
}

AK_HIDE
void
ak_3mf_append_optional_3mf_path(AK3MFBuffer * __restrict buf,
                                const char  * __restrict path) {
  if (!path)
    return;

  AK_3MF_BUF_LIT(buf, "\" path=\"/");
  ak_3mf_buf_attr(buf, path);
}
