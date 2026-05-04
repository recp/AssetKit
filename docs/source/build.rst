Build AssetKit
================================

| **AssetKit** core uses bundled submodules. Optional glTF decoder side
  libraries can fetch their decoder dependencies when enabled.

CMake (All platforms):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ cmake -S . -B build
  $ cmake --build build
  $ cmake --install build # [Optional]

**make** will build **AssetKit** into **build** folder.
If you don't want to install **AssetKit** to your system's folder you can get static and dynamic libs in this folder.

**CMake Options:**

.. code-block:: CMake
  :linenos:

  option(AK_SHARED "Shared build" ON)
  option(AK_STATIC "Static build" OFF)
  option(AK_USE_C99 "" OFF) # C11 
  option(AK_USE_TEST "Enable Tests" OFF) # for make check - make test
  option(AK_BUILD_GLTF_DRACO_DECODER "Build optional glTF Draco decoder shim" ON)
  option(AK_BUILD_GLTF_MESHOPT_DECODER "Build optional glTF meshoptimizer decoder shim" ON)
  option(AK_FETCH_DEPS "Fetch optional decoder dependencies into AK_DEPS_ROOT when missing" ON)

**Use with your CMake project example**

.. code-block:: CMake
  :linenos:

  cmake_minimum_required(VERSION 3.8.2)
  
  project(<Your Project Name>)
  
  add_executable(${PROJECT_NAME} src/main.c)
  target_link_libraries(${LIBRARY_NAME} PRIVATE assetkit)
  
  add_subdirectory(external/assetkit/)

Optional glTF compression decoders:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ cmake -S . -B build
  $ cmake --build build

Decoder side libraries are built next to ``libassetkit`` by default, but not
linked into the main C library. CMake fetches missing decoder dependencies into
``deps/`` by default. Use ``-DAK_FETCH_DEPS=OFF`` with ``AK_DRACO_ROOT`` /
``AK_MESHOPT_ROOT`` for offline or packaged builds. Use
``-DAK_BUILD_GLTF_DRACO_DECODER=OFF`` or
``-DAK_BUILD_GLTF_MESHOPT_DECODER=OFF`` to skip these side libraries.

Windows:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Windows builds use CMake.

.. code-block:: bash
  :linenos:

  $ cmake -S . -B build
  $ cmake --build build --config Release

Currently tests are not available on Windows.

Documentation (Sphinx):
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**AssetKit** uses sphinx framework for documentation, it allows lot of formats for documentation. To see all options see sphinx build page:

https://www.sphinx-doc.org/en/master/man/sphinx-build.html

Example build:

.. code-block:: bash
  :linenos:

  $ cd assetkit/docs
  $ sphinx-build source build
