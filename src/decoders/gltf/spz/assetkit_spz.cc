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
 * AssetKit Gaussian Splat decoder shim — SPZ format (Niantic Spatial).
 *
 * Compile: links against `libspz.a` (https://github.com/nianticlabs/spz).
 * Build flag: AK_BUILD_GLTF_SPZ_DECODER. CMake FetchContent pulls SPZ if not
 * pre-installed (mirroring Draco / meshopt patterns).
 *
 * Usage at runtime: AssetKit dlopens this dylib (`AK_OPT_GLTF_GSPLAT_DECODER_PATH`
 * or autoload). When a glTF primitive carries the future
 * `EXT_gsplat_compression_spz` (or any compression sub-extension that names
 * "spz" as the format), the dispatcher calls `decodePrimitive` here. We
 * read the SPZ payload from the referenced bufferView, decode via
 * `spz::loadSpzFromMemory`, then allocate fresh AssetKit accessors and
 * populate the primitive's input chain with standard
 * KHR_gaussian_splatting attributes (POSITION / ROTATION / SCALE / OPACITY /
 * COLOR_0 + SH coefficients in COLOR_1..N if the file ships them).
 *
 * Out-of-band format wrapper note: glTF doesn't yet ratify a per-format
 * compression sub-extension for SPZ. Until it does, apps targeting this
 * decoder should agree on a vendor-prefixed extension name (e.g.
 * `EXT_gsplat_compression_spz`) and pass the bufferView index in
 * `compression.bufferView`. The shim below treats whichever sub-extension
 * the dispatcher hands it as opaque — it just needs the bufferView index.
 */

#include <spz/load-spz.h>

#include <ak/assetkit.h>

#include <cmath>
#include <memory>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

#if defined(_WIN32)
#  define AK_GSPLAT_EXPORT __declspec(dllexport)
#else
#  define AK_GSPLAT_EXPORT __attribute__((visibility("default")))
#endif

/*--------------------------------------------------------------------*/
/* AssetKit accessor + buffer helpers, scoped to this shim. We mirror */
/* the Draco shim's storage strategy: each decoded attribute lives in */
/* its own AkBuffer + AkAccessor pair, attached to the primitive's    */
/* input chain. Lifetime is the primitive's heap parent.              */
/*--------------------------------------------------------------------*/

namespace {

constexpr size_t kFloatSize = sizeof(float);

struct GLTFStateLite {
  AkHeap *heap;
  void   *doc;
  /* unused remainder; we only touch `heap` and the document's
     buffer/accessor flists which we don't access directly here */
};

/* Allocate an AkBuffer of given byte length under the primitive heap;
   copy the source vector into it. */
AkBuffer*
make_buffer_from_vector(AkHeap *heap, void *parent,
                        const std::vector<float> &src) {
  const size_t   bytes = src.size() * kFloatSize;
  AkBuffer      *buf   = (AkBuffer *)ak_heap_calloc(heap, parent, sizeof(AkBuffer));
  if (!buf) return nullptr;
  buf->length = bytes;
  buf->data   = ak_heap_alloc(heap, buf, bytes);
  if (!buf->data) return nullptr;
  std::memcpy(buf->data, src.data(), bytes);
  return buf;
}

/* Single-component-type accessor over a buffer of `count` × `comps` floats. */
AkAccessor*
make_float_accessor(AkHeap *heap, void *parent,
                    AkBuffer *buf, uint32_t count, uint32_t comps) {
  AkComponentSize componentSize = AK_COMPONENT_SIZE_SCALAR;

  if (comps == 2)
    componentSize = AK_COMPONENT_SIZE_VEC2;
  else if (comps == 3)
    componentSize = AK_COMPONENT_SIZE_VEC3;
  else if (comps == 4)
    componentSize = AK_COMPONENT_SIZE_VEC4;

  AkAccessor *acc = (AkAccessor *)ak_heap_calloc(heap, parent, sizeof(AkAccessor));
  if (!acc) return nullptr;
  acc->buffer            = buf;
  acc->byteOffset        = 0;
  acc->count             = count;
  acc->bytesPerComponent = (uint32_t)kFloatSize;
  acc->componentSize     = componentSize;
  acc->componentType     = AKT_FLOAT;
  acc->componentCount    = comps;
  acc->fillByteSize      = (size_t)comps * kFloatSize;
  acc->byteStride        = acc->fillByteSize;
  acc->byteLength        = (size_t)count * acc->fillByteSize;
  acc->originalComponentType = AKT_FLOAT;
  return acc;
}

/* Append an input to the primitive's input chain. Caller holds the head;
   we do simple prepend to match how the rest of the loader builds it. */
AkInput*
prepend_input(AkHeap *heap, AkMeshPrimitive *prim,
              AkAccessor *acc, AkInputSemantic sem,
              const char *semanticRaw, uint32_t set) {
  AkInput *inp = (AkInput *)ak_heap_calloc(heap, prim, sizeof(AkInput));
  if (!inp) return nullptr;
  inp->accessor    = acc;
  inp->semantic    = sem;
  inp->semanticRaw = ak_heap_strdup(heap, inp, semanticRaw);
  inp->set         = set;

  inp->next   = prim->input;
  prim->input = inp;
  prim->inputCount++;
  return inp;
}

}  /* anonymous namespace */

/*--------------------------------------------------------------------*/
/* Decoder entrypoint — invoked by AssetKit when a primitive's        */
/* gaussian-splat compression sub-extension references SPZ data.      */
/*                                                                     */
/* Args:                                                               */
/*   gst           — AkGLTFState* (opaque to us; we only need heap)   */
/*   prim          — primitive to populate                             */
/*   jprim         — primitive JSON (unused, kept for parity)          */
/*   jcompression  — the sub-extension JSON (carries `bufferView`)    */
/*--------------------------------------------------------------------*/

/* Forward declarations so `assetkit_gsplat_create` can wire both function
   pointers in the decoder struct regardless of definition order below. */
extern "C" AK_GSPLAT_EXPORT
int
ak_spz_decodeBytes(AkHeap            *heap,
                   AkMeshPrimitive   *prim,
                   const uint8_t     *data,
                   size_t             size);

extern "C" AK_GSPLAT_EXPORT
int
ak_spz_decodePrimitive(struct AkGLTFState    *gst_opaque,
                       AkMeshPrimitive       *prim,
                       const struct json_t   * /*jprim*/,
                       const struct json_t   *jcompression) {
  if (!gst_opaque || !prim || !jcompression)
    return -1;

  /* Pull `heap` out of AkGLTFState. The state's first field is a heap
     pointer (matches the layout the Draco shim relies on). */
  AkHeap *heap = *reinterpret_cast<AkHeap **>(gst_opaque);

  /* Locate the bufferView the compression sub-extension points at,
     resolve to its raw bytes. The dispatcher should have surfaced the
     bufferView pointer already; we assume the sub-extension provides
     a "bufferView": <int> field. AssetKit's flist of bufferViews is
     reachable but indexing requires the full state — defer that to the
     dispatcher in ext.c, which calls us with `jcompression` already
     containing pre-resolved buffer pointer + size in fields the spec
     locks down once approved. For now we expect a `data` ptr + `size`
     callback wired by the dispatcher. */
  /* TODO: once a per-format compression sub-extension is ratified, the
     dispatcher in src/io/gltf/imp/core/ext.c will resolve the
     bufferView and pass `data`+`size` here directly. The skeleton
     above leaves that wiring open. */
  (void)heap;
  (void)prim;
  return -1;  /* not yet wired — bufferView resolution path pending spec */
}

extern "C" AK_GSPLAT_EXPORT
int
assetkit_gsplat_create(AkGaussianSplatDecoder *out) {
  if (!out)
    return -1;
  out->userdata        = nullptr;
  out->decodeBytes     = ak_spz_decodeBytes;       /* preferred path */
  out->decodePrimitive = ak_spz_decodePrimitive;   /* legacy fallback */
  out->close           = nullptr;
  return 0;
}

/*--------------------------------------------------------------------*/
/* SPZ → AssetKit conversion (helper used by the decoder once the     */
/* dispatcher delivers raw bytes). Public so a host integration test  */
/* can exercise the path with a known SPZ blob.                       */
/*--------------------------------------------------------------------*/

extern "C" AK_GSPLAT_EXPORT
int
ak_spz_decodeBytes(AkHeap            *heap,
                   AkMeshPrimitive   *prim,
                   const uint8_t     *data,
                   size_t             size) {
  if (!heap || !prim || !data || size == 0)
    return -1;

  spz::GaussianCloud cloud;
  try {
    spz::UnpackOptions opts;
    cloud = spz::loadSpz(data, size, opts);
  } catch (...) {
    return -2;
  }
  if (cloud.numPoints <= 0)
    return -3;

  const uint32_t n = static_cast<uint32_t>(cloud.numPoints);

  /* libspz returns *encoded* fields per the SPZ format convention:
       scales : log-scale       → caller must exp(x)
       alphas : pre-sigmoid       → caller must sigmoid(x) = 1/(1+exp(-x))
       colors : SH DC component   → caller must 0.5 + 0.282095·x  (clamp 0..1)
     We decode in-shim so AssetKit consumers see standard renderer-ready
     values: scale (linear units), opacity (0..1), color (0..1 RGB).
     Higher-order SH coefficients pass through unchanged — renderers that
     care evaluate them direction-dependent.
     NaN/Inf guards: corrupted SPZ payloads or extreme outliers in legit
     data can produce NaN after exp() or sigmoid() when raw values are
     beyond representable range. We sanitize each field as it's converted
     so a single bad splat doesn't blow out the whole renderer. */
  auto sanitize = [](float v, float fallback) {
    return std::isfinite(v) ? v : fallback;
  };

  /* POSITION (vec3) — sanitize NaN/Inf to origin (visually clusters
     bad splats at the model origin where they're easy to spot). */
  {
    std::vector<float> safePos(cloud.positions.size());
    for (size_t i = 0; i < cloud.positions.size(); ++i)
      safePos[i] = sanitize(cloud.positions[i], 0.0f);
    if (auto *buf = make_buffer_from_vector(heap, prim, safePos)) {
      auto *acc = make_float_accessor(heap, prim, buf, n, 3);
      prepend_input(heap, prim, acc, AK_INPUT_POSITION, "POSITION", 0);
    }
  }

  /* ROTATION (vec4 quat xyzw) — sanitize NaN to identity quat (0,0,0,1).
     A degenerate quat can collapse to a non-rotation, but identity
     means the splat aligns with object axes which is renderable. */
  {
    std::vector<float> safeRot(cloud.rotations.size());
    for (size_t i = 0; i < cloud.rotations.size(); i += 4) {
      float x = sanitize(cloud.rotations[i + 0], 0.0f);
      float y = sanitize(cloud.rotations[i + 1], 0.0f);
      float z = sanitize(cloud.rotations[i + 2], 0.0f);
      float w = sanitize(cloud.rotations[i + 3], 1.0f);
      // If the quat collapsed (all zeros after sanitize), force identity.
      if (x == 0.0f && y == 0.0f && z == 0.0f && w == 0.0f) {
        w = 1.0f;
      }
      safeRot[i + 0] = x;
      safeRot[i + 1] = y;
      safeRot[i + 2] = z;
      safeRot[i + 3] = w;
    }
    if (auto *buf = make_buffer_from_vector(heap, prim, safeRot)) {
      auto *acc = make_float_accessor(heap, prim, buf, n, 4);
      prepend_input(heap, prim, acc, AK_INPUT_OTHER, "ROTATION", 0);
    }
  }

  /* SCALE (vec3) — decode log → linear via exp(x). Clamp the log range
     before exp() so an extreme value doesn't produce inf scale. Real
     splat scales are sub-meter; even log(100) = ~4.6 is generous. */
  {
    std::vector<float> linearScales(cloud.scales.size());
    for (size_t i = 0; i < cloud.scales.size(); ++i) {
      float logS = sanitize(cloud.scales[i], 0.0f);  // 0 in log → 1 linear
      // Clamp log to ±15 (linear range ≈ 3·10⁻⁷ … 3·10⁶). Beyond this
      // is virtually-certainly bad data.
      if (logS < -15.0f) logS = -15.0f;
      if (logS >  15.0f) logS =  15.0f;
      linearScales[i] = std::exp(logS);
    }
    if (auto *buf = make_buffer_from_vector(heap, prim, linearScales)) {
      auto *acc = make_float_accessor(heap, prim, buf, n, 3);
      prepend_input(heap, prim, acc, AK_INPUT_OTHER, "SCALE", 0);
    }
  }

  /* OPACITY (float) — decode logit → 0..1 via sigmoid. NaN logit defaults
     to 0 (sigmoid(0) = 0.5 ≈ "moderately visible"). */
  {
    std::vector<float> opacities(cloud.alphas.size());
    for (size_t i = 0; i < cloud.alphas.size(); ++i) {
      const float x = sanitize(cloud.alphas[i], 0.0f);
      // sigmoid(x) on extreme x is fine: sigmoid(±∞) → 0/1. We sanitized
      // for NaN; ±Inf path produces clean 0 or 1.
      opacities[i] = 1.0f / (1.0f + std::exp(-x));
    }
    if (auto *buf = make_buffer_from_vector(heap, prim, opacities)) {
      auto *acc = make_float_accessor(heap, prim, buf, n, 1);
      prepend_input(heap, prim, acc, AK_INPUT_OTHER, "OPACITY", 0);
    }
  }

  /* COLOR_0 (vec3) — decode SH DC component → 0..1 RGB.
     Formula `0.5 + 0.282095·x` is the SH band-0 inverse normalization
     constant (1 / (2·sqrt(π)) ≈ 0.282095). Clamp to [0,1] to guard
     against extrapolation on splats with extreme DC values; sanitize
     NaN→0 first so the clamp produces 0.5 (mid-gray fallback). */
  {
    std::vector<float> rgb(cloud.colors.size());
    constexpr float kSH0 = 0.282094791773878f;  /* 1 / (2*sqrt(pi)) */
    for (size_t i = 0; i < cloud.colors.size(); ++i) {
      float dc = sanitize(cloud.colors[i], 0.0f);
      float v  = 0.5f + kSH0 * dc;
      if (v < 0.0f) v = 0.0f;
      if (v > 1.0f) v = 1.0f;
      rgb[i] = v;
    }
    if (auto *buf = make_buffer_from_vector(heap, prim, rgb)) {
      auto *acc = make_float_accessor(heap, prim, buf, n, 3);
      prepend_input(heap, prim, acc, AK_INPUT_COLOR, "COLOR", 0);
    }
  }

  /* Higher-order SH coefficients (COLOR_1..COLOR_N) — vec3 each, packed
     as `numPoints × shDim × 3`. shDim depends on the spherical-harmonic
     degree (degree 1 → 3 dirs, degree 2 → 8 dirs, degree 3 → 15 dirs).
     We split into per-dir accessors so renderers that read COLOR_n can
     fetch any subset. NaN sanitize → 0 so a bad coefficient gives no
     view-dependent contribution rather than blowing out colors. */
  if (cloud.shDegree > 0 && !cloud.sh.empty()) {
    const int shDim = cloud.shDegree * (cloud.shDegree + 2);
    for (int d = 0; d < shDim; ++d) {
      std::vector<float> band(static_cast<size_t>(n) * 3);
      for (uint32_t i = 0; i < n; ++i) {
        const size_t srcBase = (size_t)i * (size_t)shDim * 3 + (size_t)d * 3;
        const size_t dstBase = (size_t)i * 3;
        band[dstBase + 0] = sanitize(cloud.sh[srcBase + 0], 0.0f);
        band[dstBase + 1] = sanitize(cloud.sh[srcBase + 1], 0.0f);
        band[dstBase + 2] = sanitize(cloud.sh[srcBase + 2], 0.0f);
      }
      auto *buf = make_buffer_from_vector(heap, prim, band);
      auto *acc = make_float_accessor(heap, prim, buf, n, 3);
      prepend_input(heap, prim, acc, AK_INPUT_COLOR, "COLOR",
                    static_cast<uint32_t>(d + 1));
    }
  }

  return 0;
}
