# 🎨 AssetKit
3D asset importer, exporter library. This library also includes common 3D utils funcs. It is written with C99 but C++ wrappers or other language bindings can be written in the future. 

|| **Unix (gcc, clang)** | **Windows (msvc)** |
|---|---|---|
| **Build** | [![Build Status](https://travis-ci.org/recp/assetkit.svg?branch=master)](https://travis-ci.org/recp/assetkit)|[![Build status](https://ci.appveyor.com/api/projects/status/e52s0fr8mnbmsqy4?svg=true)](https://ci.appveyor.com/project/recp/assetkit)


This library will full support COLLADA specs and glTF specs, plus other 3D formats e.g .obj, .stl maybe supported by sub-libraries e.g. assetkit-stl, assetkit-fbx.

There is also an optional renderer library called [libgk](https://github.com/recp/libgk) for render AssetKit contents. You can see how to load AssetKit to libgk in [assetkit-gl](https://github.com/recp/assetkit-gl) lib/repo.

I've also created a basic program (included a screenshot of render result and dae) to show full steps: [simple-collada-viewer](http://github.com/recp/simple-collada-viewer)

##Tasklist

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

# Notes
- Actual README and documents for how to use this will be written after first version
- I'm working hard to finish this lib, I'll update README when finished, for now <b>DON'T USE THIS LIB UNTIL FINISHED</b>
- Don't create pull requests for now, if you really want then, first contact me because I may work on the feature you want or I've planned it with an architecture to do that
