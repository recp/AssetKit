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

#include "../../../test_export_common.h"

static bool
ak_test_ply_files_equal(const char *aPath, const char *bPath) {
  unsigned char aBuffer[16u * 1024u];
  unsigned char bBuffer[16u * 1024u];
  FILE         *a;
  FILE         *b;
  size_t        aRead;
  size_t        bRead;
  bool          equal;

  a = fopen(aPath, "rb");
  b = fopen(bPath, "rb");
  if (!a || !b) {
    if (a)
      fclose(a);
    if (b)
      fclose(b);
    return false;
  }

  equal = true;
  do {
    aRead = fread(aBuffer, 1u, sizeof(aBuffer), a);
    bRead = fread(bBuffer, 1u, sizeof(bBuffer), b);
    if (aRead != bRead || memcmp(aBuffer, bBuffer, aRead) != 0) {
      equal = false;
      break;
    }
  } while (aRead == sizeof(aBuffer));

  if (ferror(a) || ferror(b))
    equal = false;
  fclose(a);
  fclose(b);
  return equal;
}

static AkDoc *
ak_test_make_ply_triangle_doc(bool withAttributes) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkScene    *scene;
  AkNode     *root, *node;
  AkGeometry *geom;
  AkMesh     *mesh;
  AkMeshPrimitive *prim;
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

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  if (!heap || !doc)
    return NULL;
  ak_heap_setdata(heap, doc);

  scene       = ak_heap_calloc(heap, doc, sizeof(*scene));
  root        = ak_heap_calloc(heap, scene, sizeof(*root));
  node        = ak_heap_calloc(heap, doc, sizeof(*node));
  if (!scene || !root || !node)
    return NULL;
  node->name  = "PLY Node";
  scene->node = root;
  doc->scene  = scene;

  geom = ak_test_make_triangle_geom(heap, doc, positions);
  if (!geom)
    return NULL;
  mesh = ak_objGet(geom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  if (!prim)
    return NULL;

  if (withAttributes) {
    AkInput *colorInput;
    const uint8_t colors[12] = {
      255u, 0u,   0u,   255u,
      0u,   128u, 0u,   255u,
      0u,   0u,   255u, 255u
    };

    if (!ak_test_add_texcoord_input(heap, prim, 0))
      return NULL;
    colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
    if (!colorInput)
      return NULL;
    colorInput->semantic = AK_INPUT_COLOR;
    colorInput->set      = 0;
    colorInput->index    = 0;
    colorInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                       colorInput,
                                                       colors,
                                                       4,
                                                       3);
    if (!colorInput->accessor)
      return NULL;
    colorInput->next     = prim->input;
    prim->input          = colorInput;
    prim->inputCount++;
  }

  doc->lib.geometries.first = geom;
  doc->lib.geometries.last  = geom;
  doc->lib.geometries.count = 1;

  ak_addSubNode(root, node, false);
  ak_nodeSetTransformMatrix(node, matrix);
  if (!ak_nodeAttachGeometry(node, geom))
    return NULL;

  return doc;
}

TEST_IMPL(ply_export_binary_triangle_smoke) {
  AkDoc     *doc;
  AkDoc     *roundTrip;
  uintptr_t  savedFormat;
  uintptr_t  savedUV;
  uintptr_t  savedColorMode;
  struct stat st;
  const char *outDir  = "./assetkit_export_ply_binary_triangle_smoke";
  const char *plyPath = "./assetkit_export_ply_binary_triangle_smoke/model.ply";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_ply_triangle_doc(false);
  ASSERT(doc != NULL);

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_BINARY_LITTLE);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_NONE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(stat(plyPath, &st) == 0);
  ASSERT(st.st_size > 0);
  ASSERT(ak_test_file_contains(plyPath, "format binary_little_endian 1.0"));
  ASSERT(ak_test_file_contains(plyPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(plyPath, "element face 1"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, plyPath, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_ascii_attributes_smoke) {
  AkDoc     *doc;
  AkDoc     *roundTrip;
  uintptr_t  savedFormat;
  uintptr_t  savedUV;
  uintptr_t  savedColorMode;
  const char *outDir  = "./assetkit_export_ply_ascii_attributes_smoke";
  const char *plyPath = "./assetkit_export_ply_ascii_attributes_smoke/model.ply";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_ply_triangle_doc(true);
  ASSERT(doc != NULL);

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, true);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_LINEAR);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(ak_test_file_contains(plyPath, "format ascii 1.0"));
  ASSERT(ak_test_file_contains(plyPath, "property float s"));
  ASSERT(ak_test_file_contains(plyPath, "property float red"));
  ASSERT(ak_test_file_contains(plyPath, "property float alpha"));
  ASSERT(ak_test_file_contains(plyPath, "2 3 4 0 0 1 0 0 1"));
  ASSERT(ak_test_file_contains(plyPath, "3 0 1 2"));

  roundTrip = NULL;
  ASSERT(ak_load(&roundTrip, plyPath, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_bakes_base_color_texture) {
  AkDoc             *doc;
  AkDoc             *parallelRoundTrip;
  AkHeap            *heap;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkMaterial        *material;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *textureRef;
  AkTexture         *texture;
  AkSampler         *sampler;
  AkImage           *image;
  AkInput           *texInput;
  AkInput           *colorInput;
  AkAccessor        *savedTexAccessor;
  AkTextureTransform *textureTransform;
  AkColor           *borderColor;
  uint8_t           *multiIndices;
  uintptr_t          savedFormat;
  uintptr_t          savedNormals;
  uintptr_t          savedUV;
  uintptr_t          savedColorMode;
  uintptr_t          savedBakeTextures;
  const char        *outDir  = "./assetkit_export_ply_texture_bake";
  const char        *plyPath = "./assetkit_export_ply_texture_bake/model.ply";
  const char        *offDir  = "./assetkit_export_ply_texture_bake_off";
  const char        *offPath = "./assetkit_export_ply_texture_bake_off/model.ply";
  const char        *missingImageDir =
    "./assetkit_export_ply_texture_bake_missing_image";
  const char        *missingImagePath =
    "./assetkit_export_ply_texture_bake_missing_image/model.ply";
  const char        *missingUVDir =
    "./assetkit_export_ply_texture_bake_missing_uv";
  const char        *missingUVPath =
    "./assetkit_export_ply_texture_bake_missing_uv/model.ply";
  const char        *combinedDir =
    "./assetkit_export_ply_texture_bake_combined";
  const char        *combinedPath =
    "./assetkit_export_ply_texture_bake_combined/model.ply";
  const char        *canonicalDir =
    "./assetkit_export_ply_texture_bake_canonical";
  const char        *canonicalPath =
    "./assetkit_export_ply_texture_bake_canonical/model.ply";
  const char        *mirrorDir =
    "./assetkit_export_ply_texture_bake_mirror";
  const char        *mirrorPath =
    "./assetkit_export_ply_texture_bake_mirror/model.ply";
  const char        *borderDir =
    "./assetkit_export_ply_texture_bake_border";
  const char        *borderPath =
    "./assetkit_export_ply_texture_bake_border/model.ply";
  const char        *linearDir =
    "./assetkit_export_ply_texture_bake_linear";
  const char        *linearPath =
    "./assetkit_export_ply_texture_bake_linear/model.ply";
  const char        *nonfiniteUVDir =
    "./assetkit_export_ply_texture_bake_nonfinite_uv";
  const char        *nonfiniteUVPath =
    "./assetkit_export_ply_texture_bake_nonfinite_uv/model.ply";
  const char        *nonfiniteColorDir =
    "./assetkit_export_ply_texture_bake_nonfinite_color";
  const char        *nonfiniteColorPath =
    "./assetkit_export_ply_texture_bake_nonfinite_color/model.ply";
  const char        *parallelADir =
    "./assetkit_export_ply_texture_bake_parallel_a";
  const char        *parallelAPath =
    "./assetkit_export_ply_texture_bake_parallel_a/model.ply";
  const char        *parallelBDir =
    "./assetkit_export_ply_texture_bake_parallel_b";
  const char        *parallelBPath =
    "./assetkit_export_ply_texture_bake_parallel_b/model.ply";
  const float        repeatedUVs[6] = {
    0.0f, 0.0f,
    2.0f, 0.0f,
    0.0f, 2.0f
  };
  const float unitUVs[6] = {
    0.0f, 0.0f,
    1.0f, 0.0f,
    0.0f, 1.0f
  };
  const float wideUVs[6] = {
    0.0f,  0.0f,
    16.0f, 0.0f,
    0.0f, 16.0f
  };
  const float originalMatrix[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    2.0f, 3.0f, 4.0f, 1.0f
  };
  const float parallelMatrix[16] = {
    4.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 4.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 4.0f, 0.0f,
    2.0f, 3.0f, 4.0f, 1.0f
  };
  const float negativeUVs[6] = {
    -0.25f, 0.25f,
    -0.25f, 0.25f,
    -0.25f, 0.25f
  };
  const float centerUVs[6] = {
    0.5f, 0.5f,
    0.5f, 0.5f,
    0.5f, 0.5f
  };
  const float nonfiniteUVs[6] = {
    NAN,       0.0f,
    INFINITY,  0.0f,
    -INFINITY, 0.0f
  };
  const float whiteTexelUVs[6] = {
    0.75f, 0.75f,
    0.75f, 0.75f,
    0.75f, 0.75f
  };
  const float nonfiniteColors[12] = {
    INFINITY, NAN, -INFINITY, 1.0f,
    INFINITY, NAN, -INFINITY, 1.0f,
    INFINITY, NAN, -INFINITY, 1.0f
  };
  const uint8_t halfColors[12] = {
    128u, 128u, 128u, 128u,
    255u, 255u, 255u, 255u,
    255u, 255u, 255u, 255u
  };
  const uint8_t whiteColors[12] = {
    255u, 255u, 255u, 255u,
    255u, 255u, 255u, 255u,
    255u, 255u, 255u, 255u
  };
  const uint8_t rgbaPixels[16] = {
    255u, 0u,   0u,   128u,
    0u,   255u, 0u,   64u,
    0u,   0u,   255u, 255u,
    255u, 255u, 255u, 0u
  };

  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(offDir);
  ak_test_export_cleanup(missingImageDir);
  ak_test_export_cleanup(missingUVDir);
  ak_test_export_cleanup(combinedDir);
  ak_test_export_cleanup(canonicalDir);
  ak_test_export_cleanup(mirrorDir);
  ak_test_export_cleanup(borderDir);
  ak_test_export_cleanup(linearDir);
  ak_test_export_cleanup(nonfiniteUVDir);
  ak_test_export_cleanup(nonfiniteColorDir);
  ak_test_export_cleanup(parallelADir);
  ak_test_export_cleanup(parallelBDir);
  doc = ak_test_make_ply_triangle_doc(false);
  ASSERT(doc != NULL);

  heap = ak_heap_getheap(doc);
  geom = doc->lib.geometries.first;
  mesh = geom && geom->gdata ? ak_objGet(geom->gdata) : NULL;
  prim = mesh ? mesh->primitive : NULL;
  ASSERT(heap != NULL && prim != NULL);
  doc->inf = ak_heap_calloc(heap, doc, sizeof(*doc->inf));
  ASSERT(doc->inf != NULL);
  doc->inf->flipImage = false;
  texInput = ak_test_add_texcoord_input(heap, prim, 0u);
  ASSERT(texInput != NULL);
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    repeatedUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);

  material   = ak_heap_calloc(heap, doc, sizeof(*material));
  surface    = material ? ak_heap_calloc(heap, material, sizeof(*surface)) : NULL;
  baseColor  = surface ? ak_test_material_input(heap, surface) : NULL;
  textureRef = baseColor ? ak_heap_calloc(heap, baseColor, sizeof(*textureRef)) : NULL;
  texture    = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler    = texture ? ak_heap_calloc(heap, texture, sizeof(*sampler)) : NULL;
  image      = texture ? ak_heap_calloc(heap, texture, sizeof(*image)) : NULL;
  ASSERT(material != NULL && surface != NULL && baseColor != NULL);
  ASSERT(textureRef != NULL && texture != NULL && sampler != NULL && image != NULL);

  sampler->magfilter   = AK_MAGFILTER_NEAREST;
  sampler->wrapS       = AK_WRAP_MODE_WRAP;
  sampler->wrapT       = AK_WRAP_MODE_WRAP;
  texture->image       = image;
  texture->sampler     = sampler;
  textureRef->texture  = texture;
  textureRef->slot     = 0;
  textureRef->channels = AK_TEXTURE_CHANNEL_RGBA;
  textureRef->colorSpace = AK_TEXTURE_COLORSPACE_SRGB;

  baseColor->source       = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType    = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture      = textureRef;
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;
  surface->type           = AK_MATERIAL_TYPE_PBR_METALLIC_ROUGHNESS;
  surface->baseColor      = baseColor;
  material->surface       = surface;
  prim->material          = material;

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedNormals   = ak_opt_get(AK_OPT_PLY_EXPORT_NORMALS);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  savedBakeTextures = ak_opt_get(AK_OPT_PLY_EXPORT_BAKE_TEXTURES);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_SRGB);

  /* Bake-off is the fast path: original topology, material/vertex colors,
     and no texture image sampling. All three authored UV corners would hit
     the same red texel, so sampling only original vertices is insufficient. */
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, false);
  ASSERT(ak_export(doc, offDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(offPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(offPath, "element face 1"));
  ASSERT(ak_test_file_contains(offPath, "2 3 4 255 255 255"));
  ASSERT(!ak_test_file_contains(offPath, "property uchar alpha"));
  ASSERT(image->data == NULL);

  /* Missing image data and missing UV accessors are deterministic fallback
     paths: retain the source triangle instead of emitting partial bake data. */
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, true);
  ASSERT(ak_export(doc, missingImageDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(missingImagePath, "element vertex 3"));
  ASSERT(ak_test_file_contains(missingImagePath, "element face 1"));
  ASSERT(ak_test_file_contains(missingImagePath,
                               "comment texture_bake enabled_no_textures"));

  /* Bake-on subdivides in UV/image space. The generated x=2.25 vertex has
     UV (0.5, 0), proving that the green checker texel survives even though
     every original corner samples red. */
  image->data = ak_test_load_rgba_file(heap, image, NULL, false);
  ASSERT(image->data != NULL);
  savedTexAccessor = texInput->accessor;
  texInput->accessor = NULL;
  ASSERT(ak_export(doc, missingUVDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(missingUVPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(missingUVPath, "element face 1"));
  texInput->accessor = savedTexAccessor;

  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, true);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, savedNormals);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, savedBakeTextures);

  ASSERT(ak_test_file_contains(plyPath, "comment texture_bake enabled"));
  ASSERT(ak_test_file_contains(plyPath, "element vertex 91"));
  ASSERT(ak_test_file_contains(plyPath, "element face 144"));
  ASSERT(ak_test_file_contains(plyPath, "2 3 4 255 0 0 255"));
  ASSERT(ak_test_file_contains(plyPath, "2.25 3 4 0 255 0 255"));

  /* A large binary triangle crosses the parallel bake threshold. Repeated
     exports must remain byte-identical even though independent grid-row
     ranges are sampled and packed by the shared worker pool. */
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    wideUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  ak_nodeSetTransformMatrix(doc->scene->node->chld, parallelMatrix);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_BINARY_LITTLE);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_SRGB);
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, true);
  ASSERT(ak_export(doc, parallelADir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_export(doc, parallelBDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(parallelAPath, "element vertex 4278"));
  ASSERT(ak_test_file_contains(parallelAPath, "element face 8281"));
  ASSERT(ak_test_ply_files_equal(parallelAPath, parallelBPath));
  parallelRoundTrip = NULL;
  ASSERT(ak_load(&parallelRoundTrip, parallelAPath, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(parallelRoundTrip != NULL);
  ak_free(parallelRoundTrip);
  texInput->accessor = savedTexAccessor;
  ak_nodeSetTransformMatrix(doc->scene->node->chld, originalMatrix);

  /* One combined case covers texture transforms, clamp+nearest,
     material/vertex/texture alpha multiplication, and a UV index offset that
     differs from the position offset. */
  memcpy(image->data->data, rgbaPixels, sizeof(rgbaPixels));
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    unitUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  textureTransform = ak_heap_calloc(heap,
                                    textureRef,
                                    sizeof(*textureTransform));
  ASSERT(textureTransform != NULL);
  textureTransform->scale[0]  = 1.0f;
  textureTransform->scale[1]  = 1.0f;
  textureTransform->offset[0] = 0.5f;
  textureRef->transform       = textureTransform;

  colorInput = ak_heap_calloc(heap, prim, sizeof(*colorInput));
  ASSERT(colorInput != NULL);
  colorInput->semantic = AK_INPUT_COLOR;
  colorInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                     colorInput,
                                                     halfColors,
                                                     4u,
                                                     3u);
  ASSERT(colorInput->accessor != NULL);
  colorInput->isIndexed   = true;
  colorInput->indexOffset = 0u;
  colorInput->next        = prim->input;
  prim->input             = colorInput;
  prim->inputCount++;

  prim->indices = ak_indexArrayAlloc(heap, prim, 6u, AKT_UBYTE);
  ASSERT(prim->indices != NULL);
  multiIndices = prim->indices->items;
  multiIndices[0] = 0u; multiIndices[1] = 2u;
  multiIndices[2] = 1u; multiIndices[3] = 1u;
  multiIndices[4] = 2u; multiIndices[5] = 0u;
  prim->indices->max  = 2u;
  prim->indexStride   = 2u;
  prim->pos->isIndexed = true;
  prim->pos->indexOffset = 0u;
  texInput->isIndexed = true;
  texInput->indexOffset = 1u;

  baseColor->color.rgba.R = 0.5f;
  baseColor->color.rgba.G = 0.25f;
  baseColor->color.rgba.B = 0.75f;
  baseColor->color.rgba.A = 0.8f;
  sampler->magfilter      = AK_MAGFILTER_NEAREST;
  sampler->wrapS          = AK_WRAP_MODE_CLAMP;
  sampler->wrapT          = AK_WRAP_MODE_CLAMP;
  textureRef->colorSpace  = AK_TEXTURE_COLORSPACE_SRGB;

  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_SRGB);
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, true);
  doc->inf->flipImage = false;
  ASSERT(ak_export(doc, canonicalDir, AK_FILE_TYPE_PLY) == AK_OK);
  doc->inf->flipImage = true;
  ASSERT(ak_export(doc, combinedDir, AK_FILE_TYPE_PLY) == AK_OK);
  /* AkImageData is canonical after loading. Source-format image orientation
     metadata must not change which texel the PLY baker samples. */
  ASSERT(ak_test_ply_files_equal(canonicalPath, combinedPath));
  /* Distinct canonical texels stay attached to their authored UV/positions. */
  ASSERT(ak_test_file_contains(combinedPath, "2 3 4 137 99 165 0"));
  ASSERT(ak_test_file_contains(combinedPath, "3 3 4 0 137 0 51"));

  /* Negative mirror and border coordinates must not collapse to ordinary
     repeat behavior. Keep each triangle's UV constant to isolate sampling. */
  colorInput->accessor = ak_test_make_ubyte_accessor(heap,
                                                     colorInput,
                                                     whiteColors,
                                                     4u,
                                                     3u);
  ASSERT(colorInput->accessor != NULL);
  baseColor->color.rgba.R = 1.0f;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = 1.0f;
  textureRef->transform   = NULL;
  textureRef->colorSpace  = AK_TEXTURE_COLORSPACE_LINEAR;
  doc->inf->flipImage     = false;
  multiIndices[1] = multiIndices[3] = multiIndices[5] = 0u;
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    negativeUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  sampler->wrapS = AK_WRAP_MODE_MIRROR;
  sampler->wrapT = AK_WRAP_MODE_MIRROR;
  ASSERT(ak_export(doc, mirrorDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(mirrorPath, "2 3 4 255 0 0 128"));

  borderColor = ak_heap_aligned_calloc(heap,
                                       sampler,
                                       AK_ALIGNOF(AkColor),
                                       sizeof(*borderColor));
  ASSERT(borderColor != NULL);
  borderColor->rgba.R = 1.0f;
  borderColor->rgba.G = 0.0f;
  borderColor->rgba.B = 1.0f;
  borderColor->rgba.A = 0.5f;
  sampler->borderColor = borderColor;
  sampler->wrapS       = AK_WRAP_MODE_BORDER;
  sampler->wrapT       = AK_WRAP_MODE_CLAMP;
  ASSERT(ak_export(doc, borderDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(borderPath, "2 3 4 255 0 255 128"));

  /* The default linear magnification path bilinearly averages the four 2x2
     texels at UV (0.5, 0.5), including alpha. */
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    centerUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  sampler->magfilter = AK_MAGFILTER_LINEAR;
  sampler->wrapS     = AK_WRAP_MODE_CLAMP;
  sampler->wrapT     = AK_WRAP_MODE_CLAMP;
  ASSERT(ak_export(doc, linearDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(linearPath, "2 3 4 188 188 188 112"));

  /* Every non-finite UV component has the same deterministic origin
     fallback before transform, wrap, filtering, or texel conversion. Keep
     the UV indices distinct so NaN, +infinity, and -infinity are each read. */
  multiIndices[1] = 0u;
  multiIndices[3] = 1u;
  multiIndices[5] = 2u;
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    nonfiniteUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  sampler->magfilter = AK_MAGFILTER_NEAREST;
  sampler->wrapS     = AK_WRAP_MODE_CLAMP;
  sampler->wrapT     = AK_WRAP_MODE_CLAMP;
  ASSERT(ak_export(doc, nonfiniteUVDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(nonfiniteUVPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(nonfiniteUVPath, "element face 1"));
  ASSERT(ak_test_file_contains(nonfiniteUVPath,
                               "2 3 4 255 0 0 128"));
  ASSERT(ak_test_file_contains(nonfiniteUVPath,
                               "3 3 4 255 0 0 128"));
  ASSERT(ak_test_file_contains(nonfiniteUVPath,
                               "2 4 4 255 0 0 128"));

  /* Malformed material and vertex colors must never reach either float to
     byte conversion. RGB exercises the linear-sRGB conversion path, while
     alpha exercises the linear scalar path. */
  texInput->accessor = ak_test_make_float_accessor(heap,
                                                    texInput,
                                                    whiteTexelUVs,
                                                    2u,
                                                    3u);
  ASSERT(texInput->accessor != NULL);
  colorInput->accessor = ak_test_make_float_accessor(heap,
                                                      colorInput,
                                                      nonfiniteColors,
                                                      4u,
                                                      3u);
  ASSERT(colorInput->accessor != NULL);
  baseColor->color.rgba.R = NAN;
  baseColor->color.rgba.G = 1.0f;
  baseColor->color.rgba.B = 1.0f;
  baseColor->color.rgba.A = NAN;
  ASSERT(ak_export(doc, nonfiniteColorDir, AK_FILE_TYPE_PLY) == AK_OK);
  ASSERT(ak_test_file_contains(nonfiniteColorPath, "element vertex 3"));
  ASSERT(ak_test_file_contains(nonfiniteColorPath, "element face 1"));
  ASSERT(ak_test_file_contains(nonfiniteColorPath, "2 3 4 0 0 0 0"));

  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, savedNormals);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);
  ak_opt_set(AK_OPT_PLY_EXPORT_BAKE_TEXTURES, savedBakeTextures);

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  ak_test_export_cleanup(offDir);
  ak_test_export_cleanup(missingImageDir);
  ak_test_export_cleanup(missingUVDir);
  ak_test_export_cleanup(combinedDir);
  ak_test_export_cleanup(canonicalDir);
  ak_test_export_cleanup(mirrorDir);
  ak_test_export_cleanup(borderDir);
  ak_test_export_cleanup(linearDir);
  ak_test_export_cleanup(nonfiniteUVDir);
  ak_test_export_cleanup(nonfiniteColorDir);
  ak_test_export_cleanup(parallelADir);
  ak_test_export_cleanup(parallelBDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_unlabelled_space_is_yup_metres) {
  AkDoc      *doc;
  AkUnit      unit;
  uintptr_t   savedFormat;
  uintptr_t   savedUV;
  uintptr_t   savedColorMode;
  const char *outDir  = "./assetkit_export_ply_canonical_space";
  const char *plyPath = "./assetkit_export_ply_canonical_space/model.ply";

  ak_test_export_cleanup(outDir);
  doc = ak_test_make_ply_triangle_doc(false);
  ASSERT(doc != NULL);

  memset(&unit, 0, sizeof(unit));
  unit.name     = "inch";
  unit.dist     = 0.0254;
  doc->unit     = &unit;
  doc->coordSys = AK_ZUP;

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_NONE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(ak_test_file_contains(plyPath, "0.0508 0.1016 -0.0762"));

  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_groups_faces_before_edges) {
  AkHeap     *heap;
  AkDoc      *doc;
  AkGeometry *faceA;
  AkGeometry *edge;
  AkGeometry *faceB;
  uintptr_t   savedFormat;
  uintptr_t   savedNormals;
  uintptr_t   savedUV;
  uintptr_t   savedColorMode;
  const char *outDir  = "./assetkit_export_ply_face_edge_order";
  const char *plyPath = "./assetkit_export_ply_face_edge_order/model.ply";
  const float positionsA[9] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const float positionsEdge[6] = {
    2.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f
  };
  const float positionsB[9] = {
    4.0f, 0.0f, 0.0f,
    5.0f, 0.0f, 0.0f,
    4.0f, 1.0f, 0.0f
  };

  ak_test_export_cleanup(outDir);
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ASSERT(heap != NULL && doc != NULL);
  ak_heap_setdata(heap, doc);

  faceA = ak_test_make_triangle_geom(heap, doc, positionsA);
  edge  = ak_test_make_line_geom_with_positions(heap,
                                                 doc,
                                                 positionsEdge,
                                                 2u,
                                                 AK_LINES);
  faceB = ak_test_make_triangle_geom(heap, doc, positionsB);
  ASSERT(faceA != NULL && edge != NULL && faceB != NULL);

  faceA->next = edge;
  edge->next  = faceB;
  doc->lib.geometries.first = faceA;
  doc->lib.geometries.last  = faceB;
  doc->lib.geometries.count = 3u;

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedNormals   = ak_opt_get(AK_OPT_PLY_EXPORT_NORMALS);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_NONE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, savedNormals);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(ak_test_file_contains(plyPath, "element vertex 8"));
  ASSERT(ak_test_file_contains(plyPath, "element face 2"));
  ASSERT(ak_test_file_contains(plyPath, "element edge 1"));
  ASSERT(ak_test_file_contains(plyPath, "3 0 1 2\n3 5 6 7\n3 4\n"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}

TEST_IMPL(ply_export_discovers_late_global_schema_before_counting) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkGeometry      *indexedGeom;
  AkGeometry      *normalGeom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *normalInput;
  uintptr_t        savedFormat;
  uintptr_t        savedNormals;
  uintptr_t        savedUV;
  uintptr_t        savedColorMode;
  const char      *outDir  = "./assetkit_export_ply_late_schema";
  const char      *plyPath = "./assetkit_export_ply_late_schema/model.ply";
  const float indexedPositions[12] = {
    0.0f, 0.0f, 0.0f,
    1.0f, 0.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    0.0f, 1.0f, 0.0f
  };
  const uint8_t indices[6] = {0u, 1u, 2u, 0u, 2u, 3u};
  const float normalPositions[9] = {
    2.0f, 0.0f, 0.0f,
    3.0f, 0.0f, 0.0f,
    2.0f, 1.0f, 0.0f
  };
  const float normals[9] = {
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f,
    0.0f, 0.0f, 1.0f
  };

  ak_test_export_cleanup(outDir);
  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ASSERT(heap != NULL && doc != NULL);
  ak_heap_setdata(heap, doc);

  indexedGeom = ak_test_make_geom_with_positions(heap,
                                                  doc,
                                                  indexedPositions,
                                                  4u);
  normalGeom  = ak_test_make_triangle_geom(heap, doc, normalPositions);
  ASSERT(indexedGeom != NULL && normalGeom != NULL);

  mesh = ak_objGet(indexedGeom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  ASSERT(prim != NULL);
  prim->nPolygons    = 2u;
  prim->indexStride  = 1u;
  prim->indexAccessor = ak_test_make_ubyte_accessor(heap,
                                                     prim,
                                                     indices,
                                                     1u,
                                                     6u);
  ASSERT(prim->indexAccessor != NULL);

  mesh = ak_objGet(normalGeom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  ASSERT(prim != NULL);
  normalInput = ak_heap_calloc(heap, prim, sizeof(*normalInput));
  ASSERT(normalInput != NULL);
  normalInput->semantic = AK_INPUT_NORMAL;
  normalInput->accessor = ak_test_make_float_accessor(heap,
                                                       normalInput,
                                                       normals,
                                                       3u,
                                                       3u);
  ASSERT(normalInput->accessor != NULL);
  normalInput->next = prim->input;
  prim->input       = normalInput;
  prim->inputCount++;

  indexedGeom->next = normalGeom;
  doc->lib.geometries.first = indexedGeom;
  doc->lib.geometries.last  = normalGeom;
  doc->lib.geometries.count = 2u;

  savedFormat    = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  savedNormals   = ak_opt_get(AK_OPT_PLY_EXPORT_NORMALS);
  savedUV        = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  savedColorMode = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, true);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, false);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_NONE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_PLY) == AK_OK);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, savedFormat);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, savedNormals);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, savedUV);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, savedColorMode);

  ASSERT(ak_test_file_contains(plyPath, "element vertex 9"));
  ASSERT(ak_test_file_contains(plyPath, "element face 3"));

  ak_heap_destroy(heap);
  ak_test_export_cleanup(outDir);
  TEST_SUCCESS
}
