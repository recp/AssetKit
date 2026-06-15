# AssetKit Samples

These samples are small, copyable C programs for common read paths. They answer
the minimal usage request from issue #32 without pulling benchmark or tool code
into user projects.

Build from the repository root:

```sh
cmake -S . -B build -DAK_BUILD_SAMPLES=ON
cmake --build build --target assetkit_samples
cmake --build build --target assetkit_samples_help
```

Build as a standalone project after installing AssetKit:

```sh
cmake -S samples -B samples-build -DCMAKE_PREFIX_PATH=/path/to/assetkit/install
cmake --build samples-build
cmake --build samples-build --target assetkit_samples_help
```

The help target prints the exact executable paths for the current build. When
building from the repository root, the executables normally appear as
`./build/sample_*`.

Run the readers with any supported asset path. glTF/GLB is a good first test
because public sample assets are easy to find, but the same readers can also
load DAE, OBJ, STL, PLY, and 3MF when those importers are enabled:

```sh
./build/sample_inspect_scene /path/to/model.gltf
./build/sample_read_meshes /path/to/model.glb
./build/sample_inspect_materials /path/to/model.dae
./build/sample_inspect_textures /path/to/model.gltf
./build/sample_inspect_animation /path/to/animated-model.gltf
./build/sample_inspect_deformers /path/to/skinned-or-morphed-model.gltf
./build/sample_deformer_playback /path/to/skinned-or-morphed-model.gltf
./build/sample_export_scene /path/to/model.gltf /tmp/assetkit-export obj
./build/sample_gl_basic /path/to/model.gltf
./build/sample_gl_material /path/to/textured-model.gltf
./build/sample_gl_lighting /path/to/model-with-lights.gltf
```

Samples:

- `read_meshes.c` loads a document, walks scene node geometry instances, reads
  mesh primitive attributes, and handles both materialized indices and source
  index accessors.
- `inspect_materials.c` prints PBR/classic material inputs, texture channels,
  feature extensions, variants, and material property sets.
- `inspect_textures.c` prints texture usages from material inputs, channel
  masks, color space, source storage, and unused texture/image library entries.
- `inspect_scene.c` prints scenes, node hierarchy, camera and light libraries,
  camera/light scene usage, and animation channels.
- `inspect_animation.c` prints animation channels and samplers, asks AssetKit
  for a conflict-free playback set, and previews baked node-local matrices.
- `inspect_deformers.c` prints skin and morph instances, calls the
  format-agnostic skin-weight helper, and previews morph interleave buffers.
- `deformer_playback.c` is a compact cglm-based runtime preview for skin
  weights, morph interleave buffers, and baked animation matrices. If cglm is
  missing, CMake skips this optional target.
- `export_scene.c` loads a model, applies per-format export options, writes
  DAE/glTF/GLB/OBJ/STL/PLY/3MF, and attempts to reload the exported main file.
- `gl/01_mesh` is the optional OpenGL 3.3 mesh sample behind
  `sample_gl_basic`. It asks AssetKit for a single-index triangle
  primitive and uploads vertex/index buffers.
- `gl/02_material` resolves base material color and, when `libpng` is available
  at build time, uploads base-color PNG textures from file or embedded buffers.
- `gl/03_lighting` resolves scene lights with `ak_lightResolve(... PREVIEW)` and
  previews punctual lighting with material color/texture. If OpenGL, `glfw3`,
  `glew`, or cglm are missing, CMake prints a warning and skips GL targets while
  still building the other samples.
