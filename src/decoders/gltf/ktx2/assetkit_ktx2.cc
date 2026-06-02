/*
 * Copyright (C) 2026 Recep Aslantas
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

/*
 * AssetKit KTX2 / BasisU image decoder shim — KHR_texture_basisu support.
 *
 * Compile: links against `libktx.a` (KhronosGroup/KTX-Software) which
 * provides KTX2 container parsing + BasisU transcoding (UASTC / ETC1S →
 * RGBA8 by default; the host can request a different target). Build flag:
 * AK_BUILD_GLTF_KTX2_DECODER. CMake FetchContent pulls KTX-Software when
 * not pre-installed.
 *
 * Runtime entry: AssetKit dlopens this dylib via
 * AK_OPT_GLTF_KTX2_DECODER_PATH (or autoload) and calls
 * `assetkit_ktx2_decode` with raw KTX2 bytes (read from a bufferView or
 * resolved URI). The shim transcodes to RGBA8 and returns the decoded
 * pixel buffer + width/height/channels so the caller can wire it into an
 * AkImage / AkBuffer pair.
 *
 * Why a separate shim and not in-tree libktx: KTX-Software is large
 * (>15 MB built) and pulls in BasisU's substantial codec (~5 MB). Most
 * AssetKit consumers don't ship KTX2-encoded textures; making it
 * runtime-loadable keeps the core build slim. Apps that need it drop
 * the dylib next to their binary.
 */

#include <ktx.h>

#include <ak/assetkit.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#if defined(_WIN32)
#  define AK_KTX2_EXPORT __declspec(dllexport)
#else
#  define AK_KTX2_EXPORT __attribute__((visibility("default")))
#endif

/*--------------------------------------------------------------------*/
/* libktx integration.                                                 */
/*--------------------------------------------------------------------*/

namespace {

/* Return the requested transcode format for our target. RGBA8 is the
   universal-fallback that's CPU-decodable on any platform; on macOS
   we hand the resulting bytes straight to CGBitmapContextCreate /
   SCNMaterialProperty. Future: pick BC7 / ASTC for GPU-direct upload
   when the renderer hints support — out of scope here. */
constexpr ktx_transcode_fmt_e kTargetFormat = KTX_TTF_RGBA32;

int
ktx2_decode_to_rgba8(const uint8_t      *data,
                     size_t              size,
                     AkKTX2DecodedImage *out) {
  if (!data || size == 0 || !out)
    return -1;
  std::memset(out, 0, sizeof(*out));

  ktxTexture2 *tex = nullptr;
  KTX_error_code rc =
      ktxTexture2_CreateFromMemory(data,
                                   size,
                                   KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
                                   &tex);
  if (rc != KTX_SUCCESS || !tex)
    return -2;

  /* Transcode if the texture is BasisU-compressed (UASTC or ETC1S). */
  if (ktxTexture2_NeedsTranscoding(tex)) {
    rc = ktxTexture2_TranscodeBasis(tex, kTargetFormat, 0);
    if (rc != KTX_SUCCESS) {
      ktxTexture_Destroy(ktxTexture(tex));
      return -3;
    }
  }

  const uint32_t baseW    = tex->baseWidth;
  const uint32_t baseH    = tex->baseHeight;
  const uint32_t mipCount = tex->numLevels > 0 ? tex->numLevels : 1;

  /* Two-pass: (1) compute total bytes + per-mip dimensions, (2) allocate
     a single contiguous buffer + copy each mip in. Concatenated layout
     keeps the caller's free() simple (one buffer, one free). */
  AkKTX2MipLevel *mips = (AkKTX2MipLevel *)std::malloc(
      sizeof(AkKTX2MipLevel) * mipCount);
  if (!mips) {
    ktxTexture_Destroy(ktxTexture(tex));
    return -5;
  }

  size_t totalBytes = 0;
  for (uint32_t mip = 0; mip < mipCount; ++mip) {
    const uint32_t w = std::max<uint32_t>(1u, baseW  >> mip);
    const uint32_t h = std::max<uint32_t>(1u, baseH >> mip);
    const size_t   sz = (size_t)w * h * 4;

    mips[mip].width      = w;
    mips[mip].height     = h;
    mips[mip].byteOffset = (uint32_t)totalBytes;
    mips[mip].byteLength = (uint32_t)sz;
    totalBytes          += sz;
  }

  uint8_t *pixels = (uint8_t *)std::malloc(totalBytes);
  if (!pixels) {
    std::free(mips);
    ktxTexture_Destroy(ktxTexture(tex));
    return -6;
  }

  for (uint32_t mip = 0; mip < mipCount; ++mip) {
    ktx_size_t mipOffset = 0;
    rc = ktxTexture_GetImageOffset(ktxTexture(tex), mip, 0, 0, &mipOffset);
    if (rc != KTX_SUCCESS) {
      std::free(pixels);
      std::free(mips);
      ktxTexture_Destroy(ktxTexture(tex));
      return -4;
    }
    std::memcpy(pixels + mips[mip].byteOffset,
                ktxTexture_GetData(ktxTexture(tex)) + mipOffset,
                mips[mip].byteLength);
  }

  out->data       = pixels;
  out->dataLength = totalBytes;
  out->width      = baseW;
  out->height     = baseH;
  out->channels   = 4;
  out->mipCount   = mipCount;
  out->mips       = mips;

  ktxTexture_Destroy(ktxTexture(tex));
  return 0;
}

}  /* anonymous namespace */

extern "C" AK_KTX2_EXPORT
int
assetkit_ktx2_decode(const uint8_t      *data,
                     size_t              size,
                     AkKTX2DecodedImage *out) {
  return ktx2_decode_to_rgba8(data, size, out);
}

extern "C" AK_KTX2_EXPORT
int
assetkit_ktx2_create(AkKTX2Decoder *out) {
  if (!out)
    return -1;
  out->userdata = nullptr;
  out->decode   = ktx2_decode_to_rgba8;
  out->close    = nullptr;
  return 0;
}
