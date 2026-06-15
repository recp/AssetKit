/*
 * Shared helpers for the OpenGL samples.
 *
 * These helpers deliberately stay small and explicit. They show the runtime
 * path an engine usually needs: pick a scene primitive, request one index
 * stream, resolve material/light data, and upload plain GPU buffers.
 */

#ifndef assetkit_gl_sample_common_h
#define assetkit_gl_sample_common_h

#if defined(__APPLE__)
#  ifndef GL_SILENCE_DEPRECATION
#    define GL_SILENCE_DEPRECATION
#  endif
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/struct.h>

#include "../sample_common.h"

#include <float.h>
#include <math.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(ASSETKIT_SAMPLE_GL_HAS_LIBPNG)
#  include <png.h>
#endif

#ifndef ASSETKIT_SAMPLE_GL_ROOT
#  define ASSETKIT_SAMPLE_GL_ROOT "."
#endif

#define SAMPLE_GL_MAX_LIGHTS 16u
#define SAMPLE_GL_SHADER_PATH_CAP 4096u

typedef struct SampleGLVertex {
  float position[3];
  float normal[3];
  float uv[2];
} SampleGLVertex;

typedef struct SampleGLMesh {
  SampleGLVertex *vertices;
  uint32_t       *indices;
  size_t          vertex_count;
  size_t          index_count;
  float           base_color[4];
  AkImage        *base_color_image;
  vec3            world_center;
  float           world_radius;
  int             has_uv;
} SampleGLMesh;

typedef struct SampleGLLight {
  int   type;
  vec3  position;
  vec3  direction;
  vec3  color;
  float intensity;
  float range;
  float inner_cos;
  float outer_cos;
} SampleGLLight;

static int
sample_gl_join_path(char *out, size_t out_cap, const char *a, const char *b) {
  size_t a_len;
  int written;

  if (!out || !out_cap || !a || !b)
    return 0;

  a_len = strlen(a);
  written = snprintf(out,
                     out_cap,
                     "%s%s%s",
                     a,
                     a_len && (a[a_len - 1u] == '/' || a[a_len - 1u] == '\\') ? "" : "/",
                     b);
  return written > 0 && (size_t)written < out_cap;
}

static int
sample_gl_join_path3(char *out,
                     size_t out_cap,
                     const char *a,
                     const char *b,
                     const char *c) {
  char tmp[SAMPLE_GL_SHADER_PATH_CAP];

  return sample_gl_join_path(tmp, sizeof(tmp), a, b)
         && sample_gl_join_path(out, out_cap, tmp, c);
}

static int
sample_gl_read_text_file(const char *path, char **out) {
  FILE *file;
  long size;
  char *data;
  size_t got;

  file = fopen(path, "rb");
  if (!file)
    return 0;

  if (fseek(file, 0, SEEK_END) != 0) {
    fclose(file);
    return 0;
  }

  size = ftell(file);
  if (size < 0 || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return 0;
  }

  data = malloc((size_t)size + 1u);
  if (!data) {
    fclose(file);
    return 0;
  }

  got = fread(data, 1u, (size_t)size, file);
  fclose(file);
  if (got != (size_t)size) {
    free(data);
    return 0;
  }

  data[got] = '\0';
  *out = data;
  return 1;
}

static int
sample_gl_read_shader(const char *sample_dir,
                      const char *name,
                      char **out,
                      char *path,
                      size_t path_cap) {
  const char *env_root;
  const char *roots[3];
  size_t count;
  size_t i;

  env_root = getenv("ASSETKIT_SAMPLE_GL_ROOT");
  count = 0;
  if (env_root && env_root[0])
    roots[count++] = env_root;
  roots[count++] = ASSETKIT_SAMPLE_GL_ROOT;
  roots[count++] = "samples/gl";

  for (i = 0; i < count; i++) {
    if (!sample_gl_join_path3(path, path_cap, roots[i], sample_dir, name))
      continue;
    if (sample_gl_read_text_file(path, out))
      return 1;
  }

  fprintf(stderr,
          "could not read shader %s/%s; set ASSETKIT_SAMPLE_GL_ROOT if running outside the source tree\n",
          sample_dir,
          name);
  return 0;
}

static GLuint
sample_gl_compile_shader(GLenum type, const char *path, const char *source) {
  GLuint shader;
  GLint ok;

  shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[2048];

    glGetShaderInfoLog(shader, (GLsizei)sizeof(log), NULL, log);
    fprintf(stderr, "shader compile failed (%s): %s\n", path, log);
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

static GLuint
sample_gl_make_program(const char *sample_dir,
                       const char *vert_name,
                       const char *frag_name) {
  char *vs;
  char *fs;
  char vs_path[SAMPLE_GL_SHADER_PATH_CAP];
  char fs_path[SAMPLE_GL_SHADER_PATH_CAP];
  GLuint vert;
  GLuint frag;
  GLuint program;
  GLint ok;

  vs = NULL;
  fs = NULL;
  if (!sample_gl_read_shader(sample_dir, vert_name, &vs, vs_path, sizeof(vs_path)) ||
      !sample_gl_read_shader(sample_dir, frag_name, &fs, fs_path, sizeof(fs_path))) {
    free(vs);
    free(fs);
    return 0;
  }

  vert = sample_gl_compile_shader(GL_VERTEX_SHADER, vs_path, vs);
  frag = sample_gl_compile_shader(GL_FRAGMENT_SHADER, fs_path, fs);
  free(vs);
  free(fs);
  if (!vert || !frag) {
    if (vert)
      glDeleteShader(vert);
    if (frag)
      glDeleteShader(frag);
    return 0;
  }

  program = glCreateProgram();
  glAttachShader(program, vert);
  glAttachShader(program, frag);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &ok);
  glDeleteShader(vert);
  glDeleteShader(frag);

  if (!ok) {
    char log[2048];

    glGetProgramInfoLog(program, (GLsizei)sizeof(log), NULL, log);
    fprintf(stderr, "program link failed: %s\n", log);
    glDeleteProgram(program);
    return 0;
  }

  return program;
}

static int
sample_gl_read_index(AkAccessor *acc, uint32_t index, uint32_t *out) {
  const unsigned char *src;
  size_t stride;
  size_t offset;
  size_t bytes;

  if (!acc || !out || !acc->buffer || !acc->buffer->data || index >= acc->count)
    return 0;

  bytes = acc->bytesPerComponent;
  stride = acc->byteStride ? acc->byteStride : bytes;
  if (!bytes || stride < bytes)
    return 0;

  offset = acc->byteOffset + (size_t)index * stride;
  if (offset > acc->buffer->length || bytes > acc->buffer->length - offset)
    return 0;

  src = (const unsigned char *)acc->buffer->data + offset;
  switch (acc->componentType) {
    case AKT_UBYTE:
      *out = src[0];
      return 1;
    case AKT_USHORT: {
      uint16_t value;
      memcpy(&value, src, sizeof(value));
      *out = value;
      return 1;
    }
    case AKT_UINT: {
      uint32_t value;
      memcpy(&value, src, sizeof(value));
      *out = value;
      return 1;
    }
    default:
      return 0;
  }
}

static AkInput *
sample_gl_input(AkMeshPrimitive *prim, AkInputSemantic semantic, uint32_t set) {
  AkInput *input;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    if (input->semantic != semantic)
      continue;
    if (set == UINT32_MAX || input->set == set || input->index == set)
      return input;
  }

  return NULL;
}

static float *
sample_gl_accessor_floats(AkAccessor *acc, size_t *out_count, uint32_t *out_comps) {
  size_t float_count;
  float *values;

  if (out_count)
    *out_count = 0;
  if (out_comps)
    *out_comps = 0;

  if (!acc || !acc->count || !acc->componentCount)
    return NULL;
  if (acc->count > SIZE_MAX / acc->componentCount)
    return NULL;

  float_count = (size_t)acc->count * acc->componentCount;
  values = malloc(float_count * sizeof(*values));
  if (!values)
    return NULL;

  if (ak_accessorAsFloat(acc, values, float_count) != float_count) {
    free(values);
    return NULL;
  }

  if (out_count)
    *out_count = acc->count;
  if (out_comps)
    *out_comps = acc->componentCount;
  return values;
}

static void
sample_gl_input_color(const AkMaterialInput *input,
                      const float fallback[4],
                      float out[4]) {
  out[0] = fallback[0];
  out[1] = fallback[1];
  out[2] = fallback[2];
  out[3] = fallback[3];

  if (!input)
    return;

  switch (input->valueType) {
    case AK_MATERIAL_VALUE_COLOR:
      out[0] = input->color.rgba.R;
      out[1] = input->color.rgba.G;
      out[2] = input->color.rgba.B;
      out[3] = input->color.rgba.A;
      break;
    case AK_MATERIAL_VALUE_FLOAT:
      out[0] = input->value[0];
      out[1] = input->value[0];
      out[2] = input->value[0];
      out[3] = fallback[3];
      break;
    case AK_MATERIAL_VALUE_FLOAT3:
      out[0] = input->value[0];
      out[1] = input->value[1];
      out[2] = input->value[2];
      out[3] = fallback[3];
      break;
    case AK_MATERIAL_VALUE_FLOAT4:
      out[0] = input->value[0];
      out[1] = input->value[1];
      out[2] = input->value[2];
      out[3] = input->value[3];
      break;
    default:
      break;
  }
}

static const AkMaterialInput *
sample_gl_classic_diffuse(const AkMaterialSurface *surface) {
  const AkMaterialFeature *feature;

  for (feature = surface ? surface->features : NULL; feature; feature = feature->next) {
    if (feature->type == AK_MATERIAL_FEATURE_CLASSIC)
      return ((const AkMaterialClassicFeature *)feature)->diffuse;
  }

  return NULL;
}

static void
sample_gl_resolve_material(AkMeshPrimitive *prim,
                           AkInstanceGeometry *inst,
                           float out_color[4],
                           AkImage **out_image) {
  static const float fallback[4] = {0.78f, 0.78f, 0.78f, 1.0f};
  AkResolvedMaterial resolved;
  AkMaterialProperty *property;
  AkMaterialSurface *surface;
  const AkMaterialInput *input;
  const AkTextureRef *texref;

  out_color[0] = fallback[0];
  out_color[1] = fallback[1];
  out_color[2] = fallback[2];
  out_color[3] = fallback[3];
  if (out_image)
    *out_image = NULL;

  memset(&resolved, 0, sizeof(resolved));
  if (!ak_materialResolve(prim, inst, UINT32_MAX, &resolved))
    return;

  surface = resolved.surface;
  if (!surface && resolved.material)
    surface = resolved.material->surface;

  input = surface ? surface->baseColor : NULL;
  property = ak_resolvedMaterialProperty(&resolved);
  if (property && property->baseColor)
    input = property->baseColor;
  if (!input)
    input = sample_gl_classic_diffuse(surface);

  sample_gl_input_color(input, fallback, out_color);
  out_color[3] *= ak_materialOpacityFactor(surface);

  texref = ak_materialInputTexture(input);
  if (out_image && texref && texref->texture)
    *out_image = texref->texture->image;
}

static void
sample_gl_generate_normals(SampleGLMesh *mesh) {
  size_t tri_count;
  size_t i;

  if (!mesh || !mesh->vertices || mesh->vertex_count < 3u)
    return;

  for (i = 0; i < mesh->vertex_count; i++)
    glm_vec3_zero(mesh->vertices[i].normal);

  tri_count = mesh->index_count ? mesh->index_count / 3u : mesh->vertex_count / 3u;
  for (i = 0; i < tri_count; i++) {
    uint32_t ia;
    uint32_t ib;
    uint32_t ic;
    vec3 a;
    vec3 b;
    vec3 c;
    vec3 ab;
    vec3 ac;
    vec3 n;

    ia = mesh->index_count ? mesh->indices[i * 3u + 0u] : (uint32_t)(i * 3u + 0u);
    ib = mesh->index_count ? mesh->indices[i * 3u + 1u] : (uint32_t)(i * 3u + 1u);
    ic = mesh->index_count ? mesh->indices[i * 3u + 2u] : (uint32_t)(i * 3u + 2u);
    if (ia >= mesh->vertex_count || ib >= mesh->vertex_count || ic >= mesh->vertex_count)
      continue;

    glm_vec3_copy(mesh->vertices[ia].position, a);
    glm_vec3_copy(mesh->vertices[ib].position, b);
    glm_vec3_copy(mesh->vertices[ic].position, c);
    glm_vec3_sub(b, a, ab);
    glm_vec3_sub(c, a, ac);
    glm_vec3_cross(ab, ac, n);
    if (glm_vec3_norm2(n) <= 1.0e-12f)
      continue;
    glm_vec3_normalize(n);
    glm_vec3_add(mesh->vertices[ia].normal, n, mesh->vertices[ia].normal);
    glm_vec3_add(mesh->vertices[ib].normal, n, mesh->vertices[ib].normal);
    glm_vec3_add(mesh->vertices[ic].normal, n, mesh->vertices[ic].normal);
  }

  for (i = 0; i < mesh->vertex_count; i++) {
    if (glm_vec3_norm2(mesh->vertices[i].normal) <= 1.0e-12f) {
      mesh->vertices[i].normal[0] = 0.0f;
      mesh->vertices[i].normal[1] = 0.0f;
      mesh->vertices[i].normal[2] = 1.0f;
    } else {
      glm_vec3_normalize(mesh->vertices[i].normal);
    }
  }
}

static int
sample_gl_copy_primitive(AkMeshPrimitive *prim,
                         AkInstanceGeometry *inst,
                         mat4 world,
                         SampleGLMesh *out) {
  AkAccessor *pos_acc;
  AkAccessor *idx_acc;
  AkInput *normal_input;
  AkInput *uv_input;
  float *positions;
  float *normals;
  float *uvs;
  size_t pos_count;
  size_t normal_count;
  size_t uv_count;
  uint32_t pos_comps;
  uint32_t normal_comps;
  uint32_t uv_comps;
  vec3 minv;
  vec3 maxv;
  mat4 normal_matrix;
  int have_normal_matrix;
  size_t i;

  memset(out, 0, sizeof(*out));
  out->base_color[0] = 0.78f;
  out->base_color[1] = 0.78f;
  out->base_color[2] = 0.78f;
  out->base_color[3] = 1.0f;

  if (!prim || prim->type != AK_PRIMITIVE_TRIANGLES || !prim->pos)
    return 0;
  if (((AkTriangles *)prim)->mode != 0 && ((AkTriangles *)prim)->mode != AK_TRIANGLES)
    return 0;

  /*
   * GPU APIs use a single index per vertex. AssetKit can import that way by
   * option; this explicit call keeps the sample correct for tuple-indexed DAE
   * and OBJ input as well.
   */
  if (ak_meshPrimitiveEnsureSingleIndex(prim) != AK_OK)
    return 0;

  pos_acc = prim->pos->accessor;
  positions = sample_gl_accessor_floats(pos_acc, &pos_count, &pos_comps);
  if (!positions || !pos_count || pos_comps < 3u) {
    free(positions);
    return 0;
  }

  out->vertices = calloc(pos_count, sizeof(*out->vertices));
  if (!out->vertices) {
    free(positions);
    return 0;
  }
  out->vertex_count = pos_count;

  glm_vec3_fill(minv, FLT_MAX);
  glm_vec3_fill(maxv, -FLT_MAX);
  for (i = 0; i < pos_count; i++) {
    vec3 p;
    vec3 tp;
    uint32_t c;

    p[0] = positions[i * pos_comps + 0u];
    p[1] = positions[i * pos_comps + 1u];
    p[2] = positions[i * pos_comps + 2u];
    glm_mat4_mulv3(world, p, 1.0f, tp);
    glm_vec3_copy(tp, out->vertices[i].position);

    for (c = 0; c < 3u; c++) {
      if (tp[c] < minv[c])
        minv[c] = tp[c];
      if (tp[c] > maxv[c])
        maxv[c] = tp[c];
    }
  }
  free(positions);

  out->world_center[0] = (minv[0] + maxv[0]) * 0.5f;
  out->world_center[1] = (minv[1] + maxv[1]) * 0.5f;
  out->world_center[2] = (minv[2] + maxv[2]) * 0.5f;
  out->world_radius = 0.0f;
  for (i = 0; i < out->vertex_count; i++) {
    float dist = glm_vec3_distance(out->vertices[i].position, out->world_center);
    if (dist > out->world_radius)
      out->world_radius = dist;
  }
  if (out->world_radius <= 0.0f || !isfinite(out->world_radius))
    out->world_radius = 1.0f;

  for (i = 0; i < out->vertex_count; i++) {
    glm_vec3_sub(out->vertices[i].position, out->world_center, out->vertices[i].position);
    glm_vec3_scale(out->vertices[i].position,
                   1.0f / out->world_radius,
                   out->vertices[i].position);
  }

  idx_acc = ak_meshPrimitiveIndexAccessor(prim);
  out->index_count = idx_acc ? ak_meshPrimitiveIndexCount(prim) : 0u;
  if (out->index_count) {
    out->indices = malloc(out->index_count * sizeof(*out->indices));
    if (!out->indices)
      return 0;
    for (i = 0; i < out->index_count; i++) {
      if (!sample_gl_read_index(idx_acc, (uint32_t)i, &out->indices[i]))
        return 0;
    }
  }

  have_normal_matrix = 1;
  glm_mat4_inv(world, normal_matrix);
  glm_mat4_transpose(normal_matrix);
  normal_input = sample_gl_input(prim, AK_INPUT_NORMAL, UINT32_MAX);
  normals = normal_input
            ? sample_gl_accessor_floats(normal_input->accessor, &normal_count, &normal_comps)
            : NULL;
  if (normals && normal_count >= out->vertex_count && normal_comps >= 3u) {
    for (i = 0; i < out->vertex_count; i++) {
      vec3 n;
      vec3 tn;

      n[0] = normals[i * normal_comps + 0u];
      n[1] = normals[i * normal_comps + 1u];
      n[2] = normals[i * normal_comps + 2u];
      if (have_normal_matrix)
        glm_mat4_mulv3(normal_matrix, n, 0.0f, tn);
      else
        glm_vec3_copy(n, tn);
      if (glm_vec3_norm2(tn) <= 1.0e-12f)
        tn[2] = 1.0f;
      glm_vec3_normalize_to(tn, out->vertices[i].normal);
    }
  } else {
    sample_gl_generate_normals(out);
  }
  free(normals);

  uv_input = sample_gl_input(prim, AK_INPUT_TEXCOORD, 0u);
  if (!uv_input)
    uv_input = sample_gl_input(prim, AK_INPUT_UV, 0u);
  uvs = uv_input ? sample_gl_accessor_floats(uv_input->accessor, &uv_count, &uv_comps) : NULL;
  if (uvs && uv_count >= out->vertex_count && uv_comps >= 2u) {
    out->has_uv = 1;
    for (i = 0; i < out->vertex_count; i++) {
      out->vertices[i].uv[0] = uvs[i * uv_comps + 0u];
      out->vertices[i].uv[1] = uvs[i * uv_comps + 1u];
    }
  }
  free(uvs);

  sample_gl_resolve_material(prim, inst, out->base_color, &out->base_color_image);
  return 1;
}

static int
sample_gl_find_first_mesh(AkNode *node,
                          mat4 parent,
                          unsigned depth,
                          SampleGLMesh *out,
                          const char **out_node_name,
                          const char **out_geom_name) {
  AkNode *child;
  AkInstanceGeometry *inst;
  AkInstanceNode *node_ref;

  if (depth > 32u)
    return 0;

  for (; node; node = node->next) {
    mat4 local;
    mat4 world;

    ak_transformCombine(node->transform, local[0]);
    glm_mat4_mul(parent, local, world);

    for (inst = node->geometry; inst; inst = (AkInstanceGeometry *)inst->base.next) {
      AkGeometry *geom;
      AkMesh *mesh;
      AkMeshPrimitive *prim;

      geom = ak_instanceObject(&inst->base);
      mesh = sample_mesh_from_geometry(geom);
      for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next) {
        if (sample_gl_copy_primitive(prim, inst, world, out)) {
          if (out_node_name)
            *out_node_name = node->name;
          if (out_geom_name)
            *out_geom_name = geom ? geom->name : NULL;
          return 1;
        }
      }
    }

    for (child = node->chld; child; child = child->next) {
      if (sample_gl_find_first_mesh(child, world, depth + 1u, out, out_node_name, out_geom_name))
        return 1;
    }

    for (node_ref = node->node; node_ref; node_ref = node_ref->next) {
      AkNode *target = ak_instanceNodeTarget(node_ref);
      if (target && sample_gl_find_first_mesh(target,
                                              world,
                                              depth + 1u,
                                              out,
                                              out_node_name,
                                              out_geom_name))
        return 1;
    }
  }

  return 0;
}

static void
sample_gl_free_mesh(SampleGLMesh *mesh) {
  if (!mesh)
    return;
  free(mesh->vertices);
  free(mesh->indices);
  memset(mesh, 0, sizeof(*mesh));
}

static void
sample_gl_normalize_world_point(const SampleGLMesh *mesh, vec3 p) {
  vec3 center;

  center[0] = mesh->world_center[0];
  center[1] = mesh->world_center[1];
  center[2] = mesh->world_center[2];
  glm_vec3_sub(p, center, p);
  glm_vec3_scale(p, 1.0f / mesh->world_radius, p);
}

static inline void
sample_gl_collect_lights(AkDoc *doc,
                         AkNode *node,
                         mat4 parent,
                         const SampleGLMesh *mesh,
                         unsigned depth,
                         SampleGLLight *out,
                         size_t cap,
                         size_t *count) {
  AkNode *child;
  AkInstanceBase *inst;
  AkInstanceNode *node_ref;

  if (!out || !count || *count >= cap || depth > 32u)
    return;

  for (; node; node = node->next) {
    mat4 local;
    mat4 world;

    ak_transformCombine(node->transform, local[0]);
    glm_mat4_mul(parent, local, world);

    for (inst = node->light; inst && *count < cap; inst = inst->next) {
      AkLight *light;
      AkResolvedLight resolved;
      SampleGLLight *dst;
      vec3 origin = {0.0f, 0.0f, 0.0f};
      vec3 dir;

      light = ak_instanceObject(inst);
      if (!ak_lightResolve(doc, light, AK_LIGHT_RESOLVE_PREVIEW, &resolved))
        continue;

      dst = &out[*count];
      memset(dst, 0, sizeof(*dst));
      dst->type = (int)resolved.type;
      dst->color[0] = resolved.color.rgba.R;
      dst->color[1] = resolved.color.rgba.G;
      dst->color[2] = resolved.color.rgba.B;
      dst->intensity = resolved.intensity;
      dst->range = resolved.range > 0.0f ? resolved.range / mesh->world_radius : 0.0f;
      dst->inner_cos = cosf(resolved.innerConeAngle);
      dst->outer_cos = cosf(resolved.outerConeAngle);

      glm_mat4_mulv3(world, origin, 1.0f, dst->position);
      sample_gl_normalize_world_point(mesh, dst->position);

      dir[0] = resolved.direction[0];
      dir[1] = resolved.direction[1];
      dir[2] = resolved.direction[2];
      if (glm_vec3_norm2(dir) <= 1.0e-12f) {
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = -1.0f;
      }
      glm_mat4_mulv3(world, dir, 0.0f, dst->direction);
      if (glm_vec3_norm2(dst->direction) <= 1.0e-12f)
        glm_vec3_copy(dir, dst->direction);
      glm_vec3_normalize(dst->direction);
      (*count)++;
    }

    for (child = node->chld; child && *count < cap; child = child->next)
      sample_gl_collect_lights(doc, child, world, mesh, depth + 1u, out, cap, count);

    for (node_ref = node->node; node_ref && *count < cap; node_ref = node_ref->next) {
      AkNode *target = ak_instanceNodeTarget(node_ref);
      if (target)
        sample_gl_collect_lights(doc, target, world, mesh, depth + 1u, out, cap, count);
    }
  }
}

static int
sample_gl_init_window(GLFWwindow **out_window, const char *title) {
  GLFWwindow *window;

  *out_window = NULL;
  if (!glfwInit()) {
    fprintf(stderr, "glfwInit failed\n");
    return 0;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif

  window = glfwCreateWindow(960, 640, title, NULL, NULL);
  if (!window) {
    fprintf(stderr, "could not create GLFW window\n");
    glfwTerminate();
    return 0;
  }

  glfwMakeContextCurrent(window);
  glfwSwapInterval(1);
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    fprintf(stderr, "glewInit failed\n");
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
  }

  *out_window = window;
  return 1;
}

#if defined(ASSETKIT_SAMPLE_GL_HAS_LIBPNG)
static AkImageData *
sample_gl_png_finish(AkImage *image, png_image *png, bool flip_vertically) {
  AkImageData *out;
  unsigned char *pixels;
  png_alloc_size_t stride;
  size_t bytes;

  png->format = PNG_FORMAT_RGBA;
  stride = PNG_IMAGE_ROW_STRIDE(*png);
  bytes = PNG_IMAGE_SIZE(*png);

  out = ak_calloc(image, sizeof(*out));
  if (!out) {
    png_image_free(png);
    return NULL;
  }

  pixels = ak_malloc(out, bytes);
  if (!pixels) {
    png_image_free(png);
    ak_free(out);
    return NULL;
  }

  if (!png_image_finish_read(png, NULL, pixels, (png_int_32)stride, NULL)) {
    ak_free(out);
    return NULL;
  }

  if (flip_vertically && png->height > 1u) {
    unsigned char *tmp = malloc((size_t)stride);
    uint32_t y;

    if (tmp) {
      for (y = 0; y < png->height / 2u; y++) {
        unsigned char *a = pixels + (size_t)y * stride;
        unsigned char *b = pixels + (size_t)(png->height - 1u - y) * stride;
        memcpy(tmp, a, (size_t)stride);
        memcpy(a, b, (size_t)stride);
        memcpy(b, tmp, (size_t)stride);
      }
      free(tmp);
    }
  }

  out->data = pixels;
  out->width = png->width;
  out->height = png->height;
  out->comp = 4;
  return out;
}

static AkImageData *
sample_gl_png_load_from_file(AkHeap *heap,
                             AkImage *image,
                             const char *path,
                             bool flip_vertically) {
  png_image png;

  (void)heap;
  memset(&png, 0, sizeof(png));
  png.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_file(&png, path))
    return NULL;
  return sample_gl_png_finish(image, &png, flip_vertically);
}

static AkImageData *
sample_gl_png_load_from_memory(AkHeap *heap,
                               AkImage *image,
                               AkBuffer *buffer,
                               bool flip_vertically) {
  png_image png;

  (void)heap;
  if (!buffer || !buffer->data || !buffer->length)
    return NULL;

  memset(&png, 0, sizeof(png));
  png.version = PNG_IMAGE_VERSION;
  if (!png_image_begin_read_from_memory(&png, buffer->data, buffer->length))
    return NULL;
  return sample_gl_png_finish(image, &png, flip_vertically);
}

static void
sample_gl_register_image_loader(void) {
  ak_imageInitLoader(sample_gl_png_load_from_file, sample_gl_png_load_from_memory);
}
#else
static void
sample_gl_register_image_loader(void) {
}
#endif

static inline GLuint
sample_gl_upload_texture(AkImage *image) {
  GLuint tex;

  if (!image)
    return 0;
  ak_imageLoad(image);
  if (!image->data || !image->data->data || !image->data->width || !image->data->height)
    return 0;

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA8,
               (GLsizei)image->data->width,
               (GLsizei)image->data->height,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               image->data->data);
  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

static inline GLuint
sample_gl_create_white_texture(void) {
  static const unsigned char white[4] = {255u, 255u, 255u, 255u};
  GLuint tex;

  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexImage2D(GL_TEXTURE_2D,
               0,
               GL_RGBA8,
               1,
               1,
               0,
               GL_RGBA,
               GL_UNSIGNED_BYTE,
               white);
  glBindTexture(GL_TEXTURE_2D, 0);
  return tex;
}

#endif /* assetkit_gl_sample_common_h */
