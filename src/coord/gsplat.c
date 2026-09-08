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

#include "common.h"

typedef struct AkSHPolynomial {
  double norm;
  int8_t term[6][4]; /* x/y/z powers and integer multiplier */
} AkSHPolynomial;

/* Homogeneous real SH polynomials, ordered by l*l + (m+l).
   On the unit sphere, x*x + y*y + z*z = 1. */
static const AkSHPolynomial ak_sh_basis[25] = {
  {0.28209479177387814, {{0, 0, 0, 1}}},
  {0.4886025119029199,  {{0, 1, 0, -1}}},
  {0.4886025119029199,  {{0, 0, 1, 1}}},
  {0.4886025119029199,  {{1, 0, 0, -1}}},
  {1.0925484305920792,  {{1, 1, 0, 1}}},
  {1.0925484305920792,  {{0, 1, 1, -1}}},
  {0.31539156525252005, {{0, 0, 2, 2}, {2, 0, 0, -1}, {0, 2, 0, -1}}},
  {1.0925484305920792,  {{1, 0, 1, -1}}},
  {0.5462742152960396,  {{2, 0, 0, 1}, {0, 2, 0, -1}}},
  {0.5900435899266435,  {{2, 1, 0, -3}, {0, 3, 0, 1}}},
  {2.890611442640554,   {{1, 1, 1, 1}}},
  {0.4570457994644658,  {{0, 1, 2, -4}, {2, 1, 0, 1}, {0, 3, 0, 1}}},
  {0.3731763325901154,  {{0, 0, 3, 2}, {2, 0, 1, -3}, {0, 2, 1, -3}}},
  {0.4570457994644658,  {{1, 0, 2, -4}, {3, 0, 0, 1}, {1, 2, 0, 1}}},
  {1.445305721320277,   {{2, 0, 1, 1}, {0, 2, 1, -1}}},
  {0.5900435899266435,  {{3, 0, 0, -1}, {1, 2, 0, 3}}},
  {2.5033429417967046,  {{3, 1, 0, 1}, {1, 3, 0, -1}}},
  {1.7701307697799304,  {{2, 1, 1, -3}, {0, 3, 1, 1}}},
  {0.9461746957575601,  {{1, 1, 2, 6}, {3, 1, 0, -1}, {1, 3, 0, -1}}},
  {0.6690465435572892,  {{0, 1, 3, -4}, {2, 1, 1, 3}, {0, 3, 1, 3}}},
  {0.10578554691520431, {{0, 0, 4, 8}, {4, 0, 0, 3}, {0, 4, 0, 3},
                        {2, 2, 0, 6}, {2, 0, 2, -24}, {0, 2, 2, -24}}},
  {0.6690465435572892,  {{1, 0, 3, -4}, {3, 0, 1, 3}, {1, 2, 1, 3}}},
  {0.47308734787878004, {{2, 0, 2, 6}, {0, 2, 2, -6}, {4, 0, 0, -1}, {0, 4, 0, 1}}},
  {1.7701307697799304,  {{3, 0, 1, -1}, {1, 2, 1, 3}}},
  {0.6258357354491761,  {{4, 0, 0, 1}, {2, 2, 0, -6}, {0, 4, 0, 1}}}
};

AK_HIDE
void
ak_coordSHBasis(AkSHBasisChange *change,
                AkCoordSys      *oldCoordSys,
                AkCoordSys      *newCoordSys) {
  static const double moment[] = {1.0, 1.0, 3.0, 15.0, 105.0};
  static const double denominator[] = {1.0, 3.0, 15.0, 105.0, 945.0};
  const AkSHPolynomial *src, *dst;
  double                weight, integral;
  uint32_t              l, start, end, i, j, a, b, c, slot;
  int32_t               axis[3], sign[3], powers[3], sum[3], multiplier;
  vec3                  unit, converted;

  memset(change, 0, sizeof(*change));

  for (i = 0; i < 3; i++) {
    glm_vec3_zero(unit);
    unit[i] = 1.0f;
    ak_coordCvtVectorTo(oldCoordSys, unit, newCoordSys, converted);

    for (j = 0; j < 3; j++) {
      if (converted[j] == 0.0f)
        continue;

      axis[i] = (int32_t)j;
      sign[i] = converted[j] < 0.0f ? -1 : 1;
    }
  }

  /* R[j,i] = integral Y[j](d) * Y[i](C^-1 d) over the unit sphere.
     A signed axis permutation changes only polynomial powers and signs.
     Integrate monomials exactly; no directional sampling or matrix solve. */
  for (l = 1; l <= 4; l++) {
    start = l * l;
    end   = (l + 1) * (l + 1);

    for (j = start; j < end; j++) {
      dst = &ak_sh_basis[j];

      for (i = start; i < end; i++) {
        src    = &ak_sh_basis[i];
        weight = 0.0;

        for (a = 0; a < 6 && src->term[a][3]; a++) {
          multiplier = src->term[a][3];

          for (c = 0; c < 3; c++) {
            powers[axis[c]] = src->term[a][c];
            if (src->term[a][c] & 1)
              multiplier *= sign[c];
          }

          for (b = 0; b < 6 && dst->term[b][3]; b++) {
            for (c = 0; c < 3; c++)
              sum[c] = powers[c] + dst->term[b][c];

            if ((sum[0] | sum[1] | sum[2]) & 1)
              continue;

            integral = 12.56637061435917295385 * moment[sum[0] / 2]
                       * moment[sum[1] / 2] * moment[sum[2] / 2] / denominator[l];
            weight  += multiplier * dst->term[b][3] * integral;
          }
        }

        weight *= src->norm * dst->norm;
        if (fabs(weight) < 1e-7)
          continue;

        slot                   = change->count[j]++;
        change->column[j][slot] = (uint8_t)(i - start);
        change->weight[j][slot] = (float)weight;
      }
    }
  }
}

AK_HIDE
bool
ak_coordSHBand(AkAccessor **accessors, uint32_t degree, const AkSHBasisChange *change) {
  AkAccessor    *acc;
  unsigned char *data[9];
  size_t         stride[9], last, rowBytes;
  uint32_t       count, width, start, i, j, c, row, column;
  float          values[9][3], result[3], weight;

  if (degree < 1 || degree > 4 || !accessors[0])
    return false;

  start = degree * degree;
  width = 2 * degree + 1;
  count = accessors[0]->count;

  for (i = 0; i < width; i++) {
    if (!(acc = accessors[i]) || acc->count != count || acc->componentCount != 3)
      return false;

    if (acc->componentType != AKT_FLOAT || acc->normalized
        || acc->bytesPerComponent != sizeof(float))
      ak_accessorMakeFloat(acc);

    if (acc->componentType != AKT_FLOAT || acc->normalized
        || acc->bytesPerComponent != sizeof(float) || !acc->buffer || !acc->buffer->data)
      return false;

    rowBytes  = 3 * sizeof(float);
    stride[i] = acc->byteStride ? acc->byteStride : rowBytes;
    if (stride[i] < rowBytes || acc->byteOffset > acc->buffer->length)
      return false;

    if (count) {
      if ((size_t)(count - 1) > (SIZE_MAX - acc->byteOffset) / stride[i])
        return false;

      last = acc->byteOffset + (size_t)(count - 1) * stride[i];
      if (last > acc->buffer->length || rowBytes > acc->buffer->length - last)
        return false;
    }

    if (!ak_coordBufferWritable(acc->buffer))
      return false;

    data[i] = (unsigned char *)acc->buffer->data + acc->byteOffset;
  }

  for (row = 0; row < count; row++) {
    for (i = 0; i < width; i++)
      memcpy(values[i], data[i] + (size_t)row * stride[i], sizeof(values[i]));

    for (i = 0; i < width; i++) {
      glm_vec3_zero(result);

      for (j = 0; j < change->count[start + i]; j++) {
        column = change->column[start + i][j];
        weight = change->weight[start + i][j];

        for (c = 0; c < 3; c++)
          result[c] += values[column][c] * weight;
      }

      memcpy(data[i] + (size_t)row * stride[i], result, sizeof(result));
    }
  }

  for (i = 0; i < width; i++) {
    accessors[i]->min = NULL;
    accessors[i]->max = NULL;
  }

  return true;
}
