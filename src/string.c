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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../include/ak/assetkit.h"
#include "common.h"

AK_EXPORT
const char*
ak_strltrim_fast(const char * __restrict str) {
  const char *ptr;
  size_t      len, i;
  char        c;

  len = strlen(str);
  ptr = str;

  if (len == 0)
    return ptr;

  for (i = 0; i < len; i++) {
    c = str[i];
    if (AK_ARRAY_SEP_CHECK) {
      ptr++;
      continue;
    } else {
      return ptr;
    }
  }

  return ptr;
}

AK_EXPORT
int
ak_strtok_count(char * __restrict buff,
                char * __restrict sep,
                size_t           *len) {
  int i, count, itemc, buflen, found_sep;

  buflen = (int)strlen(buff);
  if (buflen == 0)
    return 0;

  count = itemc = 0;

  /* because of buff[j + 1] */
  if (buflen == 1)
    return 1;

  found_sep = false;
  for (i = 0; i < buflen; i++) {
    if (strchr(sep, buff[i])){
      if (!found_sep) {
        itemc++;
        found_sep = true;
      }
      continue;
    }

    found_sep = false;
    count++;
  }

  if (len)
    *len = buflen - count;

  /* left trim */
  if (strchr(sep, buff[0]))
    itemc--;

  /* right trim */
  if (strchr(sep, buff[buflen - 1]))
    itemc--;

  return itemc + 1;
}

AK_EXPORT
int
ak_strtok_count_fast(char * __restrict buff,
                     size_t           *len) {
  int i, count, itemc, buflen, found_sep;
  char c;

  buflen = (int)strlen(buff);
  if (buflen == 0)
    return 0;

  count = itemc = 0;

  /* because of buff[j + 1] */
  if (buflen == 1)
    return 1;

  found_sep = false;
  for (i = 0; i < buflen; i++) {
    c = buff[i];
    if (AK_ARRAY_SEP_CHECK) {
      if (!found_sep) {
        itemc++;
        found_sep = true;
      }
      continue;
    }

    found_sep = false;
    count++;
  }

  /* left trim */
  c = buff[0];
  if (AK_ARRAY_SEP_CHECK)
    itemc--;

  /* right trim */
  c = buff[buflen - 1];
  if (AK_ARRAY_SEP_CHECK)
    itemc--;

  if (len)
    *len = buflen - count;
  
  return itemc + 1;
}

AK_EXPORT
unsigned long
ak_strtof(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkFloat * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;
  tok = src;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = strtof(tok, &tok_end);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = strtof(tok, &tok_end);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }

  return n;
}

AK_EXPORT
unsigned long
ak_strtof_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkFloat * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;
  tok = src;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
      
      *(dest - --n) = strtof(tok, &tok_end);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
      
      *(dest - --n) = strtof(tok, &tok_end);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }

  return n;
}

AK_EXPORT
unsigned long
ak_strtod(char     * __restrict src,
          size_t                srclen,
          unsigned long         n,
          AkDouble * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;
  tok = src;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = strtod(tok, &tok_end);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = strtod(tok, &tok_end);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }

  return n;
}

AK_EXPORT
unsigned long
ak_strtoui(char    * __restrict src,
           size_t               srclen,
           unsigned long        n,
           AkUInt  * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;

  if (n == 0)
    return 0;

  dest = dest + n - 1ul;
  tok = src;

  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtoul(tok, &tok_end, 10);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtoul(tok, &tok_end, 10);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }

  return n;
}

AK_EXPORT
unsigned long
ak_strtoi(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkInt   * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;
  
  if (n == 0)
    return 0;
  
  dest = dest + n - 1ul;
  tok = src;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtol(tok, &tok_end, 10);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtol(tok, &tok_end, 10);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }
  
  return n;
}

AK_EXPORT
unsigned long
ak_strtoi_line(char    * __restrict src,
               size_t               srclen,
               unsigned long        n,
               AkInt   * __restrict dest) {
  char *tok, *tok_end, *end;
  char  c;
  
  if (n == 0)
    return 0;
  
  dest = dest + n - 1ul;
  tok = src;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtol(tok, &tok_end, 10);
      tok = tok_end;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
      
      *(dest - --n) = (AkUInt)strtol(tok, &tok_end, 10);
      tok = tok_end;
      
      while (((void)(c = *tok), AK_ARRAY_SEPLINE_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }
  
  return n;
}

AK_EXPORT
unsigned long
ak_strtob(char    * __restrict src,
          size_t               srclen,
          unsigned long        n,
          AkBool  * __restrict dest) {
  char *tok, *end;
  char  c;

  if (n == 0)
    return 0;
  
  dest = dest + n - 1ul;
  tok = src;
  
  if (srclen != 0) {
    end = src + srclen;
    
    do {
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = tok[0] == 't' || tok[0] == 'T';
      tok++;
      
      while (tok < end && ((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && tok < end);
  } else {
    do {
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
      
      *(dest - --n) = tok[0] == 't' || tok[0] == 'T';
      tok++;
      
      while (((void)(c = *tok), AK_ARRAY_SEP_CHECK))
        tok++;
    } while (n > 0ul && *tok != '\0');
  }
  
  return n;
}

AK_EXPORT
char*
ak_tolower(char *str) {
  char *p;
  for (p = str; *p; ++p) *p = tolower(*p);
  return str;
}

AK_EXPORT
char*
ak_toupper(char *str) {
  char *p;
  for (p = str; *p; ++p) *p = toupper(*p);
  return str;
}
