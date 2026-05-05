# Extensions

AssetKit handles glTF extensions in three ways:

- Typed support for extensions that affect AssetKit's runtime model.
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

Compression and heavy decoders live outside the main C library. AssetKit loads
these side libraries only when needed:

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

SPZ is only a decoder format. Public Gaussian splat data is generic and lives
in `AkGaussianSplat`.

More detail: [docs/source/extensions.rst](docs/source/extensions.rst)
