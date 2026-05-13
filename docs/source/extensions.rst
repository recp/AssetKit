glTF Extensions
===============

AssetKit separates glTF extension handling into three paths:

* typed support for extensions that affect AssetKit's runtime model
* optional side decoders for compressed payloads
* preserved JSON payload through :c:func:`ak_extra`

This keeps the core C library small while still letting applications inspect
or handle metadata that AssetKit does not consume directly.

Supported vs Preserved
----------------------

If an extension is needed to load correct geometry, animation, texture or
material data, AssetKit must either implement it or reject the asset when the
extension appears in ``extensionsRequired``.

If an extension appears only in ``extensionsUsed`` and AssetKit does not need
it for correct loading, the payload can be preserved in ``ak_extra()``. This
lets higher-level tools or viewers handle vendor-specific data without making
the core loader depend on every ecosystem extension.

Optional Decoder Libraries
--------------------------

Compression extensions are handled by side libraries. The main
``libassetkit`` does not link these libraries directly. At runtime AssetKit
loads them when needed:

* ``libassetkit_draco`` for Draco mesh compression
* ``libassetkit_meshoptimizer`` for meshoptimizer buffers
* ``libassetkit_spz`` for compressed Gaussian splats
* ``libassetkit_ktx2`` for KTX2/BasisU textures

On Windows the same names use ``.dll`` without the ``lib`` prefix. On macOS
they use ``.dylib`` and on Linux they use ``.so``. AssetKit first searches next
to the loaded ``libassetkit`` binary, then falls back to the platform loader
search path.

CMake builds these side libraries by default when decoder dependencies are
available or can be fetched:

.. code-block:: bash

  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build

Use these options to control that behavior:

.. code-block:: cmake

  -DAK_FETCH_DEPS=OFF
  -DAK_BUILD_GLTF_DRACO_DECODER=OFF
  -DAK_BUILD_GLTF_MESHOPT_DECODER=OFF
  -DAK_BUILD_GLTF_SPZ_DECODER=OFF
  -DAK_BUILD_GLTF_KTX2_DECODER=OFF

Material Variants
-----------------

``KHR_materials_variants`` is represented on the document and primitive:

* ``AkDoc.materialVariants`` stores variant names in source order.
* ``AkMeshPrimitive.variantMappings`` stores per-primitive material overrides.

Use :c:func:`ak_materialVariantByName` when a UI needs to resolve a variant by
name.

Gaussian Splatting
------------------

Gaussian splat metadata is stored in ``AkGaussianSplat`` on
``AkMeshPrimitive.gsplat``. The primitive keeps normal point topology and
attribute inputs, so applications without a splat renderer can still inspect
or draw the data as a point cloud.

SPZ is only a decoder format. The public ``AkGaussianSplat`` model is generic
and should not expose SPZ-specific details.

Animation Pointer
-----------------

``KHR_animation_pointer`` maps glTF JSON pointer targets into AssetKit
animation target records where possible. Typed support is preferred for
properties that affect runtime animation. Unknown or unsupported pointer
payloads should remain available through the preserved extension tree.

Custom Extensions
-----------------

Custom application extensions should be stored in glTF ``extensions`` or
``extras`` as usual. AssetKit preserves that data in ``ak_extra()``. If the
extension is required for correct rendering, applications should verify that
they understand it before rendering the asset.
