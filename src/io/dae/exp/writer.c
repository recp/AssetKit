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

#include "writer.h"
#include "../../common/text_number.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

AK_HIDE
void
dae_w_flush(DAEExpWriter * __restrict w) {
  if (w->len == 0)
    return;

  if (w->result == AK_OK
      && fwrite(w->buffer, 1, w->len, w->file) != w->len)
    w->result = AK_ERR;

  w->len = 0;
}

AK_HIDE
void
dae_w_raw(DAEExpWriter * __restrict w,
          const void   * __restrict data,
          size_t                    len) {
  const unsigned char *src;

  src = data;
  while (len > 0) {
    size_t avail;
    size_t n;

    avail = DAE_EXP_WRITER_CAP - w->len;
    if (avail == 0) {
      dae_w_flush(w);
      avail = DAE_EXP_WRITER_CAP;
    }

    if (w->len == 0 && len >= DAE_EXP_WRITER_CAP) {
      if (w->result == AK_OK && fwrite(src, 1, len, w->file) != len)
        w->result = AK_ERR;
      return;
    }

    n = len < avail ? len : avail;
    memcpy(w->buffer + w->len, src, n);
    w->len += n;
    src    += n;
    len    -= n;
  }
}

AK_HIDE
void
dae_w_name(DAEExpWriter * __restrict w, DAEExpName name) {
  dae_w_raw(w, name.ptr, name.len);
}

AK_HIDE
void
dae_w_cstr(DAEExpWriter * __restrict w, const char * __restrict str) {
  dae_w_raw(w, str, strlen(str));
}

AK_HIDE
void
dae_w_uint(DAEExpWriter * __restrict w, size_t val) {
  char   buf[32];
  size_t i;

  i = sizeof(buf);
  do {
    buf[--i] = (char)('0' + (val % 10u));
    val /= 10u;
  } while (val > 0 && i > 0);

  dae_w_raw(w, buf + i, sizeof(buf) - i);
}

AK_HIDE
void
dae_w_float(DAEExpWriter * __restrict w, float val) {
  char   buf[48];
  size_t outLen;

  if (!ak_io_text_format_float9(buf, sizeof(buf), val, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  dae_w_raw(w, buf, outLen);
}

AK_HIDE
void
dae_w_double(DAEExpWriter * __restrict w, double val) {
  char   buf[64];
  size_t outLen;

  if (!ak_io_text_format_double15(buf, sizeof(buf), val, &outLen)) {
    w->result = AK_ERR;
    return;
  }

  dae_w_raw(w, buf, outLen);
}

AK_HIDE
void
dae_w_xml(DAEExpWriter * __restrict w,
          const char   * __restrict str,
          bool                      attr) {
  size_t i;

  if (!str)
    return;

  i = 0;
  while (str[i]) {
    size_t spanStart;

    spanStart = i;
    while (str[i]
           && str[i] != '&'
           && str[i] != '<'
           && str[i] != '>'
           && (!attr || (str[i] != '"' && str[i] != '\''))) {
      i++;
    }

    if (i > spanStart)
      dae_w_raw(w, str + spanStart, i - spanStart);

    switch (str[i]) {
      case '\0':
        break;
      case '&':
        dae_w_lit(w, "&amp;");
        i++;
        break;
      case '<':
        dae_w_lit(w, "&lt;");
        i++;
        break;
      case '>':
        dae_w_lit(w, "&gt;");
        i++;
        break;
      case '"':
        dae_w_lit(w, "&quot;");
        i++;
        break;
      case '\'':
        dae_w_lit(w, "&apos;");
        i++;
        break;
      default:
        break;
    }
  }
}

static
void
dae_write_extra_node(DAEExpWriter * __restrict w,
                     AkTreeNode   * __restrict node,
                     uint32_t                  depth) {
  AkTreeNodeAttr *attr;
  AkTreeNode     *child;

  if (!node)
    return;

  if (depth > DAE_EXP_MAX_NODE_DEPTH) {
    w->result = AK_ERR;
    return;
  }

  if (!node->name) {
    for (child = node->chld; child; child = child->next)
      dae_write_extra_node(w, child, depth + 1u);
    return;
  }

  dae_w_ch(w, '<');
  dae_w_cstr(w, node->name);
  for (attr = node->attribs; attr; attr = attr->next) {
    if (!attr->name)
      continue;

    dae_w_ch(w, ' ');
    dae_w_cstr(w, attr->name);
    dae_w_lit(w, "=\"");
    dae_w_xml(w, attr->val ? attr->val : "", true);
    dae_w_ch(w, '"');
  }

  if (!node->val && !node->chld) {
    dae_w_lit(w, "/>");
    return;
  }

  dae_w_ch(w, '>');
  if (node->val)
    dae_w_xml(w, node->val, false);
  for (child = node->chld; child; child = child->next)
    dae_write_extra_node(w, child, depth + 1u);
  dae_w_lit(w, "</");
  dae_w_cstr(w, node->name);
  dae_w_ch(w, '>');
}

AK_HIDE
void
dae_write_extra(DAEExpWriter * __restrict w, AkTreeNode * __restrict extra) {
  AkTreeNode *child;

  if (!extra)
    return;

  if (extra->name && strcmp(extra->name, _s_dae_extra) == 0) {
    dae_write_extra_node(w, extra, 0);
    return;
  }

  dae_w_lit(w, "<extra>");
  if (extra->name) {
    dae_write_extra_node(w, extra, 1);
  } else {
    for (child = extra->chld; child; child = child->next)
      dae_write_extra_node(w, child, 1);
  }
  dae_w_lit(w, "</extra>");
}

AK_HIDE
void
dae_w_attr_uint(DAEExpWriter * __restrict w,
                DAEExpName                name,
                size_t                    value) {
  dae_w_ch(w, ' ');
  dae_w_name(w, name);
  dae_w_lit(w, "=\"");
  dae_w_uint(w, value);
  dae_w_ch(w, '"');
}

AK_HIDE
void
dae_w_id(DAEExpWriter * __restrict w,
         DAEExpName                prefix,
         uint32_t                  idx) {
  dae_w_name(w, prefix);
  dae_w_ch(w, '_');
  dae_w_uint(w, idx);
}
