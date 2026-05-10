/*
 * Base64 encoding/decoding (RFC1341)
 * Copyright (c) 2005-2011, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 *
 * Modified by Recep Aslantas (github @recp)
 */

#include "base64.h"
#include "simd/base64.h"
#include <string.h>

static const unsigned char base64_table[65] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const unsigned char base64_dtable[256] = {
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x3e, 0x80, 0x80, 0x80, 0x3f,
	0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x80, 0x80, 0x80, 0x00, 0x80, 0x80,
	0x80, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
	0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
};

/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
AK_HIDE
unsigned char*
base64_encode(AkHeap              * __restrict heap,
              void                * __restrict memparent,
              const unsigned char * __restrict src,
              size_t                           len,
              size_t              * __restrict out_len) {
	unsigned char       *out, *pos;
	const unsigned char *end, *in;
	size_t               olen;
	int                  line_len;

	olen = len * 4 / 3 + 4; /* 3-byte blocks to 4-byte */
	olen += olen / 72; /* line feeds */
	olen++; /* nul termination */
  
	if (olen < len)
		return NULL; /* integer overflow */
	out = ak_heap_alloc(heap, memparent, olen);
	if (out == NULL)
		return NULL;

	end = src + len;
	in = src;
	pos = out;
	line_len = 0;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
		line_len += 4;
		if (line_len >= 72) {
			*pos++ = '\n';
			line_len = 0;
		}
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
		line_len += 4;
	}

	if (line_len)
		*pos++ = '\n';

	*pos = '\0';
	if (out_len)
		*out_len = pos - out;
	return out;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
AK_HIDE
unsigned char*
base64_decode_slow(AkHeap              * __restrict heap,
                   void                * __restrict memparent,
                   const unsigned char * __restrict src,
                   size_t                           len,
                   size_t              * __restrict out_len) {
	unsigned char dtable[256], *out, *pos, block[4], tmp;
	size_t        i, count, olen;
	int           pad = 0;

	memset(dtable, 0x80, 256);

	for (i = 0; i < sizeof(base64_table) - 1; i++)
		dtable[base64_table[i]] = (unsigned char) i;

	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80)
			count++;
	}

	if (count == 0 || count % 4)
		return NULL;

	olen = count / 4 * 3;
	pos  = out = ak_heap_alloc(heap, memparent, olen);
	if (out == NULL)
		return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80)
			continue;

		if (src[i] == '=')
			pad++;
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
			if (pad) {
				if (pad == 1)
					pos--;
				else if (pad == 2)
					pos -= 2;
				else {
					/* Invalid padding */
					ak_free(out);
					return NULL;
				}
				break;
			}
		}
	}

	*out_len = pos - out;
	return out;
}

/**
 * Fast path for compact data URIs. glTF embedded buffers/images are normally
 * emitted without whitespace, so avoid the validation/counting pass.
 */
AK_HIDE
unsigned char*
base64_decode(AkHeap              * __restrict heap,
              void                * __restrict memparent,
              const unsigned char * __restrict src,
              size_t                           len,
              size_t              * __restrict out_len) {
	unsigned char *out, *pos;
	size_t         i, olen;
	size_t         simd_consumed, simd_written;
	int            pad;

	if (len == 0 || (len & 3) != 0)
		return base64_decode_slow(heap, memparent, src, len, out_len);

	pad = 0;
	if (src[len - 1] == '=') {
		pad++;
		if (src[len - 2] == '=')
			pad++;
	}

	olen = len / 4 * 3 - (size_t)pad;
	pos  = out = ak_heap_alloc(heap, memparent, olen ? olen : 1);
	if (out == NULL)
		return NULL;

	if (!ak_simd_base64_decode(src, len, out, &simd_consumed, &simd_written)) {
		ak_free(out);
		return base64_decode_slow(heap, memparent, src, len, out_len);
	}
	i = simd_consumed;
	pos = out + simd_written;

	for (; i < len; i += 4) {
		unsigned char a, b, c, d;
		int           final;

		final = i + 4 == len;
		if (src[i] == '=' || src[i + 1] == '='
		    || ((src[i + 2] == '=' || src[i + 3] == '=') && !final)
		    || (src[i + 2] == '=' && src[i + 3] != '=')) {
			ak_free(out);
			return base64_decode_slow(heap, memparent, src, len, out_len);
		}

		a = base64_dtable[src[i]];
		b = base64_dtable[src[i + 1]];
		c = src[i + 2] == '=' ? 0 : base64_dtable[src[i + 2]];
		d = src[i + 3] == '=' ? 0 : base64_dtable[src[i + 3]];

		if ((a | b | c | d) & 0x80) {
			ak_free(out);
			return base64_decode_slow(heap, memparent, src, len, out_len);
		}

		*pos++ = (unsigned char)((a << 2) | (b >> 4));
		if (src[i + 2] != '=') {
			*pos++ = (unsigned char)((b << 4) | (c >> 2));
			if (src[i + 3] != '=')
				*pos++ = (unsigned char)((c << 6) | d);
		}
	}

	*out_len = (size_t)(pos - out);
	return out;
}

AK_HIDE
void
base64_buff(const char * __restrict b64,
            size_t                  len,
            AkBuffer   * __restrict buff) {
  const char *b64Data;
  const char *marker;
  size_t      markerLen;

  markerLen = sizeof(";base64,") - 1;
  marker    = memchr(b64, ';', len);
  if (!marker || (size_t)(len - (uintptr_t)(marker - b64)) <= markerLen)
    return;

  if (memcmp(marker, ";base64,", markerLen) != 0)
    return;

  b64Data = marker + markerLen;
  buff->data = base64_decode(ak_heap_getheap(buff),
                             buff,
                             (const unsigned char *)b64Data,
                             len - (uintptr_t)(b64Data - b64),
                             &buff->length);
}
