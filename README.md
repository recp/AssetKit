# 🎨 AssetKit
[![Build Status](https://travis-ci.org/recp/assetkit.svg?branch=master)](https://travis-ci.org/recp/assetkit)
[![Build status](https://ci.appveyor.com/api/projects/status/e52s0fr8mnbmsqy4?svg=true)](https://ci.appveyor.com/project/recp/assetkit)
[![Coverage Status](https://coveralls.io/repos/github/recp/assetkit/badge.svg?branch=master)](https://coveralls.io/github/recp/assetkit?branch=master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/6edde2ba446148759437eb0148c799b6)](https://www.codacy.com/app/recp/assetkit?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=recp/assetkit&amp;utm_campaign=Badge_Grade)
![status](https://img.shields.io/badge/glTF-2%2E0-green.svg?style=flat)

3D asset importer, exporter library. This library also includes common 3D utils funcs. It is written with C99 but C++ wrappers or other language bindings can be written in the future.

This library will full support COLLADA specs and glTF specs, plus other 3D formats e.g .obj, .stl maybe supported by sub-libraries e.g. assetkit-stl, assetkit-fbx.

There is also an optional renderer library called [libgk](https://github.com/recp/libgk) for render AssetKit contents. You can see how to load AssetKit to libgk in [assetkit-gl](https://github.com/recp/assetkit-gl) lib/repo.

I've also created a basic program (included a screenshot of render result and dae) to show full steps: [simple-collada-viewer](http://github.com/recp/simple-collada-viewer)

## Project Status
Currently I'm working on renderer which is also important to test AssetKit. When I implement a feature in AssetKit then I'm adding extra features to [Graphics Kernel / Graphics Kit](https://github.com/recp/libgk) to test AssetKit. The renderer is not just for testing purpose so I'm trying to do my best and this takes some time.

So working on multiple projects ([cglm](https://github.com/recp/cglm), [libgk](https://github.com/recp/libgk), [libds](https://github.com/recp/libds), ...) makes the progress very slow, but in the end the results will be perfect

## glTF
- [x] Load buffers, bufferViews
- [x] Load acessors, meshes / primitives
- [x] Load nodes
- [x] Load scenes
- [x] Load cameras
- [ ] Load materials
  - [x] Images
  - [x] Samplers
  - [x] Textures
  - [x] PBR Materials
      - [x] Metallic Roughness
      - [x] Specular Glossiness
  - [x] Other textures
      - [x] Occlusion map
      - [x] Emissive map
      - [x] Normal Map
      - [ ] others?
  - [ ] alphaMode
- [ ] Animations
- [ ] Extensions
  - [ ] Lights
  - [ ] Common materials

## Tasklist

- [x] Hierarchical Allocator
  - [x] Extensible memory node
- [x] Flexible Coordinate System, loader can laod any coords sys and can convert it to any coord sys even left handed
- [x] Implement RB-tree for ID resolver
- [x] Options
- [x] Load Core elements
  - [x] Geometries
    - [x] Meshes
      - [x] Load Polygons
      - [x] Load Triangles
      - [x] Load Lines
      - [x] Convert multi-index mesh to single-index mesh if wanted
         - [x] Find and fix edges attribs e.g. same position can't use different normals
      - [x] Convert mesh data (positions, normals...) to current coord sys
      - [x] Compute bounding box if wanted
      - [x] Triangulate Polygons if wanted
      - [x] Material resolver
      - [ ] Generate face normals if wanted
    - [x] B-rep
  - [x] Cameras
    - [x] Attach first found camera to scene as default camera
    - [x] Attach all camera instances to scene
  - [x] Scenes
     - [x] Nodes
       - [x] Fix camera node transform
       - [x] Transforms
         - [x] Utils for transforms
         - [x] Convert transforms to current coord sys
       - [x] Implement bind_material
  - [x] Load external DAE files and cache them
  - [ ] Load animations
    - [ ] Parse MathML formulas
- [x] Load FX
- [ ] Load Physics
- [ ] Load Kinematics
- [ ] Exporting

## Build

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

`git`, `nuget` and `python` commands should be installed/accessible.

```Powershell
$ cd win
$ .\build.bat
$ msbuild assetkit.vcxproj /p:Configuration=Release
```
if `msbuild` won't work correctly then try to build with `devenv`:
```Powershell
$ devenv assetkit.sln /Build Release
```

### Test
Currently tests are implemented for Autotools (make check), there is separate repo for storing test models to avoid increase repo size: http://github.com/recp/sample-models . By running `build-deps.sh`, `git submodule update --init --recursive` command will be executed and all sample models will be fecthed for testing into `assetkit/test` folder

`make check` uses this repo samples to test.

# Notes
- Actual README and documents for how to use this will be written after first version
- I'm working hard to finish this lib, I'll update README when finished, for now <b>DON'T USE THIS LIB UNTIL FINISHED</b>
- Don't create pull requests for now, if you really want then, first contact me because I may work on the feature you want or I've planned it with an architecture to do that

