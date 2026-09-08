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

#include <ak/assetkit.h>
#include <ak/options.h>
#include <load-spz.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <vector>

#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); std::exit(1); } } while (0)

static AkInput*
find_input(AkMeshPrimitive *prim, AkInputSemantic semantic, unsigned set) {
  AkInput *input;

  for (input = prim->input; input; input = input->next)
    if (input->semantic == semantic && input->set == set)
      return input;
  return nullptr;
}

static AkMeshPrimitive*
primitive(AkDoc *doc) {
  AkScene *scene;
  AkMesh  *mesh;

  CHECK((scene = ak_activeSceneOrFirst(doc)));
  CHECK(ak_sceneRoots(scene));
  CHECK((mesh = ak_meshFromGeometry(doc->lib.geometries.first)));
  CHECK(mesh->primitive && mesh->primitive->gsplat);
  CHECK(mesh->primitive->pos && mesh->primitive->type == AK_PRIMITIVE_POINTS);
  return mesh->primitive;
}

static float
value(AkMeshPrimitive *prim, AkInputSemantic semantic, unsigned set, unsigned row, unsigned c) {
  AkInput    *input;
  AkAccessor *acc;
  float       result;

  CHECK((input = find_input(prim, semantic, set)));
  acc = input->accessor;
  CHECK(acc && row < acc->count && c < acc->componentCount);
  std::memcpy(&result, (char *)acc->buffer->data + acc->byteOffset
                       + row * acc->byteStride + c * sizeof(float), sizeof(result));
  return result;
}

static void
write_ply(const std::filesystem::path &path, unsigned degree, int binary, bool faces = false) {
  static const char *fields[] = {"x", "y", "z", "nx", "ny", "nz", "f_dc_0", "f_dc_1", "f_dc_2",
                                 "opacity", "scale_0", "scale_1", "scale_2", "rot_0", "rot_1", "rot_2", "rot_3"};
  std::ofstream out(path, std::ios::binary);
  std::vector<float> values = {1, 2, 3, 0, 0, 0, -2, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0};
  unsigned rest = degree * (degree + 2) * 3;

  out << "ply\nformat " << (binary == 1 ? "binary_little_endian" : binary == 2 ? "binary_big_endian" : "ascii")
      << " 1.0\nelement vertex 1\n";
  for (const char *name : fields)
    out << "property float " << name << '\n';
  for (unsigned i = 0; i < rest; i++) {
    out << "property float f_rest_" << i << '\n';
    values.push_back((float)(i + 1));
  }
  if (faces)
    out << "element face 1\nproperty list uchar int vertex_indices\n";
  out << "end_header\n";
  for (float f : values) {
    if (!binary) {
      out << f << ' ';
    } else {
      uint32_t bits;
      unsigned char bytes[4];

      std::memcpy(&bits, &f, 4);
      for (unsigned i = 0; i < 4; i++)
        bytes[binary == 2 ? 3 - i : i] = (unsigned char)(bits >> (8 * i));
      out.write((char *)bytes, 4);
    }
  }
  if (!binary) {
    out << '\n';
    if (faces)
      out << "3 0 0 0\n";
  }
}

int main(int argc, char **argv) {
  AkDoc           *doc;
  AkMeshPrimitive *prim;
  AkFileType       type;
  std::filesystem::path dir, path;
  spz::GaussianCloud cloud;
  spz::PackOptions options;

  CHECK(argc >= 2);
  ak_opt_set(AK_OPT_GLTF_GSPLAT_DECODER_PATH, (uintptr_t)argv[1]);
  dir = std::filesystem::temp_directory_path() / ("assetkit-splat-" +
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  CHECK(std::filesystem::create_directory(dir));
  path = dir / "points.spz";
  cloud.numPoints   = 2;
  cloud.antialiased = true;
  cloud.positions  = {1, 2, 3, -1, -2, -3};
  cloud.rotations  = {0, 0, 0, 1, 0.5f, 0.5f, 0.5f, 0.5f};
  cloud.scales     = {0, 1, -1, 0, 0, 0};
  cloud.alphas     = {0, 1};
  cloud.colors     = {-2, 0, 2, 0, 1, -1};
  options.from     = spz::CoordinateSystem::LUF;
  for (unsigned version = 2; version <= 4; version++) {
    for (unsigned degree = 0; degree <= (version == 4 ? 4u : 3u); degree++) {
      cloud.shDegree = (int)degree;
      cloud.sh.resize(2 * degree * (degree + 2) * 3);
      for (size_t i = 0; i < cloud.sh.size(); i++)
        cloud.sh[i] = (float)((int)(i % 11) - 5) / 8.0f;
      options.version = version;
      CHECK(spz::saveSpz(cloud, options, path.string()));
      CHECK(ak_probeFileType(path.c_str(), &type) == AK_OK && type == AK_FILE_TYPE_SPZ);
      CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) == AK_OK);
      prim = primitive(doc);
      CHECK(prim->gsplat->decodedCount == 2 && prim->gsplat->shDegree == degree);
      CHECK(prim->gsplat->antialiased);
      CHECK(std::fabs(value(prim, AK_INPUT_POSITION, 0, 0, 0) - 1) < 0.001f);
      CHECK(std::fabs(value(prim, AK_INPUT_POSITION, 0, 0, 1) - 2) < 0.001f);
      CHECK(std::fabs(value(prim, AK_INPUT_SCALE, 0, 0, 1) - std::exp(1.0f)) < 0.01f);
      CHECK(std::fabs(value(prim, AK_INPUT_OPACITY, 0, 0, 0) - 0.5f) < 0.01f);
      CHECK(std::fabs(value(prim, AK_INPUT_SH, 0, 0, 0) + 2) < 0.1f);
      for (unsigned i = 1; i < (degree + 1) * (degree + 1); i++)
        for (unsigned c = 0; c < 3; c++)
          CHECK(std::fabs(value(prim, AK_INPUT_SH, i, 1, c) -
                cloud.sh[(size_t)degree * (degree + 2) * 3 + (i - 1) * 3 + c]) < 0.15f);
      ak_free(doc);
    }
  }
  for (int binary = 0; binary < 3; binary++) {
    for (unsigned degree = 0; degree <= 4; degree++) {
      path = dir / "points.ply";
      write_ply(path, degree, binary);
      CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) == AK_OK);
      prim = primitive(doc);
      CHECK(prim->gsplat->shDegree == degree);
      CHECK(!find_input(prim, AK_INPUT_NORMAL, 0));
      CHECK(value(prim, AK_INPUT_POSITION, 0, 0, 0) == -1);
      CHECK(value(prim, AK_INPUT_POSITION, 0, 0, 1) == -2);
      CHECK(value(prim, AK_INPUT_POSITION, 0, 0, 2) == 3);
      CHECK(value(prim, AK_INPUT_ROTATION, 0, 0, 3) == 1);
      CHECK(value(prim, AK_INPUT_SCALE, 0, 0, 0) == 1);
      CHECK(value(prim, AK_INPUT_OPACITY, 0, 0, 0) == 0.5f);
      CHECK(value(prim, AK_INPUT_SH, 0, 0, 0) == -2);
      if (degree) {
        CHECK(value(prim, AK_INPUT_SH, 1, 0, 0) == -1);
        CHECK(value(prim, AK_INPUT_SH, 1, 0, 1) == -(float)(degree * (degree + 2) + 1));
      }
      ak_free(doc);
      std::filesystem::resize_file(path, std::filesystem::file_size(path) - (binary ? 2 : 8));
      CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) != AK_OK && !doc);
    }
  }
  /* Required nested SPZ extension, with spec-style placeholder accessors. */
  options.version = 2;
  cloud.shDegree  = 1;
  cloud.sh.resize(18);
  path = dir / "points.spz";
  CHECK(spz::saveSpz(cloud, options, path.string()));
  {
    std::ofstream out(dir / "compressed.gltf");
    size_t size = std::filesystem::file_size(path);

    out << R"({"asset":{"version":"2.0"},"extensionsUsed":["KHR_gaussian_splatting","KHR_gaussian_splatting_compression_spz_2"],"extensionsRequired":["KHR_gaussian_splatting","KHR_gaussian_splatting_compression_spz_2"],"buffers":[{"uri":"points.spz","byteLength":)"
        << size << R"(}],"bufferViews":[{"buffer":0,"byteLength":)" << size
        << R"(}],"accessors":[{"componentType":5126,"count":2,"type":"VEC3","min":[-1,-2,-3],"max":[1,2,3]}],"meshes":[{"primitives":[{"mode":0,"attributes":{"POSITION":0},"extensions":{"KHR_gaussian_splatting":{"kernel":"ellipse","colorSpace":"srgb_rec709_display","extensions":{"KHR_gaussian_splatting_compression_spz_2":{"bufferView":0}}}}}]}],"nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
  }
  path = dir / "compressed.gltf";
  CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) == AK_OK);
  prim = primitive(doc);
  CHECK(prim->inputCount == 8 && prim->gsplat->shDegree == 1);
  CHECK(value(prim, AK_INPUT_POSITION, 0, 0, 0) == 1);
  ak_free(doc);

  /* Uncompressed namespaced inputs, including the SH band/index mapping. */
  {
    std::ofstream out(dir / "plain.bin", std::ios::binary);
    float values[] = {1, 2, 3, 0, 0, 0, 1, 1, 1, 1, 0.5f, -2, 0, 2, 3, 4, 5};
    out.write((char *)values, sizeof(values));
  }
  {
    std::ofstream out(dir / "plain.gltf");
    out << R"({"asset":{"version":"2.0"},"extensionsUsed":["KHR_gaussian_splatting"],"buffers":[{"uri":"plain.bin","byteLength":68}],"bufferViews":[{"buffer":0,"byteLength":68}],"accessors":[)";
    unsigned offsets[] = {0, 12, 28, 40, 44, 56};
    const char *types[] = {"VEC3", "VEC4", "VEC3", "SCALAR", "VEC3", "VEC3"};
    for (unsigned i = 0; i < 6; i++) {
      if (i) out << ',';
      out << "{\"bufferView\":0,\"byteOffset\":" << offsets[i]
          << ",\"componentType\":5126,\"count\":1,\"type\":\"" << types[i] << "\"}";
    }
    out << R"(],"meshes":[{"primitives":[{"mode":0,"attributes":{"POSITION":0,"KHR_gaussian_splatting:ROTATION":1,"KHR_gaussian_splatting:SCALE":2,"KHR_gaussian_splatting:OPACITY":3,"KHR_gaussian_splatting:SH_DEGREE_0_COEF_0":4,"KHR_gaussian_splatting:SH_DEGREE_1_COEF_0":5,"KHR_gaussian_splatting:SH_DEGREE_1_COEF_1":5,"KHR_gaussian_splatting:SH_DEGREE_1_COEF_2":5},"extensions":{"KHR_gaussian_splatting":{"kernel":"ellipse","colorSpace":"srgb_rec709_display"}}}]}],"nodes":[{"mesh":0}],"scenes":[{"nodes":[0]}],"scene":0})";
  }
  path = dir / "plain.gltf";
  CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) == AK_OK);
  prim = primitive(doc);
  CHECK(prim->gsplat->shDegree == 1);
  CHECK(value(prim, AK_INPUT_SH, 0, 0, 0) == -2);
  CHECK(value(prim, AK_INPUT_SH, 3, 0, 1) == 4);
  CHECK(value(prim, AK_INPUT_OPACITY, 0, 0, 0) == 0.5f);
  ak_free(doc);
  path = dir / "mesh.ply";
  write_ply(path, 0, 0, true);
  CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_AUTO) == AK_OK);
  CHECK(!ak_meshFromGeometry(doc->lib.geometries.first)->primitive->gsplat);
  ak_free(doc);
  path = dir / "bad.spz";
  { std::ofstream out(path); out << "invalid spz"; }
  CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_SPZ) != AK_OK && !doc);

  /* A missing decoder must fail without publishing a partial document. */
  ak_opt_set(AK_OPT_GLTF_EXT_DECODER_AUTOLOAD, 0);
  ak_opt_set(AK_OPT_GLTF_GSPLAT_DECODER_PATH, 0);
  path = dir / "points.spz";
  CHECK(ak_load(&doc, path.c_str(), AK_FILE_TYPE_SPZ) != AK_OK && !doc);
  ak_opt_set(AK_OPT_GLTF_GSPLAT_DECODER_PATH, (uintptr_t)argv[1]);

  if (argc > 2) {
    CHECK(ak_load(&doc, argv[2], AK_FILE_TYPE_SPZ) == AK_OK);
    prim = primitive(doc);
    std::printf("sample: %u splats, SH degree %u\n", prim->gsplat->decodedCount, prim->gsplat->shDegree);
    ak_free(doc);
  }
  std::filesystem::remove_all(dir);
  std::puts("SPZ v2/v3/v4 and Gaussian PLY ASCII/LE/BE passed");
}
