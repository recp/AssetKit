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

#include "../test_export_common.h"
#include <cglm/cglm.h>
#include <math.h>

/* Independent real-SH evaluation through associated Legendre recurrence. */
static
void
ak_test_sh_basis(const vec3 direction, double basis[25]) {
  static const double factorial[] = {1, 1, 2, 6, 24, 120, 720, 5040, 40320};
  double phi, z, radial, pmm, previous, current, next, norm;
  int    l, m, k;

  phi    = atan2(direction[1], direction[0]);
  z      = direction[2];
  radial = sqrt(fmax(0.0, 1.0 - z * z));
  pmm    = 1.0;

  for (m = 0; m <= 4; m++) {
    if (m)
      pmm *= -(2 * m - 1) * radial;

    previous = 0.0;
    current  = pmm;

    for (l = m; l <= 4; l++) {
      if (l > m) {
        next     = ((2 * l - 1) * z * current - (l + m - 1) * previous) / (l - m);
        previous = current;
        current  = next;
      }

      norm = sqrt((2 * l + 1) / (4 * GLM_PI)
                  * factorial[l - m] / factorial[l + m]);
      k    = l * l + l;

      if (!m) {
        basis[k] = norm * current;
      } else {
        norm         *= sqrt(2.0);
        basis[k + m]  = norm * current * cos(m * phi);
        basis[k - m]  = norm * current * sin(m * phi);
      }
    }
  }
}

static
float
ak_test_sh_value(uint32_t row, uint32_t set, uint32_t channel) {
  return sinf((float)(row * 75 + set * 3 + channel + 1) * 0.31f) * 0.7f;
}

TEST_IMPL(gltf_gaussian_validation) {
  static const char *metadata[] = {
    "\"kernel\":\"ellipse\",\"colorSpace\":\"srgb_rec709_display\"",
    "\"kernel\":\"other\"", "\"kernel\":1", "\"kernel\":null",
    "\"colorSpace\":\"other\"", "\"colorSpace\":1",
    "\"projection\":\"other\"", "\"sortingMethod\":\"none\"",
    "\"kernel\":\"ellipse\"", "\"colorSpace\":\"lin_rec709_display\""
  };
  AkDoc      *doc;
  FILE       *file;
  char       *dir;
  uint32_t    i, field, invalid;
  AkResult    result;
  char        tmp[] = "/tmp/ak-gsplat-validation-XXXXXX", path[512];

  ASSERT((dir = mkdtemp(tmp)));
  snprintf(path, sizeof(path), "%s/points.gltf", dir);

  for (invalid = 0; invalid < 5; invalid++) {
    for (i = 0; i < sizeof(metadata) / sizeof(*metadata); i++) {
      ASSERT((file = fopen(path, "wb")));
      fputs("{\"asset\":{\"version\":\"2.0\"},\"accessors\":[", file);

      for (field = 0; field < 5; field++)
        fprintf(file, "%s{\"componentType\":5126,\"count\":%u,\"type\":\"%s\"}",
                field ? "," : "", invalid == 1 && field == 1 ? 2 : 1,
                field == 1 ? "VEC4" : field == 3 && invalid != 2 ? "SCALAR" : "VEC3");

      fputs("],\"meshes\":[{\"primitives\":[{\"mode\":0,\"attributes\":{"
            "\"POSITION\":0,\"KHR_gaussian_splatting:ROTATION\":1,"
            "\"KHR_gaussian_splatting:SCALE\":2,\"KHR_gaussian_splatting:OPACITY\":3", file);
      if (invalid != 3)
        fputs(",\"KHR_gaussian_splatting:SH_DEGREE_0_COEF_0\":4", file);
      if (invalid == 4)
        fputs(",\"KHR_gaussian_splatting:SH_DEGREE_1_COEF_0\":4", file);
      fprintf(file, "},\"extensions\":{\"KHR_gaussian_splatting\":{%s}}}]}],"
                    "\"nodes\":[{\"mesh\":0}],\"scenes\":[{\"nodes\":[0]}]}", metadata[i]);
      ASSERT(fclose(file) == 0);

      doc    = NULL;
      result = ak_load(&doc, path, AK_FILE_TYPE_GLTF);

      if (!invalid && (i == 0 || i >= 8)) {
        ASSERT(result == AK_OK && doc);
        ak_free(doc);
      } else {
        ASSERT(result != AK_OK && !doc);
      }
    }
  }

  unlink(path);
  rmdir(dir);
  TEST_SUCCESS
}

static
AkGeometry*
ak_test_splat_geom(AkHeap *heap, AkDoc *doc, uint32_t degree) {
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *input, *last;
  AkAccessor      *acc;
  AkBuffer        *buffer;
  unsigned char   *data;
  size_t           stride, offset;
  uint32_t         row, i, c, components, set, width;
  AkInputSemantic  semantic;
  float            values[86], positions[6] = {1, 2, 3, -4, 5, 6};
  versor           rotation = {0.18f, -0.32f, 0.22f, 0.8f};

  geom = ak_test_make_geom_with_positions(heap, doc, positions, 2);
  mesh = ak_objGet(geom->gdata);
  prim = mesh->primitive;

  prim->type            = AK_PRIMITIVE_POINTS;
  prim->gsplat          = ak_heap_calloc(heap, prim, sizeof(*prim->gsplat));
  prim->gsplat->shDegree = (uint8_t)degree;

  width          = (degree + 1) * (degree + 1);
  stride         = (11 + width * 3) * sizeof(float) + 1;
  buffer         = ak_heap_calloc(heap, prim, sizeof(*buffer));
  buffer->length = stride * 2;
  buffer->data   = ak_heap_calloc(heap, buffer, buffer->length);
  data           = buffer->data;

  glm_quat_normalize(rotation);

  for (row = 0; row < 2; row++) {
    memcpy(values, positions + row * 3, 3 * sizeof(float));
    memcpy(values + 3, rotation, sizeof(rotation));
    values[7]  = 0.3f;
    values[8]  = 1.2f;
    values[9]  = 2.4f;
    values[10] = 0.42f;

    for (i = 0; i < width; i++) {
      for (c = 0; c < 3; c++)
        values[11 + i * 3 + c] = ak_test_sh_value(row, i, c);
    }

    memcpy(data + row * stride + 1, values, (11 + width * 3) * sizeof(float));
  }

  last   = NULL;
  offset = 1;

  for (i = 0; i < 4 + width; i++) {
    semantic   = AK_INPUT_SH;
    components = 3;
    set        = i >= 4 ? i - 4 : 0;

    switch (i) {
      case 0: semantic = AK_INPUT_POSITION; break;
      case 1: semantic = AK_INPUT_ROTATION; components = 4; break;
      case 2: semantic = AK_INPUT_SCALE; break;
      case 3: semantic = AK_INPUT_OPACITY; components = 1; break;
      default: break;
    }

    input = i ? ak_heap_calloc(heap, prim, sizeof(*input)) : prim->pos;
    acc   = ak_heap_calloc(heap, input, sizeof(*acc));

    input->semantic = semantic;
    input->set      = set;
    input->accessor = acc;

    acc->buffer            = buffer;
    acc->byteOffset        = offset;
    acc->byteStride        = stride;
    acc->byteLength        = stride + components * sizeof(float);
    acc->fillByteSize      = components * sizeof(float);
    acc->bytesPerComponent = sizeof(float);
    acc->componentType     = AKT_FLOAT;
    acc->componentSize     = components == 4 ? AK_COMPONENT_SIZE_VEC4
                            : components == 3 ? AK_COMPONENT_SIZE_VEC3
                                              : AK_COMPONENT_SIZE_SCALAR;
    acc->componentCount    = components;
    acc->count             = 2;

    if (last)
      last->next = input;

    last    = input;
    offset += components * sizeof(float);
  }

  return geom;
}

static
AkInput*
ak_test_splat_input(AkMeshPrimitive *prim, AkInputSemantic semantic, uint32_t set) {
  AkInput *input;

  for (input = prim->input; input; input = input->next) {
    if (input->semantic == semantic && input->set == set)
      return input;
  }

  return NULL;
}

static
void
ak_test_splat_read(AkMeshPrimitive *prim, AkInputSemantic semantic,
                   uint32_t set, uint32_t row, float *values) {
  AkAccessor *acc;

  acc = ak_test_splat_input(prim, semantic, set)->accessor;
  memcpy(values, (unsigned char *)acc->buffer->data + acc->byteOffset
                 + row * acc->byteStride, acc->componentCount * sizeof(float));
}

static
void
ak_test_splat_covariance(versor rotation, vec3 scale, mat3 covariance) {
  mat3     axes;
  uint32_t i, j, k;

  glm_quat_mat3(rotation, axes);

  for (i = 0; i < 3; i++) {
    for (j = 0; j < 3; j++) {
      covariance[i][j] = 0;

      for (k = 0; k < 3; k++)
        covariance[i][j] += axes[k][i] * axes[k][j] * scale[k] * scale[k];
    }
  }
}

static
test_status_t
ak_test_splat_directions(AkMeshPrimitive *prim, AkCoordSys *target,
                         uint32_t degree, bool shared) {
  double   oldBasis[25], newBasis[25], before[3], after[3];
  uint32_t row, direction, i, c, width;
  vec3     oldDirection, newDirection, coefficient;

  width = (degree + 1) * (degree + 1);

  for (row = 0; row < 2; row++) {
    for (direction = 0; direction < 19; direction++) {
      oldDirection[0] = cosf((float)direction * 1.17f);
      oldDirection[1] = sinf((float)direction * 0.81f);
      oldDirection[2] = cosf((float)direction * 0.43f);
      glm_vec3_normalize(oldDirection);
      ak_coordCvtVectorTo(AK_YUP, oldDirection, target, newDirection);
      ak_test_sh_basis(oldDirection, oldBasis);
      ak_test_sh_basis(newDirection, newBasis);
      memset(before, 0, sizeof(before));
      memset(after, 0, sizeof(after));

      for (i = 0; i < width; i++) {
        ak_test_splat_read(prim, AK_INPUT_SH, i, row, coefficient);

        for (c = 0; c < 3; c++) {
          before[c] += oldBasis[i] * ak_test_sh_value(row, shared && i < 4 ? 0 : i, c);
          after[c]  += newBasis[i] * coefficient[c];
        }
      }

      for (c = 0; c < 3; c++)
        ASSERT(fabs(before[c] - after[c]) < 0.00002);
    }
  }

  TEST_SUCCESS
}

TEST_IMPL(coord_gaussian_signed_axes) {
  static const int permutations[6][3] = {
    {0, 1, 2}, {0, 2, 1}, {1, 0, 2}, {1, 2, 0}, {2, 0, 1}, {2, 1, 0}
  };
  AkHeap          *heap;
  AkDoc           *doc;
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkAccessor      *originalSH;
  AkBuffer        *originalBuffer;
  AkCoordSys       target;
  test_status_t    status;
  mat3             basis, oldCovariance, expectedCovariance, covariance, temp, transposed;
  versor           oldRotation, rotation;
  vec3             position, expectedPosition, scale, oldScale, unit;
  float            opacity, sign[3];
  uint32_t         p, bits, entry, degree, row, i, j;

  for (p = 0; p < 6; p++) {
    for (bits = 0; bits < 8; bits++) {
      for (i = 0; i < 3; i++)
        sign[i] = bits & (1u << i) ? -1.0f : 1.0f;

      target      = *AK_YUP;
      target.axis = (AkAxisOrientation){
        (AkAxis)((permutations[p][0] + 1) * sign[0]),
        (AkAxis)((permutations[p][1] + 1) * sign[1]),
        (AkAxis)(-(permutations[p][2] + 1) * sign[2])
      };

      for (i = 0; i < 3; i++) {
        glm_vec3_zero(unit);
        unit[i] = 1;
        ak_coordCvtVectorTo(AK_YUP, unit, &target, basis[i]);
      }

      target.rotDirection = glm_mat3_det(basis) < 0 ? AK_AXIS_ROT_DIR_LH : AK_AXIS_ROT_DIR_RH;

      for (entry = 0; entry < 3; entry++) {
        for (degree = 0; degree <= 4; degree++) {
          heap = ak_heap_new(NULL, NULL, NULL);
          doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
          ak_heap_setdata(heap, doc);
          doc->coordSys = AK_YUP;

          geom = ak_test_splat_geom(heap, doc, degree);
          mesh = ak_objGet(geom->gdata);
          prim = mesh->primitive;

          doc->lib.geometries.first = geom;
          originalSH               = ak_test_splat_input(prim, AK_INPUT_SH, degree ? 1 : 0)->accessor;
          originalBuffer           = originalSH->buffer;

          ak_test_splat_read(prim, AK_INPUT_ROTATION, 0, 0, oldRotation);
          ak_test_splat_read(prim, AK_INPUT_SCALE, 0, 0, oldScale);
          ak_test_splat_covariance(oldRotation, oldScale, oldCovariance);
          glm_mat3_transpose_to(basis, transposed);
          glm_mat3_mul(basis, oldCovariance, temp);
          glm_mat3_mul(temp, transposed, expectedCovariance);

          switch (entry) {
            case 0: ak_changeCoordSys(doc, &target); break;
            case 1: ak_changeCoordSysGeom(geom, &target); break;
            case 2: ak_changeCoordSysMesh(mesh, &target); break;
          }

          ASSERT(originalSH == ak_test_splat_input(prim, AK_INPUT_SH, degree ? 1 : 0)->accessor);
          ASSERT(originalBuffer == originalSH->buffer);
          ASSERT(doc->coordSys == (entry ? AK_YUP : &target));

          for (row = 0; row < 2; row++) {
            unit[0] = row ? -4 : 1;
            unit[1] = row ? 5 : 2;
            unit[2] = row ? 6 : 3;
            ak_coordCvtVectorTo(AK_YUP, unit, &target, expectedPosition);
            ak_test_splat_read(prim, AK_INPUT_POSITION, 0, row, position);
            ak_test_splat_read(prim, AK_INPUT_ROTATION, 0, row, rotation);
            ak_test_splat_read(prim, AK_INPUT_SCALE, 0, row, scale);
            ak_test_splat_read(prim, AK_INPUT_OPACITY, 0, row, &opacity);
            ak_test_splat_covariance(rotation, scale, covariance);

            ASSERT(fabsf(opacity - 0.42f) < 1e-6f);

            for (i = 0; i < 3; i++) {
              ASSERT(fabsf(position[i] - expectedPosition[i]) < 1e-6f);
              ASSERT(scale[i] > 0);

              for (j = 0; j < 3; j++)
                ASSERT(fabsf(covariance[i][j] - expectedCovariance[i][j]) < 1e-5f);
            }
          }

          status = ak_test_splat_directions(prim, &target, degree, false);
          ASSERT(status.status == TEST_OK);

          doc->coordSys = &target;
          ak_changeCoordSys(doc, AK_YUP);
          status = ak_test_splat_directions(prim, AK_YUP, degree, false);
          ASSERT(status.status == TEST_OK);

          ak_heap_destroy(heap);
        }
      }
    }
  }

  TEST_SUCCESS
}

TEST_IMPL(coord_gaussian_shared_sh) {
  AkHeap          *heap;
  AkDoc           *doc;
  AkGeometry      *geom, *secondGeom;
  AkMesh          *mesh, *secondMesh;
  AkMeshPrimitive *prim, *second, *alias;
  AkInput         *input, *copy, *last;
  AkAccessor      *dc;
  test_status_t    status;
  uint32_t         i;

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);
  doc->coordSys = AK_YUP;

  geom       = ak_test_splat_geom(heap, doc, 4);
  mesh       = ak_objGet(geom->gdata);
  prim       = mesh->primitive;
  secondGeom = ak_test_splat_geom(heap, doc, 4);
  secondMesh = ak_objGet(secondGeom->gdata);
  second     = secondMesh->primitive;
  dc         = ak_test_splat_input(prim, AK_INPUT_SH, 0)->accessor;
  last       = NULL;

  /* Different coefficient sets and different geometries may use one accessor. */
  for (i = 1; i < 4; i++)
    ak_test_splat_input(prim, AK_INPUT_SH, i)->accessor = dc;

  for (input = prim->input; input; input = input->next) {
    copy  = ak_heap_alloc(heap, second, sizeof(*copy));
    *copy = *input;

    if (last)
      last->next = copy;
    else
      second->input = second->pos = copy;

    last = copy;
  }

  alias                    = ak_heap_alloc(heap, ak_objFrom(mesh), sizeof(*alias));
  *alias                   = *prim;
  prim->next               = alias;
  geom->next               = secondGeom;
  doc->lib.geometries.first = geom;

  ak_changeCoordSys(doc, AK_ZUP);

  ASSERT(ak_test_splat_input(prim, AK_INPUT_SH, 0)->accessor == dc);
  ASSERT(ak_test_splat_input(prim, AK_INPUT_SH, 1)->accessor != dc);
  ASSERT(ak_test_splat_input(second, AK_INPUT_SH, 1)->accessor
         != ak_test_splat_input(prim, AK_INPUT_SH, 1)->accessor);

  status = ak_test_splat_directions(prim, AK_ZUP, 4, true);
  ASSERT(status.status == TEST_OK);
  status = ak_test_splat_directions(second, AK_ZUP, 4, true);
  ASSERT(status.status == TEST_OK);
  status = ak_test_splat_directions(alias, AK_ZUP, 4, true);
  ASSERT(status.status == TEST_OK);

  ak_heap_destroy(heap);
  TEST_SUCCESS
}
