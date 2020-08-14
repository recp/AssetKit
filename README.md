<p align="center">
   <img alt="" src="assetkit.png" width="550" />
</p>
<br>

<p align="center">
    <a href="https://travis-ci.org/recp/assetkit">
        <img src="https://travis-ci.org/recp/assetkit.svg?branch=master"
             alt="Build Status">
    </a>
    <a href="https://ci.appveyor.com/project/recp/assetkit">
        <img src="https://ci.appveyor.com/api/projects/status/e52s0fr8mnbmsqy4?svg=true"
             alt="Build Status">
    </a>
    <a href="https://www.codacy.com/app/recp/assetkit?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=recp/assetkit&amp;utm_campaign=Badge_Grade">
        <img src="https://api.codacy.com/project/badge/Grade/6edde2ba446148759437eb0148c799b6"
             alt="Codacy Badge"/>
    </a>
    <a href="https://coveralls.io/github/recp/assetkit?branch=master">
        <img src="https://coveralls.io/repos/github/recp/assetkit/badge.svg?branch=master"
             alt="Coverage Status"/>
    </a>
    <img src="https://img.shields.io/badge/glTF-2%2E0-green.svg?style=flat"
         alt="glTF Badge">
    <br /><br />
    <a href="https://patreon.com/recp">
      <img src="https://img.shields.io/badge/Patreon-Become a patron-orange.svg"
           alt="Patreon: Become a patron">
    </a>
</p>

<br>

<p align="center">
Brand-new modern 3D asset importer, exporter library. This library will include common 3D utils funcs. It is written with C99 but C++ wrappers or other language bindings can be written in the future.

This library will try to full support COLLADA specs and glTF specs, plus well-known other 3D formats e.g .obj, .stl, .ply... 

There is also an optional renderer library called [libgk](https://github.com/recp/libgk) for render AssetKit contents. You can see how to load AssetKit to libgk in [assetkit-gl](https://github.com/recp/assetkit-gl) repo.

</p>

#### Documentation (In Progress)

Almost all functions (inline versions) and parameters will be documented inside the corresponding headers. <br />
Complete documentation: http://assetkit.readthedocs.io

## Supported Formats

* [x] COLLADA 1.4 and COLLADA 1.4.1
* [x] COLLADA 1.5
* [x] glTF 2.0 (Embedded or Separated (.gltf), Binary (.glb), Extensions...)
* [x] Wavefront Obj (.obj + .mtl)
* [x] STL (ASCII, Binary)
* [x] PLY (ASCII, Binary)
* [ ] USD and friends (License?)
* [ ] Alembic (License?)
* [ ] Draco
* [ ] X3D
* [x] in progress for next...

## Features

- Single interface for glTF 2.0 (with extensions), COLLADA 1.4/1.4.1/1.5, Wavefront Obj and others...
- Very very small and very fast library
- Javascript-like API to get URL or ID `obj = ak_getObjectById(doc, objectId)`...
- Options to Generate Mesh Normals *(Default: enabled)*
- Option to Triangulate Polygons *(Default: enabled)*
- Option to change Coordinate System *(Default: enabled)*
- Option to calculate Bounding Boxes *(Default: enabled)*
- Unique and Flexible Coordinate System
  - Support multiple coordinate system
  - Can convert any coordinate system to another with adding transform or with changing transform, vertex data...
- Unique and Flexible Memory Management System
  - Hierarchical unique memory management
    - When a node is freed then all sub memories will be freed
  - COLLADA's **sid** and **ID** values are mapped to memory nodes itself to reduce memory size and make it easy to manage things.
  - Allow attach ID, sid or user data to a memory node
- Object-based Asset support; resolve asset element for any element
- Bugfix some DAE files
- Will be optimized to be fastest, smallest and most flexible, extendible Asset loader.
- Uses **mmap** to load files, you can disable this if needed
- [ ] Documentation
- [x] Cmake support
- [ ] Tests

## Build

### CMake (All platforms)
```bash
$ mkdir build
$ cd build
$ cmake .. # [Optional] -DAK_SHARED=ON
$ make
$ sudo make install # [Optional]
```

##### Cmake options with Defaults:

```CMake
option(AK_SHARED "Shared build" ON)
option(AK_STATIC "Static build" OFF)
option(AK_USE_TEST "Enable Tests" OFF) # for make check - make test
```

#### Use with your CMake project
* Example:
```cmake
cmake_minimum_required(VERSION 3.8.2)

project(<Your Project Name>)

add_executable(${PROJECT_NAME} src/main.c)
target_link_libraries(${LIBRARY_NAME} PRIVATE assetkit)

add_subdirectory(external/assetkit/)

# or you can use find_package() to configure assetkit
```

### Unix (Autotools)
Step 1: First you should build dependencies, do this only once:
```bash
$ sh ./build-deps.sh
```

Step 2: Build, Test and Install AssetKit
```bash
$ sh autogen.sh
$ ./configure
$ make
$ make check
$ [sudo] make install
```

Step 3: Change install name if required, after make finished make automaticall runs `sh ./post-build.sh` script. It changes install names. You may want to edit build scripts and `post-build.sh` script if you want to build AssetKit with existing libraries. Default behavior is that AssetKit will look up sub libraries inside `.libs` folder, if you only need to change `.libs` name then change it in `post-build.sh` script file.

### Windows (MSBuild)
Windows related build files, project files are located in `win` folder, make sure you are inside `assetkit/win` folder. Code Analysis are enabled to it may take awhile to build

`git` and `python` commands should be installed/accessible.

```Powershell
$ cd win
$ .\build.bat
$ msbuild assetkit.vcxproj /p:Configuration=Release
```
if `msbuild` won't work correctly then try to build with `devenv`:
```Powershell
$ devenv assetkit.sln /Build Release
```

### Trademarks

glTF and COLLADA and their logos are trademarks of Khronos Group.
