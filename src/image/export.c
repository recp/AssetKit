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

#include "export.h"
#include "../io/common/uri.h"

#include <ak/path.h>
#include <libdeflate.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATH_MAX
#  define PATH_MAX 260
#endif

static
bool
ak_imageExportDataOK(AkImageData * __restrict data,
                     int         * __restrict width,
                     int         * __restrict height,
                     int         * __restrict channels) {
  size_t pixels;

  if (!data || !data->data || data->width == 0 || data->height == 0)
    return false;

  if (data->width > (uint32_t)INT_MAX || data->height > (uint32_t)INT_MAX)
    return false;

  if (data->comp < 1 || data->comp > 4)
    return false;

  if ((size_t)data->width > (size_t)-1 / (size_t)data->height)
    return false;

  pixels = (size_t)data->width * (size_t)data->height;
  if (pixels > (size_t)-1 / (size_t)data->comp)
    return false;

  *width    = (int)data->width;
  *height   = (int)data->height;
  *channels = (int)data->comp;

  return true;
}

static
void
ak_imageExportWriteU32BE(unsigned char * __restrict dst, uint32_t val) {
  dst[0] = (unsigned char)(val >> 24);
  dst[1] = (unsigned char)(val >> 16);
  dst[2] = (unsigned char)(val >> 8);
  dst[3] = (unsigned char)val;
}

static
unsigned char*
ak_imageExportWritePNGChunk(unsigned char       * __restrict dst,
                            const char          * __restrict type,
                            const unsigned char * __restrict data,
                            uint32_t                         len) {
  uint32_t crc;

  ak_imageExportWriteU32BE(dst, len);
  memcpy(dst + 4, type, 4);
  if (len > 0)
    memcpy(dst + 8, data, len);

  crc = libdeflate_crc32(0u, dst + 4, (size_t)len + 4u);
  ak_imageExportWriteU32BE(dst + 8u + len, crc);

  return dst + 12u + len;
}

static
uint16_t
ak_imageExportReadU16LE(const unsigned char * __restrict src) {
  return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

static
uint32_t
ak_imageExportReadU32LE(const unsigned char * __restrict src) {
  return (uint32_t)src[0]
         | ((uint32_t)src[1] << 8)
         | ((uint32_t)src[2] << 16)
         | ((uint32_t)src[3] << 24);
}

static
const char*
ak_imageExportSourcePath(AkImageExportRequest * __restrict req,
                         char                 * __restrict pathbuf,
                         char                 * __restrict uribuf) {
  AkImageSource *source;
  const char    *uri;
  const char    *path;

  if (!req || !req->image)
    return NULL;

  source = ak_imageSource(req->image);
  if (!source || source->type != AK_IMAGE_SOURCE_URI || !source->uri)
    return NULL;

  if (source->resolvedPath)
    return source->resolvedPath;

  uri = source->uri;
  if (io_uri_has_prefix(uri, IO_URI_FILE_PREFIX, IO_URI_FILE_PREFIX_LEN))
    uri += IO_URI_FILE_PREFIX_LEN;

  if (!io_uri_decode_path(uri, uribuf, PATH_MAX))
    return NULL;

  path = uribuf;
  if (req->doc && req->doc->inf && req->doc->inf->dir
      && path[0] != '/'
      && !(path[0] && path[1] == ':'))
    path = ak_fullpathn(req->doc, path, pathbuf, PATH_MAX);

  return path;
}

static
bool
ak_imageExportLoadBMP(AkImageExportRequest * __restrict req,
                      AkImageData          * __restrict out) {
  unsigned char header[54];
  unsigned char *fileData;
  unsigned char *pixels;
  const char    *path;
  char           pathbuf[PATH_MAX];
  char           uribuf[PATH_MAX];
  FILE          *file;
  long           fileLenLong;
  size_t         fileLen;
  uint32_t       pixelOffset;
  uint32_t       dibSize;
  uint32_t       width;
  uint32_t       absHeight;
  uint16_t       planes;
  uint16_t       bpp;
  uint32_t       compression;
  uint32_t       rowStride;
  uint32_t       channels;
  int32_t        signedHeight;
  bool           topDown;
  uint32_t       y;

  if (!out)
    return false;

  memset(out, 0, sizeof(*out));

  path = ak_imageExportSourcePath(req, pathbuf, uribuf);
  if (!path)
    return false;

  file = fopen(path, "rb");
  if (!file)
    return false;

  if (fread(header, 1, sizeof(header), file) != sizeof(header)
      || header[0] != 'B'
      || header[1] != 'M') {
    fclose(file);
    return false;
  }

  pixelOffset = ak_imageExportReadU32LE(header + 10);
  dibSize     = ak_imageExportReadU32LE(header + 14);
  width       = ak_imageExportReadU32LE(header + 18);
  signedHeight = (int32_t)ak_imageExportReadU32LE(header + 22);
  planes      = ak_imageExportReadU16LE(header + 26);
  bpp         = ak_imageExportReadU16LE(header + 28);
  compression = ak_imageExportReadU32LE(header + 30);

  if (dibSize < 40
      || width == 0
      || signedHeight == 0
      || planes != 1
      || compression != 0
      || (bpp != 24 && bpp != 32)) {
    fclose(file);
    return false;
  }

  topDown   = signedHeight < 0;
  absHeight = topDown ? (uint32_t)-signedHeight : (uint32_t)signedHeight;
  channels  = bpp == 32 ? 4u : 3u;

  if (width > UINT32_MAX / bpp
      || width * bpp > UINT32_MAX - 31u
      || absHeight > UINT32_MAX / channels
      || width > SIZE_MAX / absHeight
      || (size_t)width * (size_t)absHeight > SIZE_MAX / channels) {
    fclose(file);
    return false;
  }

  rowStride = ((width * bpp + 31u) / 32u) * 4u;
  if (absHeight > 0 && rowStride > (uint32_t)(SIZE_MAX / absHeight)) {
    fclose(file);
    return false;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }
  fileLenLong = ftell(file);
  if (fileLenLong < 0) {
    fclose(file);
    return false;
  }
  fileLen = (size_t)fileLenLong;

  if (pixelOffset > fileLen
      || (size_t)rowStride * (size_t)absHeight > fileLen - pixelOffset) {
    fclose(file);
    return false;
  }

  fileData = malloc(fileLen - pixelOffset);
  if (!fileData) {
    fclose(file);
    return false;
  }

  if (fseek(file, (long)pixelOffset, SEEK_SET) != 0
      || fread(fileData, 1, fileLen - pixelOffset, file)
         != fileLen - pixelOffset) {
    free(fileData);
    fclose(file);
    return false;
  }
  fclose(file);

  pixels = malloc((size_t)width * (size_t)absHeight * (size_t)channels);
  if (!pixels) {
    free(fileData);
    return false;
  }

  for (y = 0; y < absHeight; y++) {
    uint32_t srcY;
    uint32_t x;
    const unsigned char *src;
    unsigned char       *dst;

    srcY = topDown ? y : absHeight - 1u - y;
    src  = fileData + (size_t)srcY * rowStride;
    dst  = pixels + (size_t)y * (size_t)width * channels;

    for (x = 0; x < width; x++) {
      dst[0] = src[2];
      dst[1] = src[1];
      dst[2] = src[0];
      if (channels == 4)
        dst[3] = src[3];
      src += bpp / 8u;
      dst += channels;
    }
  }

  free(fileData);

  out->data   = pixels;
  out->width  = width;
  out->height = absHeight;
  out->comp   = channels;

  return true;
}

static
uint32_t
ak_imageExportAdler32(const unsigned char * __restrict data, size_t len) {
  uint32_t a;
  uint32_t b;
  size_t   i;

  a = 1;
  b = 0;
  for (i = 0; i < len; i++) {
    a += data[i];
    b += a;
    if ((i & 0x0fffu) == 0x0fffu) {
      a %= 65521u;
      b %= 65521u;
    }
  }

  a %= 65521u;
  b %= 65521u;

  return (b << 16) | a;
}

static
void
ak_imageExportWriteU16LE(unsigned char * __restrict dst, uint32_t val) {
  dst[0] = (unsigned char)val;
  dst[1] = (unsigned char)(val >> 8);
}

static
bool
ak_imageExportZlibStoredLen(size_t rawLen, size_t * __restrict outLen) {
  size_t blockCount;

  if (rawLen > (size_t)-1 - 6u)
    return false;

  blockCount = rawLen == 0
               ? 1u
               : rawLen / 65535u + (rawLen % 65535u ? 1u : 0u);
  if (blockCount > ((size_t)-1 - 6u - rawLen) / 5u)
    return false;

  *outLen = 2u + rawLen + blockCount * 5u + 4u;

  return true;
}

static
bool
ak_imageExportWriteZlibStored(unsigned char       * __restrict dst,
                              const unsigned char * __restrict raw,
                              size_t                           rawLen,
                              size_t                           zlibLen) {
  unsigned char *it;
  size_t         remaining;
  uint32_t       adler;

  if (zlibLen < 11u)
    return false;

  it     = dst;
  it[0]  = 0x78;
  it[1]  = 0x01;
  it    += 2u;

  remaining = rawLen;
  while (remaining > 0 || rawLen == 0) {
    size_t blockLen;
    bool   final;

    blockLen = remaining > 65535u ? 65535u : remaining;
    final    = remaining <= 65535u;

    *it++ = final ? 0x01u : 0x00u;
    ak_imageExportWriteU16LE(it, (uint32_t)blockLen);
    it += 2u;
    ak_imageExportWriteU16LE(it, (uint32_t)(0xffffu - blockLen));
    it += 2u;

    if (blockLen > 0) {
      memcpy(it, raw + (rawLen - remaining), blockLen);
      it += blockLen;
      remaining -= blockLen;
    }

    if (final)
      break;
  }

  adler = ak_imageExportAdler32(raw, rawLen);
  ak_imageExportWriteU32BE(it, adler);
  it += 4u;

  return (size_t)(it - dst) == zlibLen;
}

static
void*
ak_imageExportPNGEncode(AkImageData * __restrict data,
                        int                      width,
                        int                      height,
                        int                      channels,
                        size_t      * __restrict outLen) {
  static const unsigned char pngSig[8] = {
    0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a
  };
  static const unsigned char colorTypes[5] = {0, 0, 4, 2, 6};
  unsigned char *raw;
  unsigned char *zlib;
  unsigned char *png;
  unsigned char *it;
  size_t         rawLen;
  size_t         zlibLen;
  size_t         rowBytes;
  size_t         scanlineBytes;
  size_t         pngLen;
  int            y;
  unsigned char  ihdr[13];

  *outLen = 0;

  rowBytes = (size_t)width * (size_t)channels;
  if (rowBytes > (size_t)-1 - 1u)
    return NULL;

  scanlineBytes = rowBytes + 1u;
  if ((size_t)height > (size_t)-1 / scanlineBytes)
    return NULL;

  rawLen = scanlineBytes * (size_t)height;
  raw    = malloc(rawLen);
  if (!raw)
    return NULL;

  for (y = 0; y < height; y++) {
    unsigned char *dst;
    const unsigned char *src;

    dst    = raw + (size_t)y * scanlineBytes;
    src    = (const unsigned char *)data->data + (size_t)y * rowBytes;
    dst[0] = 0;
    memcpy(dst + 1, src, rowBytes);
  }

  if (!ak_imageExportZlibStoredLen(rawLen, &zlibLen)) {
    free(raw);
    return NULL;
  }

  zlib = malloc(zlibLen);
  if (!zlib) {
    free(raw);
    return NULL;
  }

  if (!ak_imageExportWriteZlibStored(zlib, raw, rawLen, zlibLen)) {
    free(zlib);
    free(raw);
    return NULL;
  }

  free(raw);

  if (zlibLen > UINT32_MAX || zlibLen > (size_t)-1 - 57u) {
    free(zlib);
    return NULL;
  }

  pngLen = 57u + zlibLen;
  png    = malloc(pngLen);
  if (!png) {
    free(zlib);
    return NULL;
  }

  memset(ihdr, 0, sizeof(ihdr));
  ak_imageExportWriteU32BE(ihdr, (uint32_t)width);
  ak_imageExportWriteU32BE(ihdr + 4, (uint32_t)height);
  ihdr[8]  = 8;
  ihdr[9]  = colorTypes[channels];
  ihdr[10] = 0;
  ihdr[11] = 0;
  ihdr[12] = 0;

  it = png;
  memcpy(it, pngSig, sizeof(pngSig));
  it += sizeof(pngSig);
  it = ak_imageExportWritePNGChunk(it, "IHDR", ihdr, sizeof(ihdr));
  it = ak_imageExportWritePNGChunk(it,
                                   "IDAT",
                                   zlib,
                                   (uint32_t)zlibLen);
  it = ak_imageExportWritePNGChunk(it, "IEND", NULL, 0);
  free(zlib);

  if ((size_t)(it - png) != pngLen) {
    free(png);
    return NULL;
  }

  *outLen = pngLen;

  return png;
}

bool
ak_imageExportPNG(AkImageExportRequest * __restrict req,
                  AkImageExportPayload * __restrict payload) {
  AkImageData *data;
  AkImageData  bmpData;
  void        *png;
  size_t       pngLen;
  int          width;
  int          height;
  int          channels;
  bool         releaseBmp;

  if (!payload)
    return false;

  memset(payload, 0, sizeof(*payload));

  if (!req || !req->image)
    return false;

  if (!req->image->data)
    ak_imageLoad(req->image);

  data       = req->image->data;
  releaseBmp = false;
  if (!ak_imageExportDataOK(data, &width, &height, &channels)) {
    if (!ak_imageExportLoadBMP(req, &bmpData))
      return false;
    data       = &bmpData;
    releaseBmp = true;
    if (!ak_imageExportDataOK(data, &width, &height, &channels)) {
      free(bmpData.data);
      return false;
    }
  }

  pngLen = 0;
  png    = ak_imageExportPNGEncode(data, width, height, channels, &pngLen);
  if (releaseBmp)
    free(bmpData.data);

  if (!png || pngLen == 0)
    return false;

  payload->data       = png;
  payload->byteLength = pngLen;
  payload->mimeType   = req->targetMimeType
                        ? req->targetMimeType
                        : "image/png";

  return true;
}

void
ak_imageExportPayloadRelease(AkImageExportPayload * __restrict payload) {
  if (!payload)
    return;

  if (payload->data)
    free(payload->data);

  memset(payload, 0, sizeof(*payload));
}
