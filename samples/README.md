# AssetKit Samples

These samples are small, copyable C programs for common read paths. They answer
the minimal usage request from issue #32 without pulling benchmark or tool code
into user projects.

Build from the repository root:

```sh
cmake -S . -B build -DAK_BUILD_SAMPLES=ON
cmake --build build --target assetkit_sample_read_meshes
cmake --build build --target assetkit_sample_inspect_materials
cmake --build build --target assetkit_sample_inspect_scene
```

Build as a standalone project after installing AssetKit:

```sh
cmake -S samples -B samples-build -DCMAKE_PREFIX_PATH=/path/to/assetkit/install
cmake --build samples-build
```

Run:

```sh
./build/assetkit_sample_read_meshes path/to/model.gltf
./build/assetkit_sample_inspect_materials path/to/model.gltf
./build/assetkit_sample_inspect_scene path/to/model.gltf
```

Samples:

- `read_meshes.c` loads a document, walks scene node geometry instances, reads
  mesh primitive attributes, and handles both materialized indices and source
  index accessors.
- `inspect_materials.c` prints PBR/classic material inputs, texture channels,
  feature extensions, variants, and material property sets.
- `inspect_scene.c` prints scenes, node hierarchy, camera and light libraries,
  camera/light scene usage, and animation channels.
