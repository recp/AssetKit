/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_common.h"

#include <ak/options.h>

#include <locale.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>

#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif

#define AK_TEST_EXPORT_GLTF_PATHS(STEM)                                      \
  const char *outDir   = "./" STEM;                                          \
  const char *gltfPath = "./" STEM "/model.gltf";                           \
  const char *binPath  = "./" STEM "/model.bin"

#define AK_TEST_EXPORT_GLB_PATHS(STEM)                                       \
  const char *outDir  = "./" STEM;                                           \
  const char *glbPath = "./" STEM "/model.glb";                             \
  const char *binPath = "./" STEM "/model.bin"

static
bool
ak_test_tree_has_name(AkTree *tree, const char *name) {
  AkTree *child;

  if (!tree || !name)
    return false;

  if (tree->name && strcmp(tree->name, name) == 0)
    return true;

  for (child = tree->chld; child; child = child->next) {
    if (ak_test_tree_has_name(child, name))
      return true;
  }

  return false;
}

static
AkTreeNode*
ak_test_extra_pair(AkHeap    *heap,
                   void      *parent,
                   const char *key,
                   const char *value) {
  AkTreeNode *extra;
  AkTreeNode *note;

  extra = ak_heap_calloc(heap, parent, sizeof(*extra));
  note  = ak_heap_calloc(heap, extra, sizeof(*note));
  if (!extra || !note)
    return NULL;

  extra->name  = "extras";
  extra->chld  = note;
  extra->chldc = 1;
  note->name   = key;
  note->val    = (char *)value;
  note->parent = extra;

  return extra;
}

static
AkTreeNode*
ak_test_extra_extension_pair(AkHeap     *heap,
                             void       *parent,
                             const char *extensionName,
                             const char *key,
                             const char *value) {
  AkTreeNode *extra;
  AkTreeNode *extensions;
  AkTreeNode *extension;
  AkTreeNode *field;

  extra      = ak_heap_calloc(heap, parent, sizeof(*extra));
  extensions = ak_heap_calloc(heap, extra, sizeof(*extensions));
  extension  = ak_heap_calloc(heap, extensions, sizeof(*extension));
  field      = ak_heap_calloc(heap, extension, sizeof(*field));
  if (!extra || !extensions || !extension || !field)
    return NULL;

  extra->name       = "root";
  extra->chld       = extensions;
  extra->chldc      = 1;
  extensions->name  = "extensions";
  extensions->parent = extra;
  extensions->chld  = extension;
  extensions->chldc = 1;
  extension->name   = extensionName;
  extension->parent = extensions;
  extension->chld   = field;
  extension->chldc  = 1;
  field->name       = key;
  field->val        = (char *)value;
  field->parent     = extension;

  return extra;
}

static
AkTreeNode*
ak_test_extra_required_extensions(AkHeap           *heap,
                                  void             *parent,
                                  const char *const *names,
                                  uint32_t          count) {
  AkTreeNode     *extra;
  AkTreeNode     *required;
  AkTreeNodeAttr *arrayAttr;
  AkTreeNode     *prev;
  uint32_t        i;

  extra     = ak_heap_calloc(heap, parent, sizeof(*extra));
  required  = ak_heap_calloc(heap, extra, sizeof(*required));
  arrayAttr = ak_heap_calloc(heap, required, sizeof(*arrayAttr));
  if (!extra || !required || !arrayAttr)
    return NULL;

  extra->name          = "root";
  extra->chld          = required;
  extra->chldc         = 1;
  required->name       = "extensionsRequired";
  required->parent     = extra;
  required->attribs    = arrayAttr;
  required->attrc      = 1;
  required->chldc      = count;
  arrayAttr->name      = "type";
  arrayAttr->val       = (char *)"array";

  prev = NULL;
  for (i = 0; i < count; i++) {
    AkTreeNode *item;

    item = ak_heap_calloc(heap, required, sizeof(*item));
    if (!item)
      return NULL;

    item->parent = required;
    item->val    = (char *)names[i];
    if (prev)
      prev->next = item;
    else
      required->chld = item;
    prev = item;
  }

  return extra;
}

static
bool
ak_test_files_equal(const char *pathA, const char *pathB) {
  unsigned char bufA[64 * 1024];
  unsigned char bufB[64 * 1024];
  FILE         *fileA;
  FILE         *fileB;
  size_t        readA;
  size_t        readB;
  bool          equal;

  fileA = fopen(pathA, "rb");
  if (!fileA)
    return false;

  fileB = fopen(pathB, "rb");
  if (!fileB) {
    fclose(fileA);
    return false;
  }

  equal = true;
  do {
    readA = fread(bufA, 1, sizeof(bufA), fileA);
    readB = fread(bufB, 1, sizeof(bufB), fileB);
    if (readA != readB || memcmp(bufA, bufB, readA) != 0) {
      equal = false;
      break;
    }
  } while (readA == sizeof(bufA));

  if (ferror(fileA) || ferror(fileB))
    equal = false;

  fclose(fileB);
  fclose(fileA);

  return equal;
}

static
bool
ak_test_file_contains(const char *path, const char *needle) {
  FILE  *file;
  char  *data;
  long   len;
  size_t needleLen;
  size_t i;
  bool   found;

  file = fopen(path, "rb");
  if (!file)
    return false;

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return false;
  }

  len = ftell(file);
  if (len < 0) {
    fclose(file);
    return false;
  }

  rewind(file);
  data = malloc((size_t)len + 1);
  if (!data) {
    fclose(file);
    return false;
  }

  if (fread(data, 1, (size_t)len, file) != (size_t)len) {
    free(data);
    fclose(file);
    return false;
  }

  needleLen = strlen(needle);
  found     = false;
  if (needleLen == 0) {
    found = true;
  } else if ((size_t)len >= needleLen) {
    for (i = 0; i <= (size_t)len - needleLen; i++) {
      if (memcmp(data + i, needle, needleLen) == 0) {
        found = true;
        break;
      }
    }
  }

  free(data);
  fclose(file);

  return found;
}

static
size_t
ak_test_file_count(const char *path, const char *needle) {
  FILE       *file;
  char       *data;
  char       *pos;
  long        len;
  size_t      needleLen;
  size_t      count;

  file = fopen(path, "rb");
  if (!file)
    return 0;

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return 0;
  }

  len = ftell(file);
  if (len < 0) {
    fclose(file);
    return 0;
  }

  rewind(file);
  data = malloc((size_t)len + 1);
  if (!data) {
    fclose(file);
    return 0;
  }

  if (fread(data, 1, (size_t)len, file) != (size_t)len) {
    free(data);
    fclose(file);
    return 0;
  }

  data[len] = '\0';
  needleLen = strlen(needle);
  count     = 0;
  pos       = data;

  while (needleLen > 0 && (pos = strstr(pos, needle))) {
    count++;
    pos += needleLen;
  }

  free(data);
  fclose(file);

  return count;
}

static
void
ak_test_export_cleanup(const char *outDir) {
  static const char *names[] = {
    "model.gltf",
    "model.glb",
    "model.bin",
    "Empty.gltf",
    "Empty.glb",
    "Empty.bin",
    "Box_Name_.gltf",
    "Box_Name_.bin",
    "textures/wood.png",
    "textures/Wood.PNG",
    "textures/Wood File.PNG",
    "textures/Extra.PNG",
    "WoodFile.PNG",
    "assetkit_export_cwd_relative_texture.png",
    "image_0_WoodFile.PNG",
    "image_0_leak.png",
    "image_1_WoodFile.PNG",
    "image_1_1_WoodFile.PNG"
  };
  char   path[PATH_MAX];
  size_t i;

  if (!outDir)
    return;

  for (i = 0; i < AK_ARRAY_LEN(names); i++) {
    snprintf(path, sizeof(path), "%s/%s", outDir, names[i]);
    unlink(path);
  }
  snprintf(path, sizeof(path), "%s/textures", outDir);
  rmdir(path);
  rmdir(outDir);
}

static
bool
ak_test_read_u32le(const char *path, long offset, uint32_t *out) {
  unsigned char bytes[4];
  FILE         *file;

  file = fopen(path, "rb");
  if (!file)
    return false;

  if (fseek(file, offset, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }

  if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes)) {
    fclose(file);
    return false;
  }

  fclose(file);

  *out = (uint32_t)bytes[0]
         | ((uint32_t)bytes[1] << 8)
         | ((uint32_t)bytes[2] << 16)
         | ((uint32_t)bytes[3] << 24);

  return true;
}

static
AkImageData*
ak_test_load_rgba_file(AkHeap     * __restrict heap,
                       AkImage    * __restrict image,
                       const char * __restrict path,
                       bool                    flipVertically) {
  AkImageData   *data;
  unsigned char *pixels;
  const unsigned char rgba[16] = {
    255, 0,   0,   255,
    0,   255, 0,   255,
    0,   0,   255, 255,
    255, 255, 255, 255
  };

  (void)path;
  (void)flipVertically;

  data = ak_heap_calloc(heap, image, sizeof(*data));
  if (!data)
    return NULL;

  pixels = ak_heap_alloc(heap, data, sizeof(rgba));
  if (!pixels)
    return NULL;

  memcpy(pixels, rgba, sizeof(rgba));

  data->data   = pixels;
  data->width  = 2;
  data->height = 2;
  data->comp   = 4;

  return data;
}

static
bool
ak_test_write_dae_two_roots(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"rootA\" name=\"RootA\"/>"
        "<node id=\"rootB\" name=\"RootB\"/>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_bad_vertex_ref_single_vertices(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"shape\" name=\"Shape\"><mesh>"
        "<source id=\"shape-positions\"><float_array id=\"shape-positions-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#shape-positions-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"shape-vertices\"><input semantic=\"POSITION\" source=\"#shape-positions\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#missing-vertices\" offset=\"0\"/>"
        "<p>0 1 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><instance_geometry url=\"#shape\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_utf16le_ascii(FILE *file, const char *text) {
  static const unsigned char bom[] = {0xff, 0xfe};
  size_t i, len;

  if (!file || !text)
    return false;

  if (fwrite(bom, 1, sizeof(bom), file) != sizeof(bom))
    return false;

  len = strlen(text);
  for (i = 0; i < len; i++) {
    unsigned char bytes[2];

    bytes[0] = (unsigned char)text[i];
    bytes[1] = 0;
    if (fwrite(bytes, 1, sizeof(bytes), file) != sizeof(bytes))
      return false;
  }

  return true;
}

static
bool
ak_test_write_dae_utf16le_minimal(const char *path) {
  FILE *file;
  bool  ok;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = ak_test_write_utf16le_ascii(
         file,
         "<?xml version=\"1.0\"?>\n"
         "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
         "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
         "<library_visual_scenes><visual_scene id=\"Scene\">"
         "<node id=\"root\" name=\"Root\"/>"
         "</visual_scene></library_visual_scenes>\n"
         "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
         "</COLLADA>\n");

  return fclose(file) == 0 && ok;
}

static
bool
ak_test_write_dae_brep_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.5.0\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"brepGeom\" name=\"BRepGeom\"><brep/>"
        "</geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_geometry url=\"#brepGeom\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_bmp_1x1(const char *path) {
  static const unsigned char bmp[] = {
    0x42, 0x4d, 0x3a, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00, 0x00, 0x00, 0x28, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x01, 0x00, 0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0x00,
    0x00, 0x00
  };
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  if (fwrite(bmp, 1, sizeof(bmp), file) != sizeof(bmp)) {
    fclose(file);
    return false;
  }

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_camera_light_extra(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_cameras><camera id=\"cam\" name=\"Camera\">"
        "<optics><technique_common><perspective>"
        "<yfov>45</yfov><znear>0.1</znear><zfar>100</zfar>"
        "</perspective></technique_common></optics>"
        "<extra><technique profile=\"Test\"><cameraTag>yes</cameraTag></technique></extra>"
        "</camera></library_cameras>\n"
        "<library_lights><light id=\"lamp\" name=\"Lamp\">"
        "<technique_common><point><color>1 1 1</color></point></technique_common>"
        "<extra><technique profile=\"Test\"><lightTag>yes</lightTag></technique></extra>"
        "</light></library_lights>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"camNode\"><instance_camera url=\"#cam\"/></node>"
        "<node id=\"lightNode\"><instance_light url=\"#lamp\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_instance_node(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_cameras><camera id=\"cam\"><optics><technique_common>"
        "<perspective><yfov>45</yfov><znear>0.1</znear><zfar>100</zfar></perspective>"
        "</technique_common></optics></camera></library_cameras>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node name=\"SharedUse\" url=\"#shared\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<library_nodes><node id=\"shared\" name=\"Shared\"><instance_camera url=\"#cam\"/></node></library_nodes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf_root(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"nodes\":[{\"name\":\"Root\"}],"
        "\"scenes\":[{\"nodes\":[0]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf_light(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("{"
        "\"asset\":{\"version\":\"2.0\"},"
        "\"extensionsUsed\":[\"KHR_lights_punctual\"],"
        "\"extensions\":{\"KHR_lights_punctual\":{\"lights\":["
        "{\"type\":\"point\",\"name\":\"Key\",\"intensity\":2.0,\"range\":10.0}"
        "]}},"
        "\"nodes\":[{\"name\":\"LightNode\",\"extensions\":{"
        "\"KHR_lights_punctual\":{\"light\":0}"
        "}}],"
        "\"scenes\":[{\"nodes\":[0]}],"
        "\"scene\":0"
        "}\n",
        file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_gltf_materialless_triangle(const char *gltfPath,
                                         const char *binName,
                                         const char *binPath) {
  static const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  FILE *file;

  file = fopen(binPath, "wb");
  if (!file)
    return false;

  if (fwrite(positions, 1, sizeof(positions), file) != sizeof(positions)) {
    fclose(file);
    return false;
  }

  if (fclose(file) != 0)
    return false;

  file = fopen(gltfPath, "wb");
  if (!file)
    return false;

  fprintf(file,
          "{"
          "\"asset\":{\"version\":\"2.0\"},"
          "\"buffers\":[{\"uri\":\"%s\",\"byteLength\":36}],"
          "\"bufferViews\":[{\"buffer\":0,\"byteOffset\":0,\"byteLength\":36}],"
          "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,"
          "\"count\":3,\"type\":\"VEC3\",\"min\":[0,0,0],\"max\":[1,1,0]}],"
          "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0}}]}],"
          "\"nodes\":[{\"mesh\":0}],"
          "\"scenes\":[{\"nodes\":[0]}],"
          "\"scene\":0"
          "}\n",
          binName);

  return fclose(file) == 0;
}

static
bool
ak_test_write_obj_triangle(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("o Triangle\n"
        "v 0 0 0\n"
        "v 1 0 0\n"
        "v 0 1 0\n"
        "f 1 2 3\n",
        file);

  return fclose(file) == 0;
}

static
AkGeometry *
ak_test_make_geom_with_positions(AkHeap      *heap,
                                 void        *parent,
                                 const float *positions,
                                 uint32_t     positionCount) {
  AkGeometry  *geom;
  AkObject    *meshObj;
  AkMesh      *mesh;
  AkTriangles *tri;
  AkInput     *pos;
  AkAccessor  *acc;
  AkBuffer    *buff;
  size_t       byteLength;

  if (!positions || positionCount == 0)
    return NULL;

  geom    = ak_heap_calloc(heap, parent, sizeof(*geom));
  meshObj = ak_objAlloc(heap, geom, sizeof(*mesh), AK_GEOMETRY_MESH, true);
  mesh    = ak_objGet(meshObj);
  tri     = ak_heap_calloc(heap, meshObj, sizeof(*tri));
  pos     = ak_heap_calloc(heap, tri, sizeof(*pos));
  acc     = ak_heap_calloc(heap, pos, sizeof(*acc));
  buff    = ak_heap_calloc(heap, acc, sizeof(*buff));

  byteLength   = sizeof(float) * 3u * positionCount;
  buff->length = byteLength;
  buff->data   = ak_heap_alloc(heap, buff, buff->length);
  memcpy(buff->data, positions, buff->length);

  acc->buffer                 = buff;
  acc->byteLength             = buff->length;
  acc->byteStride             = sizeof(float) * 3;
  acc->fillByteSize           = sizeof(float) * 3;
  acc->bytesPerComponent      = sizeof(float);
  acc->componentSize          = AK_COMPONENT_SIZE_VEC3;
  acc->componentType          = AKT_FLOAT;
  acc->originalComponentType  = AKT_FLOAT;
  acc->componentCount         = 3;
  acc->count                  = positionCount;

  pos->accessor = acc;
  pos->semantic = AK_INPUT_POSITION;

  tri->base.mesh      = mesh;
  tri->base.pos       = pos;
  tri->base.input     = pos;
  tri->base.type      = AK_PRIMITIVE_TRIANGLES;
  tri->base.nPolygons = 1;
  tri->mode           = AK_TRIANGLES;

  mesh->geom           = geom;
  mesh->primitive      = (AkMeshPrimitive *)tri;
  mesh->primitiveCount = 1;

  geom->gdata = meshObj;

  return geom;
}

static
AkGeometry *
ak_test_make_triangle_geom(AkHeap *heap, void *parent, const float positions[9]) {
  return ak_test_make_geom_with_positions(heap, parent, positions, 3);
}

static
void
ak_test_add_skin_inputs(AkHeap *heap, AkMeshPrimitive *prim) {
  AkInput  *jointInput;
  AkInput  *weightInput;
  AkAccessor *jointAcc;
  AkAccessor *weightAcc;
  AkBuffer *jointBuff;
  AkBuffer *weightBuff;
  const uint16_t joints[12] = {
    0, 1, 0, 0,
    0, 1, 0, 0,
    0, 1, 0, 0
  };
  const float weights[12] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f, 0.0f
  };

  jointInput = ak_heap_calloc(heap, prim, sizeof(*jointInput));
  weightInput = ak_heap_calloc(heap, prim, sizeof(*weightInput));
  jointAcc = ak_heap_calloc(heap, jointInput, sizeof(*jointAcc));
  weightAcc = ak_heap_calloc(heap, weightInput, sizeof(*weightAcc));
  jointBuff = ak_heap_calloc(heap, jointAcc, sizeof(*jointBuff));
  weightBuff = ak_heap_calloc(heap, weightAcc, sizeof(*weightBuff));

  jointBuff->length = sizeof(joints);
  jointBuff->data = ak_heap_alloc(heap, jointBuff, jointBuff->length);
  memcpy(jointBuff->data, joints, sizeof(joints));

  weightBuff->length = sizeof(weights);
  weightBuff->data = ak_heap_alloc(heap, weightBuff, weightBuff->length);
  memcpy(weightBuff->data, weights, sizeof(weights));

  jointAcc->buffer = jointBuff;
  jointAcc->byteLength = jointBuff->length;
  jointAcc->byteStride = sizeof(uint16_t) * 4;
  jointAcc->fillByteSize = sizeof(uint16_t) * 4;
  jointAcc->bytesPerComponent = sizeof(uint16_t);
  jointAcc->componentSize = AK_COMPONENT_SIZE_VEC4;
  jointAcc->componentType = AKT_USHORT;
  jointAcc->originalComponentType = AKT_USHORT;
  jointAcc->componentCount = 4;
  jointAcc->count = 3;

  weightAcc->buffer = weightBuff;
  weightAcc->byteLength = weightBuff->length;
  weightAcc->byteStride = sizeof(float) * 4;
  weightAcc->fillByteSize = sizeof(float) * 4;
  weightAcc->bytesPerComponent = sizeof(float);
  weightAcc->componentSize = AK_COMPONENT_SIZE_VEC4;
  weightAcc->componentType = AKT_FLOAT;
  weightAcc->originalComponentType = AKT_FLOAT;
  weightAcc->componentCount = 4;
  weightAcc->count = 3;

  jointInput->accessor = jointAcc;
  jointInput->semantic = AK_INPUT_JOINT;
  jointInput->set = 0;

  weightInput->accessor = weightAcc;
  weightInput->semantic = AK_INPUT_WEIGHT;
  weightInput->set = 0;

  weightInput->next = prim->input;
  jointInput->next = weightInput;
  prim->input = jointInput;
  prim->inputCount += 2;
}

static
AkAccessor*
ak_test_make_float_accessor(AkHeap      *heap,
                            void        *parent,
                            const float *data,
                            uint32_t     componentCount,
                            uint32_t     count) {
  AkAccessor     *acc;
  AkBuffer       *buff;
  size_t          byteLength;
  AkComponentSize componentSize;

  componentSize = AK_COMPONENT_SIZE_UNKNOWN;
  switch (componentCount) {
    case 1: componentSize = AK_COMPONENT_SIZE_SCALAR; break;
    case 2: componentSize = AK_COMPONENT_SIZE_VEC2;   break;
    case 3: componentSize = AK_COMPONENT_SIZE_VEC3;   break;
    case 4: componentSize = AK_COMPONENT_SIZE_VEC4;   break;
    default: break;
  }

  if (componentSize == AK_COMPONENT_SIZE_UNKNOWN)
    return NULL;

  acc  = ak_heap_calloc(heap, parent, sizeof(*acc));
  buff = ak_heap_calloc(heap, acc, sizeof(*buff));
  byteLength = sizeof(float) * componentCount * count;

  buff->length = byteLength;
  buff->data   = ak_heap_alloc(heap, buff, byteLength);
  memcpy(buff->data, data, byteLength);

  acc->buffer                = buff;
  acc->byteLength            = byteLength;
  acc->byteStride            = sizeof(float) * componentCount;
  acc->fillByteSize          = sizeof(float) * componentCount;
  acc->bytesPerComponent     = sizeof(float);
  acc->componentSize         = componentSize;
  acc->componentType         = AKT_FLOAT;
  acc->originalComponentType = AKT_FLOAT;
  acc->componentCount        = componentCount;
  acc->count                 = count;

  return acc;
}

static
AkInput*
ak_test_add_texcoord_input(AkHeap          *heap,
                           AkMeshPrimitive *prim,
                           uint32_t         set) {
  AkInput *input;
  const float uvs[6] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f
  };

  input = ak_heap_calloc(heap, prim, sizeof(*input));
  input->semantic = AK_INPUT_TEXCOORD;
  input->set      = set;
  input->index    = set;
  input->accessor = ak_test_make_float_accessor(heap, input, uvs, 2, 3);
  input->next     = prim->input;
  prim->input     = input;
  prim->inputCount++;

  return input;
}

static
AkAccessor*
ak_test_make_ubyte_accessor(AkHeap       *heap,
                            void         *parent,
                            const uint8_t *data,
                            uint32_t      componentCount,
                            uint32_t      count) {
  AkAccessor     *acc;
  AkBuffer       *buff;
  size_t          byteLength;
  AkComponentSize componentSize;

  componentSize = AK_COMPONENT_SIZE_UNKNOWN;
  switch (componentCount) {
    case 1: componentSize = AK_COMPONENT_SIZE_SCALAR; break;
    case 2: componentSize = AK_COMPONENT_SIZE_VEC2;   break;
    case 3: componentSize = AK_COMPONENT_SIZE_VEC3;   break;
    case 4: componentSize = AK_COMPONENT_SIZE_VEC4;   break;
    default: break;
  }

  if (componentSize == AK_COMPONENT_SIZE_UNKNOWN)
    return NULL;

  acc  = ak_heap_calloc(heap, parent, sizeof(*acc));
  buff = ak_heap_calloc(heap, acc, sizeof(*buff));
  byteLength = sizeof(uint8_t) * componentCount * count;

  buff->length = byteLength;
  buff->data   = ak_heap_alloc(heap, buff, byteLength);
  memcpy(buff->data, data, byteLength);

  acc->buffer                = buff;
  acc->byteLength            = byteLength;
  acc->byteStride            = sizeof(uint8_t) * componentCount;
  acc->fillByteSize          = sizeof(uint8_t) * componentCount;
  acc->bytesPerComponent     = sizeof(uint8_t);
  acc->componentSize         = componentSize;
  acc->componentType         = AKT_UBYTE;
  acc->originalComponentType = AKT_UBYTE;
  acc->componentCount        = componentCount;
  acc->count                 = count;

  return acc;
}

TEST_IMPL(instance_attach_helpers) {
  AkHeap             *heap;
  AkNode             *node, *subNode;
  AkNode             *targetNode;
  AkGeometry         *geomA, *geomB;
  AkInstanceGeometry *instA, *instB;
  AkInstanceNode          *instNode;

  heap  = ak_heap_new(NULL, NULL, NULL);
  node  = ak_heap_calloc(heap, NULL, sizeof(*node));
  targetNode = ak_heap_calloc(heap, NULL, sizeof(*targetNode));
  geomA = ak_heap_calloc(heap, NULL, sizeof(*geomA));
  geomB = ak_heap_calloc(heap, NULL, sizeof(*geomB));

  instA = ak_nodeAttachGeometry(node, geomA);
  ASSERT(instA != NULL);
  ASSERT(node->geometry == instA);
  ASSERT(instA->base.node == node);
  ASSERT(instA->base.object == geomA);
  ASSERT(instA->base.prev == NULL);
  ASSERT(instA->base.next == NULL);

  instB = ak_nodeAttachGeometry(node, geomB);
  ASSERT(instB != NULL);
  ASSERT(node->geometry == instB);
  ASSERT(instB->base.node == node);
  ASSERT(instB->base.object == geomB);
  ASSERT(instB->base.prev == NULL);
  ASSERT(instB->base.next == &instA->base);
  ASSERT(instA->base.prev == &instB->base);
  ASSERT(instA->base.next == NULL);

  subNode = ak_instanceMoveToSubNode(node, &instB->base);
  ASSERT(subNode != NULL);
  ASSERT(subNode->geometry == instB);
  ASSERT(instB->base.node == subNode);
  ASSERT(instB->base.prev == NULL);
  ASSERT(instB->base.next == NULL);
  ASSERT(node->geometry == instA);
  ASSERT(instA->base.prev == NULL);
  ASSERT(instA->base.next == NULL);

  instNode = ak_nodeAttachNodeInstance(node, targetNode);
  ASSERT(instNode != NULL);
  ASSERT(node->node == instNode);
  ASSERT(instNode->owner == node);
  ASSERT(instNode->target == targetNode);
  ASSERT(ak_instanceNodeTarget(instNode) == targetNode);
  ASSERT(instNode->prev == NULL);
  ASSERT(instNode->next == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_traversal) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *nodeA, *nodeB;
  AkGeometry    *geomA, *geomB;
  AkBoundingBox *bboxA, *bboxB;

  heap  = ak_heap_new(NULL, NULL, NULL);
  scene = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root  = ak_heap_calloc(heap, scene, sizeof(*root));
  nodeA = ak_heap_calloc(heap, NULL, sizeof(*nodeA));
  nodeB = ak_heap_calloc(heap, NULL, sizeof(*nodeB));
  geomA = ak_heap_calloc(heap, NULL, sizeof(*geomA));
  geomB = ak_heap_calloc(heap, NULL, sizeof(*geomB));
  bboxA = ak_heap_calloc(heap, geomA, sizeof(*bboxA));
  bboxB = ak_heap_calloc(heap, geomB, sizeof(*bboxB));

  scene->node = root;

  bboxA->min[0] = 0.0f;
  bboxA->min[1] = 0.0f;
  bboxA->min[2] = 0.0f;
  bboxA->max[0] = 1.0f;
  bboxA->max[1] = 1.0f;
  bboxA->max[2] = 1.0f;
  bboxA->isvalid = true;

  bboxB->min[0] = 10.0f;
  bboxB->min[1] = 0.0f;
  bboxB->min[2] = 0.0f;
  bboxB->max[0] = 11.0f;
  bboxB->max[1] = 1.0f;
  bboxB->max[2] = 1.0f;
  bboxB->isvalid = true;

  geomA->bbox = bboxA;
  geomB->bbox = bboxB;

  ASSERT(ak_nodeAttachGeometry(nodeA, geomA) != NULL);
  ASSERT(ak_nodeAttachGeometry(nodeB, geomB) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, nodeA) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, nodeB) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);

  bboxB->min[0] = 5.0f;
  bboxB->max[0] = 6.0f;
  ak_bbox_scene(scene);

  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 6.0f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_lazy_geometry) {
  AkHeap     *heap;
  AkScene    *scene;
  AkNode     *root, *target;
  AkGeometry *geom;
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_test_make_triangle_geom(heap, NULL, positions);

  scene->node = root;

  ASSERT(geom->bbox == NULL);
  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(geom->bbox != NULL);
  ASSERT(geom->bbox->isvalid);
  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(isfinite(scene->bbox->min[0]));
  ASSERT(isfinite(scene->bbox->max[0]));
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 1.0f);
  ASSERT(scene->bbox->min[1] == 0.0f);
  ASSERT(scene->bbox->max[1] == 1.0f);
  ASSERT(scene->bbox->min[2] == 0.0f);
  ASSERT(scene->bbox->max[2] == 0.0f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  struct stat stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_triangle_smoke");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    2.0f, 3.0f, 4.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  node->name  = "Node \"A\"\\B\n";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0,0,0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1,1,0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34962"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"name\":\"Node \\\"A\\\"\\\\B\\n\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\\\\\\\\B"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[2,3,4]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_glb_triangle_smoke) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  struct stat stGlb, stBin;
  uint32_t    magic;
  uint32_t    version;
  uint32_t    length;
  uint32_t    jsonChunkType;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_triangle_smoke");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 0, &magic));
  ASSERT(ak_test_read_u32le(glbPath, 4, &version));
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(ak_test_read_u32le(glbPath, 16, &jsonChunkType));
  ASSERT(magic == 0x46546c67u);
  ASSERT(version == 2u);
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(jsonChunkType == 0x4e4f534au);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  ASSERT(roundTrip->scene->node->chld != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_zero_triangle_mode_defaults_to_list) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkTriangles     *tri;
  struct stat      stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_zero_triangle_mode");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  tri  = (AkTriangles *)mesh->primitive;
  tri->mode = 0;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(!ak_test_file_contains(gltfPath, "\"mode\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_numbers_are_locale_independent) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  char        oldLocale[128];
  const char *locale;
  const char *old;
  AkResult    result;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_locale_numbers");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    1.25f, 2.5f, 3.75f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  old = setlocale(LC_NUMERIC, NULL);
  if (old) {
    strncpy(oldLocale, old, sizeof(oldLocale) - 1u);
    oldLocale[sizeof(oldLocale) - 1u] = '\0';
  } else {
    oldLocale[0] = '\0';
  }

  locale = setlocale(LC_NUMERIC, "de_DE.UTF-8");
  if (!locale)
    locale = setlocale(LC_NUMERIC, "fr_FR.UTF-8");
  if (!locale)
    locale = setlocale(LC_NUMERIC, "tr_TR.ISO8859-9");

  if (!locale) {
    ak_heap_destroy(heap);
    ak_test_export_cleanup(outDir);
    TEST_SUCCESS
  }

  result = ak_export(doc, outDir, AK_FILE_TYPE_GLTF);
  if (oldLocale[0])
    setlocale(LC_NUMERIC, oldLocale);
  else
    setlocale(LC_NUMERIC, "C");

  ASSERT(result == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[1.25,2.5,3.75]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_empty_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  struct stat  stGltf;
  const char *outDir   = "./assetkit_export_empty_scene";
  const char *gltfPath = "./assetkit_export_empty_scene/Empty.gltf";
  const char *binPath  = "./assetkit_export_empty_scene/Empty.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene      = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->name = "Empty";
  doc->scene = scene;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stGltf) != 0);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"scenes\":[{\"name\":\"Empty\"}],\"scene\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"nodes\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"buffers\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_null_scene) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  struct stat  stGltf;
  const char  *outDir   = "./assetkit_export_null_scene";
  const char  *gltfPath = "./assetkit_export_null_scene/model.gltf";
  const char  *binPath  = "./assetkit_export_null_scene/model.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stGltf) != 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{}],\"scene\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"nodes\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"buffers\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_empty_scene_glb) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  struct stat  stGlb;
  struct stat  stBin;
  uint32_t     magic;
  uint32_t     version;
  uint32_t     length;
  uint32_t     jsonChunkType;
  const char  *outDir  = "./assetkit_export_empty_scene";
  const char  *glbPath = "./assetkit_export_empty_scene/Empty.glb";
  const char  *binPath = "./assetkit_export_empty_scene/Empty.bin";

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  scene->name = "Empty";
  doc->scene  = scene;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 0, &magic));
  ASSERT(ak_test_read_u32le(glbPath, 4, &version));
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(ak_test_read_u32le(glbPath, 16, &jsonChunkType));
  ASSERT(magic == 0x46546c67u);
  ASSERT(version == 2u);
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(jsonChunkType == 0x4e4f534au);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_scene_entrypoint_transform) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_scene_entrypoint_transform");
  const float rootMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 7.0f, 0.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);

  ak_nodeSetTransformMatrix(root, rootMatrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{\"nodes\":[0]}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[1]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"translation\":[0,7,0]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_nested_sibling_children_are_contiguous) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *root, *parent, *childA, *grandChild, *childB;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_nested_sibling_children");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  parent      = ak_heap_calloc(heap, doc, sizeof(*parent));
  childA      = ak_heap_calloc(heap, doc, sizeof(*childA));
  grandChild  = ak_heap_calloc(heap, doc, sizeof(*grandChild));
  childB      = ak_heap_calloc(heap, doc, sizeof(*childB));
  scene->node = root;
  doc->scene  = scene;

  root->visible       = true;
  parent->visible     = true;
  childA->visible     = true;
  grandChild->visible = true;
  childB->visible     = true;

  ak_addSubNode(root, parent, false);
  ak_addSubNode(parent, childA, false);
  ak_addSubNode(childA, grandChild, false);
  ak_addSubNode(parent, childB, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\":[{\"nodes\":[0]}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[1,2]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"children\":[3]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_nonfinite_float) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_nonfinite_float");
  const float matrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    INFINITY, 0.0f, 0.0f, 1.0f
  };
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_nodeSetTransformMatrix(node, matrix);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stFile) != 0);
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_auto_file_type) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_auto_type_nested/out");
  const char *parentDir = "./assetkit_export_auto_type_nested";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  rmdir(parentDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_AUTO) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stGltf.st_size > 0);
  ASSERT(stBin.st_size == (off_t)(sizeof(float) * 9));
  ASSERT(ak_test_file_contains(gltfPath, "\"asset\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"buffers\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  rmdir(parentDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_sanitizes_output_name) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  const char  *outDir   = "./assetkit_export_sanitized_name";
  const char  *gltfPath = "./assetkit_export_sanitized_name/Box_Name_.gltf";
  const char  *binPath  = "./assetkit_export_sanitized_name/Box_Name_.bin";
  const char  *modelPath = "./assetkit_export_sanitized_name/model.gltf";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf       = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name = "/tmp/Box:Name?.dae";

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);
  ASSERT(stat(modelPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_blank_output_name_uses_model) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stGltf, stBin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_blank_name");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf       = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->name = "   .dae";

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stGltf) == 0);
  ASSERT(stat(binPath, &stBin) == 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_file_output_dir) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *root, *node;
  AkGeometry  *geom;
  struct stat  stFile;
  FILE        *file;
  const char  *outDir = "./assetkit_export_collision_file";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  unlink(outDir);
  file = fopen(outDir, "wb");
  ASSERT(file != NULL);
  ASSERT(fclose(file) == 0);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_EBADF);
  ASSERT(stat(outDir, &stFile) == 0);

  ak_heap_destroy(heap);
  unlink(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_out_of_bounds_accessor) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->buffer->length = sizeof(float) * 3;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_accepts_component_count_without_component_size) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_component_count_type");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->componentSize = AK_COMPONENT_SIZE_UNKNOWN;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"VEC3\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_attribute_accessor_shape) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_attribute_accessor_shape");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->pos->accessor->componentCount = 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_position_input_without_prim_pos) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_position_input_only");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(prim->pos != NULL);
  prim->pos = NULL;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":0}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_duplicate_attribute_semantic) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *normalA;
  AkInput         *normalB;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_duplicate_attribute");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float normals[9] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  normalA = ak_heap_calloc(heap, prim, sizeof(*normalA));
  normalB = ak_heap_calloc(heap, prim, sizeof(*normalB));
  normalA->semantic = AK_INPUT_NORMAL;
  normalB->semantic = AK_INPUT_NORMAL;
  normalA->accessor = ak_test_make_float_accessor(heap,
                                                  normalA,
                                                  normals,
                                                  3,
                                                  3);
  normalB->accessor = ak_test_make_float_accessor(heap,
                                                  normalB,
                                                  normals,
                                                  3,
                                                  3);
  ASSERT(normalA->accessor != NULL);
  ASSERT(normalB->accessor != NULL);

  normalB->next = prim->input;
  normalA->next = normalB;
  prim->input = normalA;
  prim->inputCount += 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"NORMAL\""));
  ASSERT(ak_test_file_count(gltfPath, "\"NORMAL\"") == 1);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_normalizes_float_normals) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkGeometry      *roundGeom;
  AkMesh          *mesh;
  AkMesh          *roundMesh;
  AkMeshPrimitive *prim;
  AkMeshPrimitive *roundPrim;
  AkInput         *normal;
  AkInput         *roundInput;
  AkAccessor      *acc;
  uint32_t         i;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_normalized_normals");
  const char      *roundDir      = "./assetkit_export_normalized_normals_round";
  const char      *roundGltfPath = "./assetkit_export_normalized_normals_round/model.gltf";
  const char      *roundBinPath  = "./assetkit_export_normalized_normals_round/model.bin";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float normals[9] = {
    0.0f, 0.0f, 2.0f,
    0.0f, 3.0f, 0.0f,
    4.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  normal = ak_heap_calloc(heap, prim, sizeof(*normal));
  normal->semantic = AK_INPUT_NORMAL;
  normal->accessor = ak_test_make_float_accessor(heap, normal, normals, 3, 3);
  ASSERT(normal->accessor != NULL);
  normal->next = prim->input;
  prim->input = normal;
  prim->inputCount++;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"NORMAL\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  roundGeom = roundTrip->lib.geometries.first;
  ASSERT(roundGeom != NULL);
  roundMesh = ak_objGet(roundGeom->gdata);
  ASSERT(roundMesh != NULL);
  roundPrim = roundMesh->primitive;
  ASSERT(roundPrim != NULL);

  acc = NULL;
  for (roundInput = roundPrim->input; roundInput; roundInput = roundInput->next) {
    if (roundInput->semantic == AK_INPUT_NORMAL) {
      acc = roundInput->accessor;
      break;
    }
  }
  ASSERT(acc != NULL);
  ASSERT(acc->buffer != NULL);
  ASSERT(acc->buffer->data != NULL);
  ASSERT(acc->componentType == AKT_FLOAT);
  ASSERT(acc->componentCount == 3);

  for (i = 0; i < acc->count; i++) {
    const unsigned char *item;
    size_t               stride;
    float                x;
    float                y;
    float                z;
    float                len;

    stride = acc->byteStride ? acc->byteStride : acc->fillByteSize;
    item   = (const unsigned char *)acc->buffer->data
             + acc->byteOffset
             + (size_t)i * stride;
    memcpy(&x, item, sizeof(x));
    memcpy(&y, item + sizeof(float), sizeof(y));
    memcpy(&z, item + sizeof(float) * 2u, sizeof(z));
    len = sqrtf(x * x + y * y + z * z);
    ASSERT(fabsf(len - 1.0f) < 0.00001f);
  }

  ASSERT(ak_export(roundTrip, roundDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_files_equal(gltfPath, roundGltfPath));
  ASSERT(ak_test_files_equal(binPath, roundBinPath));

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  TEST_SUCCESS
}

TEST_IMPL(mesh_gen_normals_position_accessor_layout) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *posAcc;
  AkBuffer        *posBuff;
  AkInput         *input;
  AkAccessor      *normalAcc;
  uint8_t         *indices;
  unsigned char   layout[64];
  size_t          stride;
  uint32_t        i;
  bool            foundNormal;
  const float     positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float     v0[4] = {0.0f, 0.0f, 0.0f, 77.0f};
  const float     v1[4] = {1.0f, 0.0f, 0.0f, 88.0f};
  const float     v2[4] = {0.0f, 1.0f, 0.0f, 99.0f};

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(geom != NULL);

  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);

  posAcc  = prim->pos->accessor;
  posBuff = posAcc->buffer;
  ASSERT(posBuff != NULL);

  memset(layout, 0x7f, sizeof(layout));
  memcpy(layout + 16u, v0, sizeof(v0));
  memcpy(layout + 32u, v1, sizeof(v1));
  memcpy(layout + 48u, v2, sizeof(v2));

  posBuff->length = sizeof(layout);
  posBuff->data   = ak_heap_alloc(heap, posBuff, sizeof(layout));
  ASSERT(posBuff->data != NULL);
  memcpy(posBuff->data, layout, sizeof(layout));

  posAcc->byteOffset    = 16;
  posAcc->byteStride    = sizeof(float) * 4u;
  posAcc->fillByteSize  = sizeof(float) * 3u;
  posAcc->byteLength    = posAcc->byteStride * 3u;
  posAcc->componentSize = AK_COMPONENT_SIZE_VEC3;
  posAcc->componentType = AKT_FLOAT;
  posAcc->componentCount = 3;
  posAcc->count         = 3;

  prim->indices = ak_indexArrayAlloc(heap, prim, 3, AKT_UBYTE);
  ASSERT(prim->indices != NULL);
  indices    = (uint8_t *)prim->indices->items;
  indices[0] = 0;
  indices[1] = 1;
  indices[2] = 2;
  prim->indices->max = 2;
  prim->indexStride  = 1;

  ak_meshGenNormals(mesh);

  foundNormal = false;
  normalAcc   = NULL;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_NORMAL) {
      foundNormal = true;
      normalAcc = input->accessor;
      break;
    }
  }

  ASSERT(foundNormal);
  ASSERT(normalAcc != NULL);
  ASSERT(normalAcc->buffer != NULL);
  ASSERT(normalAcc->buffer->data != NULL);
  ASSERT(normalAcc->componentType == AKT_FLOAT);
  ASSERT(normalAcc->componentCount == 3);
  ASSERT(normalAcc->count > 0);

  stride = normalAcc->byteStride ? normalAcc->byteStride : normalAcc->fillByteSize;
  for (i = 0; i < normalAcc->count; i++) {
    const unsigned char *item;
    float                x;
    float                y;
    float                z;

    item = (const unsigned char *)normalAcc->buffer->data
           + normalAcc->byteOffset
           + (size_t)i * stride;
    memcpy(&x, item, sizeof(x));
    memcpy(&y, item + sizeof(float), sizeof(y));
    memcpy(&z, item + sizeof(float) * 2u, sizeof(z));

    ASSERT(fabsf(x) < 0.00001f);
    ASSERT(fabsf(y) < 0.00001f);
    ASSERT(fabsf(z - 1.0f) < 0.00001f);
  }

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_mesh_input_accessor) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *other;
  const double     extraValues[3] = {1.0, 2.0, 3.0};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_skip_unsupported_input");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  other = ak_heap_calloc(heap, prim, sizeof(*other));
  other->semantic = AK_INPUT_OTHER;
  other->semanticRaw = "ROTATION";
  other->accessor = ak_heap_calloc(heap, other, sizeof(*other->accessor));
  other->accessor->buffer = ak_heap_calloc(heap,
                                           other->accessor,
                                           sizeof(*other->accessor->buffer));
  other->accessor->buffer->data = (void *)extraValues;
  other->accessor->buffer->length = sizeof(extraValues);
  other->accessor->byteLength = sizeof(extraValues);
  other->accessor->byteStride = sizeof(double);
  other->accessor->fillByteSize = sizeof(double);
  other->accessor->bytesPerComponent = sizeof(double);
  other->accessor->componentSize = AK_COMPONENT_SIZE_SCALAR;
  other->accessor->componentType = AKT_DOUBLE;
  other->accessor->originalComponentType = AKT_DOUBLE;
  other->accessor->componentCount = 1;
  other->accessor->count = 3;
  other->next = prim->input;
  prim->input = other;
  prim->inputCount++;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":0}"));
  ASSERT(!ak_test_file_contains(gltfPath, "ROTATION"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_native_index_accessor) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_native_index_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[3] = {0u, 1u, 2u};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                    prim,
                                                    indices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"attributes\":{\"POSITION\":1}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"indices\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5121"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34963"));
  ASSERT(ak_test_file_contains(gltfPath, "\"target\":34962"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_promotes_primitive_restart_index_array) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  uint8_t         *items;
  float            positions[256 * 3];
  uint32_t         i;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_promote_restart_index");

  for (i = 0; i < 256; i++) {
    positions[(size_t)i * 3u + 0u] = (float)i;
    positions[(size_t)i * 3u + 1u] = 0.0f;
    positions[(size_t)i * 3u + 2u] = 0.0f;
  }

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 256);
  ASSERT(geom != NULL);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indices = ak_indexArrayAlloc(heap, prim, 3, AKT_UBYTE);
  ASSERT(prim->indices != NULL);

  items       = (uint8_t *)prim->indices->items;
  items[0]    = 0u;
  items[1]    = UINT8_MAX;
  items[2]    = 1u;
  prim->indices->max = UINT8_MAX;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"indices\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5123"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"componentType\":5121"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_index_accessor_type) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_index_accessor");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float badIndices[3] = {0.0f, 1.0f, 2.0f};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_float_accessor(heap,
                                                    prim,
                                                    badIndices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_invalid_index_accessor_shape) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  struct stat      stGltf;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bad_index_accessor_shape");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[3] = {0u, 1u, 2u};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                    prim,
                                                    indices,
                                                    1,
                                                    3);
  ASSERT(prim->indexAccessor != NULL);
  prim->indexAccessor->componentSize = AK_COMPONENT_SIZE_VEC2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_ERR);
  ASSERT(stat(gltfPath, &stGltf) != 0);
  ASSERT(stat(binPath, &stGltf) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_shared_node_reuses_mesh) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *shared;
  AkNode         *roundRoot;
  AkGeometry     *geom;
  AkInstanceNode *useA, *useB;
  uint32_t        rootCount;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_shared_node");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  shared      = ak_heap_calloc(heap, doc, sizeof(*shared));
  scene->node = root;
  doc->scene  = scene;

  shared->name = "Shared";
  geom         = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(ak_nodeAttachGeometry(shared, geom) != NULL);

  useA = ak_nodeAttachNodeInstance(root, shared);
  useB = ak_nodeAttachNodeInstance(root, shared);
  ASSERT(useA != NULL);
  ASSERT(useB != NULL);
  useA->name = "UseA";
  useB->name = "UseB";

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"meshes\":["));
  ASSERT(ak_test_file_count(gltfPath, "\"mesh\":0") == 2);

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);

  rootCount = 0;
  for (roundRoot = roundTrip->scene->node->chld;
       roundRoot;
       roundRoot = roundRoot->next)
    rootCount++;
  ASSERT(rootCount == 2);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_primitive_material) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *opacity;
  AkMaterialInput   *metallic;
  AkMaterialInput   *roughness;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_primitive_material");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  opacity   = ak_heap_calloc(heap, surface, sizeof(*opacity));
  metallic  = ak_heap_calloc(heap, surface, sizeof(*metallic));
  roughness = ak_heap_calloc(heap, surface, sizeof(*roughness));

  mat->name        = "mat_red";
  mat->surface     = surface;
  surface->type    = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->flags   = AK_MATERIAL_FLAG_ALPHA_BLEND
                     | AK_MATERIAL_FLAG_DOUBLE_SIDED;
  surface->baseColor = baseColor;
  surface->opacity   = opacity;
  surface->metallic  = metallic;
  surface->roughness = roughness;

  baseColor->source      = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType   = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.25f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 1.0f;

  opacity->source    = AK_MATERIAL_INPUT_CONSTANT;
  opacity->valueType = AK_MATERIAL_VALUE_FLOAT;
  opacity->value[0]  = 0.8f;

  metallic->source    = AK_MATERIAL_INPUT_CONSTANT;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.0f;

  roughness->source    = AK_MATERIAL_INPUT_CONSTANT;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.5f;

  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"materials\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorFactor\":[0.25,0.5,0.75,0.800000012]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"metallicFactor\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"roughnessFactor\":0.5"));
  ASSERT(ak_test_file_contains(gltfPath, "\"alphaMode\":\"BLEND\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"doubleSided\":true"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_extras) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTreeNode        *extra;
  AkTreeNode        *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_extras");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  extra     = ak_heap_calloc(heap, mat, sizeof(*extra));
  note      = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  baseColor->source  = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "materialNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(mat, extra);

  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"materialNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_mesh_primitive_extras) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkTreeNode      *meshExtra;
  AkTreeNode      *meshNote;
  AkTreeNode      *primExtra;
  AkTreeNode      *primNote;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_mesh_primitive_extras");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  meshExtra = ak_heap_calloc(heap, doc, sizeof(*meshExtra));
  meshNote  = ak_heap_calloc(heap, meshExtra, sizeof(*meshNote));
  primExtra = ak_heap_calloc(heap, doc, sizeof(*primExtra));
  primNote  = ak_heap_calloc(heap, primExtra, sizeof(*primNote));
  ASSERT(meshExtra != NULL);
  ASSERT(meshNote != NULL);
  ASSERT(primExtra != NULL);
  ASSERT(primNote != NULL);

  meshExtra->name  = "extras";
  meshExtra->chld  = meshNote;
  meshExtra->chldc = 1;
  meshNote->name   = "meshNote";
  meshNote->val    = (char *)"roundtrip";
  meshNote->parent = meshExtra;
  mesh->extra = meshExtra;

  primExtra->name  = "extras";
  primExtra->chld  = primNote;
  primExtra->chldc = 1;
  primNote->name   = "primitiveNote";
  primNote->val    = (char *)"roundtrip";
  primNote->parent = primExtra;
  prim->extra = primExtra;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"meshNote\":\"roundtrip\"}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"primitiveNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_omits_imported_default_material) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         gltfPath[PATH_MAX];
  char         binPath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outGltfPath[PATH_MAX];
  char         outBinPath[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-materialless-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/materialless.gltf", tmpdir);
  snprintf(binPath, sizeof(binPath), "%s/tri.bin", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outGltfPath, sizeof(outGltfPath), "%s/materialless.gltf", outDir);
  snprintf(outBinPath, sizeof(outBinPath), "%s/materialless.bin", outDir);

  ASSERT(ak_test_write_gltf_materialless_triangle(gltfPath,
                                                  "tri.bin",
                                                  binPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(outGltfPath, "\"meshes\""));
  ASSERT(!ak_test_file_contains(outGltfPath, "\"materials\""));
  ASSERT(!ak_test_file_contains(outGltfPath, "\"material\":"));

  ak_free(doc);
  unlink(outGltfPath);
  unlink(outBinPath);
  rmdir(outDir);
  unlink(gltfPath);
  unlink(binPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_uri");
  const char *sourceDir = "./assetkit_export_material_texture_uri_src";
  const char *sourceTexDir = "./assetkit_export_material_texture_uri_src/textures";
  const char *sourceTexPath = "./assetkit_export_material_texture_uri_src/textures/Wood File.PNG";
  const char *copiedTexPath = "./assetkit_export_material_texture_uri/textures/Wood File.PNG";
  const char *glbOutDir = "./assetkit_export_material_texture_uri_glb";
  const char *glbPath = "./assetkit_export_material_texture_uri_glb/model.glb";
  const char *glbCopiedTexPath = "./assetkit_export_material_texture_uri_glb/textures/Wood File.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(glbOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "textures/Wood File.PNG";
  image->name  = "wood";
  image->source = source;

  sampler->wrapS     = AK_WRAP_MODE_CLAMP;
  sampler->wrapT     = AK_WRAP_MODE_WRAP;
  sampler->minfilter = AK_MINFILTER_NEAREST;
  sampler->magfilter = AK_MAGFILTER_NEAREST;

  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 1;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface        = surface;
  surface->type       = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor  = baseColor;
  prim->material      = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"samplers\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"wrapS\":33071"));
  ASSERT(ak_test_file_contains(gltfPath, "\"minFilter\":9728"));
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"textures/Wood%20File.PNG\""));
  ASSERT(!ak_test_file_contains(gltfPath, "Wood%2520File"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));
  ASSERT(ak_test_file_contains(gltfPath, "\"textures\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"texCoord\":1}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ASSERT(ak_export(doc, glbOutDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(ak_test_file_contains(glbPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(glbPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(glbPath, "\"uri\":\"textures/Wood%20File.PNG\""));
  ASSERT(!ak_test_file_contains(glbCopiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(glbOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_metallic_roughness_texture_channels) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *metallic;
  AkMaterialInput   *roughness;
  AkTextureRef      *metalRef;
  AkTextureRef      *roughRef;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  const char        *compatibleOutDir = "./assetkit_export_mr_texture_channels";
  const char        *compatibleGltfPath = "./assetkit_export_mr_texture_channels/model.gltf";
  const char        *incompatibleOutDir = "./assetkit_export_mr_texture_channels_incompatible";
  const char        *incompatibleGltfPath = "./assetkit_export_mr_texture_channels_incompatible/model.gltf";
  const char        *sourceDir = "./assetkit_export_mr_texture_channels_src";
  const char        *sourceTexDir = "./assetkit_export_mr_texture_channels_src/textures";
  const char        *sourceTexPath = "./assetkit_export_mr_texture_channels_src/textures/MR.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(compatibleOutDir);
  ak_test_export_cleanup(incompatibleOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  metallic  = ak_heap_calloc(heap, surface, sizeof(*metallic));
  roughness = ak_heap_calloc(heap, surface, sizeof(*roughness));
  metalRef  = ak_heap_calloc(heap, metallic, sizeof(*metalRef));
  roughRef  = ak_heap_calloc(heap, roughness, sizeof(*roughRef));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/MR.PNG";
  image->source = source;
  texture->image = image;

  metalRef->texture  = texture;
  metalRef->channels = AK_TEXTURE_CHANNEL_GB;
  roughRef->texture  = texture;
  roughRef->channels = AK_TEXTURE_CHANNEL_GB;

  metallic->source    = AK_MATERIAL_INPUT_TEXTURE;
  metallic->valueType = AK_MATERIAL_VALUE_FLOAT;
  metallic->value[0]  = 0.25f;
  metallic->channels  = AK_TEXTURE_CHANNEL_B;
  metallic->texture   = metalRef;

  roughness->source    = AK_MATERIAL_INPUT_TEXTURE;
  roughness->valueType = AK_MATERIAL_VALUE_FLOAT;
  roughness->value[0]  = 0.75f;
  roughness->channels  = AK_TEXTURE_CHANNEL_G;
  roughness->texture   = roughRef;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->metallic  = metallic;
  surface->roughness = roughness;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, compatibleOutDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(compatibleGltfPath,
                               "\"metallicRoughnessTexture\":{\"index\":0}"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"metallicFactor\":0.25"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"roughnessFactor\":0.75"));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(compatibleGltfPath, "\"textures\":["));

  roughRef->channels = AK_TEXTURE_CHANNEL_R;
  ASSERT(ak_export(doc, incompatibleOutDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(incompatibleGltfPath,
                                "\"metallicRoughnessTexture\""));
  ASSERT(ak_test_file_contains(incompatibleGltfPath, "\"metallicFactor\":0.25"));
  ASSERT(ak_test_file_contains(incompatibleGltfPath, "\"roughnessFactor\":0.75"));
  ASSERT(!ak_test_file_contains(incompatibleGltfPath, "\"images\""));
  ASSERT(!ak_test_file_contains(incompatibleGltfPath, "\"textures\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(compatibleOutDir);
  ak_test_export_cleanup(incompatibleOutDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_texture_extras) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_texture_extras");
  const char *sourceDir     = "./assetkit_export_texture_extras_src";
  const char *sourceTexDir  = "./assetkit_export_texture_extras_src/textures";
  const char *sourceTexPath = "./assetkit_export_texture_extras_src/textures/Extra.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "textures/Extra.PNG";
  image->source  = source;
  texture->image = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;

  ak_setypeid(texref, AKT_TEXTURE_REF);
  ak_setypeid(texture, AKT_TEXTURE);
  ak_setypeid(sampler, AKT_SAMPLER2D);
  ak_extra_set(texref, ak_test_extra_pair(heap, texref, "texrefNote", "roundtrip"));
  ak_extra_set(texture, ak_test_extra_pair(heap, texture, "textureNote", "roundtrip"));
  ak_extra_set(image, ak_test_extra_pair(heap, image, "imageNote", "roundtrip"));
  ak_extra_set(sampler, ak_test_extra_pair(heap, sampler, "samplerNote", "roundtrip"));

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extras\":{\"texrefNote\":\"roundtrip\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"samplers\":[{\"extras\":{\"samplerNote\":\"roundtrip\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"uri\":\"textures/Extra.PNG\",\"extras\":{\"imageNote\":\"roundtrip\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"textures\":[{\"sampler\":0,\"source\":0,\"extras\":{\"textureNote\":\"roundtrip\"}}]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_file_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_file_uri");
  const char *sourceDir = "./assetkit_export_material_texture_file_uri_src";
  const char *sourceTexPath = "./assetkit_export_material_texture_file_uri_src/WoodFile.PNG";
  const char *copiedTexPath = "./assetkit_export_material_texture_file_uri/image_0_WoodFile.PNG";
  char        absTexPath[PATH_MAX];
  char        fileUri[PATH_MAX + 8u];
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }
  ASSERT(realpath(sourceTexPath, absTexPath) != NULL);
  snprintf(fileUri, sizeof(fileUri), "file://%s", absTexPath);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = fileUri;
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_0_WoodFile.PNG\""));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_cwd_relative_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_cwd_relative_uri");
  const char *sourceTexPath = "./assetkit_export_cwd_relative_texture.png";
  const char *sourceUri     = "assetkit_export_cwd_relative_texture.png";
  const char *copiedTexPath = "./assetkit_export_material_texture_cwd_relative_uri/assetkit_export_cwd_relative_texture.png";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = sourceUri;
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"uri\":\"assetkit_export_cwd_relative_texture.png\""));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rewrites_encoded_unsafe_texture_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_encoded_unsafe_uri");
  const char *sourceDir = "./assetkit_export_encoded_unsafe_uri_src";
  const char *sourceTexDir = "./assetkit_export_encoded_unsafe_uri_src/textures";
  const char *sourceTexPath = "./assetkit_export_encoded_unsafe_uri_src/leak.png";
  const char *copiedTexPath = "./assetkit_export_encoded_unsafe_uri/image_0_leak.png";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/%2e%2e/leak.png";
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_0_leak.png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "%2e%2e"));
  ASSERT(!ak_test_file_contains(gltfPath, "../"));
  ASSERT(ak_test_file_contains(copiedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_uri_collision) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkMaterialInput   *normal;
  AkTextureRef      *baseRef;
  AkTextureRef      *normalRef;
  AkTexture         *baseTexture;
  AkTexture         *normalTexture;
  AkImage           *baseImage;
  AkImage           *normalImage;
  AkImageSource     *baseSource;
  AkImageSource     *normalSource;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_image_uri_collision");
  const char *sourceDir = "./assetkit_export_image_uri_collision_src";
  const char *sourceTexPath = "./assetkit_export_image_uri_collision_src/WoodFile.PNG";
  const char *authoredTexPath = "./assetkit_export_image_uri_collision/image_1_WoodFile.PNG";
  const char *generatedTexPath = "./assetkit_export_image_uri_collision/image_1_1_WoodFile.PNG";
  char        absTexPath[PATH_MAX];
  char        fileUri[PATH_MAX + 8u];
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }
  ASSERT(realpath(sourceTexPath, absTexPath) != NULL);
  snprintf(fileUri, sizeof(fileUri), "file://%s", absTexPath);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat           = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface       = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor     = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  normal        = ak_heap_calloc(heap, surface, sizeof(*normal));
  baseRef       = ak_heap_calloc(heap, baseColor, sizeof(*baseRef));
  normalRef     = ak_heap_calloc(heap, normal, sizeof(*normalRef));
  baseTexture   = ak_heap_calloc(heap, doc, sizeof(*baseTexture));
  normalTexture = ak_heap_calloc(heap, doc, sizeof(*normalTexture));
  baseImage     = ak_heap_calloc(heap, doc, sizeof(*baseImage));
  normalImage   = ak_heap_calloc(heap, doc, sizeof(*normalImage));
  baseSource    = ak_heap_calloc(heap, baseImage, sizeof(*baseSource));
  normalSource  = ak_heap_calloc(heap, normalImage, sizeof(*normalSource));

  baseSource->type = AK_IMAGE_SOURCE_URI;
  baseSource->uri  = "image_1_WoodFile.PNG";
  baseImage->source = baseSource;
  baseTexture->image = baseImage;
  baseRef->texture = baseTexture;

  normalSource->type = AK_IMAGE_SOURCE_URI;
  normalSource->uri  = fileUri;
  normalImage->source = normalSource;
  normalTexture->image = normalImage;
  normalRef->texture = normalTexture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = baseRef;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  normal->source    = AK_MATERIAL_INPUT_TEXTURE;
  normal->valueType = AK_MATERIAL_VALUE_TEXTURE;
  normal->texture   = normalRef;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->normal    = normal;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_1_WoodFile.PNG\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"image_1_1_WoodFile.PNG\""));
  ASSERT(!ak_test_file_contains(authoredTexPath, "PNGDATA"));
  ASSERT(ak_test_file_contains(generatedTexPath, "PNGDATA"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_instance_texcoord_binding) {
  AkHeap                 *heap;
  AkDoc                  *doc;
  AkScene                *scene;
  AkNode                 *root, *node;
  AkGeometry             *geom;
  AkMesh                 *mesh;
  AkMeshPrimitive        *prim;
  AkInstanceGeometry     *inst;
  AkMaterial             *mat;
  AkMaterialSurface      *surface;
  AkMaterialInput        *baseColor;
  AkTextureRef           *texref;
  AkTexture              *texture;
  AkImage                *image;
  AkImageSource          *source;
  AkMaterialBinding      *binding;
  AkMaterialInputBinding *inputBinding;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_instance_texcoord_binding");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "data:image/png;base64,QUJD";
  image->source  = source;
  texture->image = image;

  texref->slot     = 0;
  texref->texcoord = "UVSET";
  texref->texture  = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;

  binding      = ak_heap_calloc(heap, node, sizeof(*binding));
  inputBinding = ak_heap_calloc(heap, binding, sizeof(*inputBinding));
  binding->material      = mat;
  binding->primitive     = prim;
  binding->scope         = AK_MATERIAL_BIND_OBJECT;
  binding->propertyIndex = UINT32_MAX;
  binding->variantIndex  = UINT32_MAX;
  binding->inputBindings = inputBinding;
  inputBinding->semantic = "UVSET";
  inputBinding->inputSet = 1;

  ak_addSubNode(root, node, false);
  inst = ak_nodeAttachGeometry(node, geom);
  ASSERT(inst != NULL);
  inst->objectBindings = binding;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"TEXCOORD_1\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":1"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_texcoord_binding_uses_source_set_zero) {
  AkHeap                 *heap;
  AkDoc                  *doc;
  AkScene                *scene;
  AkNode                 *root, *node;
  AkGeometry             *geom;
  AkMesh                 *mesh;
  AkMeshPrimitive        *prim;
  AkInput                *tex0;
  AkInput                *tex1;
  AkInstanceGeometry     *inst;
  AkMaterial             *mat;
  AkMaterialSurface      *surface;
  AkMaterialInput        *baseColor;
  AkTextureRef           *texref;
  AkTexture              *texture;
  AkImage                *image;
  AkImageSource          *source;
  AkMaterialBinding      *binding;
  AkMaterialInputBinding *inputBinding;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_texcoord_source_set_zero");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  tex0 = ak_test_add_texcoord_input(heap, prim, 0);
  tex1 = ak_test_add_texcoord_input(heap, prim, 1);
  ASSERT(tex0 != NULL);
  ASSERT(tex1 != NULL);

  tex0->index = 1;
  tex1->index = 0;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "data:image/png;base64,QUJD";
  image->source  = source;
  texture->image = image;

  texref->slot     = 0;
  texref->texcoord = "UVSET0";
  texref->texture  = texture;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;

  binding      = ak_heap_calloc(heap, node, sizeof(*binding));
  inputBinding = ak_heap_calloc(heap, binding, sizeof(*inputBinding));
  binding->material      = mat;
  binding->primitive     = prim;
  binding->scope         = AK_MATERIAL_BIND_OBJECT;
  binding->propertyIndex = UINT32_MAX;
  binding->variantIndex  = UINT32_MAX;
  binding->inputBindings = inputBinding;
  inputBinding->semantic = "UVSET0";
  inputBinding->inputSet = 0;

  ak_addSubNode(root, node, false);
  inst = ak_nodeAttachGeometry(node, geom);
  ASSERT(inst != NULL);
  inst->objectBindings = binding;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_0\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"TEXCOORD_1\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":1"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_image_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unsupported_image_uri");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "textures/diffuse.tga";
  image->source = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"images\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"textures\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"baseColorTexture\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_data_uri) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_data_uri");
  const char *dataUri = "data:image/png;base64,QUJD";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = dataUri;
  image->source = source;

  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"uri\":\"data:image/png;base64,QUJD\"}]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_loaded_unsupported_image_to_png) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AkResult           exportResult;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_converted_image_png");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "textures/diffuse.tga";
  image->source  = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ak_imageInitLoader(ak_test_load_rgba_file, NULL);
  exportResult = ak_export(doc, outDir, AK_FILE_TYPE_GLTF);
  ak_imageInitLoader(NULL, NULL);

  ASSERT(exportResult == AK_OK);
  ASSERT(source->resolvedPath != NULL);
  ASSERT(strcmp(source->resolvedPath, "textures/diffuse.tga") == 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "diffuse.tga"));
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_bmp_uri_to_png_without_loader) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_bmp_image_png");
  const char *sourceDir = "./assetkit_export_bmp_source";
  const char *sourceBmp = "./assetkit_export_bmp_source/diffuse.bmp";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceBmp);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(ak_test_write_bmp_1x1(sourceBmp));

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = sourceBmp;
  image->source  = source;
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  mat->surface         = surface;
  surface->type        = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor   = baseColor;
  prim->material       = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ak_imageInitLoader(NULL, NULL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(gltfPath, "diffuse.bmp"));
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceBmp);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_converts_decoded_image_to_png) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_decoded_image_png");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));

  image->data = ak_test_load_rgba_file(heap, image, NULL, false);
  ASSERT(image->data != NULL);
  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"mimeType\":\"image/png\""));
  ASSERT(ak_test_file_count(gltfPath, "\"uri\"") == 1u);
  ASSERT(ak_test_file_contains(binPath, "PNG"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_glb_embeds_decoded_uri_image) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_glb_decoded_uri_image");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "missing/Wood.PNG";
  image->source = source;
  image->data   = ak_test_load_rgba_file(heap, image, NULL, false);
  ASSERT(image->data != NULL);

  texture->image = image;
  texref->texture = texture;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(ak_test_file_contains(glbPath, "\"bufferView\""));
  ASSERT(ak_test_file_contains(glbPath, "\"mimeType\":\"image/png\""));
  ASSERT(!ak_test_file_contains(glbPath, "\"uri\":\"missing/Wood.PNG\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_ktx2) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkScene          *scene;
  AkNode           *root, *node;
  AkGeometry       *geom;
  AkMesh           *mesh;
  AkMeshPrimitive  *prim;
  AkMaterial       *mat;
  AkMaterialSurface *surface;
  AkMaterialInput  *baseColor;
  AkTextureRef     *texref;
  AkTexture        *texture;
  AkImage          *image;
  AkImageSource    *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_ktx2");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "albedo.ktx2";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor        = baseColor;
  mat->surface              = surface;
  prim->material            = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_texture_basisu\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"KHR_texture_basisu\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_texture_basisu\":{\"source\":0}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"albedo.ktx2\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_webp) {
  AkHeap           *heap;
  AkDoc            *doc;
  AkScene          *scene;
  AkNode           *root, *node;
  AkGeometry       *geom;
  AkMesh           *mesh;
  AkMeshPrimitive  *prim;
  AkMaterial       *mat;
  AkMaterialSurface *surface;
  AkMaterialInput  *baseColor;
  AkTextureRef     *texref;
  AkTexture        *texture;
  AkImage          *image;
  AkImageSource    *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_webp");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type  = AK_IMAGE_SOURCE_URI;
  source->uri   = "albedo.webp";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor        = baseColor;
  mat->surface              = surface;
  prim->material            = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_texture_webp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"EXT_texture_webp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_texture_webp\":{\"source\":0}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"uri\":\"albedo.webp\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_transform) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkMaterial         *mat;
  AkMaterialSurface  *surface;
  AkMaterialInput    *baseColor;
  AkTextureRef       *texref;
  AkTextureTransform *transform;
  AkTexture          *texture;
  AkImage            *image;
  AkImageSource      *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_transform");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);
  ASSERT(ak_test_add_texcoord_input(heap, prim, 1) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  transform = ak_heap_calloc(heap, texref, sizeof(*transform));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "albedo.png";
  image->source = source;
  texture->image = image;

  transform->offset[0] = 0.25f;
  transform->offset[1] = 0.5f;
  transform->rotation  = 1.0f;
  transform->scale[0]  = 2.0f;
  transform->scale[1]  = 3.0f;
  transform->slot      = 1;

  texref->texture   = texture;
  texref->slot      = 0;
  texref->transform = transform;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior       = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_texture_transform\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"texCoord\":1,\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.25,0.5],\"rotation\":1,\"scale\":[2,3],\"texCoord\":1}}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_transform_invalid_slot) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkScene            *scene;
  AkNode             *root, *node;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkMaterial         *mat;
  AkMaterialSurface  *surface;
  AkMaterialInput    *baseColor;
  AkTextureRef       *texref;
  AkTextureTransform *transform;
  AkTexture          *texture;
  AkImage            *image;
  AkImageSource      *source;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_transform_bad_slot");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  transform = ak_heap_calloc(heap, texref, sizeof(*transform));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "data:image/png;base64,QUJD";
  image->source = source;
  texture->image = image;

  transform->offset[0] = 0.25f;
  transform->offset[1] = 0.5f;
  transform->scale[0]  = 1.0f;
  transform->scale[1]  = 1.0f;
  transform->slot      = 7;

  texref->texture   = texture;
  texref->slot      = 0;
  texref->transform = transform;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior       = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_texture_transform\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extensions\":{\"KHR_texture_transform\":{\"offset\":[0.25,0.5]}}}"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":7"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"texCoord\":0"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_texture_buffer_view) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkImage           *image;
  AkImageSource     *source;
  AkBuffer          *imageBuffer;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_texture_buffer");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const unsigned char pngBytes[8] = {
    0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat         = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface     = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor   = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref      = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture     = ak_heap_calloc(heap, doc, sizeof(*texture));
  image       = ak_heap_calloc(heap, doc, sizeof(*image));
  source      = ak_heap_calloc(heap, image, sizeof(*source));
  imageBuffer = ak_heap_calloc(heap, source, sizeof(*imageBuffer));

  imageBuffer->length = sizeof(pngBytes);
  imageBuffer->data = ak_heap_alloc(heap, imageBuffer, imageBuffer->length);
  memcpy(imageBuffer->data, pngBytes, sizeof(pngBytes));

  source->type     = AK_IMAGE_SOURCE_BUFFER;
  source->buffer   = imageBuffer;
  source->mimeType = "image/png";
  image->source    = source;

  texture->image = image;
  texref->texture = texture;
  texref->slot = 0;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;

  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  prim->material     = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"images\":["));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"accessors\":[{\"bufferView\":0"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"images\":[{\"bufferView\":1,\"mimeType\":\"image/png\"}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"byteLength\":8"));
  ASSERT(ak_test_file_contains(gltfPath, "\"baseColorTexture\":{\"index\":0}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_extensions) {
  AkHeap                     *heap;
  AkDoc                      *doc;
  AkDoc                      *roundTrip;
  AkScene                    *scene;
  AkNode                     *root, *node;
  AkGeometry                 *geom;
  AkMesh                     *mesh;
  AkMeshPrimitive            *prim;
  AkMaterial                 *mat;
  AkMaterialSurface          *surface;
  AkMaterialClearcoatFeature *clearcoat;
  AkMaterialSpecularFeature  *specular;
  AkMaterialInput            *clearcoatFactor;
  AkMaterialInput            *specularColor;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_ext");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat             = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface         = ak_heap_calloc(heap, mat, sizeof(*surface));
  clearcoat       = ak_heap_calloc(heap, surface, sizeof(*clearcoat));
  specular        = ak_heap_calloc(heap, surface, sizeof(*specular));
  clearcoatFactor = ak_heap_calloc(heap, clearcoat, sizeof(*clearcoatFactor));
  specularColor   = ak_heap_calloc(heap, specular, sizeof(*specularColor));

  mat->surface                = surface;
  surface->type               = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior                = 1.33f;
  surface->emissiveStrength   = 2.0f;
  surface->features           = &clearcoat->base;
  clearcoat->base.next        = &specular->base;
  clearcoat->base.type        = AK_MATERIAL_FEATURE_CLEARCOAT;
  clearcoat->factor           = clearcoatFactor;
  specular->base.type         = AK_MATERIAL_FEATURE_SPECULAR;
  specular->color             = specularColor;
  clearcoatFactor->source     = AK_MATERIAL_INPUT_CONSTANT;
  clearcoatFactor->valueType  = AK_MATERIAL_VALUE_FLOAT;
  clearcoatFactor->value[0]   = 0.7f;
  specularColor->source       = AK_MATERIAL_INPUT_CONSTANT;
  specularColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  specularColor->color.vec[0] = 0.5f;
  specularColor->color.vec[1] = 0.6f;
  specularColor->color.vec[2] = 0.7f;
  specularColor->color.vec[3] = 1.0f;
  prim->material              = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_unlit\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_emissive_strength\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"emissiveStrength\":2"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_ior\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"ior\":1.33000004"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_clearcoat\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"clearcoatFactor\":0.699999988"));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_specular\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"specularColorFactor\":[0.5,0.600000024,0.699999988]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_specular_glossiness) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkMaterial      *mat;
  AkMaterialSurface *surface;
  AkMaterialSpecularGlossinessFeature *sg;
  AkMaterialInput *diffuse;
  AkMaterialInput *specular;
  AkMaterialInput *glossiness;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_specgloss");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  sg        = ak_heap_calloc(heap, surface, sizeof(*sg));
  diffuse   = ak_heap_calloc(heap, sg, sizeof(*diffuse));
  specular  = ak_heap_calloc(heap, sg, sizeof(*specular));
  glossiness = ak_heap_calloc(heap, sg, sizeof(*glossiness));

  surface->type             = AK_MATERIAL_TYPE_PBR_SPECULAR_GLOSSINESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->features         = (AkMaterialFeature *)sg;
  sg->base.type             = AK_MATERIAL_FEATURE_SPECULAR_GLOSSINESS;
  sg->diffuse               = diffuse;
  sg->specular              = specular;
  sg->glossiness            = glossiness;

  diffuse->source      = AK_MATERIAL_INPUT_CONSTANT;
  diffuse->valueType   = AK_MATERIAL_VALUE_COLOR;
  diffuse->color.rgba.R = 0.2f;
  diffuse->color.rgba.G = 0.3f;
  diffuse->color.rgba.B = 0.4f;
  diffuse->color.rgba.A = 0.5f;

  specular->source      = AK_MATERIAL_INPUT_CONSTANT;
  specular->valueType   = AK_MATERIAL_VALUE_COLOR;
  specular->color.rgba.R = 0.6f;
  specular->color.rgba.G = 0.7f;
  specular->color.rgba.B = 0.8f;
  specular->color.rgba.A = 1.0f;

  glossiness->source    = AK_MATERIAL_INPUT_CONSTANT;
  glossiness->valueType = AK_MATERIAL_VALUE_FLOAT;
  glossiness->value[0]  = 0.9f;

  mat->surface = surface;
  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_pbrSpecularGlossiness\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"diffuseFactor\":[0.200000003,0.300000012,0.400000006,0.5]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"specularFactor\":[0.600000024,0.699999988,0.800000012]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"glossinessFactor\":0.899999976"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_volume_scatter) {
  AkHeap                     *heap;
  AkDoc                      *doc;
  AkDoc                      *roundTrip;
  AkScene                    *scene;
  AkNode                     *root, *node;
  AkGeometry                 *geom;
  AkMesh                     *mesh;
  AkMeshPrimitive            *prim;
  AkMaterial                 *mat;
  AkMaterialSurface          *surface;
  AkMaterialSubsurfaceFeature *subsurface;
  AkMaterialInput            *color;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_volscatter");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat        = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface    = ak_heap_calloc(heap, mat, sizeof(*surface));
  subsurface = ak_heap_calloc(heap, surface, sizeof(*subsurface));
  color      = ak_heap_calloc(heap, subsurface, sizeof(*color));

  surface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->ior              = 1.5f;
  surface->emissiveStrength = 1.0f;
  surface->features         = &subsurface->base;
  subsurface->base.type     = AK_MATERIAL_FEATURE_SUBSURFACE;
  subsurface->color         = color;
  subsurface->anisotropy    = 0.125f;

  color->source       = AK_MATERIAL_INPUT_CONSTANT;
  color->valueType    = AK_MATERIAL_VALUE_COLOR;
  color->color.vec[0] = 0.25f;
  color->color.vec[1] = 0.5f;
  color->color.vec[2] = 0.75f;
  color->color.vec[3] = 1.0f;

  mat->surface   = surface;
  prim->material = mat;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_materials_volume_scatter\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"multiscatterColorFactor\":[0.25,0.5,0.75]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"scatterAnisotropy\":0.125"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_material_variants) {
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkDoc                    *roundTrip;
  AkScene                  *scene;
  AkNode                   *root, *node;
  AkGeometry               *geom;
  AkMesh                   *mesh;
  AkMeshPrimitive          *prim;
  AkMaterial               *baseMat;
  AkMaterial               *redMat;
  AkMaterial               *blueMat;
  AkMaterial               *greenMat;
  AkMaterialVariant        *redVariant;
  AkMaterialVariant        *blueVariant;
  AkMaterialVariant        *greenVariant;
  AkMaterialVariantMapping *redMapping;
  AkMaterialVariantMapping *blueMapping;
  AkMaterialVariantMapping *greenMapping;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_material_variants");
  const char *roundDir      = "./assetkit_export_material_variants_round";
  const char *roundGltfPath = "./assetkit_export_material_variants_round/model.gltf";
  const char *roundBinPath  = "./assetkit_export_material_variants_round/model.bin";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  redVariant   = ak_heap_calloc(heap, doc, sizeof(*redVariant));
  blueVariant  = ak_heap_calloc(heap, doc, sizeof(*blueVariant));
  greenVariant = ak_heap_calloc(heap, doc, sizeof(*greenVariant));
  redVariant->name       = "red";
  redVariant->next       = blueVariant;
  blueVariant->name      = "blue";
  blueVariant->next      = greenVariant;
  greenVariant->name     = "green";
  doc->materialVariants  = redVariant;
  doc->materialVariantCount = 0;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  baseMat      = ak_heap_calloc(heap, doc, sizeof(*baseMat));
  redMat       = ak_heap_calloc(heap, doc, sizeof(*redMat));
  blueMat      = ak_heap_calloc(heap, doc, sizeof(*blueMat));
  greenMat     = ak_heap_calloc(heap, doc, sizeof(*greenMat));
  redMapping   = ak_heap_calloc(heap, prim, sizeof(*redMapping));
  blueMapping  = ak_heap_calloc(heap, prim, sizeof(*blueMapping));
  greenMapping = ak_heap_calloc(heap, prim, sizeof(*greenMapping));
  baseMat->name  = "base";
  redMat->name   = "redMat";
  blueMat->name  = "blueMat";
  greenMat->name = "greenMat";
  prim->material = baseMat;

  redMapping->material     = redMat;
  redMapping->variantIndex = 0;
  redMapping->next         = blueMapping;
  blueMapping->material     = blueMat;
  blueMapping->variantIndex = 1;
  blueMapping->next         = greenMapping;
  greenMapping->material     = greenMat;
  greenMapping->variantIndex = 2;
  prim->variantMappings      = redMapping;
  prim->variantMappingCount  = 3;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_materials_variants\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"variants\":[{\"name\":\"red\"},{\"name\":\"blue\"},{\"name\":\"green\"}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"mappings\":[{\"material\":1,\"variants\":[0]},{\"material\":2,\"variants\":[1]},{\"material\":3,\"variants\":[2]}]"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->materialVariantCount == 3);
  ASSERT(ak_export(roundTrip, roundDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_files_equal(gltfPath, roundGltfPath));
  ASSERT(ak_test_files_equal(binPath, roundBinPath));
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(roundDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_root_xmp_extension) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkTreeNode     *extra;
  AkTreeNode     *extensions;
  AkTreeNode     *xmp;
  AkTreeNode     *packets;
  AkTreeNode     *packet;
  AkTreeNode     *xml;
  AkTreeNodeAttr *arrayAttr;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_root_xmp");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  extra      = ak_heap_calloc(heap, doc, sizeof(*extra));
  extensions = ak_heap_calloc(heap, extra, sizeof(*extensions));
  xmp        = ak_heap_calloc(heap, extensions, sizeof(*xmp));
  packets    = ak_heap_calloc(heap, xmp, sizeof(*packets));
  packet     = ak_heap_calloc(heap, packets, sizeof(*packet));
  xml        = ak_heap_calloc(heap, packet, sizeof(*xml));
  arrayAttr  = ak_heap_calloc(heap, packets, sizeof(*arrayAttr));
  ASSERT(extra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(xmp != NULL);
  ASSERT(packets != NULL);
  ASSERT(packet != NULL);
  ASSERT(xml != NULL);
  ASSERT(arrayAttr != NULL);

  extra->name      = "root";
  extra->chld      = extensions;
  extra->chldc     = 1;
  extensions->name = "extensions";
  extensions->parent = extra;
  extensions->chld   = xmp;
  extensions->chldc  = 1;
  xmp->name       = "KHR_xmp";
  xmp->parent     = extensions;
  xmp->chld       = packets;
  xmp->chldc      = 1;
  packets->name   = "packets";
  packets->parent = xmp;
  packets->attribs = arrayAttr;
  packets->attrc   = 1;
  packets->chld    = packet;
  packets->chldc   = 1;
  arrayAttr->name  = "type";
  arrayAttr->val   = (char *)"array";
  packet->parent   = packets;
  packet->chld     = xml;
  packet->chldc    = 1;
  xml->name        = "xml";
  xml->val         = (char *)"<x:xmpmeta/>";
  xml->parent      = packet;

  ak_extra_set(doc, extra);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_xmp\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":[\"KHR_xmp\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"packets\":[{\"xml\":\"<x:xmpmeta/>\"}]"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"extensionsRequired\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_root_extension) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkTreeNode     *extra;
  AkTreeNode     *extensions;
  AkTreeNode     *required;
  AkTreeNode     *requiredItem;
  AkTreeNode     *ibl;
  AkTreeNode     *lights;
  AkTreeNode     *light;
  AkTreeNode     *intensity;
  AkTreeNodeAttr *lightsArrayAttr;
  AkTreeNodeAttr *requiredArrayAttr;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_root_extension");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  extra             = ak_heap_calloc(heap, doc, sizeof(*extra));
  extensions        = ak_heap_calloc(heap, extra, sizeof(*extensions));
  required          = ak_heap_calloc(heap, extra, sizeof(*required));
  requiredItem      = ak_heap_calloc(heap, required, sizeof(*requiredItem));
  ibl               = ak_heap_calloc(heap, extensions, sizeof(*ibl));
  lights            = ak_heap_calloc(heap, ibl, sizeof(*lights));
  light             = ak_heap_calloc(heap, lights, sizeof(*light));
  intensity         = ak_heap_calloc(heap, light, sizeof(*intensity));
  lightsArrayAttr   = ak_heap_calloc(heap, lights, sizeof(*lightsArrayAttr));
  requiredArrayAttr = ak_heap_calloc(heap, required, sizeof(*requiredArrayAttr));
  ASSERT(extra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(required != NULL);
  ASSERT(requiredItem != NULL);
  ASSERT(ibl != NULL);
  ASSERT(lights != NULL);
  ASSERT(light != NULL);
  ASSERT(intensity != NULL);
  ASSERT(lightsArrayAttr != NULL);
  ASSERT(requiredArrayAttr != NULL);

  extra->name       = "root";
  extra->chld       = extensions;
  extra->chldc      = 2;
  extensions->name  = "extensions";
  extensions->parent = extra;
  extensions->next  = required;
  extensions->chld  = ibl;
  extensions->chldc = 1;
  required->name    = "extensionsRequired";
  required->parent  = extra;
  required->attribs = requiredArrayAttr;
  required->attrc   = 1;
  required->chld    = requiredItem;
  required->chldc   = 1;
  requiredArrayAttr->name = "type";
  requiredArrayAttr->val  = (char *)"array";
  requiredItem->parent = required;
  requiredItem->val    = (char *)"EXT_lights_image_based";

  ibl->name       = "EXT_lights_image_based";
  ibl->parent     = extensions;
  ibl->chld       = lights;
  ibl->chldc      = 1;
  lights->name    = "lights";
  lights->parent  = ibl;
  lights->attribs = lightsArrayAttr;
  lights->attrc   = 1;
  lights->chld    = light;
  lights->chldc   = 1;
  lightsArrayAttr->name = "type";
  lightsArrayAttr->val  = (char *)"array";
  light->parent    = lights;
  light->chld      = intensity;
  light->chldc     = 1;
  intensity->name  = "intensity";
  intensity->val   = (char *)"2";
  intensity->parent = light;

  ak_extra_set(doc, extra);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_lights_image_based\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"EXT_lights_image_based\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_lights_image_based\":{\"lights\":[{\"intensity\":2}]}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_object_extension) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTreeNode        *docExtra;
  AkTreeNode        *required;
  AkTreeNode        *requiredItem;
  AkTreeNodeAttr    *requiredArrayAttr;
  AkTreeNode        *matExtra;
  AkTreeNode        *extensions;
  AkTreeNode        *metadata;
  AkTreeNode        *level;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_object_extension");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  root->visible = true;
  node->visible = true;
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);

  mat->name         = "preservedExtMat";
  mat->surface      = surface;
  surface->type     = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->emissiveStrength = 1.0f;
  surface->ior      = 1.5f;
  baseColor->source = AK_MATERIAL_INPUT_CONSTANT;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->color.rgba.R = 0.4f;
  baseColor->color.rgba.G = 0.5f;
  baseColor->color.rgba.B = 0.6f;
  baseColor->color.rgba.A = 1.0f;

  matExtra   = ak_heap_calloc(heap, mat, sizeof(*matExtra));
  extensions = ak_heap_calloc(heap, matExtra, sizeof(*extensions));
  metadata   = ak_heap_calloc(heap, extensions, sizeof(*metadata));
  level      = ak_heap_calloc(heap, metadata, sizeof(*level));
  ASSERT(matExtra != NULL);
  ASSERT(extensions != NULL);
  ASSERT(metadata != NULL);
  ASSERT(level != NULL);

  matExtra->name      = "root";
  matExtra->chld      = extensions;
  matExtra->chldc     = 1;
  extensions->name    = "extensions";
  extensions->parent  = matExtra;
  extensions->chld    = metadata;
  extensions->chldc   = 1;
  metadata->name      = "AGI_stk_metadata";
  metadata->parent    = extensions;
  metadata->chld      = level;
  metadata->chldc     = 1;
  level->name         = "level";
  level->val          = (char *)"7";
  level->parent       = metadata;
  ak_extra_set(mat, matExtra);

  docExtra          = ak_heap_calloc(heap, doc, sizeof(*docExtra));
  required          = ak_heap_calloc(heap, docExtra, sizeof(*required));
  requiredItem      = ak_heap_calloc(heap, required, sizeof(*requiredItem));
  requiredArrayAttr = ak_heap_calloc(heap, required, sizeof(*requiredArrayAttr));
  ASSERT(docExtra != NULL);
  ASSERT(required != NULL);
  ASSERT(requiredItem != NULL);
  ASSERT(requiredArrayAttr != NULL);

  docExtra->name          = "root";
  docExtra->chld          = required;
  docExtra->chldc         = 1;
  required->name          = "extensionsRequired";
  required->parent        = docExtra;
  required->attribs       = requiredArrayAttr;
  required->attrc         = 1;
  required->chld          = requiredItem;
  required->chldc         = 1;
  requiredArrayAttr->name = "type";
  requiredArrayAttr->val  = (char *)"array";
  requiredItem->parent    = required;
  requiredItem->val       = (char *)"AGI_stk_metadata";
  ak_extra_set(doc, docExtra);

  prim->material = mat;
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"AGI_stk_metadata\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"AGI_stk_metadata\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"AGI_stk_metadata\":{\"level\":7}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_preserved_object_extensions_across_kinds) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkScene           *scene;
  AkNode            *root, *node;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkImageSource     *source;
  const char *requiredNames[] = {
    "EXT_scene_object",
    "EXT_node_object",
    "EXT_mesh_object",
    "EXT_primitive_object",
    "EXT_texinfo_object",
    "EXT_texture_object",
    "EXT_image_object",
    "EXT_sampler_object"
  };
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_preserved_object_ext_kinds");
  const char *sourceDir     = "./assetkit_export_preserved_object_ext_src";
  const char *sourceTexDir  = "./assetkit_export_preserved_object_ext_src/textures";
  const char *sourceTexPath = "./assetkit_export_preserved_object_ext_src/textures/Ext.PNG";
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);
  ASSERT(mkdir(sourceDir, 0777) == 0);
  ASSERT(mkdir(sourceTexDir, 0777) == 0);
  {
    FILE *file;

    file = fopen(sourceTexPath, "wb");
    ASSERT(file != NULL);
    ASSERT(fwrite("PNGDATA", 1, 7, file) == 7);
    ASSERT(fclose(file) == 0);
  }

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->inf      = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  doc->inf->dir = sourceDir;

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ASSERT(ak_test_add_texcoord_input(heap, prim, 0) != NULL);

  mat       = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface   = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_heap_calloc(heap, surface, sizeof(*baseColor));
  texref    = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture   = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler   = ak_heap_calloc(heap, doc, sizeof(*sampler));
  image     = ak_heap_calloc(heap, doc, sizeof(*image));
  source    = ak_heap_calloc(heap, image, sizeof(*source));
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(image != NULL);
  ASSERT(source != NULL);

  source->type   = AK_IMAGE_SOURCE_URI;
  source->uri    = "textures/Ext.PNG";
  image->source  = source;
  texture->image = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;
  ak_setypeid(texref, AKT_TEXTURE_REF);
  ak_setypeid(texture, AKT_TEXTURE);
  ak_setypeid(sampler, AKT_SAMPLER2D);

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;
  mat->surface       = surface;
  surface->type      = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor = baseColor;
  surface->emissiveStrength = 1.0f;
  surface->ior       = 1.5f;
  prim->material     = mat;

  ak_extra_set(doc, ak_test_extra_required_extensions(
                 heap, doc, requiredNames, (uint32_t)AK_ARRAY_LEN(requiredNames)));
  ak_extra_set(scene, ak_test_extra_extension_pair(
                 heap, doc, "EXT_scene_object", "scope", "scene"));
  ak_extra_set(node, ak_test_extra_extension_pair(
                 heap, doc, "EXT_node_object", "scope", "node"));
  mesh->extra = ak_test_extra_extension_pair(
    heap, doc, "EXT_mesh_object", "scope", "mesh");
  prim->extra = ak_test_extra_extension_pair(
    heap, doc, "EXT_primitive_object", "scope", "primitive");
  ak_extra_set(texref, ak_test_extra_extension_pair(
                 heap, doc, "EXT_texinfo_object", "scope", "texinfo"));
  ak_extra_set(texture, ak_test_extra_extension_pair(
                 heap, doc, "EXT_texture_object", "scope", "texture"));
  ak_extra_set(image, ak_test_extra_extension_pair(
                 heap, doc, "EXT_image_object", "scope", "image"));
  ak_extra_set(sampler, ak_test_extra_extension_pair(
                 heap, doc, "EXT_sampler_object", "scope", "sampler"));

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsUsed\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"extensionsRequired\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"material\":0"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_emissive_strength\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_scene_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_node_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_mesh_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_primitive_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_texinfo_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_texture_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_image_object\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"EXT_sampler_object\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_scene_object\":{\"scope\":\"scene\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_node_object\":{\"scope\":\"node\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_mesh_object\":{\"scope\":\"mesh\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_primitive_object\":{\"scope\":\"primitive\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"baseColorTexture\":{\"index\":0,\"extensions\":{\"EXT_texinfo_object\":{\"scope\":\"texinfo\"}}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_texture_object\":{\"scope\":\"texture\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_image_object\":{\"scope\":\"image\"}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"EXT_sampler_object\":{\"scope\":\"sampler\"}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  unlink(sourceTexPath);
  rmdir(sourceTexDir);
  rmdir(sourceDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_omits_unused_material_variants) {
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkScene                  *scene;
  AkNode                   *root, *node;
  AkGeometry               *geom;
  AkMesh                   *mesh;
  AkMeshPrimitive          *prim;
  AkMaterial               *noopMat;
  AkMaterial               *badSlotMat;
  AkMaterialSurface        *noopSurface;
  AkMaterialSurface        *badSlotSurface;
  AkMaterialInput          *baseColor;
  AkTextureRef             *texref;
  AkTexture                *texture;
  AkImage                  *image;
  AkImageSource            *source;
  AkMaterialVariant        *variant;
  AkMaterialVariantMapping *mapping;
  AkMaterialVariantMapping *badSlotMapping;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unused_material_variants");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  variant = ak_heap_calloc(heap, doc, sizeof(*variant));
  variant->name = "unused";
  doc->materialVariants = variant;
  doc->materialVariantCount = 1;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  noopMat = ak_heap_calloc(heap, doc, sizeof(*noopMat));
  noopSurface = ak_heap_calloc(heap, noopMat, sizeof(*noopSurface));
  noopSurface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  noopSurface->alphaCutoff      = 0.5f;
  noopSurface->ior              = 1.5f;
  noopSurface->emissiveStrength = 1.0f;
  noopMat->surface              = noopSurface;

  badSlotMat     = ak_heap_calloc(heap, doc, sizeof(*badSlotMat));
  badSlotSurface = ak_heap_calloc(heap, badSlotMat, sizeof(*badSlotSurface));
  baseColor      = ak_heap_calloc(heap, badSlotSurface, sizeof(*baseColor));
  texref         = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture        = ak_heap_calloc(heap, doc, sizeof(*texture));
  image          = ak_heap_calloc(heap, doc, sizeof(*image));
  source         = ak_heap_calloc(heap, image, sizeof(*source));

  source->type = AK_IMAGE_SOURCE_URI;
  source->uri  = "data:image/png;base64,QUJD";
  image->source = source;
  texture->image = image;
  texref->texture = texture;
  texref->slot = 9;

  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  badSlotSurface->type             = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  badSlotSurface->alphaCutoff      = 0.5f;
  badSlotSurface->ior              = 1.5f;
  badSlotSurface->emissiveStrength = 1.0f;
  badSlotSurface->baseColor        = baseColor;
  badSlotMat->name                 = "badSlot";
  badSlotMat->surface              = badSlotSurface;

  mapping                      = ak_heap_calloc(heap, prim, sizeof(*mapping));
  badSlotMapping               = ak_heap_calloc(heap, prim, sizeof(*badSlotMapping));
  mapping->material            = noopMat;
  mapping->variantIndex        = 0;
  mapping->next                = badSlotMapping;
  badSlotMapping->material     = badSlotMat;
  badSlotMapping->variantIndex = 0;
  prim->variantMappings        = mapping;
  prim->variantMappingCount    = 2;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_materials_variants\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_perspective_camera) {
  AkHeap   *heap;
  AkDoc    *doc;
  AkScene  *scene;
  AkNode   *root, *node;
  AkCamera *camera;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_perspective_camera");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  camera->name = "MainCamera";

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"cameras\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"camera\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"perspective\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"yfov\":0.785398185"));
  ASSERT(ak_test_file_contains(gltfPath, "\"znear\":0.100000001"));
  ASSERT(ak_test_file_contains(gltfPath, "\"zfar\":1000"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_perspective_camera_glb) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkDoc      *roundTrip;
  AkScene    *scene;
  AkNode     *root, *node;
  AkCamera   *camera;
  struct stat stGlb;
  struct stat stBin;
  uint32_t    length;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_perspective_camera_glb");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  camera->name = "MainCamera";

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(ak_test_file_contains(glbPath, "\"cameras\":["));
  ASSERT(ak_test_file_contains(glbPath, "\"camera\":0"));
  ASSERT(ak_test_file_contains(glbPath, "\"type\":\"perspective\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_camera_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkCamera   *camera;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_camera_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "cameraNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(camera, extra);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"cameraNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_punctual_light) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *root, *node;
  AkLight *light;
  AkLightBase *base;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_punctual_light");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);
  light->name = "PointLight";
  base = light->data;
  ASSERT(base != NULL);
  base->color.rgba.R = 0.25f;
  base->color.rgba.G = 0.5f;
  base->color.rgba.B = 0.75f;
  base->intensity    = 4.0f;
  base->range        = 12.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_lights_punctual\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"KHR_lights_punctual\":{\"lights\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"point\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"color\":[0.25,0.5,0.75]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"intensity\":4"));
  ASSERT(ak_test_file_contains(gltfPath, "\"range\":12"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_lights_punctual\":{\"light\":0}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_light_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkLight    *light;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_light_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "lightNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(light, extra);

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"lightNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_punctual_light_glb) {
  AkHeap      *heap;
  AkDoc       *doc;
  AkDoc       *roundTrip;
  AkScene     *scene;
  AkNode      *root, *node;
  AkLight     *light;
  AkLightBase *base;
  struct stat  stGlb;
  struct stat  stBin;
  uint32_t     length;
  AK_TEST_EXPORT_GLB_PATHS("assetkit_export_punctual_light_glb");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_POINT);
  ASSERT(light != NULL);
  light->name = "PointLight";
  base = light->data;
  ASSERT(base != NULL);
  base->color.rgba.R = 0.25f;
  base->color.rgba.G = 0.5f;
  base->color.rgba.B = 0.75f;
  base->intensity    = 4.0f;
  base->range        = 12.0f;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(stat(glbPath, &stGlb) == 0);
  ASSERT(stat(binPath, &stBin) != 0);
  ASSERT(stGlb.st_size > 20);
  ASSERT(ak_test_read_u32le(glbPath, 8, &length));
  ASSERT(length == (uint32_t)stGlb.st_size);
  ASSERT(ak_test_file_contains(glbPath, "\"KHR_lights_punctual\""));
  ASSERT(ak_test_file_contains(glbPath, "\"type\":\"point\""));
  ASSERT(ak_test_file_contains(glbPath, "\"light\":0"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, glbPath, AK_FILE_TYPE_GLB) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_invalid_camera) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkCamera      *camera;
  AkPerspective *persp;
  struct stat    stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_invalid_camera");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  camera = ak_camMakePerspective(doc, doc, 0.785398185f, 1.77777779f,
                                 0.1f, 1000.0f);
  ASSERT(camera != NULL);
  persp = (AkPerspective *)camera->optics->proj;
  persp->zfar = INFINITY;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachCamera(node, camera) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stFile) == 0);
  ASSERT(!ak_test_file_contains(gltfPath, "\"cameras\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"camera\":"));
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_invalid_punctual_light) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkLight    *light;
  AkSpotLight *spot;
  struct stat stFile;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_invalid_punctual_light");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  light = ak_lightMake(doc, doc, AK_LIGHT_TYPE_SPOT);
  ASSERT(light != NULL);
  spot = (AkSpotLight *)light->data;
  ASSERT(spot != NULL);
  spot->innerConeAngle = spot->outerConeAngle;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachLight(node, light) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(stat(gltfPath, &stFile) == 0);
  ASSERT(!ak_test_file_contains(gltfPath, "\"KHR_lights_punctual\""));
  ASSERT(!ak_test_file_contains(gltfPath, "\"light\":"));
  ASSERT(stat(binPath, &stFile) != 0);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_visibility) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_node_visibility");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = false;
  node->name    = "hidden";

  ak_addSubNode(root, node, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_node_visibility\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_node_visibility\":{\"visible\":false}}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_extras) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkTreeNode *extra;
  AkTreeNode *note;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_node_extras");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  extra       = ak_heap_calloc(heap, node, sizeof(*extra));
  note        = ak_heap_calloc(heap, extra, sizeof(*note));
  ASSERT(scene != NULL);
  ASSERT(root != NULL);
  ASSERT(node != NULL);
  ASSERT(extra != NULL);
  ASSERT(note != NULL);

  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;
  node->name    = "extraNode";

  extra->name   = "extras";
  extra->chld   = note;
  extra->chldc  = 1;
  note->name    = "authorNote";
  note->val     = (char *)"roundtrip";
  note->parent  = extra;
  ak_extra_set(node, extra);

  ak_addSubNode(root, node, false);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"authorNote\":\"roundtrip\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_gpu_instancing) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkGpuInstancing *instancing;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_gpu_instancing");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    2.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  instancing              = ak_heap_calloc(heap, node, sizeof(*instancing));
  instancing->translation = ak_test_make_float_accessor(heap,
                                                        instancing,
                                                        translations,
                                                        3,
                                                        2);
  instancing->count       = 2;
  node->gpuInstancing     = instancing;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"EXT_mesh_gpu_instancing\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"EXT_mesh_gpu_instancing\":{\"attributes\":{\"TRANSLATION\":1}}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_mesh_quantization) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *node;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *posAcc;
  AkBuffer        *posBuff;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_mesh_quantization");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const int16_t quantized[9] = {
    0, 0, 0,
    1000, 0, 0,
    0, 1000, 0
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  posAcc = prim->pos->accessor;
  posBuff = ak_heap_calloc(heap, posAcc, sizeof(*posBuff));
  posBuff->length = sizeof(quantized);
  posBuff->data   = ak_heap_alloc(heap, posBuff, posBuff->length);
  memcpy(posBuff->data, quantized, sizeof(quantized));

  posAcc->buffer                = posBuff;
  posAcc->byteLength            = posBuff->length;
  posAcc->byteStride            = sizeof(int16_t) * 3;
  posAcc->fillByteSize          = sizeof(int16_t) * 3;
  posAcc->bytesPerComponent     = sizeof(int16_t);
  posAcc->componentType         = AKT_SHORT;
  posAcc->originalComponentType = AKT_SHORT;
  posAcc->normalized            = false;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsUsed\":[\"KHR_mesh_quantization\"]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensionsRequired\":[\"KHR_mesh_quantization\"]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5122"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_native_skin) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *jointA, *jointB, *meshNode;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin *skinner;
  AkSkin         *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_native_skin");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  size_t i;

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  jointA->name = "JointA";
  jointB->name = "JointB";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton = jointA;
  skin->invBindPoses = ak_heap_alloc(heap,
                                     skin,
                                     sizeof(*skin->invBindPoses) * 2);
  for (i = 0; i < 32; i++)
    ((float *)skin->invBindPoses)[i] = ((i % 16) % 5) == 0 ? 1.0f : 0.0f;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(root, jointA, false);
  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, meshNode, false);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skins\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"skin\":0"));
  ASSERT(ak_test_file_contains(gltfPath, "\"inverseBindMatrices\":3"));
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));
  ASSERT(ak_test_file_contains(gltfPath, "\"joints\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"JOINTS_0\":1"));
  ASSERT(ak_test_file_contains(gltfPath, "\"WEIGHTS_0\":2"));
  ASSERT(ak_test_file_contains(gltfPath, "\"type\":\"MAT4\""));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_unscened_skin_joints_scene_root) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkDoc           *roundTrip;
  AkScene         *scene;
  AkNode          *root, *jointA, *jointB, *meshNode;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin  *skinner;
  AkSkin          *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_unscened_skin");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  root->name     = "SceneRoot";
  jointA->name   = "JointA";
  jointB->name   = "JointB";
  meshNode->name = "Mesh";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton  = jointA;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, meshNode, false);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Mesh") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "JointA") != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_instanced_skin_joints_not_scene_root) {
  AkHeap             *heap;
  AkDoc              *doc;
  AkDoc              *roundTrip;
  AkScene            *scene;
  AkNode             *root, *group, *jointA, *jointB, *meshNode;
  AkGeometry         *geom;
  AkMesh             *mesh;
  AkMeshPrimitive    *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceSkin     *skinner;
  AkSkin             *skin;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_instanced_skin_joint");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  group       = ak_heap_calloc(heap, doc, sizeof(*group));
  jointA      = ak_heap_calloc(heap, doc, sizeof(*jointA));
  jointB      = ak_heap_calloc(heap, doc, sizeof(*jointB));
  meshNode    = ak_heap_calloc(heap, doc, sizeof(*meshNode));
  scene->node = root;
  doc->scene  = scene;

  root->name     = "SceneRoot";
  group->name    = "Group";
  jointA->name   = "JointA";
  jointB->name   = "JointB";
  meshNode->name = "Mesh";

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;
  ak_test_add_skin_inputs(heap, prim);

  skin = ak_heap_calloc(heap, doc, sizeof(*skin));
  skin->nJoints = 2;
  skin->joints = ak_heap_alloc(heap, skin, sizeof(*skin->joints) * 2);
  skin->joints[0] = jointA;
  skin->joints[1] = jointB;
  skin->skeleton  = jointA;

  skinner = ak_heap_calloc(heap, meshNode, sizeof(*skinner));
  skinner->skin = skin;

  ak_addSubNode(jointA, jointB, false);
  ak_addSubNode(root, group, false);
  ak_addSubNode(root, meshNode, false);
  ASSERT(ak_nodeAttachNodeInstance(group, jointA) != NULL);
  instGeom = ak_nodeAttachGeometry(meshNode, geom);
  ASSERT(instGeom != NULL);
  instGeom->skinner = skinner;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"skeleton\":"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Group") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "Mesh") != NULL);
  ASSERT(ak_sceneFindRoot(roundTrip->scene, "JointA") == NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_bakes_non_trs_leaf_mesh_transform) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkObject       *matrixObj;
  AkMatrix       *matrix;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_non_trs_bake");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  node->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);

  node->transform = ak_heap_calloc(heap, node, sizeof(*node->transform));
  matrixObj       = ak_objAlloc(heap,
                                node->transform,
                                sizeof(*matrix),
                                AKT_MATRIX,
                                true);
  matrix          = ak_objGet(matrixObj);
  matrix->val[0][0] = 1.0f;
  matrix->val[0][1] = 0.0f;
  matrix->val[0][2] = 1.0f;
  matrix->val[0][3] = 0.0f;
  matrix->val[1][0] = 0.0f;
  matrix->val[1][1] = 1.0f;
  matrix->val[1][2] = 1.0f;
  matrix->val[1][3] = 0.0f;
  matrix->val[2][0] = 0.0f;
  matrix->val[2][1] = 0.0f;
  matrix->val[2][2] = 1.0f;
  matrix->val[2][3] = 0.0f;
  matrix->val[3][0] = 0.0f;
  matrix->val[3][1] = 0.0f;
  matrix->val[3][2] = 0.0f;
  matrix->val[3][3] = 1.0f;
  node->transform->base = matrixObj;
  node->transform->item = matrixObj;

  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_count(gltfPath, "\"matrix\"") == 0);
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1,1,1]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"mesh\":0"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_morph_target) {
  AkHeap         *heap;
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root, *node;
  AkGeometry     *geom;
  AkMesh         *mesh;
  AkMeshPrimitive *prim;
  AkInstanceGeometry *instGeom;
  AkInstanceMorph *morpher;
  AkMorph        *morph;
  AkMorphTarget  *target;
  AkMorphable    *morphable;
  AkObject       *targetObj;
  AkInput        *targetPos;
  AkAccessor     *targetAcc;
  AkBuffer       *targetBuff;
  AkFloatArray   *meshWeights;
  AkFloatArray   *nodeWeights;
  const char     *targetNames[1] = {"raise"};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_morph_target");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float deltas[9] = {
    0.0f, 0.0f, 0.1f,
    0.0f, 0.0f, 0.2f,
    0.0f, 0.0f, 0.3f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  morph      = ak_heap_calloc(heap, doc, sizeof(*morph));
  target     = ak_heap_calloc(heap, morph, sizeof(*target));
  targetObj  = ak_objAlloc(heap,
                           target,
                           sizeof(*morphable),
                           AK_MORPHABLE_MORPHABLE,
                           true);
  morphable  = ak_objGet(targetObj);
  targetPos  = ak_heap_calloc(heap, targetObj, sizeof(*targetPos));
  targetAcc  = ak_heap_calloc(heap, targetPos, sizeof(*targetAcc));
  targetBuff = ak_heap_calloc(heap, targetAcc, sizeof(*targetBuff));

  targetBuff->length = sizeof(deltas);
  targetBuff->data   = ak_heap_alloc(heap, targetBuff, targetBuff->length);
  memcpy(targetBuff->data, deltas, sizeof(deltas));

  targetAcc->buffer                = targetBuff;
  targetAcc->byteLength            = targetBuff->length;
  targetAcc->byteStride            = sizeof(float) * 3;
  targetAcc->fillByteSize          = sizeof(float) * 3;
  targetAcc->bytesPerComponent     = sizeof(float);
  targetAcc->componentSize         = AK_COMPONENT_SIZE_VEC3;
  targetAcc->componentType         = AKT_FLOAT;
  targetAcc->originalComponentType = AKT_FLOAT;
  targetAcc->componentCount        = 3;
  targetAcc->count                 = 3;

  targetPos->accessor = targetAcc;
  targetPos->semantic = AK_INPUT_POSITION;
  morphable->input = targetPos;
  morphable->inputCount = 1;

  target->target = targetObj;
  target->primitiveCount = 1;
  morph->target = target;
  morph->method = AK_MORPH_METHOD_ADDITIVE;
  morph->targetCount = 1;
  morph->targetNames = targetNames;

  meshWeights = ak_heap_alloc(heap,
                              doc,
                              sizeof(*meshWeights)
                              + sizeof(meshWeights->items[0]));
  meshWeights->count = 1;
  meshWeights->items[0] = 0.25f;
  mesh->weights = meshWeights;

  nodeWeights = ak_heap_alloc(heap,
                              node,
                              sizeof(*nodeWeights)
                              + sizeof(nodeWeights->items[0]));
  nodeWeights->count = 1;
  nodeWeights->items[0] = 0.75f;

  morpher = ak_heap_calloc(heap, node, sizeof(*morpher));
  morpher->morph = morph;
  morpher->overrideWeights = nodeWeights;

  ak_addSubNode(root, node, false);
  instGeom = ak_nodeAttachGeometry(node, geom);
  ASSERT(instGeom != NULL);
  instGeom->morpher = morpher;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"targets\":[{\"POSITION\":1}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0,0,0.100000001"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[0,0,0.300000011"));
  ASSERT(ak_test_file_contains(gltfPath, "\"weights\":[0.25]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"weights\":[0.75]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extras\":{\"targetNames\":[\"raise\"]}"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->lib.morphs.first != NULL);
  ak_free(roundTrip);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_translation_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkTranslate   *translate;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_translation");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 2.0f, 3.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  translateObj = ak_getTransformTRS(node, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);
  translate = ak_objGet(translateObj);
  translate->val[0] = 0.0f;
  translate->val[1] = 0.0f;
  translate->val[2] = 0.0f;

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  anim->name = "Move";
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     translations,
                                                     3,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input = timeInput;
  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"animations\":["));
  ASSERT(ak_test_file_contains(gltfPath, "\"samplers\":[{\"input\":1,\"output\":2}]"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"translation\":[0,0,0]"));
  ASSERT(!ak_test_file_contains(gltfPath, "\"matrix\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"min\":[0]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"max\":[1]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_reused_node_animation_duplicates_channels) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *targetNode;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkTranslate   *translate;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_reused_node_anim");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float translations[6] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  targetNode  = ak_heap_calloc(heap, doc, sizeof(*targetNode));
  scene->node = root;
  doc->scene  = scene;
  root->visible = true;
  targetNode->visible = true;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ASSERT(ak_nodeAttachGeometry(targetNode, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, targetNode) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, targetNode) != NULL);

  translateObj = ak_getTransformTRS(targetNode, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);
  translate = ak_objGet(translateObj);
  translate->val[0] = 0.0f;
  translate->val[1] = 0.0f;
  translate->val[2] = 0.0f;

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     translations,
                                                     3,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  timeInput->next = valueInput;
  sampler->input = timeInput;
  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_LINEAR;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_count(gltfPath, "\"primitives\"") == 1);
  ASSERT(ak_test_file_count(gltfPath, "\"mesh\":0") == 2);
  ASSERT(ak_test_file_count(gltfPath, "\"path\":\"translation\"") == 2);
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"node\":0,\"path\":\"translation\"}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"node\":1,\"path\":\"translation\"}"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_cubic_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkGeometry    *geom;
  AkObject      *translateObj;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_cubic");
  const float positions[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float times[2] = {0.0f, 1.0f};
  const float packedCubic[18] = {
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 2.0f, 3.0f,
    0.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  ak_addSubNode(root, node, false);
  ASSERT(ak_nodeAttachGeometry(node, geom) != NULL);

  translateObj = ak_getTransformTRS(node, AKT_TRANSLATE);
  ASSERT(translateObj != NULL);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_float_accessor(heap,
                                                     valueInput,
                                                     packedCubic,
                                                     3,
                                                     6);
  ASSERT(valueInput->accessor != NULL);

  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_HERMITE;

  target->target = translateObj;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_POSITION;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"interpolation\":\"CUBICSPLINE\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"channels\":[{\"sampler\":0,\"target\":{\"node\":0,\"path\":\"translation\"}}]"));
  ASSERT(ak_test_file_contains(gltfPath, "\"translation\":[0,0,0]"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_node_visibility_animation) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkDoc         *roundTrip;
  AkScene       *scene;
  AkNode        *root, *node;
  AkNode        *rtNode;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkChannel     *rtChannel;
  AkInput       *timeInput;
  AkInput       *valueInput;
  AkResolvedTarget *target;
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_anim_visibility");
  const float    times[2] = {0.0f, 1.0f};
  const uint8_t  values[2] = {1u, 0u};

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;
  ak_addSubNode(root, node, false);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  timeInput = ak_heap_calloc(heap, sampler, sizeof(*timeInput));
  valueInput = ak_heap_calloc(heap, sampler, sizeof(*valueInput));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  timeInput->semantic = AK_INPUT_INPUT;
  timeInput->accessor = ak_test_make_float_accessor(heap,
                                                    timeInput,
                                                    times,
                                                    1,
                                                    2);
  ASSERT(timeInput->accessor != NULL);

  valueInput->semantic = AK_INPUT_OUTPUT;
  valueInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                     valueInput,
                                                     values,
                                                     1,
                                                     2);
  ASSERT(valueInput->accessor != NULL);

  sampler->inputInput = timeInput;
  sampler->outputInput = valueInput;
  sampler->uniInterpolation = AK_INTERPOLATION_STEP;

  target->target = &node->visible;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_BOOL;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_animation_pointer\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"KHR_node_visibility\""));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"extensions\":{\"KHR_node_visibility\":{\"visible\":true}}"));
  ASSERT(ak_test_file_contains(gltfPath,
                               "\"target\":{\"path\":\"pointer\",\"extensions\":{\"KHR_animation_pointer\":{\"pointer\":\"/nodes/0/extensions/KHR_node_visibility/visible\"}}}"));
  ASSERT(ak_test_file_contains(gltfPath, "\"interpolation\":\"STEP\""));
  ASSERT(ak_test_file_contains(gltfPath, "\"componentType\":5121"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, gltfPath, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(roundTrip != NULL);
  ASSERT(roundTrip->scene != NULL);
  ASSERT(roundTrip->scene->node != NULL);
  rtNode = roundTrip->scene->node->chld;
  ASSERT(rtNode != NULL);
  ASSERT(roundTrip->lib.animations.first != NULL);
  rtChannel = roundTrip->lib.animations.first->channel;
  ASSERT(rtChannel != NULL);
  ASSERT(rtChannel->targetType == AK_TARGET_BOOL);
  ASSERT(rtChannel->resolvedTarget != NULL);
  ASSERT(rtChannel->resolvedTarget->target == &rtNode->visible);

  ak_free(roundTrip);
  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_skips_unsupported_animation_channel) {
  AkHeap        *heap;
  AkDoc         *doc;
  AkScene       *scene;
  AkNode        *root, *node;
  AkAnimation   *anim;
  AkAnimSampler *sampler;
  AkChannel     *channel;
  AkResolvedTarget *target;
  float          unsupportedTarget[2] = {0.0f, 1.0f};
  AK_TEST_EXPORT_GLTF_PATHS("assetkit_export_skip_unsupported_anim");

  ak_test_export_cleanup(outDir);

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  scene->node = root;
  doc->scene  = scene;

  root->visible = true;
  node->visible = true;
  ak_addSubNode(root, node, false);

  anim = ak_heap_calloc(heap, doc, sizeof(*anim));
  sampler = ak_heap_calloc(heap, anim, sizeof(*sampler));
  channel = ak_heap_calloc(heap, anim, sizeof(*channel));
  target = ak_heap_calloc(heap, channel, sizeof(*target));

  target->target = unsupportedTarget;
  channel->resolvedTarget = target;
  channel->targetType = AK_TARGET_VEC2;
  channel->source.ptr = sampler;

  anim->sampler = sampler;
  anim->channel = channel;
  doc->lib.animations.first = anim;
  doc->lib.animations.last = anim;
  doc->lib.animations.count = 1;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(!ak_test_file_contains(gltfPath, "\"animations\""));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_path_state) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *ownerA, *ownerB, *target;
  AkGeometry    *geom;
  AkBoundingBox *bbox;
  float          identity[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f
  };
  float          translateX[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    10.0f, 0.0f, 0.0f, 1.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  ownerA = ak_heap_calloc(heap, NULL, sizeof(*ownerA));
  ownerB = ak_heap_calloc(heap, NULL, sizeof(*ownerB));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_heap_calloc(heap, NULL, sizeof(*geom));
  bbox   = ak_heap_calloc(heap, geom, sizeof(*bbox));

  scene->node = root;

  bbox->min[0] = 0.0f;
  bbox->min[1] = 0.0f;
  bbox->min[2] = 0.0f;
  bbox->max[0] = 1.0f;
  bbox->max[1] = 1.0f;
  bbox->max[2] = 1.0f;
  bbox->isvalid = true;
  geom->bbox = bbox;

  ak_nodeSetTransformMatrix(ownerA, identity);
  ak_nodeSetTransformMatrix(ownerB, translateX);

  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, ownerA) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, ownerB) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(ownerA, target) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(ownerB, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(scene->bbox->min[0] == 0.0f);
  ASSERT(scene->bbox->max[0] == 11.0f);
  ASSERT(target->matrixWorld == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_bbox_rotated_ref) {
  AkHeap        *heap;
  AkScene       *scene;
  AkNode        *root, *target;
  AkGeometry    *geom;
  AkBoundingBox *bbox;
  float          rotZ45[16] = {
    0.70710678f, 0.70710678f, 0.0f, 0.0f,
   -0.70710678f, 0.70710678f, 0.0f, 0.0f,
    0.0f,        0.0f,        1.0f, 0.0f,
    0.0f,        0.0f,        0.0f, 1.0f
  };

  heap   = ak_heap_new(NULL, NULL, NULL);
  scene  = ak_heap_calloc(heap, NULL, sizeof(*scene));
  root   = ak_heap_calloc(heap, scene, sizeof(*root));
  target = ak_heap_calloc(heap, NULL, sizeof(*target));
  geom   = ak_heap_calloc(heap, NULL, sizeof(*geom));
  bbox   = ak_heap_calloc(heap, geom, sizeof(*bbox));

  scene->node = root;

  bbox->min[0] = 0.0f;
  bbox->min[1] = 0.0f;
  bbox->min[2] = 0.0f;
  bbox->max[0] = 1.0f;
  bbox->max[1] = 1.0f;
  bbox->max[2] = 0.0f;
  bbox->isvalid = true;
  geom->bbox = bbox;

  ak_nodeSetTransformMatrix(root, rotZ45);

  ASSERT(ak_nodeAttachGeometry(target, geom) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, target) != NULL);

  ak_bbox_scene(scene);

  ASSERT(scene->bbox != NULL);
  ASSERT(scene->bbox->isvalid);
  ASSERT(fabsf(scene->bbox->min[0] + 0.70710678f) < 0.001f);
  ASSERT(fabsf(scene->bbox->max[0] - 0.70710678f) < 0.001f);
  ASSERT(fabsf(scene->bbox->min[1]) < 0.001f);
  ASSERT(fabsf(scene->bbox->max[1] - 1.41421356f) < 0.001f);
  ASSERT(target->matrixWorld == NULL);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(node_instance_camera_world_path) {
  AkHeap   *heap;
  AkDoc    *doc;
  AkScene  *scene;
  AkNode   *root, *camNode;
  AkCamera *camera;
  AkCamera *found;
  float     rootTrans[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    5.0f, 0.0f, 0.0f, 1.0f
  };
  float     camTrans[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    10.0f, 0.0f, 0.0f, 1.0f
  };
  float     matrix[16];

  heap    = ak_heap_new(NULL, NULL, NULL);
  doc     = ak_heap_calloc(heap, NULL, sizeof(*doc));
  scene   = ak_heap_calloc(heap, doc, sizeof(*scene));
  root    = ak_heap_calloc(heap, scene, sizeof(*root));
  camNode = ak_heap_calloc(heap, doc, sizeof(*camNode));

  doc->scene           = scene;
  scene->node          = root;
  scene->firstCamNode  = camNode;

  ak_nodeSetTransformMatrix(root, rootTrans);
  ak_nodeSetTransformMatrix(camNode, camTrans);

  camera = ak_camMakePerspective(doc, doc, 1.0f, 1.0f, 0.1f, 100.0f);
  ASSERT(camera != NULL);
  ASSERT(ak_nodeAttachCamera(camNode, camera) != NULL);
  ASSERT(ak_nodeAttachNodeInstance(root, camNode) != NULL);

  ASSERT(ak_firstCamera(doc, &found, matrix, NULL) == AK_OK);
  ASSERT(found == camera);
  ASSERT(fabsf(matrix[12] - 15.0f) < 0.001f);
  ASSERT(fabsf(matrix[13]) < 0.001f);
  ASSERT(fabsf(matrix[14]) < 0.001f);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(scene_find_or_make_root_uses_child_roots) {
  AkHeap  *heap;
  AkDoc   *doc;
  AkScene *scene;
  AkNode  *madeRoot;
  AkNode  *sameRoot;
  AkNode  *otherRoot;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));

  scene = ak_heap_calloc(heap, doc, sizeof(*scene));

  madeRoot = ak_sceneFindOrMakeRoot(doc, scene, "User Cameras");
  ASSERT(madeRoot != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld == madeRoot);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(madeRoot->parent == scene->node);
  ASSERT(madeRoot->prev == NULL);
  ASSERT(madeRoot->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  sameRoot = ak_sceneFindOrMakeRoot(doc, scene, "User Cameras");
  ASSERT(sameRoot == madeRoot);
  ASSERT(doc->lib.nodes.count == 1);
  ASSERT(scene->node->chld->next == NULL);

  otherRoot = ak_sceneFindOrMakeRoot(doc, scene, "Other Root");
  ASSERT(otherRoot != NULL);
  ASSERT(otherRoot != madeRoot);
  ASSERT(scene->node->chld == otherRoot);
  ASSERT(scene->node->chld->next == madeRoot);
  ASSERT(otherRoot->parent == scene->node);
  ASSERT(madeRoot->parent == scene->node);
  ASSERT(doc->lib.nodes.count == 2);

  ak_heap_destroy(heap);

  TEST_SUCCESS
}

TEST_IMPL(dae_scene_roots_are_child_nodes) {
  AkDoc       *doc;
  AkScene     *scene;
  AkNode      *rootA, *rootB;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  const char  *tmpBase;
  uint32_t     rootCount;
  AkNode      *root;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-roots-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/roots.dae", tmpdir);
  ASSERT(ak_test_write_dae_two_roots(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);
  ASSERT(scene->node->next == NULL);
  ASSERT(doc->lib.nodes.count == 2);

  rootA = ak_sceneFindRoot(scene, "RootA");
  rootB = ak_sceneFindRoot(scene, "RootB");
  ASSERT(rootA != NULL);
  ASSERT(rootB != NULL);
  ASSERT(rootA != rootB);
  ASSERT(rootA->name && strcmp(rootA->name, "RootA") == 0);
  ASSERT(rootB->name && strcmp(rootB->name, "RootB") == 0);
  ASSERT(rootA->parent == scene->node);
  ASSERT(rootB->parent == scene->node);

  rootCount = 0;
  for (root = scene->node->chld; root; root = root->next)
    rootCount++;
  ASSERT(rootCount == 2);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_load_utf16le) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         gltfPath[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-utf16le-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/utf16.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(gltfPath, sizeof(gltfPath), "%s/utf16.gltf", outDir);

  ASSERT(ak_test_write_dae_utf16le_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->scene != NULL);
  ASSERT(doc->scene->node != NULL);
  ASSERT(doc->scene->node->chld != NULL);
  ASSERT(doc->scene->node->chld->name
         && strcmp(doc->scene->node->chld->name, "Root") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);
  ASSERT(ak_test_file_contains(gltfPath, "\"scenes\""));

  ak_free(doc);
  unlink(gltfPath);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_vertex_input_fallback_single_vertices) {
  AkDoc             *doc;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  char               dirTemplate[PATH_MAX];
  char              *tmpdir;
  char               daePath[PATH_MAX];
  char               outDir[PATH_MAX];
  const char        *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-vertex-fallback-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/vertex_fallback.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);

  ASSERT(ak_test_write_dae_bad_vertex_ref_single_vertices(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  geom = doc->lib.geometries.first;
  ASSERT(geom != NULL);
  ASSERT(geom->gdata != NULL);
  ASSERT(geom->gdata->type == AK_GEOMETRY_MESH);

  mesh = ak_objGet(geom->gdata);
  ASSERT(mesh != NULL);
  prim = mesh->primitive;
  ASSERT(prim != NULL);
  ASSERT(prim->pos != NULL);
  ASSERT(prim->pos->accessor != NULL);
  ASSERT(prim->pos->accessor->componentCount == 3);
  ASSERT(prim->pos->accessor->count == 3);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_OK);

  ak_test_export_cleanup(outDir);
  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_export_rejects_brep) {
  AkDoc       *doc;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-brep-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/brep.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);

  ASSERT(ak_test_write_dae_brep_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.geometries.first != NULL);
  ASSERT(doc->lib.geometries.first->gdata != NULL);
  ASSERT(doc->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLTF) == AK_EINVAL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_GLB) == AK_EINVAL);

  ak_free(doc);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_instance_node_is_instance_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  AkNode     *shared;
  AkInstanceNode *inst;
  AkInstanceBase *camInst;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        daePath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-instance-node-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/instance_node.dae", tmpdir);
  ASSERT(ak_test_write_dae_instance_node(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(doc->lib.nodes.count == 2);

  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->node != NULL);

  inst = root->node;
  ASSERT(inst->owner == root);
  ASSERT(inst->target != NULL);
  ASSERT(inst->reserved != NULL);
  ASSERT(inst->name && strcmp(inst->name, "SharedUse") == 0);
  ASSERT(inst->proxy == NULL);

  shared = ak_instanceNodeTarget(inst);
  ASSERT(shared != NULL);
  ASSERT(shared == inst->target);
  ASSERT(shared->name && strcmp(shared->name, "Shared") == 0);
  ASSERT(scene->firstCamNode == shared);
  ASSERT(scene->cameras.count == 1);
  ASSERT(scene->cameras.useCount == 1);
  ASSERT(scene->cameras.first != NULL);
  ASSERT(scene->cameras.first->camera != NULL);
  camInst = scene->cameras.first->firstInstance;
  ASSERT(camInst != NULL);
  ASSERT(camInst->node == shared);
  ASSERT(ak_instanceObject(camInst) == scene->cameras.first->camera);

  ak_free(doc);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_camera_light_extra_preserve_opt) {
  AkDoc      *doc;
  AkDoc      *docWithExtras;
  AkCamera   *camera;
  AkLight    *light;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        daePath[PATH_MAX];
  const char *tmpBase;
  uintptr_t   preserveExtras;
  AkResult    loadResult;

  doc = NULL;
  docWithExtras = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-camera-light-extra-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/camera_light_extra.dae", tmpdir);
  ASSERT(ak_test_write_dae_camera_light_extra(daePath));

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.cameras.first != NULL);
  ASSERT(doc->lib.lights.first != NULL);
  ASSERT(!ak_extra(doc->lib.cameras.first));
  ASSERT(!ak_extra(doc->lib.lights.first));
  ak_free(doc);

  preserveExtras = ak_opt_get(AK_OPT_PRESERVE_EXTRAS);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, true);
  loadResult = ak_load(&docWithExtras, daePath, AK_FILE_TYPE_AUTO);
  ak_opt_set(AK_OPT_PRESERVE_EXTRAS, preserveExtras);
  ASSERT(loadResult == AK_OK && docWithExtras);

  camera = docWithExtras->lib.cameras.first;
  light  = docWithExtras->lib.lights.first;
  ASSERT(camera != NULL);
  ASSERT(light != NULL);
  ASSERT(ak_extra(camera) != NULL);
  ASSERT(ak_extra(light) != NULL);
  ASSERT(ak_test_tree_has_name(ak_extra(camera), "cameraTag"));
  ASSERT(ak_test_tree_has_name(ak_extra(light), "lightTag"));

  ak_free(docWithExtras);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_root_is_child_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-root-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/root.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_root(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);

  root = scene->node->chld;
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->parent == scene->node);
  ASSERT(root->name && strcmp(root->name, "Root") == 0);
  ASSERT(root->next == NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(gltf_scene_light_cache) {
  AkDoc      *doc;
  AkScene    *scene;
  AkLight    *light;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        gltfPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-gltf-light-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(gltfPath, sizeof(gltfPath), "%s/light.gltf", tmpdir);
  ASSERT(ak_test_write_gltf_light(gltfPath));
  ASSERT(ak_load(&doc, gltfPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->lights.count == 1);
  ASSERT(scene->lights.useCount == 1);
  ASSERT(scene->lights.first != NULL);
  ASSERT(scene->lights.first->firstInstance != NULL);
  ASSERT(scene->lights.first->firstInstance->node == scene->node->chld);

  light = scene->lights.first->light;
  ASSERT(light != NULL);
  ASSERT(ak_instanceObject(scene->lights.first->firstInstance) == light);
  ASSERT(light->name && strcmp(light->name, "Key") == 0);
  ASSERT(light->data != NULL);
  ASSERT(light->data->type == AK_LIGHT_TYPE_POINT);
  ASSERT(fabsf(light->data->intensity - 2.0f) < 0.001f);
  ASSERT(fabsf(light->data->range - 10.0f) < 0.001f);

  ak_free(doc);
  unlink(gltfPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(obj_scene_root_is_child_node) {
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root;
  char        dirTemplate[PATH_MAX];
  char       *tmpdir;
  char        objPath[PATH_MAX];
  const char *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-obj-root-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(objPath, sizeof(objPath), "%s/triangle.obj", tmpdir);
  ASSERT(ak_test_write_obj_triangle(objPath));
  ASSERT(ak_load(&doc, objPath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);
  ASSERT(scene->node->chld != NULL);
  ASSERT(scene->node->node == NULL);
  ASSERT(scene->node->geometry == NULL);

  root = scene->node->chld;
  ASSERT(root != NULL);
  ASSERT(root != scene->node);
  ASSERT(root->parent == scene->node);
  ASSERT(root->geometry != NULL);
  ASSERT(doc->lib.nodes.count == 1);

  ak_free(doc);
  unlink(objPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}
