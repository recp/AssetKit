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

#ifndef assetkit_test_export_common_h
#define assetkit_test_export_common_h

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

static inline
AkMaterialInput*
ak_test_material_input(AkHeap *heap,
                       void   *parent) {
  return ak_heap_aligned_calloc(heap,
                                parent,
                                AK_ALIGNOF(AkMaterialInput),
                                sizeof(AkMaterialInput));
}

static inline
bool
ak_test_path_join(char       *dst,
                  size_t      dstCap,
                  const char *dir,
                  const char *leaf) {
  size_t dirLen;
  size_t leafLen;

  if (!dst || dstCap < 2u || !dir || !leaf)
    return false;

  dirLen  = strlen(dir);
  leafLen = strlen(leaf);
  if (dirLen > dstCap - 2u || leafLen > dstCap - dirLen - 2u)
    return false;

  memcpy(dst, dir, dirLen);
  dst[dirLen] = '/';
  memcpy(dst + dirLen + 1u, leaf, leafLen + 1u);

  return true;
}

static inline
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

static inline
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

static inline
AkTreeNode*
ak_test_dae_extra(AkHeap     *heap,
                  void       *parent,
                  const char *value) {
  AkTreeNode     *root;
  AkTreeNode     *technique;
  AkTreeNode     *note;
  AkTreeNodeAttr *profile;

  (void)parent;

  root      = ak_heap_calloc(heap, NULL, sizeof(*root));
  technique = ak_heap_calloc(heap, NULL, sizeof(*technique));
  note      = ak_heap_calloc(heap, NULL, sizeof(*note));
  profile   = ak_heap_calloc(heap, NULL, sizeof(*profile));
  if (!root || !technique || !note || !profile)
    return NULL;

  technique->name   = "technique";
  technique->attribs = profile;
  technique->attrc  = 1;
  technique->chld   = note;
  technique->chldc  = 1;
  technique->parent = root;

  profile->name = "profile";
  profile->val  = "AssetKit";

  note->name   = "note";
  note->val    = (char *)value;
  note->parent = technique;

  root->chld  = technique;
  root->chldc = 1;

  return root;
}

static inline
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

static inline
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

static inline
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

static inline
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

static inline
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

static inline
void
ak_test_export_cleanup(const char *outDir) {
  static const char *names[] = {
    "model.gltf",
    "model.glb",
    "model.dae",
    "model.obj",
    "model.mtl",
    "model.stl",
    "model.ply",
    "model.3mf",
    "model.bin",
    "Empty.gltf",
    "Empty.glb",
    "Empty.dae",
    "Empty.bin",
    "Box_Name_.gltf",
    "Box_Name_.bin",
    "textures/wood.png",
    "textures/Wood.PNG",
    "textures/Wood File.PNG",
    "textures/Extra.PNG",
    "WoodFile.PNG",
    "assetkit_export_cwd_relative_texture.png",
    "image_0.png",
    "image_0_Extra.PNG",
    "image_0_Missing.PNG",
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

static inline
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

static inline
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

static inline
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

static inline
bool
ak_test_write_dae_same_file_refs(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_cameras><camera id=\"cam\" name=\"Cam\"><optics><technique_common>"
        "<perspective><yfov>45</yfov><znear>0.1</znear><zfar>100</zfar></perspective>"
        "</technique_common></optics></camera></library_cameras>\n"
        "<library_lights><light id=\"light\" name=\"Light\"><technique_common>"
        "<point><color>1 1 1</color></point>"
        "</technique_common></light></library_lights>\n"
        "<library_effects><effect id=\"fx\"><profile_COMMON><technique sid=\"common\">"
        "<lambert><diffuse><color>0.25 0.5 0.75 1</color></diffuse></lambert>"
        "</technique></profile_COMMON></effect></library_effects>\n"
        "<library_materials><material id=\"mat\" name=\"Mat\">"
        "<instance_effect url=\"./self.dae#fx\"/>"
        "</material></library_materials>\n"
        "<library_geometries><geometry id=\"shape\" name=\"Shape\"><mesh>"
        "<source id=\"shape-positions\"><float_array id=\"shape-positions-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#shape-positions-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"shape-vertices\"><input semantic=\"POSITION\" source=\"#shape-positions\"/></vertices>"
        "<triangles count=\"1\" material=\"matSymbol\"><input semantic=\"VERTEX\" source=\"#shape-vertices\" offset=\"0\"/>"
        "<p>0 1 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"geoNode\" name=\"GeoNode\"><instance_geometry url=\"./self.dae#shape\">"
        "<bind_material><technique_common>"
        "<instance_material symbol=\"matSymbol\" target=\"./self.dae#mat\"/>"
        "</technique_common></bind_material>"
        "</instance_geometry></node>"
        "<node id=\"camNode\" name=\"CamNode\"><instance_camera url=\"./self.dae#cam\"/></node>"
        "<node id=\"lightNode\" name=\"LightNode\"><instance_light url=\"./self.dae#light\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
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

static inline
bool
ak_test_write_dae_vertex_offset_nonzero(const char *path) {
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
        "<source id=\"shape-texcoords\"><float_array id=\"shape-texcoords-array\" count=\"6\">"
        "0 0 1 0 0 1"
        "</float_array><technique_common><accessor source=\"#shape-texcoords-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"shape-vertices\"><input semantic=\"POSITION\" source=\"#shape-positions\"/></vertices>"
        "<triangles count=\"1\">"
        "<input semantic=\"TEXCOORD\" source=\"#shape-texcoords\" offset=\"0\" set=\"0\"/>"
        "<input semantic=\"VERTEX\" source=\"#shape-vertices\" offset=\"1\"/>"
        "<p>2 0 0 1 1 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><instance_geometry url=\"#shape\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae14_missing_surface_texture(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_images><image id=\"file2\" name=\"file2\"><init_from>./duckCM.tga</init_from></image></library_images>\n"
        "<library_effects><effect id=\"fx\"><profile_COMMON>"
        "<newparam sid=\"file2-sampler\"><sampler2D><source>file2-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\"><blinn><diffuse><texture texture=\"file2-sampler\" texcoord=\"TEX0\"/></diffuse></blinn></technique>"
        "</profile_COMMON></effect></library_effects>\n"
        "<library_materials><material id=\"mat\"><instance_effect url=\"#fx\"/></material></library_materials>\n"
        "<library_geometries><geometry id=\"shape\" name=\"Shape\"><mesh>"
        "<source id=\"shape-positions\"><float_array id=\"shape-positions-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#shape-positions-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"shape-texcoords\"><float_array id=\"shape-texcoords-array\" count=\"6\">"
        "0 0 1 0 0 1"
        "</float_array><technique_common><accessor source=\"#shape-texcoords-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"shape-vertices\"><input semantic=\"POSITION\" source=\"#shape-positions\"/></vertices>"
        "<triangles material=\"MatSym\" count=\"1\">"
        "<input semantic=\"VERTEX\" source=\"#shape-vertices\" offset=\"0\"/>"
        "<input semantic=\"TEXCOORD\" source=\"#shape-texcoords\" offset=\"1\" set=\"0\"/>"
        "<p>0 0 1 1 2 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><instance_geometry url=\"#shape\">"
        "<bind_material><technique_common><instance_material symbol=\"MatSym\" target=\"#mat\">"
        "<bind_vertex_input semantic=\"TEX0\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>"
        "</instance_material></technique_common></bind_material>"
        "</instance_geometry></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae14_broken_texture_refs(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_images>"
        "<image id=\"bbr_metal-2048_dds\" name=\"bbr_metal-2048_dds\"><init_from>metal-2048.dds</init_from></image>"
        "<image id=\"wood-2048_dds\" name=\"wood-2048_dds\"><init_from>exact.dds</init_from></image>"
        "<image id=\"bbr_wood-2048_dds\" name=\"bbr_wood-2048_dds\"><init_from>bbr_wood-2048.dds</init_from></image>"
        "<image id=\"a_steel-2048_dds\" name=\"a_steel-2048_dds\"><init_from>a_steel-2048.dds</init_from></image>"
        "<image id=\"b_steel-2048_dds\" name=\"b_steel-2048_dds\"><init_from>b_steel-2048.dds</init_from></image>"
        "<image id=\"path_a_dds\" name=\"path_b_dds\"><init_from>missing.dds</init_from></image>"
        "</library_images>\n"
        "<library_effects>"
        "<effect id=\"fx_recover\"><profile_COMMON>"
        "<newparam sid=\"recover-surface\"><surface type=\"2D\"><init_from>metal-2048_dds</init_from></surface></newparam>"
        "<newparam sid=\"recover-sampler\"><sampler2D><source>recover-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><texture texture=\"recover-sampler\" texcoord=\"UVSET0\"/></diffuse></lambert></technique>"
        "</profile_COMMON></effect>"
        "<effect id=\"fx_exact\"><profile_COMMON>"
        "<newparam sid=\"exact-surface\"><surface type=\"2D\"><init_from>wood-2048_dds</init_from></surface></newparam>"
        "<newparam sid=\"exact-sampler\"><sampler2D><source>exact-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><texture texture=\"exact-sampler\" texcoord=\"UVSET0\"/></diffuse></lambert></technique>"
        "</profile_COMMON></effect>"
        "<effect id=\"fx_ambiguous\"><profile_COMMON>"
        "<newparam sid=\"ambiguous-surface\"><surface type=\"2D\"><init_from>steel-2048_dds</init_from></surface></newparam>"
        "<newparam sid=\"ambiguous-sampler\"><sampler2D><source>ambiguous-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><texture texture=\"ambiguous-sampler\" texcoord=\"UVSET0\"/></diffuse></lambert></technique>"
        "</profile_COMMON></effect>"
        "<effect id=\"fx_path_ambiguous\"><profile_COMMON>"
        "<newparam sid=\"path-ambiguous-surface\"><surface type=\"2D\"><init_from>path_a_dds</init_from></surface></newparam>"
        "<newparam sid=\"path-ambiguous-sampler\"><sampler2D><source>path-ambiguous-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\"><lambert><diffuse><texture texture=\"path-ambiguous-sampler\" texcoord=\"UVSET0\"/></diffuse></lambert></technique>"
        "</profile_COMMON></effect>"
        "</library_effects>\n"
        "<library_materials>"
        "<material id=\"mat_recover\"><instance_effect url=\"#fx_recover\"/></material>"
        "<material id=\"mat_exact\"><instance_effect url=\"#fx_exact\"/></material>"
        "<material id=\"mat_ambiguous\"><instance_effect url=\"#fx_ambiguous\"/></material>"
        "<material id=\"mat_path_ambiguous\"><instance_effect url=\"#fx_path_ambiguous\"/></material>"
        "</library_materials>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\"/></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae14_nested_ref_image(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_images><image id=\"wood\" name=\"Wood\">"
        "<init_from><ref>./Textures/WoodFloor-01.png</ref></init_from>"
        "</image></library_images>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"/>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_animation_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_animations><animation id=\"move\" name=\"Move\">"
        "<source id=\"move-input\"><float_array id=\"move-input-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#move-input-array\" count=\"2\" stride=\"1\">"
        "<param name=\"TIME\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"move-output\"><float_array id=\"move-output-array\" count=\"2\">0 2</float_array>"
        "<technique_common><accessor source=\"#move-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"X\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"move-interp\"><Name_array id=\"move-interp-array\" count=\"2\">LINEAR LINEAR</Name_array>"
        "<technique_common><accessor source=\"#move-interp-array\" count=\"2\" stride=\"1\">"
        "<param name=\"INTERPOLATION\" type=\"name\"/></accessor></technique_common></source>"
        "<sampler id=\"move-sampler\">"
        "<input semantic=\"INPUT\" source=\"#move-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#move-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#move-interp\"/>"
        "</sampler>"
        "<channel source=\"#move-sampler\" target=\"node/translate.X\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><translate sid=\"translate\">0 0 0</translate></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_srgb_color_carriers(const char *path,
                                      const char *authoringTool) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset>",
        file);
  if (authoringTool && authoringTool[0]) {
    fputs("<contributor><authoring_tool>", file);
    fputs(authoringTool, file);
    fputs("</authoring_tool></contributor>", file);
  }
  fputs("<unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_effects><effect id=\"material-effect\"><profile_COMMON>"
        "<technique sid=\"common\"><lambert><diffuse>"
        "<color>0.8823529 0.8823529 0.7843137 1</color>"
        "</diffuse><extra><technique profile=\"SceneKit\">"
        "<constant_diffuse><color>0.8823529 0.8823529 0.7843137 1</color>"
        "</constant_diffuse></technique></extra></lambert>"
        "</technique></profile_COMMON></effect>"
        "</library_effects><library_materials>"
        "<material id=\"material\" name=\"edge_color225225200255\">"
        "<instance_effect url=\"#material-effect\"/></material>"
        "</library_materials>\n"
        "<library_lights><light id=\"light\"><technique_common><point>"
        "<color sid=\"color\">0.4 0.1 0 1</color>"
        "</point></technique_common></light>"
        "<light id=\"bezier-light\"><technique_common><point>"
        "<color sid=\"color\">0.4 0.1 0 1</color>"
        "</point></technique_common></light>"
        "<light id=\"hermite-light\"><technique_common><point>"
        "<color sid=\"color\">0.4 0.1 0 1</color>"
        "</point></technique_common></light></library_lights>\n"
        "<library_geometries><geometry id=\"geom\"><mesh>"
        "<source id=\"pos\"><float_array id=\"pos-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0</float_array><technique_common>"
        "<accessor source=\"#pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"color-source\"><float_array id=\"color-array\" count=\"12\">"
        "0.4 0.1 0 1 0.2 0.3 0.4 0.5 1 0.5 0.25 0.75"
        "</float_array><technique_common>"
        "<accessor source=\"#color-array\" count=\"3\" stride=\"4\">"
        "<param name=\"R\" type=\"float\"/><param name=\"G\" type=\"float\"/>"
        "<param name=\"B\" type=\"float\"/><param name=\"A\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"verts\"><input semantic=\"POSITION\" source=\"#pos\"/>"
        "<input semantic=\"COLOR\" source=\"#color-source\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#verts\" offset=\"0\"/>"
        "<p>0 1 2</p></triangles></mesh></geometry></library_geometries>\n"
        "<library_animations><animation id=\"light-color-animation\">"
        "<source id=\"light-color-input\"><float_array id=\"light-color-input-array\" count=\"2\">"
        "0 1</float_array><technique_common><accessor source=\"#light-color-input-array\" "
        "count=\"2\" stride=\"1\"><param name=\"TIME\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"light-color-output\"><float_array id=\"light-color-output-array\" count=\"8\">"
        "0.4 0.1 0 1 0.2 0.3 0.4 0.5</float_array><technique_common>"
        "<accessor source=\"#light-color-output-array\" count=\"2\" stride=\"4\">"
        "<param name=\"R\" type=\"float\"/><param name=\"G\" type=\"float\"/>"
        "<param name=\"B\" type=\"float\"/><param name=\"A\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"light-color-interp\"><Name_array id=\"light-color-interp-array\" count=\"2\">"
        "LINEAR LINEAR</Name_array><technique_common>"
        "<accessor source=\"#light-color-interp-array\" count=\"2\" stride=\"1\">"
        "<param name=\"INTERPOLATION\" type=\"Name\"/></accessor>"
        "</technique_common></source>"
        "<sampler id=\"light-color-sampler\"><input semantic=\"INPUT\" source=\"#light-color-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#light-color-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#light-color-interp\"/></sampler>"
        "<channel source=\"#light-color-sampler\" target=\"light/color\"/>"
        "</animation><animation id=\"bezier-color-animation\">"
        "<source id=\"bezier-color-input\"><float_array id=\"bezier-color-input-array\" count=\"2\">"
        "0 1</float_array><technique_common><accessor source=\"#bezier-color-input-array\" "
        "count=\"2\" stride=\"1\"><param name=\"TIME\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"bezier-color-output\"><float_array id=\"bezier-color-output-array\" count=\"8\">"
        "0.4 0.1 0 1 0.2 0.3 0.4 0.5</float_array><technique_common>"
        "<accessor source=\"#bezier-color-output-array\" count=\"2\" stride=\"4\">"
        "<param name=\"R\" type=\"float\"/><param name=\"G\" type=\"float\"/>"
        "<param name=\"B\" type=\"float\"/><param name=\"A\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"bezier-color-interp\"><Name_array id=\"bezier-color-interp-array\" count=\"2\">"
        "BEZIER BEZIER</Name_array><technique_common>"
        "<accessor source=\"#bezier-color-interp-array\" count=\"2\" stride=\"1\">"
        "<param name=\"INTERPOLATION\" type=\"Name\"/></accessor>"
        "</technique_common></source>"
        "<source id=\"bezier-color-in\"><float_array id=\"bezier-color-in-array\" count=\"10\">"
        "-0.1 0.3 0.2 0.1 0.9 0.8 0.5 0.4 0.3 0.7</float_array>"
        "<technique_common><accessor source=\"#bezier-color-in-array\" count=\"2\" stride=\"5\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"R\" type=\"float\"/>"
        "<param name=\"G\" type=\"float\"/><param name=\"B\" type=\"float\"/>"
        "<param name=\"A\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"bezier-color-out\"><float_array id=\"bezier-color-out-array\" count=\"10\">"
        "0.2 0.6 0.5 0.4 0.8 1.1 0.2 0.1 0 0.6</float_array>"
        "<technique_common><accessor source=\"#bezier-color-out-array\" count=\"2\" stride=\"5\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"R\" type=\"float\"/>"
        "<param name=\"G\" type=\"float\"/><param name=\"B\" type=\"float\"/>"
        "<param name=\"A\" type=\"float\"/></accessor></technique_common></source>"
        "<sampler id=\"bezier-color-sampler\"><input semantic=\"INPUT\" source=\"#bezier-color-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#bezier-color-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#bezier-color-interp\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#bezier-color-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#bezier-color-out\"/></sampler>"
        "<channel source=\"#bezier-color-sampler\" target=\"bezier-light/color\"/>"
        "</animation><animation id=\"hermite-color-animation\">"
        "<source id=\"hermite-color-input\"><float_array id=\"hermite-color-input-array\" count=\"2\">"
        "0 1</float_array><technique_common><accessor source=\"#hermite-color-input-array\" "
        "count=\"2\" stride=\"1\"><param name=\"TIME\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"hermite-color-output\"><float_array id=\"hermite-color-output-array\" count=\"2\">"
        "0.4 0.2</float_array><technique_common>"
        "<accessor source=\"#hermite-color-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"R\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"hermite-color-interp\"><Name_array id=\"hermite-color-interp-array\" count=\"2\">"
        "HERMITE HERMITE</Name_array><technique_common>"
        "<accessor source=\"#hermite-color-interp-array\" count=\"2\" stride=\"1\">"
        "<param name=\"INTERPOLATION\" type=\"Name\"/></accessor>"
        "</technique_common></source>"
        "<source id=\"hermite-color-in\"><float_array id=\"hermite-color-in-array\" count=\"4\">"
        "-0.25 0.5 0.75 0.25</float_array><technique_common>"
        "<accessor source=\"#hermite-color-in-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"R\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"hermite-color-out\"><float_array id=\"hermite-color-out-array\" count=\"4\">"
        "0.25 0.75 1.25 0.4</float_array><technique_common>"
        "<accessor source=\"#hermite-color-out-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"R\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<sampler id=\"hermite-color-sampler\"><input semantic=\"INPUT\" source=\"#hermite-color-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#hermite-color-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#hermite-color-interp\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#hermite-color-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#hermite-color-out\"/></sampler>"
        "<channel source=\"#hermite-color-sampler\" target=\"hermite-light/color.R\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\"><node id=\"node\">"
        "<instance_geometry url=\"#geom\"/><instance_light url=\"#light\"/>"
        "<instance_light url=\"#bezier-light\"/><instance_light url=\"#hermite-light\"/>"
        "</node></visual_scene></library_visual_scenes>"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene></COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_animation_vec3_interp(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_animations><animation id=\"move\" name=\"Move\">"
        "<source id=\"move-input\"><float_array id=\"move-input-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#move-input-array\" count=\"2\" stride=\"1\">"
        "<param name=\"TIME\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"move-output\"><float_array id=\"move-output-array\" count=\"6\">0 0 0 1 2 3</float_array>"
        "<technique_common><accessor source=\"#move-output-array\" count=\"2\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"move-interp\"><Name_array id=\"move-interp-array\" count=\"6\">"
        "BEZIER BEZIER BEZIER LINEAR LINEAR LINEAR"
        "</Name_array><technique_common><accessor source=\"#move-interp-array\" count=\"2\" stride=\"3\">"
        "<param name=\"X\" type=\"Name\"/><param name=\"Y\" type=\"Name\"/><param name=\"Z\" type=\"Name\"/>"
        "</accessor></technique_common></source>"
        "<sampler id=\"move-sampler\">"
        "<input semantic=\"INPUT\" source=\"#move-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#move-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#move-interp\"/>"
        "</sampler>"
        "<channel source=\"#move-sampler\" target=\"node/translate\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><translate sid=\"translate\">0 0 0</translate></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_skin_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"geom\" name=\"Geom\"><mesh>"
        "<source id=\"geom-pos\"><float_array id=\"geom-pos-array\" count=\"9\">0 0 0 1 0 0 0 1 0</float_array>"
        "<technique_common><accessor source=\"#geom-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"geom-verts\"><input semantic=\"POSITION\" source=\"#geom-pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_controllers><controller id=\"skin\"><skin source=\"#geom\">"
        "<bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>"
        "<source id=\"skin-joints\"><IDREF_array id=\"skin-joints-array\" count=\"2\">joint0 joint1</IDREF_array>"
        "<technique_common><accessor source=\"#skin-joints-array\" count=\"2\" stride=\"1\">"
        "<param name=\"JOINT\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"skin-bind\"><float_array id=\"skin-bind-array\" count=\"32\">"
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 "
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
        "</float_array><technique_common><accessor source=\"#skin-bind-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"skin-weights\"><float_array id=\"skin-weights-array\" count=\"6\">1 0 0.5 0.5 0 1</float_array>"
        "<technique_common><accessor source=\"#skin-weights-array\" count=\"6\" stride=\"1\">"
        "<param name=\"WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<joints><input semantic=\"JOINT\" source=\"#skin-joints\"/>"
        "<input semantic=\"INV_BIND_MATRIX\" source=\"#skin-bind\"/></joints>"
        "<vertex_weights count=\"3\"><input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<vcount>2 2 2</vcount><v>0 0 1 1 0 2 1 3 0 4 1 5</v></vertex_weights>"
        "</skin></controller></library_controllers>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"joint0\" name=\"Joint0\"><node id=\"joint1\" name=\"Joint1\"/></node>"
        "<node id=\"meshNode\" name=\"Mesh\"><instance_controller url=\"#skin\"><skeleton>#joint0</skeleton></instance_controller></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_coord_all_skin_animation(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Z_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"geom\" name=\"Geom\"><mesh>"
        "<source id=\"geom-pos\"><float_array id=\"geom-pos-array\" count=\"9\">"
        "1 2 3 4 5 6 7 8 9</float_array><technique_common>"
        "<accessor source=\"#geom-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"geom-normal\"><float_array id=\"geom-normal-array\" count=\"9\">"
        "0 0 1 0 0 1 0 0 1</float_array><technique_common>"
        "<accessor source=\"#geom-normal-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<vertices id=\"geom-verts\"><input semantic=\"POSITION\" source=\"#geom-pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-verts\" offset=\"0\"/>"
        "<input semantic=\"NORMAL\" source=\"#geom-normal\" offset=\"1\"/>"
        "<p>0 0 1 1 2 2</p></triangles></mesh></geometry></library_geometries>\n"
        "<library_controllers><controller id=\"skin\"><skin source=\"#geom\">"
        "<bind_shape_matrix>1 0 0 1 0 1 0 2 0 0 1 3 0 0 0 1</bind_shape_matrix>"
        "<source id=\"skin-joints\"><IDREF_array id=\"skin-joints-array\" count=\"1\">"
        "joint0</IDREF_array><technique_common>"
        "<accessor source=\"#skin-joints-array\" count=\"1\" stride=\"1\">"
        "<param name=\"JOINT\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"skin-bind\"><float_array id=\"skin-bind-array\" count=\"16\">"
        "1 0 0 -4 0 1 0 5 0 0 1 6 0 0 0 1</float_array><technique_common>"
        "<accessor source=\"#skin-bind-array\" count=\"1\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"skin-weights\"><float_array id=\"skin-weights-array\" count=\"1\">"
        "1</float_array><technique_common>"
        "<accessor source=\"#skin-weights-array\" count=\"1\" stride=\"1\">"
        "<param name=\"WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<joints><input semantic=\"JOINT\" source=\"#skin-joints\"/>"
        "<input semantic=\"INV_BIND_MATRIX\" source=\"#skin-bind\"/></joints>"
        "<vertex_weights count=\"3\"><input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<vcount>1 1 1</vcount><v>0 0 0 0 0 0</v></vertex_weights>"
        "</skin></controller></library_controllers>\n"
        "<library_animations><animation id=\"joint-animation\">"
        "<source id=\"anim-time\"><float_array id=\"anim-time-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#anim-time-array\" count=\"2\" stride=\"1\">"
        "<param name=\"TIME\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"anim-interp\"><Name_array id=\"anim-interp-array\" count=\"2\">"
        "BEZIER BEZIER</Name_array><technique_common>"
        "<accessor source=\"#anim-interp-array\" count=\"2\" stride=\"1\">"
        "<param name=\"INTERPOLATION\" type=\"Name\"/></accessor></technique_common></source>"
        "<source id=\"pos-output\"><float_array id=\"pos-output-array\" count=\"6\">"
        "1 2 3 4 5 6</float_array><technique_common>"
        "<accessor source=\"#pos-output-array\" count=\"2\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"pos-in\"><float_array id=\"pos-in-array\" count=\"12\">"
        "-0.1 1.5 -0.2 2.5 -0.3 3.5 0.9 4.5 0.8 5.5 0.7 6.5"
        "</float_array><technique_common>"
        "<accessor source=\"#pos-in-array\" count=\"2\" stride=\"6\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"X\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"pos-out\"><float_array id=\"pos-out-array\" count=\"12\">"
        "0.1 1.5 0.2 2.5 0.3 3.5 1.1 4.5 1.2 5.5 1.3 6.5"
        "</float_array><technique_common>"
        "<accessor source=\"#pos-out-array\" count=\"2\" stride=\"6\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"X\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"rot-output\"><float_array id=\"rot-output-array\" count=\"8\">"
        "1 2 3 10 4 5 6 20</float_array><technique_common>"
        "<accessor source=\"#rot-output-array\" count=\"2\" stride=\"4\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/><param name=\"ANGLE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"rot-in\"><float_array id=\"rot-in-array\" count=\"16\">"
        "-0.1 1 -0.2 2 -0.3 3 -0.4 9 0.9 4 0.8 5 0.7 6 0.6 19"
        "</float_array><technique_common>"
        "<accessor source=\"#rot-in-array\" count=\"2\" stride=\"8\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"X\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"ANGLE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"rot-out\"><float_array id=\"rot-out-array\" count=\"16\">"
        "0.1 1 0.2 2 0.3 3 0.4 11 1.1 4 1.2 5 1.3 6 1.4 21"
        "</float_array><technique_common>"
        "<accessor source=\"#rot-out-array\" count=\"2\" stride=\"8\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"X\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "<param name=\"TIME\" type=\"float\"/><param name=\"ANGLE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"scale-output\"><float_array id=\"scale-output-array\" count=\"6\">"
        "1 2 3 4 5 6</float_array><technique_common>"
        "<accessor source=\"#scale-output-array\" count=\"2\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
        "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-pos-output\"><float_array id=\"partial-pos-output-array\" count=\"2\">"
        "2 5</float_array><technique_common>"
        "<accessor source=\"#partial-pos-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"VALUE\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-angle-value-output\"><float_array "
        "id=\"partial-angle-value-output-array\" count=\"2\">30 60</float_array>"
        "<technique_common><accessor source=\"#partial-angle-value-output-array\" "
        "count=\"2\" stride=\"1\"><param name=\"VALUE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"partial-angle-value-in\"><float_array "
        "id=\"partial-angle-value-in-array\" count=\"4\">-0.1 25 0.9 55</float_array>"
        "<technique_common><accessor source=\"#partial-angle-value-in-array\" "
        "count=\"2\" stride=\"2\"><param name=\"TIME\" type=\"float\"/>"
        "<param name=\"VALUE\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-angle-named-output\"><float_array "
        "id=\"partial-angle-named-output-array\" count=\"2\">45 90</float_array>"
        "<technique_common><accessor source=\"#partial-angle-named-output-array\" "
        "count=\"2\" stride=\"1\"><param name=\"ANGLE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"partial-pos-in\"><float_array id=\"partial-pos-in-array\" count=\"4\">"
        "-0.1 1.5 0.9 4.5</float_array><technique_common>"
        "<accessor source=\"#partial-pos-in-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"VALUE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"partial-pos-out\"><float_array id=\"partial-pos-out-array\" count=\"4\">"
        "0.1 2.5 1.1 5.5</float_array><technique_common>"
        "<accessor source=\"#partial-pos-out-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"VALUE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"partial-scale-output\"><float_array id=\"partial-scale-output-array\" count=\"2\">"
        "2 5</float_array><technique_common>"
        "<accessor source=\"#partial-scale-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"VALUE\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-rotate-output\"><float_array id=\"partial-rotate-output-array\" count=\"2\">"
        "0.25 0.75</float_array><technique_common>"
        "<accessor source=\"#partial-rotate-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"VALUE\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-matrix-output\"><float_array id=\"partial-matrix-output-array\" count=\"2\">"
        "4 5</float_array><technique_common>"
        "<accessor source=\"#partial-matrix-output-array\" count=\"2\" stride=\"1\">"
        "<param name=\"VALUE\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"partial-matrix-in\"><float_array id=\"partial-matrix-in-array\" count=\"4\">"
        "-0.1 3.5 0.9 4.5</float_array><technique_common>"
        "<accessor source=\"#partial-matrix-in-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"VALUE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"partial-matrix-out\"><float_array id=\"partial-matrix-out-array\" count=\"4\">"
        "0.1 4.5 1.1 5.5</float_array><technique_common>"
        "<accessor source=\"#partial-matrix-out-array\" count=\"2\" stride=\"2\">"
        "<param name=\"TIME\" type=\"float\"/><param name=\"VALUE\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"matrix-output\"><float_array id=\"matrix-output-array\" count=\"32\">"
        "1 0 0 7 0 1 0 8 0 0 1 9 0 0 0 1 "
        "0 -1 0 1 1 0 0 2 0 0 1 3 0 0 0 1</float_array><technique_common>"
        "<accessor source=\"#matrix-output-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"matrix-in\"><float_array id=\"matrix-in-array\" count=\"32\">"
        "1 0 0 6 0 1 0 7 0 0 1 8 0 0 0 1 "
        "1 0 0 0 0 0 -1 1 0 1 0 2 0 0 0 1</float_array><technique_common>"
        "<accessor source=\"#matrix-in-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"matrix-out\"><float_array id=\"matrix-out-array\" count=\"32\">"
        "1 0 0 8 0 1 0 9 0 0 1 10 0 0 0 1 "
        "0 0 1 3 0 1 0 4 -1 0 0 5 0 0 0 1</float_array><technique_common>"
        "<accessor source=\"#matrix-out-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<sampler id=\"pos-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#pos-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#pos-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#pos-out\"/></sampler>"
        "<sampler id=\"rot-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#rot-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#rot-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#rot-out\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"shared-scale-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#pos-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"scale-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#scale-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-pos-sampler-z\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-pos-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#partial-pos-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#partial-pos-out\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-pos-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-pos-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#partial-pos-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#partial-pos-out\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-angle-value-sampler-a\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-angle-value-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#partial-angle-value-in\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-angle-value-sampler-b\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-angle-value-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#partial-angle-value-in\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-angle-named-sampler-a\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-angle-named-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-angle-named-sampler-b\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-angle-named-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-scale-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-scale-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-rotate-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-rotate-output\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"partial-matrix-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#partial-matrix-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#partial-matrix-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#partial-matrix-out\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<sampler id=\"matrix-sampler\"><input semantic=\"INPUT\" source=\"#anim-time\"/>"
        "<input semantic=\"OUTPUT\" source=\"#matrix-output\"/>"
        "<input semantic=\"IN_TANGENT\" source=\"#matrix-in\"/>"
        "<input semantic=\"OUT_TANGENT\" source=\"#matrix-out\"/>"
        "<input semantic=\"INTERPOLATION\" source=\"#anim-interp\"/></sampler>"
        "<channel source=\"#pos-sampler\" target=\"joint0/translation\"/>"
        "<channel source=\"#rot-sampler\" target=\"joint0/rotation\"/>"
        "<channel source=\"#shared-scale-sampler\" target=\"joint0/scalingShared\"/>"
        "<channel source=\"#scale-sampler\" target=\"joint0/scaling\"/>"
        "<channel source=\"#partial-pos-sampler\" target=\"joint0/translation.Y\"/>"
        "<channel source=\"#partial-pos-sampler-z\" target=\"joint0/translation.Z\"/>"
        "<channel source=\"#partial-scale-sampler\" target=\"joint0/scaling.Y\"/>"
        "<channel source=\"#partial-rotate-sampler\" target=\"joint0/rotation.Y\"/>"
        "<channel source=\"#partial-matrix-sampler\" target=\"joint0/matrixTransform(1)(3)\"/>"
        "<channel source=\"#partial-angle-value-sampler-a\" target=\"joint0/rotationAngleValueA.ANGLE\"/>"
        "<channel source=\"#partial-angle-value-sampler-b\" target=\"joint0/rotationAngleValueB.ANGLE\"/>"
        "<channel source=\"#partial-angle-named-sampler-a\" target=\"joint0/rotationAngleNamedA.ANGLE\"/>"
        "<channel source=\"#partial-angle-named-sampler-b\" target=\"joint0/rotationAngleNamedB.ANGLE\"/>"
        "<channel source=\"#matrix-sampler\" target=\"joint0/matrixTransform\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"joint0\" sid=\"joint0\" name=\"Joint0\" type=\"JOINT\">"
        "<translate sid=\"translation\">4 -5 -6</translate>"
        "<rotate sid=\"rotation\">0 0 1 0</rotate>"
        "<scale sid=\"scaling\">1 1 1</scale>"
        "<scale sid=\"scalingShared\">1 1 1</scale>"
        "<rotate sid=\"rotationAngleValueA\">1 0 0 0</rotate>"
        "<rotate sid=\"rotationAngleValueB\">1 0 0 0</rotate>"
        "<rotate sid=\"rotationAngleNamedA\">1 0 0 0</rotate>"
        "<rotate sid=\"rotationAngleNamedB\">1 0 0 0</rotate>"
        "<matrix sid=\"matrixTransform\">1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</matrix>"
        "</node>"
        "<node id=\"meshNode\" name=\"Mesh\"><instance_controller url=\"#skin\">"
        "<skeleton>#joint0</skeleton></instance_controller></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_skin_multi_primitive(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"geom\" name=\"Geom\"><mesh>"
        "<source id=\"geom-pos\"><float_array id=\"geom-pos-array\" count=\"18\">"
        "0 0 0 1 0 0 0 1 0 2 0 0 3 0 0 2 1 0"
        "</float_array><technique_common><accessor source=\"#geom-pos-array\" count=\"6\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"geom-verts\"><input semantic=\"POSITION\" source=\"#geom-pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-verts\" offset=\"0\"/><p>3 4 5</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_controllers><controller id=\"skin\"><skin source=\"#geom\">"
        "<bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>"
        "<source id=\"skin-joints\"><IDREF_array id=\"skin-joints-array\" count=\"2\">joint0 joint1</IDREF_array>"
        "<technique_common><accessor source=\"#skin-joints-array\" count=\"2\" stride=\"1\">"
        "<param name=\"JOINT\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"skin-bind\"><float_array id=\"skin-bind-array\" count=\"32\">"
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 "
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
        "</float_array><technique_common><accessor source=\"#skin-bind-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"skin-weights\"><float_array id=\"skin-weights-array\" count=\"12\">"
        "1 0 0.5 0.5 0 1 1 0 0.5 0.5 0 1"
        "</float_array><technique_common><accessor source=\"#skin-weights-array\" count=\"12\" stride=\"1\">"
        "<param name=\"WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<joints><input semantic=\"JOINT\" source=\"#skin-joints\"/>"
        "<input semantic=\"INV_BIND_MATRIX\" source=\"#skin-bind\"/></joints>"
        "<vertex_weights count=\"6\"><input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<vcount>2 2 2 2 2 2</vcount>"
        "<v>0 0 1 1 0 2 1 3 0 4 1 5 0 6 1 7 0 8 1 9 0 10 1 11</v>"
        "</vertex_weights></skin></controller></library_controllers>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"joint0\" name=\"Joint0\"><node id=\"joint1\" name=\"Joint1\"/></node>"
        "<node id=\"meshNode\" name=\"Mesh\"><instance_controller url=\"#skin\"><skeleton>#joint0</skeleton></instance_controller></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_skin_multi_source_primitives(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"geom\" name=\"Geom\"><mesh>"
        "<source id=\"geom-p0-pos\"><float_array id=\"geom-p0-pos-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#geom-p0-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"geom-p1-pos\"><float_array id=\"geom-p1-pos-array\" count=\"9\">"
        "2 0 0 3 0 0 2 1 0"
        "</float_array><technique_common><accessor source=\"#geom-p1-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"geom-p0-verts\"><input semantic=\"POSITION\" source=\"#geom-p0-pos\"/></vertices>"
        "<vertices id=\"geom-p1-verts\"><input semantic=\"POSITION\" source=\"#geom-p1-pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-p0-verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#geom-p1-verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_controllers><controller id=\"skin\"><skin source=\"#geom\">"
        "<bind_shape_matrix>1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1</bind_shape_matrix>"
        "<source id=\"skin-joints\"><IDREF_array id=\"skin-joints-array\" count=\"2\">joint0 joint1</IDREF_array>"
        "<technique_common><accessor source=\"#skin-joints-array\" count=\"2\" stride=\"1\">"
        "<param name=\"JOINT\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"skin-bind\"><float_array id=\"skin-bind-array\" count=\"32\">"
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1 "
        "1 0 0 0 0 1 0 0 0 0 1 0 0 0 0 1"
        "</float_array><technique_common><accessor source=\"#skin-bind-array\" count=\"2\" stride=\"16\">"
        "<param name=\"TRANSFORM\" type=\"float4x4\"/></accessor></technique_common></source>"
        "<source id=\"skin-weights\"><float_array id=\"skin-weights-array\" count=\"6\">1 1 1 1 1 1</float_array>"
        "<technique_common><accessor source=\"#skin-weights-array\" count=\"6\" stride=\"1\">"
        "<param name=\"WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<joints><input semantic=\"JOINT\" source=\"#skin-joints\"/>"
        "<input semantic=\"INV_BIND_MATRIX\" source=\"#skin-bind\"/></joints>"
        "<vertex_weights count=\"6\"><input semantic=\"JOINT\" source=\"#skin-joints\" offset=\"0\"/>"
        "<input semantic=\"WEIGHT\" source=\"#skin-weights\" offset=\"1\"/>"
        "<vcount>1 1 1 1 1 1</vcount>"
        "<v>0 0 0 1 0 2 1 3 1 4 1 5</v>"
        "</vertex_weights></skin></controller></library_controllers>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"joint0\" name=\"Joint0\"><node id=\"joint1\" name=\"Joint1\"/></node>"
        "<node id=\"meshNode\" name=\"Mesh\"><instance_controller url=\"#skin\"><skeleton>#joint0</skeleton></instance_controller></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_morph_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries>"
        "<geometry id=\"base\" name=\"Base\"><mesh>"
        "<source id=\"base-pos\"><float_array id=\"base-pos-array\" count=\"9\">0 0 0 1 0 0 0 1 0</float_array>"
        "<technique_common><accessor source=\"#base-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"base-nrm\"><float_array id=\"base-nrm-array\" count=\"9\">0 0 1 0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#base-nrm-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"base-uv\"><float_array id=\"base-uv-array\" count=\"6\">0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#base-uv-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"base-verts\"><input semantic=\"POSITION\" source=\"#base-pos\"/>"
        "<input semantic=\"NORMAL\" source=\"#base-nrm\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#base-verts\" offset=\"0\"/>"
        "<input semantic=\"TEXCOORD\" source=\"#base-uv\" offset=\"1\" set=\"0\"/>"
        "<p>0 0 1 1 2 2</p></triangles>"
        "</mesh></geometry>"
        "<geometry id=\"targetA\" name=\"TargetA\"><mesh>"
        "<source id=\"targetA-pos\"><float_array id=\"targetA-pos-array\" count=\"9\">0 0 0.25 1 0 0.25 0 1 0.25</float_array>"
        "<technique_common><accessor source=\"#targetA-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"targetA-nrm\"><float_array id=\"targetA-nrm-array\" count=\"9\">0 0 1 0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#targetA-nrm-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"targetA-uv\"><float_array id=\"targetA-uv-array\" count=\"6\">0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#targetA-uv-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"targetA-verts\"><input semantic=\"POSITION\" source=\"#targetA-pos\"/>"
        "<input semantic=\"NORMAL\" source=\"#targetA-nrm\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#targetA-verts\" offset=\"0\"/>"
        "<input semantic=\"TEXCOORD\" source=\"#targetA-uv\" offset=\"1\" set=\"0\"/>"
        "<p>0 0 1 1 2 2</p></triangles>"
        "</mesh></geometry>"
        "<geometry id=\"targetB\" name=\"TargetB\"><mesh>"
        "<source id=\"targetB-pos\"><float_array id=\"targetB-pos-array\" count=\"9\">0 0 0.5 1 0 0.5 0 1 0.5</float_array>"
        "<technique_common><accessor source=\"#targetB-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"targetB-nrm\"><float_array id=\"targetB-nrm-array\" count=\"9\">0 0 1 0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#targetB-nrm-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"targetB-uv\"><float_array id=\"targetB-uv-array\" count=\"6\">0 0 1 0 0 1</float_array>"
        "<technique_common><accessor source=\"#targetB-uv-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"targetB-verts\"><input semantic=\"POSITION\" source=\"#targetB-pos\"/>"
        "<input semantic=\"NORMAL\" source=\"#targetB-nrm\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#targetB-verts\" offset=\"0\"/>"
        "<input semantic=\"TEXCOORD\" source=\"#targetB-uv\" offset=\"1\" set=\"0\"/>"
        "<p>0 0 1 1 2 2</p></triangles>"
        "</mesh></geometry>"
        "</library_geometries>\n"
        "<library_controllers><controller id=\"morph\"><morph source=\"#base\">"
        "<source id=\"morph-targets\"><IDREF_array id=\"morph-targets-array\" count=\"2\">targetA targetB</IDREF_array>"
        "<technique_common><accessor source=\"#morph-targets-array\" count=\"2\" stride=\"1\">"
        "<param name=\"MORPH_TARGET\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"morph-weights\"><float_array id=\"morph-weights-array\" count=\"2\">0.25 0.75</float_array>"
        "<technique_common><accessor source=\"#morph-weights-array\" count=\"2\" stride=\"1\">"
        "<param name=\"MORPH_WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<targets><input semantic=\"MORPH_TARGET\" source=\"#morph-targets\"/>"
        "<input semantic=\"MORPH_WEIGHT\" source=\"#morph-weights\"/></targets>"
        "</morph></controller></library_controllers>\n"
        "<library_animations><animation id=\"morphAnim\">"
        "<source id=\"morphAnim-input\"><float_array id=\"morphAnim-input-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#morphAnim-input-array\" count=\"2\" stride=\"1\">"
        "<param name=\"TIME\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"morphAnim-output\"><float_array id=\"morphAnim-output-array\" count=\"2\">0.75 0.5</float_array>"
        "<technique_common><accessor source=\"#morphAnim-output-array\" count=\"2\" stride=\"1\">"
        "<param type=\"float\"/></accessor></technique_common></source>"
        "<sampler id=\"morphAnim-sampler\"><input semantic=\"INPUT\" source=\"#morphAnim-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#morphAnim-output\"/></sampler>"
        "<channel source=\"#morphAnim-sampler\" target=\"morph-weights(1)\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><instance_controller url=\"#morph\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_invalid_morph_target(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries>"
        "<geometry id=\"base\" name=\"Base\"><mesh>"
        "<source id=\"base-pos\"><float_array id=\"base-pos-array\" count=\"9\">0 0 0 1 0 0 0 1 0</float_array>"
        "<technique_common><accessor source=\"#base-pos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"base-verts\"><input semantic=\"POSITION\" source=\"#base-pos\"/></vertices>"
        "<triangles count=\"1\"><input semantic=\"VERTEX\" source=\"#base-verts\" offset=\"0\"/><p>0 1 2</p></triangles>"
        "</mesh></geometry>"
        "</library_geometries>\n"
        "<library_controllers><controller id=\"morph\"><morph source=\"#base\">"
        "<source id=\"morph-targets\"><IDREF_array id=\"morph-targets-array\" count=\"1\">MissingTarget</IDREF_array>"
        "<technique_common><accessor source=\"#morph-targets-array\" count=\"1\" stride=\"1\">"
        "<param name=\"MORPH_TARGET\" type=\"IDREF\"/></accessor></technique_common></source>"
        "<source id=\"morph-weights\"><float_array id=\"morph-weights-array\" count=\"1\">0.5</float_array>"
        "<technique_common><accessor source=\"#morph-weights-array\" count=\"1\" stride=\"1\">"
        "<param name=\"MORPH_WEIGHT\" type=\"float\"/></accessor></technique_common></source>"
        "<targets><input semantic=\"MORPH_TARGET\" source=\"#morph-targets\"/>"
        "<input semantic=\"MORPH_WEIGHT\" source=\"#morph-weights\"/></targets>"
        "</morph></controller></library_controllers>\n"
        "<library_animations><animation id=\"morphAnim\">"
        "<source id=\"morphAnim-input\"><float_array id=\"morphAnim-input-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#morphAnim-input-array\" count=\"2\" stride=\"1\">"
        "<param name=\"TIME\" type=\"float\"/></accessor></technique_common></source>"
        "<source id=\"morphAnim-output\"><float_array id=\"morphAnim-output-array\" count=\"2\">0 1</float_array>"
        "<technique_common><accessor source=\"#morphAnim-output-array\" count=\"2\" stride=\"1\">"
        "<param type=\"float\"/></accessor></technique_common></source>"
        "<sampler id=\"morphAnim-sampler\"><input semantic=\"INPUT\" source=\"#morphAnim-input\"/>"
        "<input semantic=\"OUTPUT\" source=\"#morphAnim-output\"/></sampler>"
        "<channel source=\"#morphAnim-sampler\" target=\"morph-weights(0)\"/>"
        "</animation></library_animations>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"node\" name=\"Node\"><instance_controller url=\"#morph\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
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

static inline
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

static inline
bool
ak_test_write_dae_brep_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.5.0\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"brepGeom\" name=\"BRepGeom\"><brep>"
        "<source id=\"brepPositions\"><float_array id=\"brepPositions-array\" count=\"6\">"
        "0 0 0 1 0 0"
        "</float_array><technique_common><accessor source=\"#brepPositions-array\" count=\"2\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"brepVertices\"><input semantic=\"POSITION\" source=\"#brepPositions\"/></vertices>"
        "<edges count=\"1\"><input semantic=\"VERTEX\" source=\"#brepVertices\" offset=\"0\"/><p>0 1</p></edges>"
        "</brep>"
        "</geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_geometry url=\"#brepGeom\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_spline_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"splineGeom\" name=\"SplineGeom\"><spline closed=\"1\">"
        "<source id=\"splinePos\"><float_array id=\"splinePos-array\" count=\"9\">"
        "0 0 0 1 0 0 1 1 0"
        "</float_array><technique_common><accessor source=\"#splinePos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<control_vertices id=\"splineCv\"><input semantic=\"POSITION\" source=\"#splinePos\"/></control_vertices>"
        "</spline></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_geometry url=\"#splineGeom\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_brep_nurbs_minimal(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.5.0\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_geometries><geometry id=\"brepNurbsGeom\" name=\"BRepNurbsGeom\"><brep>"
        "<curves><curve sid=\"nurbsCurve\"><nurbs degree=\"2\" closed=\"1\">"
        "<source id=\"curvePos\"><float_array id=\"curvePos-array\" count=\"9\">"
        "0 0 0 1 0 0 1 1 0"
        "</float_array><technique_common><accessor source=\"#curvePos-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<control_vertices id=\"curveCv\"><input semantic=\"POSITION\" source=\"#curvePos\"/></control_vertices>"
        "</nurbs></curve></curves>"
        "<surfaces><surface sid=\"nurbsSurface\"><nurbs_surface degree_u=\"1\" degree_v=\"1\">"
        "<source id=\"surfacePos\"><float_array id=\"surfacePos-array\" count=\"12\">"
        "0 0 0 1 0 0 0 1 0 1 1 0"
        "</float_array><technique_common><accessor source=\"#surfacePos-array\" count=\"4\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<control_vertices id=\"surfaceCv\"><input semantic=\"POSITION\" source=\"#surfacePos\"/></control_vertices>"
        "</nurbs_surface></surface>"
        "<surface sid=\"sweptSurface\"><swept_surface>"
        "<curve sid=\"sweepCurve\"><line><origin>0 0 0</origin><direction>0 1 0</direction></line></curve>"
        "<direction>1 0 0</direction><origin>0 0 0</origin><axis>0 0 1</axis>"
        "</swept_surface></surface></surfaces>"
        "</brep></geometry></library_geometries>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_geometry url=\"#brepNurbsGeom\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
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

static inline
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
        "<technique_common><spot><color>1 1 1</color>"
        "<constant_attenuation>1</constant_attenuation>"
        "<linear_attenuation>0.2</linear_attenuation>"
        "<falloff_angle>20</falloff_angle>"
        "</spot></technique_common>"
        "<technique profile=\"MAX3D\"><intensity>3</intensity></technique>"
        "<extra><technique profile=\"FCOLLADA\">"
        "<hotspot_beam>15</hotspot_beam><outer_cone>40</outer_cone>"
        "</technique></extra>"
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

static inline
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

static inline
bool
ak_test_write_dae_external_node_lib(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_cameras><camera id=\"cam\" name=\"ExternalCam\"><optics><technique_common>"
        "<perspective><yfov>45</yfov><znear>0.1</znear><zfar>100</zfar></perspective>"
        "</technique_common></optics></camera></library_cameras>\n"
        "<library_images><image id=\"tex\" name=\"ExternalTex\">"
        "<init_from>data:image/png;base64,QUJD</init_from>"
        "</image></library_images>\n"
        "<library_effects><effect id=\"fx\"><profile_COMMON>"
        "<newparam sid=\"tex-surface\"><surface type=\"2D\"><init_from>tex</init_from></surface></newparam>"
        "<newparam sid=\"tex-sampler\"><sampler2D><source>tex-surface</source></sampler2D></newparam>"
        "<technique sid=\"common\">"
        "<lambert><diffuse><texture texture=\"tex-sampler\" texcoord=\"UV0\"/></diffuse></lambert>"
        "</technique></profile_COMMON></effect></library_effects>\n"
        "<library_materials><material id=\"mat\" name=\"ExternalMat\">"
        "<instance_effect url=\"#fx\"/>"
        "</material></library_materials>\n"
        "<library_geometries><geometry id=\"shape\" name=\"ExternalShape\"><mesh>"
        "<source id=\"shape-positions\"><float_array id=\"shape-positions-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#shape-positions-array\" count=\"3\" stride=\"3\">"
        "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<source id=\"shape-texcoords\"><float_array id=\"shape-texcoords-array\" count=\"6\">"
        "0 0 1 0 0 1"
        "</float_array><technique_common><accessor source=\"#shape-texcoords-array\" count=\"3\" stride=\"2\">"
        "<param name=\"S\" type=\"float\"/><param name=\"T\" type=\"float\"/>"
        "</accessor></technique_common></source>"
        "<vertices id=\"shape-vertices\"><input semantic=\"POSITION\" source=\"#shape-positions\"/></vertices>"
        "<triangles count=\"1\" material=\"matSymbol\">"
        "<input semantic=\"VERTEX\" source=\"#shape-vertices\" offset=\"0\"/>"
        "<input semantic=\"TEXCOORD\" source=\"#shape-texcoords\" offset=\"1\" set=\"0\"/>"
        "<p>0 0 1 1 2 2</p></triangles>"
        "</mesh></geometry></library_geometries>\n"
        "<library_nodes><node id=\"shared\" name=\"ExternalShared\">"
        "<node id=\"camNode\"><instance_camera url=\"#cam\"/></node>"
        "<node id=\"geoNode\"><instance_geometry url=\"#shape\">"
        "<bind_material><technique_common>"
        "<instance_material symbol=\"matSymbol\" target=\"#mat\">"
        "<bind_vertex_input semantic=\"UV0\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>"
        "</instance_material>"
        "</technique_common></bind_material>"
        "</instance_geometry></node>"
        "</node></library_nodes>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_external_node_main(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node url=\"external_nodes.dae#shared\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_external_node_wrapped_main(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_nodes>"
        "<node id=\"wrapper\" name=\"Wrapper\">"
        "<instance_node url=\"external_nodes.dae#shared\"/>"
        "</node>"
        "</library_nodes>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node url=\"#wrapper\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
bool
ak_test_write_dae_missing_external_node_main(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">\n"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>\n"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"root\" name=\"Root\"><instance_node url=\"#missingShared\"/></node>"
        "</visual_scene></library_visual_scenes>\n"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene>\n"
        "</COLLADA>\n",
        file);

  return fclose(file) == 0;
}

static inline
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

static inline
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

static inline
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

static inline
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

static inline
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

static inline
AkGeometry *
ak_test_make_triangle_geom(AkHeap *heap, void *parent, const float positions[9]) {
  return ak_test_make_geom_with_positions(heap, parent, positions, 3);
}

static inline
AkGeometry *
ak_test_make_line_geom_with_positions(AkHeap      *heap,
                                      void        *parent,
                                      const float *positions,
                                      uint32_t     positionCount,
                                      AkLineMode   mode) {
  AkGeometry  *geom;
  AkObject    *meshObj;
  AkMesh      *mesh;
  AkTriangles *tri;
  AkLines     *lines;

  geom = ak_test_make_geom_with_positions(heap, parent, positions, positionCount);
  if (!geom || !geom->gdata)
    return NULL;

  meshObj = geom->gdata;
  mesh    = ak_objGet(meshObj);
  tri     = mesh ? (AkTriangles *)mesh->primitive : NULL;
  if (!tri)
    return NULL;

  lines = ak_heap_calloc(heap, meshObj, sizeof(*lines));
  if (!lines)
    return NULL;

  lines->base      = tri->base;
  lines->base.type = AK_PRIMITIVE_LINES;
  lines->mode      = mode;
  mesh->primitive  = &lines->base;

  return geom;
}

static inline
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

static inline
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
    case 16: componentSize = AK_COMPONENT_SIZE_MAT4;  break;
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

static inline
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

static inline
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

#endif /* assetkit_test_export_common_h */
