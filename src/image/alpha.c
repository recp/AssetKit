/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "alpha.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define AK_IMAGE_ALPHA_MAX_METADATA_OFFSET (64u * 1024u * 1024u)
#define AK_IMAGE_ALPHA_MAX_RECORDS 4096u

typedef struct AkImageProbe {
  const uint8_t *data;
  size_t         length;
  FILE          *file;
} AkImageProbe;

static const uint8_t ak_png_signature[8] = {
  0x89u, 'P', 'N', 'G', '\r', '\n', 0x1au, '\n'
};

static const uint8_t ak_ktx1_signature[12] = {
  0xabu, 'K', 'T', 'X', ' ', '1', '1', 0xbbu, '\r', '\n', 0x1au, '\n'
};

static const uint8_t ak_ktx2_signature[12] = {
  0xabu, 'K', 'T', 'X', ' ', '2', '0', 0xbbu, '\r', '\n', 0x1au, '\n'
};

static
bool
ak_imageProbeRead(AkImageProbe * __restrict probe,
                  uint64_t                  offset,
                  void       * __restrict dst,
                  size_t                    length) {
  if (!probe || !dst || length == 0u)
    return false;

  if (probe->length
      && (offset > probe->length || length > probe->length - (size_t)offset))
    return false;

  if (probe->data) {
    memcpy(dst, probe->data + (size_t)offset, length);
    return true;
  }

  if (!probe->file || offset > (uint64_t)LONG_MAX)
    return false;

  return fseek(probe->file, (long)offset, SEEK_SET) == 0
         && fread(dst, 1u, length, probe->file) == length;
}

static
size_t
ak_imageProbeLength(AkImageProbe * __restrict probe) {
  long fileLength;

  if (!probe || probe->length || !probe->file)
    return probe ? probe->length : 0u;

  if (fseek(probe->file, 0L, SEEK_END) != 0
      || (fileLength = ftell(probe->file)) <= 0L)
    return 0u;

  probe->length = (size_t)fileLength;
  return probe->length;
}

static
uint16_t
ak_imageU16LE(const uint8_t * __restrict data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static
uint16_t
ak_imageU16BE(const uint8_t * __restrict data) {
  return ((uint16_t)data[0] << 8) | (uint16_t)data[1];
}

static
uint32_t
ak_imageU32LE(const uint8_t * __restrict data) {
  return (uint32_t)data[0]
         | ((uint32_t)data[1] << 8)
         | ((uint32_t)data[2] << 16)
         | ((uint32_t)data[3] << 24);
}

static
uint32_t
ak_imageU32BE(const uint8_t * __restrict data) {
  return ((uint32_t)data[0] << 24)
         | ((uint32_t)data[1] << 16)
         | ((uint32_t)data[2] << 8)
         | (uint32_t)data[3];
}

static
bool
ak_imageAsciiEqualCI(const char * __restrict left,
                     const char * __restrict right) {
  char a, b;

  if (!left || !right)
    return false;

  do {
    a = *left++;
    b = *right++;
    if (a >= 'A' && a <= 'Z')
      a = (char)(a + ('a' - 'A'));
    if (b >= 'A' && b <= 'Z')
      b = (char)(b + ('a' - 'A'));
  } while (a && a == b);

  return a == b;
}

static
bool
ak_imageUriHasSuffixCI(const char * __restrict uri,
                       const char * __restrict suffix) {
  size_t uriLength, suffixLength;

  if (!uri || !suffix)
    return false;

  uriLength = strcspn(uri, "?#");
  suffixLength = strlen(suffix);
  if (uriLength < suffixLength)
    return false;

  return ak_imageAsciiEqualCI(uri + uriLength - suffixLength, suffix);
}

static
AkImageAlphaPresence
ak_imageProbePNG(AkImageProbe * __restrict probe) {
  uint8_t  signature[sizeof(ak_png_signature)];
  uint64_t offset;
  uint32_t record;

  if (!ak_imageProbeRead(probe, 0u, signature, sizeof(signature))
      || memcmp(signature, ak_png_signature, sizeof(signature)) != 0)
    return AK_IMAGE_ALPHA_UNKNOWN;

  offset = sizeof(signature);
  for (record = 0u;
       record < AK_IMAGE_ALPHA_MAX_RECORDS
       && offset <= AK_IMAGE_ALPHA_MAX_METADATA_OFFSET;
       record++) {
    uint8_t  chunkHeader[8];
    uint32_t chunkLength;

    if (!ak_imageProbeRead(probe, offset, chunkHeader, sizeof(chunkHeader)))
      return AK_IMAGE_ALPHA_UNKNOWN;

    chunkLength = ak_imageU32BE(chunkHeader);
    if (memcmp(chunkHeader + 4u, "IHDR", 4u) == 0) {
      uint8_t ihdr[13];

      if (chunkLength != sizeof(ihdr)
          || !ak_imageProbeRead(probe, offset + 8u, ihdr, sizeof(ihdr)))
        return AK_IMAGE_ALPHA_UNKNOWN;
      if (ihdr[9] == 4u || ihdr[9] == 6u)
        return AK_IMAGE_ALPHA_PRESENT;
    } else if (memcmp(chunkHeader + 4u, "tRNS", 4u) == 0) {
      return chunkLength > 0u ? AK_IMAGE_ALPHA_PRESENT
                              : AK_IMAGE_ALPHA_UNKNOWN;
    } else if (memcmp(chunkHeader + 4u, "IDAT", 4u) == 0
               || memcmp(chunkHeader + 4u, "IEND", 4u) == 0) {
      return AK_IMAGE_ALPHA_ABSENT;
    }

    if ((uint64_t)chunkLength > UINT64_MAX - offset - 12u)
      return AK_IMAGE_ALPHA_UNKNOWN;
    offset += (uint64_t)chunkLength + 12u;
  }

  return AK_IMAGE_ALPHA_UNKNOWN;
}

static
AkImageAlphaPresence
ak_imageProbeTGA(AkImageProbe * __restrict probe) {
  uint8_t header[18];
  uint8_t imageType, pixelDepth, alphaBits;
  size_t  fileLength;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  imageType  = header[2];
  pixelDepth = header[16];
  alphaBits  = header[17] & 0x0fu;
  if (!(imageType == 1u || imageType == 2u || imageType == 3u
        || imageType == 9u || imageType == 10u || imageType == 11u)
      || pixelDepth == 0u || alphaBits > pixelDepth)
    return AK_IMAGE_ALPHA_UNKNOWN;

  /* TGA 2.0 can state whether the attribute channel is usable alpha in its
     extension area. Prefer that declaration when a footer is available. */
  fileLength = ak_imageProbeLength(probe);
  if (fileLength >= 26u) {
    uint8_t footer[26];

    if (ak_imageProbeRead(probe,
                          fileLength - sizeof(footer),
                          footer,
                          sizeof(footer))
        && memcmp(footer + 8u, "TRUEVISION-XFILE.\0", 18u) == 0) {
      uint32_t extensionOffset = ak_imageU32LE(footer);
      uint8_t  extensionSizeBytes[2];
      uint8_t  attributeType;

      if (extensionOffset <= AK_IMAGE_ALPHA_MAX_METADATA_OFFSET
          && ak_imageProbeRead(probe,
                              extensionOffset,
                              extensionSizeBytes,
                              sizeof(extensionSizeBytes))
          && ak_imageU16LE(extensionSizeBytes) >= 495u
          && ak_imageProbeRead(probe,
                              (uint64_t)extensionOffset + 494u,
                              &attributeType,
                              1u)) {
        if (attributeType == 3u || attributeType == 4u)
          return AK_IMAGE_ALPHA_PRESENT;
        if (attributeType <= 1u)
          return AK_IMAGE_ALPHA_ABSENT;
        if (attributeType == 2u)
          return AK_IMAGE_ALPHA_UNKNOWN;
      }
    }
  }

  if (alphaBits == 0u)
    return AK_IMAGE_ALPHA_ABSENT;

  if ((imageType == 1u || imageType == 9u)
      && !(header[7] == 16u || header[7] == 32u))
    return AK_IMAGE_ALPHA_UNKNOWN;

  return AK_IMAGE_ALPHA_PRESENT;
}

static
AkImageAlphaPresence
ak_imageProbeWebP(AkImageProbe * __restrict probe) {
  uint8_t  header[12];
  uint64_t offset, riffEnd;
  uint32_t record;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || memcmp(header, "RIFF", 4u) != 0
      || memcmp(header + 8u, "WEBP", 4u) != 0)
    return AK_IMAGE_ALPHA_UNKNOWN;

  riffEnd = (uint64_t)ak_imageU32LE(header + 4u) + 8u;
  if (riffEnd < 20u)
    return AK_IMAGE_ALPHA_UNKNOWN;

  offset = 12u;
  for (record = 0u;
       record < AK_IMAGE_ALPHA_MAX_RECORDS
       && offset + 8u <= riffEnd
       && offset <= AK_IMAGE_ALPHA_MAX_METADATA_OFFSET;
       record++) {
    uint8_t  chunkHeader[8];
    uint32_t chunkLength;

    if (!ak_imageProbeRead(probe, offset, chunkHeader, sizeof(chunkHeader)))
      return AK_IMAGE_ALPHA_UNKNOWN;
    chunkLength = ak_imageU32LE(chunkHeader + 4u);

    if (memcmp(chunkHeader, "VP8X", 4u) == 0) {
      uint8_t flags;

      if (chunkLength < 10u
          || !ak_imageProbeRead(probe, offset + 8u, &flags, 1u))
        return AK_IMAGE_ALPHA_UNKNOWN;
      return (flags & 0x10u) ? AK_IMAGE_ALPHA_PRESENT
                             : AK_IMAGE_ALPHA_ABSENT;
    }

    if (memcmp(chunkHeader, "ALPH", 4u) == 0)
      return chunkLength > 0u ? AK_IMAGE_ALPHA_PRESENT
                              : AK_IMAGE_ALPHA_UNKNOWN;

    if (memcmp(chunkHeader, "VP8L", 4u) == 0) {
      uint8_t losslessHeader[5];

      if (chunkLength < sizeof(losslessHeader)
          || !ak_imageProbeRead(probe,
                               offset + 8u,
                               losslessHeader,
                               sizeof(losslessHeader))
          || losslessHeader[0] != 0x2fu)
        return AK_IMAGE_ALPHA_UNKNOWN;
      return (losslessHeader[4] & 0x10u) ? AK_IMAGE_ALPHA_PRESENT
                                         : AK_IMAGE_ALPHA_ABSENT;
    }

    if (memcmp(chunkHeader, "VP8 ", 4u) == 0)
      return AK_IMAGE_ALPHA_ABSENT;

    if ((uint64_t)chunkLength > UINT64_MAX - offset - 9u)
      return AK_IMAGE_ALPHA_UNKNOWN;
    offset += 8u + (uint64_t)chunkLength + (chunkLength & 1u);
  }

  return AK_IMAGE_ALPHA_UNKNOWN;
}

static
AkImageAlphaPresence
ak_imageProbeGIF(AkImageProbe * __restrict probe) {
  uint8_t  header[13];
  uint64_t offset;
  uint32_t record;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || (memcmp(header, "GIF87a", 6u) != 0
          && memcmp(header, "GIF89a", 6u) != 0))
    return AK_IMAGE_ALPHA_UNKNOWN;

  offset = 13u;
  if (header[10] & 0x80u)
    offset += 3u * (1u << ((header[10] & 0x07u) + 1u));

  for (record = 0u;
       record < AK_IMAGE_ALPHA_MAX_RECORDS
       && offset <= AK_IMAGE_ALPHA_MAX_METADATA_OFFSET;
       record++) {
    uint8_t marker;

    if (!ak_imageProbeRead(probe, offset, &marker, 1u))
      return AK_IMAGE_ALPHA_UNKNOWN;

    if (marker == 0x3bu)
      return AK_IMAGE_ALPHA_ABSENT;

    /* Only metadata before the first frame controls that displayed frame. */
    if (marker == 0x2cu)
      return AK_IMAGE_ALPHA_ABSENT;

    if (marker != 0x21u)
      return AK_IMAGE_ALPHA_UNKNOWN;

    {
      uint8_t extension[3];

      if (!ak_imageProbeRead(probe, offset, extension, sizeof(extension)))
        return AK_IMAGE_ALPHA_UNKNOWN;
      if (extension[1] == 0xf9u) {
        uint8_t control[5];

        if (extension[2] != 4u
            || !ak_imageProbeRead(probe, offset + 3u, control,
                                 sizeof(control)))
          return AK_IMAGE_ALPHA_UNKNOWN;
        if (control[0] & 0x01u)
          return AK_IMAGE_ALPHA_PRESENT;
        offset += 8u;
        continue;
      }

      offset += 2u;
      for (;;) {
        uint8_t blockLength;

        if (offset > AK_IMAGE_ALPHA_MAX_METADATA_OFFSET
            || !ak_imageProbeRead(probe, offset, &blockLength, 1u))
          return AK_IMAGE_ALPHA_UNKNOWN;
        offset++;
        if (blockLength == 0u)
          break;
        offset += blockLength;
      }
    }
  }

  return AK_IMAGE_ALPHA_UNKNOWN;
}

static
AkImageAlphaPresence
ak_imageProbeTIFF(AkImageProbe * __restrict probe) {
  uint8_t  header[8];
  uint8_t  countBytes[2];
  uint32_t ifdOffset, entryCount, i;
  bool     littleEndian;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  if (header[0] == 'I' && header[1] == 'I')
    littleEndian = true;
  else if (header[0] == 'M' && header[1] == 'M')
    littleEndian = false;
  else
    return AK_IMAGE_ALPHA_UNKNOWN;

#define AK_TIFF_U16(p) (littleEndian ? ak_imageU16LE(p) : ak_imageU16BE(p))
#define AK_TIFF_U32(p) (littleEndian ? ak_imageU32LE(p) : ak_imageU32BE(p))

  if (AK_TIFF_U16(header + 2u) != 42u)
    return AK_IMAGE_ALPHA_UNKNOWN;
  ifdOffset = AK_TIFF_U32(header + 4u);
  if (ifdOffset > AK_IMAGE_ALPHA_MAX_METADATA_OFFSET
      || !ak_imageProbeRead(probe, ifdOffset, countBytes, sizeof(countBytes)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  entryCount = AK_TIFF_U16(countBytes);
  if (entryCount > AK_IMAGE_ALPHA_MAX_RECORDS)
    return AK_IMAGE_ALPHA_UNKNOWN;

  for (i = 0u; i < entryCount; i++) {
    uint8_t  entry[12];
    uint32_t count, j;

    if (!ak_imageProbeRead(probe,
                           (uint64_t)ifdOffset + 2u + (uint64_t)i * 12u,
                           entry,
                           sizeof(entry)))
      return AK_IMAGE_ALPHA_UNKNOWN;
    if (AK_TIFF_U16(entry) != 338u)
      continue;
    if (AK_TIFF_U16(entry + 2u) != 3u)
      return AK_IMAGE_ALPHA_UNKNOWN;

    count = AK_TIFF_U32(entry + 4u);
    if (count == 0u || count > AK_IMAGE_ALPHA_MAX_RECORDS)
      return AK_IMAGE_ALPHA_UNKNOWN;
    for (j = 0u; j < count; j++) {
      uint8_t  valueBytes[2];
      uint16_t value;

      if (count <= 2u) {
        value = AK_TIFF_U16(entry + 8u + j * 2u);
      } else {
        uint32_t valuesOffset = AK_TIFF_U32(entry + 8u);

        if (valuesOffset > AK_IMAGE_ALPHA_MAX_METADATA_OFFSET
            || !ak_imageProbeRead(probe,
                                  (uint64_t)valuesOffset + (uint64_t)j * 2u,
                                  valueBytes,
                                  sizeof(valueBytes)))
          return AK_IMAGE_ALPHA_UNKNOWN;
        value = AK_TIFF_U16(valueBytes);
      }

      if (value == 1u || value == 2u)
        return AK_IMAGE_ALPHA_PRESENT;
    }
    return AK_IMAGE_ALPHA_ABSENT;
  }

#undef AK_TIFF_U16
#undef AK_TIFF_U32

  return AK_IMAGE_ALPHA_ABSENT;
}

static
AkImageAlphaPresence
ak_imageProbeBMP(AkImageProbe * __restrict probe) {
  uint8_t  header[70];
  uint32_t dibSize, compression, alphaMask;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || memcmp(header, "BM", 2u) != 0)
    return AK_IMAGE_ALPHA_UNKNOWN;

  dibSize = ak_imageU32LE(header + 14u);
  if (dibSize < 40u)
    return AK_IMAGE_ALPHA_UNKNOWN;
  if (ak_imageU16LE(header + 28u) != 32u)
    return AK_IMAGE_ALPHA_ABSENT;

  compression = ak_imageU32LE(header + 30u);
  alphaMask   = ak_imageU32LE(header + 66u);
  if ((compression == 6u || dibSize >= 56u) && alphaMask != 0u)
    return AK_IMAGE_ALPHA_PRESENT;

  return AK_IMAGE_ALPHA_ABSENT;
}

static
AkImageAlphaPresence
ak_imageProbeDDS(AkImageProbe * __restrict probe) {
  uint8_t  header[128];
  uint32_t flags, fourCC, alphaMask;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || memcmp(header, "DDS ", 4u) != 0
      || ak_imageU32LE(header + 4u) != 124u
      || ak_imageU32LE(header + 76u) != 32u)
    return AK_IMAGE_ALPHA_UNKNOWN;

  flags     = ak_imageU32LE(header + 80u);
  fourCC    = ak_imageU32LE(header + 84u);
  alphaMask = ak_imageU32LE(header + 104u);
  if ((flags & 0x03u) != 0u && alphaMask != 0u)
    return AK_IMAGE_ALPHA_PRESENT;

  /* DXT3/DXT5 always allocate a dedicated alpha block. DXT1 is deliberately
     left unknown because its optional one-bit transparency is not signalled
     reliably by legacy DDS headers. */
  if ((flags & 0x04u) != 0u
      && (fourCC == 0x33545844u || fourCC == 0x35545844u))
    return AK_IMAGE_ALPHA_PRESENT;

  if ((flags & 0x40u) != 0u)
    return AK_IMAGE_ALPHA_ABSENT;

  return AK_IMAGE_ALPHA_UNKNOWN;
}

static
AkImageAlphaPresence
ak_imageProbeKTX1(AkImageProbe * __restrict probe) {
  uint8_t  header[36];
  bool     littleEndian;
  uint32_t baseFormat;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || memcmp(header, ak_ktx1_signature, sizeof(ak_ktx1_signature)) != 0)
    return AK_IMAGE_ALPHA_UNKNOWN;

  if (memcmp(header + 12u, "\x01\x02\x03\x04", 4u) == 0)
    littleEndian = true;
  else if (memcmp(header + 12u, "\x04\x03\x02\x01", 4u) == 0)
    littleEndian = false;
  else
    return AK_IMAGE_ALPHA_UNKNOWN;

  baseFormat = littleEndian ? ak_imageU32LE(header + 32u)
                            : ak_imageU32BE(header + 32u);
  if (baseFormat == 0x1906u /* GL_ALPHA */
      || baseFormat == 0x1908u /* GL_RGBA */
      || baseFormat == 0x190au /* GL_LUMINANCE_ALPHA */)
    return AK_IMAGE_ALPHA_PRESENT;

  return AK_IMAGE_ALPHA_ABSENT;
}

static
AkImageAlphaPresence
ak_imageProbeKTX2(AkImageProbe * __restrict probe) {
  uint8_t  header[56];
  uint8_t  dfdHeader[28];
  uint32_t dfdOffset, dfdLength, blockSize, sampleCount, model, i;

  if (!ak_imageProbeRead(probe, 0u, header, sizeof(header))
      || memcmp(header, ak_ktx2_signature, sizeof(ak_ktx2_signature)) != 0)
    return AK_IMAGE_ALPHA_UNKNOWN;

  dfdOffset = ak_imageU32LE(header + 48u);
  dfdLength = ak_imageU32LE(header + 52u);
  if (dfdOffset > AK_IMAGE_ALPHA_MAX_METADATA_OFFSET
      || dfdLength < sizeof(dfdHeader)
      || !ak_imageProbeRead(probe, dfdOffset, dfdHeader, sizeof(dfdHeader)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  if (ak_imageU32LE(dfdHeader) > dfdLength)
    return AK_IMAGE_ALPHA_UNKNOWN;
  blockSize = ak_imageU32LE(dfdHeader + 8u) >> 16;
  if (blockSize < 24u || blockSize > dfdLength - 4u
      || ((blockSize - 24u) & 15u) != 0u)
    return AK_IMAGE_ALPHA_UNKNOWN;

  model       = dfdHeader[12];
  sampleCount = (blockSize - 24u) / 16u;
  for (i = 0u; i < sampleCount; i++) {
    uint8_t sampleWord[4];
    uint8_t channel;

    if (!ak_imageProbeRead(probe,
                           (uint64_t)dfdOffset + 28u + (uint64_t)i * 16u,
                           sampleWord,
                           sizeof(sampleWord)))
      return AK_IMAGE_ALPHA_UNKNOWN;
    channel = (sampleWord[3] & 0x0fu);
    if (channel == 15u
        || (model == 128u && channel == 1u) /* BC1A */
        || (model == 162u && channel == 0u) /* ASTC RGBA */
        || (model == 166u && channel == 3u)) /* UASTC RGBA */
      return AK_IMAGE_ALPHA_PRESENT;
  }

  /* BC7 and PVRTC descriptor channels do not distinguish RGB from RGBA. */
  if (model == 134u || model == 164u || model == 165u)
    return AK_IMAGE_ALPHA_UNKNOWN;

  return AK_IMAGE_ALPHA_ABSENT;
}

static
AkImageAlphaPresence
ak_imageProbeEncoded(AkImageProbe * __restrict probe,
                     const char   * __restrict uri,
                     const char   * __restrict mimeType) {
  uint8_t signature[12];

  if (!ak_imageProbeRead(probe, 0u, signature, sizeof(signature)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  if (memcmp(signature, ak_png_signature, sizeof(ak_png_signature)) == 0)
    return ak_imageProbePNG(probe);
  if (memcmp(signature, "RIFF", 4u) == 0
      && memcmp(signature + 8u, "WEBP", 4u) == 0)
    return ak_imageProbeWebP(probe);
  if (memcmp(signature, "GIF87a", 6u) == 0
      || memcmp(signature, "GIF89a", 6u) == 0)
    return ak_imageProbeGIF(probe);
  if ((signature[0] == 'I' && signature[1] == 'I')
      || (signature[0] == 'M' && signature[1] == 'M'))
    return ak_imageProbeTIFF(probe);
  if (memcmp(signature, "BM", 2u) == 0)
    return ak_imageProbeBMP(probe);
  if (memcmp(signature, "DDS ", 4u) == 0)
    return ak_imageProbeDDS(probe);
  if (memcmp(signature, ak_ktx1_signature, sizeof(ak_ktx1_signature)) == 0)
    return ak_imageProbeKTX1(probe);
  if (memcmp(signature, ak_ktx2_signature, sizeof(ak_ktx2_signature)) == 0)
    return ak_imageProbeKTX2(probe);

  /* TGA has no reliable magic number, so require an explicit type hint. */
  if (ak_imageUriHasSuffixCI(uri, ".tga")
      || ak_imageUriHasSuffixCI(uri, ".targa")
      || ak_imageAsciiEqualCI(mimeType, "image/tga")
      || ak_imageAsciiEqualCI(mimeType, "image/x-tga"))
    return ak_imageProbeTGA(probe);

  if ((signature[0] == 0xffu && signature[1] == 0xd8u)
      || ak_imageUriHasSuffixCI(uri, ".jpg")
      || ak_imageUriHasSuffixCI(uri, ".jpeg"))
    return AK_IMAGE_ALPHA_ABSENT;

  return AK_IMAGE_ALPHA_UNKNOWN;
}

AK_HIDE
AkImageAlphaPresence
ak_imageAlphaPresence(AkImage * __restrict image) {
  AkImageSource *source;
  AkImageProbe   probe;
  const char    *path;
  AkImageAlphaPresence presence;

  if (!image)
    return AK_IMAGE_ALPHA_UNKNOWN;

  source = image->source ? image->source
                         : image->image ? image->image->source : NULL;
  if (!source) {
    if (image->data && (image->data->comp == 2u || image->data->comp == 4u))
      return AK_IMAGE_ALPHA_PRESENT;
    return AK_IMAGE_ALPHA_UNKNOWN;
  }

  memset(&probe, 0, sizeof(probe));
  if (source->type == AK_IMAGE_SOURCE_BUFFER && source->buffer) {
    probe.data   = source->buffer->data;
    probe.length = source->buffer->length;
    return ak_imageProbeEncoded(&probe, source->uri, source->mimeType);
  }

  if (source->type != AK_IMAGE_SOURCE_URI
      || !(path = ak_imageResolvePath(image)))
    return AK_IMAGE_ALPHA_UNKNOWN;

  probe.file = fopen(path, "rb");
  if (!probe.file)
    return AK_IMAGE_ALPHA_UNKNOWN;
  presence = ak_imageProbeEncoded(&probe, source->uri, source->mimeType);
  fclose(probe.file);
  return presence;
}
