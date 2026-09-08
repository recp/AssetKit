# Extensions

AssetKit handles glTF extensions in three ways:

- Built-in support for extensions that affect AssetKit's runtime model.
- Optional side decoder libraries for compressed payloads.
- Preserved JSON payload through `ak_extra()` for app/vendor-specific data.

## Required Extensions

`extensionsRequired` is strict. If an extension is required for correct
geometry, animation, texture, material, or splat data, AssetKit must implement
it or reject the asset.

`extensionsUsed` can be more permissive. If an optional extension is not needed
for correct loading, AssetKit may preserve the payload in `ak_extra()` so a
viewer/tool can inspect it.

## Optional Decoder Libraries

Draco, meshoptimizer, SPZ and KTX2/BasisU decoding use optional libraries.
AssetKit loads these side libraries only when needed:

- `libassetkit_draco`
- `libassetkit_meshoptimizer`
- `libassetkit_spz`
- `libassetkit_ktx2`

On Windows they are `assetkit_*.dll`; on macOS `libassetkit_*.dylib`; on Linux
`libassetkit_*.so`. AssetKit first searches next to the loaded `libassetkit`
binary, then falls back to the platform loader search path.

CMake builds the side libraries by default when dependencies are available or
can be fetched:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Useful options:

```cmake
-DAK_FETCH_DEPS=OFF
-DAK_BUILD_GLTF_DRACO_DECODER=OFF
-DAK_BUILD_GLTF_MESHOPT_DECODER=OFF
-DAK_BUILD_GLTF_SPZ_DECODER=OFF
-DAK_BUILD_GLTF_KTX2_DECODER=OFF
```

## L-GSC

AssetKit imports standalone `.lgsc` files and the glTF/GLB extension
`KHR_gaussian_splatting_compression_lgsc`. The shared decoder is native C,
uses libdeflate, and needs no side library or C++ runtime.

```c
AkDoc *doc;

if (ak_load(&doc, "capture.lgsc", AK_FILE_TYPE_AUTO) != AK_OK)
  return 1;

/* Read Gaussian attributes through the mesh primitive's inputs. */
ak_free(doc);
```

Decoded position, rotation, scale, opacity and RGB spherical harmonics use
normal `AkAccessor` objects. Metadata is in `AkMeshPrimitive.gsplat`.
One buffer holds the decoded attributes; primitives sharing a compressed
glTF buffer view share its decoded storage. Other primitive attributes,
such as `COLOR_0`, remain available.

The glTF extension is a draft in [Khronos PR #2551](https://github.com/KhronosGroup/glTF/pull/2551).
This implementation follows revision `90fefa0c9d49c52f8811b49fa5972afc56969db0`
and the L-GSC v1.0 reference bitstream. Standalone files contain degree-three
SH fields; glTF metadata selects degree 0–3. See
[the importer notes](src/io/lgsc/README.md) for storage and coordinate conventions.

## Typed Extension Data

Some extensions are represented as normal AssetKit fields:

- `KHR_materials_variants`
  - `AkDoc.materialVariants`
  - `AkMeshPrimitive.variantMappings`
  - `ak_materialVariantByName()`

- `KHR_gaussian_splatting`
  - `AkMeshPrimitive.gsplat`
  - splat attributes stay in the primitive input chain

- `KHR_animation_pointer`
  - maps supported JSON pointer targets to AssetKit animation targets

SPZ and L-GSC are storage formats. Public Gaussian splat metadata is generic
and lives in `AkGaussianSplat`; attributes use the primitive's input chain.

More detail: [docs/source/extensions.rst](docs/source/extensions.rst)
