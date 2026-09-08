<p align="center">
   <img alt="AssetKit" src="assetkit.png" width="550" />
</p>
<br>

<p align="center">
    <a href="https://github.com/recp/AssetKit/actions/workflows/cmake.yml">
        <img src="https://github.com/recp/AssetKit/actions/workflows/cmake.yml/badge.svg?branch=main"
             alt="github actions">
    </a>
    <img src="https://img.shields.io/badge/glTF-2%2E0-green.svg?style=flat"
         alt="glTF Badge">
    <br /><br />
    <a href="https://patreon.com/recp">
      <img src="https://img.shields.io/badge/Patreon-Become a patron-orange.svg"
           alt="Patreon: Become a patron">
    </a>
    <a href="#sponsors">
        <img src="https://opencollective.com/assetkit/sponsors/badge.svg"
             alt="Sponsors on Open Collective"/>
    </a>
    <a href="#backers">
        <img src="https://opencollective.com/assetkit/backers/badge.svg"
             alt="Backers on Open Collective"/>
    </a>
</p>

<br>

<p align="center">
A full-featured 3D asset library in C, designed for low overhead and maximum performance.
Import, export and process scenes, meshes, materials, animations, skins and morph targets through one API.
</p>

### 📚 Documentation

API declarations and inline documentation live in [include/ak](include/ak).
See the [documentation](https://assetkit.readthedocs.io) and
[C samples](samples/README.md) for usage, or the notes on
[extensions and decoders](EXTENSIONS.md) and [extras](EXTRAS.md).

### Supported formats

Import and export:

- glTF 2.0 — `.gltf` and `.glb`, external or embedded resources
- COLLADA 1.4 / 1.5 — `.dae`
- Wavefront OBJ — `.obj` and `.mtl`
- STL — ASCII and binary
- PLY — ASCII and binary; meshes, point clouds and Gaussian splats
- 3MF — packaged models, resources and manufacturing data
- SPZ — standalone `.spz` and glTF Gaussian-splat compression

Draco, meshoptimizer, KTX2/BasisU and SPZ decoding use optional side libraries,
loaded on demand. They are not linked into the core C library.

### 🚀 Features

- Scene hierarchies, shared instances, cameras, lights and GPU instancing.
- Points, lines, polygons, triangle lists, strips and fans; spline and BRep geometry.
- PBR and classic materials, texture transforms, variants and instance bindings;
  clearcoat, sheen, transmission, volume, iridescence and anisotropy.
- Animation clips, sampler evaluation, channel resolution and transform baking.
- Skin weights and morph targets with inspection and interleaving helpers.
- Mesh editing, triangulation, normal generation, index remapping and bounding boxes.
- Coordinate conversion for units, up axis and handedness.
- 3MF build items, components, materials, textures and manufacturing metadata.
- Gaussian splats and compressed splat decoding.
- Hierarchical memory ownership and custom allocators.
- ID, SID and URL resolution; application-defined extras and user data.

AssetKit is renderer-independent. Accessors expose their component type, stride
and normalization directly. Source-backed glTF accessors and u8/u16/u32 indices
avoid unnecessary copies and widening; quantized attributes can be kept in their
original storage. File loading uses memory mapping where available.

### Usage

Include `<ak/assetkit.h>` and link with `assetkit`:

```c
#include <ak/assetkit.h>
#include <stdio.h>

int main(int argc, char **argv) {
  AkGeometry *geom;
  AkScene    *scene;
  AkMesh     *mesh;
  AkNode     *root;
  AkDoc      *doc;

  doc = NULL;
  if (argc != 2 || ak_load(&doc, argv[1], AK_FILE_TYPE_AUTO) != AK_OK)
    return 1;

  if ((scene = ak_activeSceneOrFirst(doc))) {
    printf("scene: %s\n", ak_nameOrUnnamed(scene->name));
    for (root = ak_sceneRoots(scene); root; root = root->next)
      printf("  root: %s\n", ak_nameOrUnnamed(root->name));
  }

  for (geom = doc->lib.geometries.first; geom; geom = geom->next) {
    if (!(mesh = ak_meshFromGeometry(geom)))
      continue;
    printf("mesh: %s, primitives: %u\n", ak_nameOrUnnamed(geom->name), mesh->primitiveCount);
  }

  ak_free(doc);
  return 0;
}
```

The scene and mesh pointers above belong to the document. `ak_free(doc)` releases
the document and everything it owns, including materials, animations and buffers.
Custom builders can use `AkHeapAllocator` and the `ak_heap_*` functions for the
same hierarchical ownership with their own allocator.

Use `ak_exportFile()` to write a loaded document, or `ak_convert()` to load and
export in one call:

```c
if (ak_convert("scene.dae", "scene.glb", AK_FILE_TYPE_AUTO) != AK_OK)
  return 1;
```

More examples: [scene traversal](samples/inspect_scene.c),
[mesh access](samples/read_meshes.c), [materials](samples/inspect_materials.c),
[animation](samples/inspect_animation.c) and [skin/morph](samples/inspect_deformers.c).

### Ownership and color space

Keep document-owned pointers only as long as the document lives. Retaining a
child does not make it survive destruction of its parent. Independently allocated
helper outputs have their own lifetime; follow the function's ownership contract.

`AkColor`, material factors, vertex colors and light colors use linear-sRGB.
Texture `colorSpace` applies to sampled pixels, not these constant values:
color textures use sRGB; normal,
metallic, roughness and other data textures use linear values. Alpha is linear.
Storage type is separate from color space: linear values can still use normalized
integer storage.

### 🔨 Building

CMake 3.16 or newer; macOS, Linux and Windows. The core builds as C11 by default
and exposes a C API usable from C++.

```sh
git clone --recursive https://github.com/recp/assetkit.git
cd assetkit
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
cmake --install build
```

For Visual Studio or Xcode, add `--config Release` to the build and install
commands. For an existing checkout, run `git submodule update --init --recursive`.
Installation is optional.

Common CMake options:

| Option | Default | Purpose |
| --- | --- | --- |
| `AK_SHARED` / `AK_STATIC` | `ON` / `OFF` | Shared or static core library |
| `AK_BUILD_EXPORTERS` | `ON` | Export support; disable for import-only builds |
| `AK_BUILD_DECODER_SHIMS` | `ON` | Optional Draco, meshoptimizer, SPZ and KTX2 decoders |
| `AK_FETCH_DEPS` | `ON` | Fetch missing decoder dependencies into the build tree |
| `AK_BUILD_CLI` | `ON` | Command-line inspection and conversion tool |
| `AK_BUILD_SAMPLES` | `OFF` | C usage examples |
| `AK_ENABLE_LTO` | `OFF` | Link-time optimization |

Exporters and decoder shims can also be disabled individually. See
[CMakeLists.txt](CMakeLists.txt) for all options and
[EXTENSIONS.md](EXTENSIONS.md) for decoder setup. Set `AK_FETCH_DEPS=OFF` for
builds that must not fetch decoder dependencies.

#### CMake integration

From an installed package:

```cmake
find_package(assetkit CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE assetkit::assetkit)
```

Or from a checkout in your project:

```cmake
add_subdirectory(external/assetkit)
target_link_libraries(my_app PRIVATE assetkit::assetkit)
```

#### Command line

After installation:

```sh
assetkit --inspect scene.glb
assetkit --convert scene.dae scene.glb
```

#### Samples

```sh
cmake -S . -B build -DAK_BUILD_SAMPLES=ON
cmake --build build --target assetkit_samples
```

See [samples/README.md](samples/README.md) for individual programs and standalone
sample builds.

### License

[Apache 2.0](LICENSE).

## Contributors

This project exists thanks to all the people who contribute.
<a href="https://github.com/recp/assetkit/graphs/contributors"><img src="https://opencollective.com/assetkit/contributors.svg?width=890&button=false" /></a>


## Backers

Thank you to all our backers! 🙏 [[Become a backer](https://opencollective.com/assetkit#backer)]

<a href="https://opencollective.com/assetkit#backers" target="_blank"><img src="https://opencollective.com/assetkit/backers.svg?width=890"></a>


## Sponsors

Support this project by becoming a sponsor. Your logo will show up here with a link to your website. [[Become a sponsor](https://opencollective.com/assetkit#sponsor)]

<a href="https://opencollective.com/assetkit/sponsor/0/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/0/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/1/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/1/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/2/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/2/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/3/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/3/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/4/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/4/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/5/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/5/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/6/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/6/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/7/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/7/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/8/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/8/avatar.svg"></a>
<a href="https://opencollective.com/assetkit/sponsor/9/website" target="_blank"><img src="https://opencollective.com/assetkit/sponsor/9/avatar.svg"></a>


### Trademarks

glTF and COLLADA and their logos are trademarks of Khronos Group.
