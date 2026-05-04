Build AssetKit
================================

| **AssetKit** core uses bundled submodules. Optional glTF decoder side
  libraries can fetch their decoder dependencies when enabled. The same CMake
  project can be used standalone, as a subdirectory in another CMake project,
  or as an installed package.

Standalone CMake build:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash
  :linenos:

  $ cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  $ cmake --build build
  $ cmake --install build # [Optional]

With multi-config generators such as Visual Studio or Xcode:

.. code-block:: bash
  :linenos:

  $ cmake -S . -B build
  $ cmake --build build --config Release
  $ cmake --install build --config Release # [Optional]

The build folder contains the main AssetKit library and, by default, optional
glTF decoder side libraries.

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
  option(AK_ENABLE_LTO "Enable link-time optimization for release builds" OFF)

``AK_ENABLE_LTO`` is optional. Release builds already use optimized compiler
flags; LTO is off by default because support differs by compiler, generator and
platform.

Embedded in another CMake project:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: CMake
  :linenos:

  cmake_minimum_required(VERSION 3.16)
  
  project(my_app LANGUAGES C)
  
  add_subdirectory(external/assetkit/)

  add_executable(my_app src/main.c)
  target_link_libraries(my_app PRIVATE assetkit::assetkit)

When embedded, AssetKit does not force a default build type on the parent
project. If dependency fetching is not wanted during parent configure:

.. code-block:: CMake
  :linenos:

  set(AK_FETCH_DEPS OFF CACHE BOOL "" FORCE)
  set(AK_BUILD_GLTF_DRACO_DECODER OFF CACHE BOOL "" FORCE)
  set(AK_BUILD_GLTF_MESHOPT_DECODER OFF CACHE BOOL "" FORCE)
  add_subdirectory(external/assetkit/)

Installed package:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: CMake
  :linenos:

  find_package(assetkit CONFIG REQUIRED)
  target_link_libraries(my_app PRIVATE assetkit::assetkit)

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

Windows builds use CMake. ``git`` must be available when ``GIT_SUBMODULE=ON``
or ``AK_FETCH_DEPS=ON``.

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
