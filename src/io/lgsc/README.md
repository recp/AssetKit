# L-GSC import

Both `.lgsc` and `KHR_gaussian_splatting_compression_lgsc` use `decode.c`.
The decoder is native C and uses AssetKit's existing libdeflate dependency.
No Qualcomm code is linked into AssetKit.

The bitstream follows [lite-3Dgsplat-codec](https://github.com/qualcomm/lite-3Dgsplat-codec)
revision `a78ee6edeb17712d2b81936db41b9fd2bfac5b88` (L-GSC v1.0).
The glTF binding follows [PR #2551](https://github.com/KhronosGroup/glTF/pull/2551)
revision `90fefa0c9d49c52f8811b49fa5972afc56969db0`; the extension is still a draft.

## Storage

Three little-endian, uint32-length-prefixed segments form one frame:

- A 487-byte plain header: point count, ten codec parameters, and float32
  min/max pairs for 59 scalar fields.
- Gzip-compressed quantized positions.
- Gzip-compressed quantized DC, SH rest, opacity, scale and rotation fields.

The decoder handles 1–24-bit quantization, point- and dimension-major layouts,
packed quaternions, sigmoid opacity, SH channel rearrangement and SH YUV coding.
Gzip sizes, stream lengths, CRCs, quantized ranges and header bounds are checked.
The reference encoder's non-gzip and combined-segment options are not serialized
in this version; files must use the reference decoder's separate gzip segments.

## Runtime data

Position, XYZW quaternion, linear scale, opacity and RGB SH coefficients are
float accessors in one document-owned buffer. A temporary quantized buffer is
reused for both gzip streams; SH rearrangement uses a bounded stack tile.
There are no per-splat allocations or full-size intermediate float arrays.

Standalone files have no coordinate or SH-degree metadata. AssetKit treats
them as INRIA/Graphdeco RDF data, converts to its Y-up LUF convention, and
exposes all degree-three fields. Scale is exponentiated and opacity is mapped
to [0, 1]. Quaternion and SH orientation follow the coordinate conversion.

For glTF, the payload uses the container's coordinate convention and
`shDegree` selects degree 0–3. Compressed accessors retain their identities;
their placeholder buffer views and offsets are not read. Each compressed
buffer view is decoded once at the highest degree its primitives request.
Normal coordinate-conversion options apply after either import path.

L-GSC export is not provided.
