# AssetKit: COLLADA Status

- [x] Single Interface for glTF and COLLADA
- [x] COLLADA 1.4 / 1.4.1
- [x] COLLADA 1.5
- [x] Object-based Asset support
  - Resolving Asset support for an element
- [x] Fix / Convert UP axis to any other
- [x] ID resolving
- [x] SID resolving
- [x] Bugfix some DAE files
- [x] Convert angles to Radians 
- [x] Convert Multi-Index indices to Single-Index indices by keeping primitive indices
- [x] Options to Generate Mesh Normals
- [x] Option to Triangulate Polygons
- [x] Libraries support
- [x] Geometries
  - [x] Meshes (Triangles, Polygons, Lines)
  - [x] Brep
    - Curves, Nurbs, Solids... 
- [x] Nodes
  - [x] Instances (instance Geometry, Light, Camera...)
  - [x] Simplified Controller mechanism into **node->morpher** and **node->skinner**
  - [x] Fix camera node transform
  - [x] **bind_material** and bind vertex input support to bind material dynamically 
- [x] Scenes
  - Active Scene
  - Visual Scenes
- [x] Cameras
- [x] Lights
- [x] Materials
  - [x] Images
  - [x] Samplers
  - [x] Textures
  - [x] Common Profile
      - [x] Phong, Blinn, Lambert and Constant
  - ⚠️ Other profiles were supported, but currently they are removed. But multi profile is still supported. We can support other profiles in the future.
  - [x] Transparency (A_ONE, RGB_ONE, A_ZERO, RGB_ZERO)
- [x] Skin
- [x] Morph
- [x] Animations
- [x] Extra
- [x] Load external DAE files and cache them
- [ ] Parse MathML formulas
- [ ] Physics
- [ ] Kinematics
- [ ] ZAE
- [ ] More to work on...

### Trademarks

glTF and COLLADA and their logos are trademarks of Khronos Group.
