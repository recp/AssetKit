/*
 * Minimal scene mesh-reading sample.
 *
 * Build:
 *   cmake -S . -B build -DAK_BUILD_SAMPLES=ON
 *   cmake --build build --target sample_read_meshes
 *
 * Run:
 *   ./build/sample_read_meshes path/to/model.gltf
 */

#include "sample_common.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *
sample_primitive_type_name(AkMeshPrimitiveType type) {
  switch (type) {
    case AK_PRIMITIVE_POINTS:
      return "points";
    case AK_PRIMITIVE_LINES:
      return "lines";
    case AK_PRIMITIVE_TRIANGLES:
      return "triangles";
    case AK_PRIMITIVE_POLYGONS:
      return "polygons";
    default:
      return "unknown";
  }
}

static size_t
sample_primitive_vertex_refs(AkMeshPrimitive *prim) {
  uint32_t stride;

  if (!prim)
    return 0;

  if (prim->indices) {
    stride = prim->indexStride ? prim->indexStride : 1u;
    return prim->indices->count / stride;
  }

  if (prim->indexAccessor)
    return prim->indexAccessor->count;

  if (prim->pos && prim->pos->accessor)
    return prim->pos->accessor->count;

  return 0;
}

static bool
sample_read_index(AkAccessor *acc, uint32_t index, AkUInt *out) {
  const unsigned char *src;
  size_t stride;
  size_t offset;
  size_t bytes;

  if (!acc || !out || !acc->buffer || !acc->buffer->data || index >= acc->count)
    return false;

  bytes  = acc->bytesPerComponent;
  stride = acc->byteStride ? acc->byteStride : bytes;
  if (bytes == 0 || stride < bytes || acc->byteOffset > acc->buffer->length)
    return false;

  if ((size_t)index > (SIZE_MAX - acc->byteOffset) / stride)
    return false;

  offset = acc->byteOffset + (size_t)index * stride;
  if (offset > acc->buffer->length || bytes > acc->buffer->length - offset)
    return false;

  src = (const unsigned char *)acc->buffer->data + offset;
  switch (acc->componentType) {
    case AKT_UBYTE:
      *out = src[0];
      return true;
    case AKT_USHORT: {
      uint16_t value;
      memcpy(&value, src, sizeof(value));
      *out = value;
      return true;
    }
    case AKT_UINT: {
      uint32_t value;
      memcpy(&value, src, sizeof(value));
      *out = value;
      return true;
    }
    default:
      return false;
  }
}

static void
sample_print_indent(unsigned depth) {
  while (depth--)
    printf("  ");
}

static void
sample_print_input_summary(AkMeshPrimitive *prim, unsigned depth) {
  AkInput *input;

  for (input = prim ? prim->input : NULL; input; input = input->next) {
    AkAccessor *acc;

    acc = input->accessor;
    sample_print_indent(depth);
    printf("input %-10s set=%u accessor_count=%u components=%u type=%s%s\n",
           input->semanticRaw ? input->semanticRaw : "?",
           input->set,
           acc ? acc->count : 0u,
           acc ? acc->componentCount : 0u,
           acc ? sample_component_type_name(acc->componentType) : "none",
           acc && acc->normalized ? " normalized" : "");
  }
}

static void
sample_print_index_preview(AkMeshPrimitive *prim, unsigned depth) {
  AkAccessor *idx;
  size_t indexCount;
  size_t previewCount;
  size_t i;

  indexCount = ak_meshPrimitiveIndexCount(prim);
  sample_print_indent(depth);
  printf("vertex_refs=%zu index_slots=%zu index_stride=%u\n",
         sample_primitive_vertex_refs(prim),
         indexCount,
         prim && prim->indexStride ? prim->indexStride : 1u);

  idx = ak_meshPrimitiveIndexAccessor(prim);
  if (!idx || indexCount == 0) {
    sample_print_indent(depth);
    printf("indices: non-indexed primitive\n");
    return;
  }

  previewCount = indexCount < 12u ? indexCount : 12u;
  sample_print_indent(depth);
  printf("first_indices:");
  for (i = 0; i < previewCount; i++) {
    AkUInt value;

    if (sample_read_index(idx, (uint32_t)i, &value))
      printf(" %u", value);
    else
      printf(" ?");
  }
  if (previewCount < indexCount)
    printf(" ...");
  printf("\n");
}

static void
sample_print_position_preview(AkMeshPrimitive *prim, unsigned depth) {
  AkAccessor *acc;
  float *values;
  size_t floatCount;
  size_t rowCount;
  size_t i;
  uint32_t comps;

  acc = prim && prim->pos ? prim->pos->accessor : NULL;
  if (!acc || acc->count == 0 || acc->componentCount == 0) {
    sample_print_indent(depth);
    printf("positions: none\n");
    return;
  }

  comps = acc->componentCount;
  if (acc->count > SIZE_MAX / comps
      || (size_t)acc->count * comps > SIZE_MAX / sizeof(float)) {
    sample_print_indent(depth);
    printf("positions: too large to preview\n");
    return;
  }

  floatCount = (size_t)acc->count * comps;
  values = malloc(floatCount * sizeof(*values));
  if (!values) {
    sample_print_indent(depth);
    printf("positions: allocation failed\n");
    return;
  }

  if (ak_accessorAsFloat(acc, values, floatCount) != floatCount) {
    sample_print_indent(depth);
    printf("positions: could not convert accessor to float\n");
    free(values);
    return;
  }

  rowCount = acc->count < 3u ? acc->count : 3u;
  sample_print_indent(depth);
  printf("first_positions:");
  for (i = 0; i < rowCount; i++) {
    size_t base;

    base = i * comps;
    printf(" (");
    printf("%.6g", values[base]);
    if (comps > 1)
      printf(", %.6g", values[base + 1u]);
    if (comps > 2)
      printf(", %.6g", values[base + 2u]);
    if (comps > 3)
      printf(", %.6g", values[base + 3u]);
    printf(")");
  }
  if (rowCount < acc->count)
    printf(" ...");
  printf("\n");

  free(values);
}

static void
sample_print_mesh_details(AkMesh *mesh, unsigned depth, uint64_t *primitiveCount) {
  AkMeshPrimitive *prim;
  uint32_t primIndex;

  primIndex = 0;
  for (prim = mesh ? mesh->primitive : NULL; prim; prim = prim->next, primIndex++) {
    if (primitiveCount)
      (*primitiveCount)++;

    sample_print_indent(depth);
    printf("primitive %u: type=%s polygons=%u material=%s\n",
           primIndex,
           sample_primitive_type_name(prim->type),
           prim->nPolygons,
           prim->material ? sample_or_unnamed(prim->material->name) : "(none)");
    sample_print_input_summary(prim, depth + 1u);
    sample_print_index_preview(prim, depth + 1u);
    sample_print_position_preview(prim, depth + 1u);
  }
}

static void
sample_print_scene_node_meshes(const AkNode *node,
                               unsigned depth,
                               uint64_t *instanceCount,
                               uint64_t *primitiveCount) {
  const AkInstanceBase *base;
  const AkInstanceNode *nodeRef;
  const AkNode *child;

  if (depth > 32u) {
    sample_print_indent(depth);
    printf("... depth limit reached\n");
    return;
  }

  for (; node; node = node->next) {
    for (base = node->geometry ? &node->geometry->base : NULL;
         base;
         base = base->next) {
      AkInstanceGeometry *inst;
      AkGeometry *geom;
      AkMesh *mesh;

      if (base->type != AK_INSTANCE_GEOMETRY)
        continue;

      inst = (AkInstanceGeometry *)base;
      geom = ak_instanceObject(&inst->base);
      mesh = sample_mesh_from_geometry(geom);

      (*instanceCount)++;
      sample_print_indent(depth);
      printf("geometry_instance: node=%s instance=%s geometry=%s mesh=%s primitives=%u bindings=%s morph=%s skin=%s\n",
             sample_or_unnamed(node->name),
             sample_or_unnamed(inst->base.name),
             geom ? sample_or_unnamed(geom->name) : "(none)",
             mesh ? sample_or_unnamed(mesh->name) : "(none)",
             mesh ? mesh->primitiveCount : 0u,
             inst->objectBindings ? "yes" : "no",
             inst->morpher ? "yes" : "no",
             inst->skinner ? "yes" : "no");

      if (mesh)
        sample_print_mesh_details(mesh, depth + 1u, primitiveCount);
    }

    for (child = node->chld; child; child = child->next)
      sample_print_scene_node_meshes(child, depth + 1u, instanceCount, primitiveCount);

    for (nodeRef = node->node; nodeRef; nodeRef = nodeRef->next) {
      AkNode *target;

      target = ak_instanceNodeTarget((AkInstanceNode *)nodeRef);
      if (target)
        sample_print_scene_node_meshes(target, depth + 1u, instanceCount, primitiveCount);
    }
  }
}

static void
sample_print_scene_meshes(AkDoc *doc) {
  AkScene *scene;
  uint64_t sceneIndex;
  uint64_t totalInstances;
  uint64_t totalPrimitives;

  totalInstances  = 0;
  totalPrimitives = 0;
  sceneIndex      = 0;

  for (scene = doc ? doc->lib.scenes.first : NULL; scene; scene = scene->next, sceneIndex++) {
    uint64_t sceneInstances;
    uint64_t scenePrimitives;

    sceneInstances  = 0;
    scenePrimitives = 0;

    printf("scene %" PRIu64 ": name=%s active=%s\n",
           sceneIndex,
           sample_or_unnamed(scene->name),
           scene == doc->scene ? "yes" : "no");
    sample_print_scene_node_meshes(scene->node ? scene->node->chld : NULL,
                                   1u,
                                   &sceneInstances,
                                   &scenePrimitives);
    printf("scene_summary %" PRIu64 ": geometry_instances=%" PRIu64 " primitives=%" PRIu64 "\n",
           sceneIndex,
           sceneInstances,
           scenePrimitives);

    totalInstances  += sceneInstances;
    totalPrimitives += scenePrimitives;
  }

  printf("scene_total: geometry_instances=%" PRIu64 " primitives=%" PRIu64 "\n",
         totalInstances,
         totalPrimitives);
}

static void
sample_print_library_mesh_summary(AkDoc *doc) {
  AkGeometry *geom;
  uint64_t meshCount;
  uint64_t primitiveCount;

  meshCount      = 0;
  primitiveCount = 0;

  for (geom = doc ? doc->lib.geometries.first : NULL; geom; geom = geom->next) {
    AkMesh *mesh;

    mesh = sample_mesh_from_geometry(geom);
    if (!mesh)
      continue;

    meshCount++;
    primitiveCount += mesh->primitiveCount;
  }

  printf("library_summary: meshes=%" PRIu64 " primitives=%" PRIu64 "\n",
         meshCount,
         primitiveCount);
}

int
main(int argc, char **argv) {
  AkDoc *doc;

  if (argc != 2) {
    fprintf(stderr, "usage: %s path/to/model\n", argv[0]);
    return 2;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  sample_print_scene_meshes(doc);
  sample_print_library_mesh_summary(doc);
  ak_free(doc);

  return 0;
}
