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

static
bool
ak_test_write_dae_many_color_sets(const char *path, uint32_t colorSetCount) {
  FILE    *file;
  uint32_t set;
  uint32_t vertex;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
        "version=\"1.4.1\">"
        "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>"
        "<library_geometries><geometry id=\"beamng-mesh\" name=\"body_a800\"><mesh>"
        "<source id=\"positions\"><float_array id=\"positions-array\" count=\"9\">"
        "0 0 0 1 0 0 0 1 0"
        "</float_array><technique_common><accessor source=\"#positions-array\" "
        "count=\"3\" stride=\"3\"><param name=\"X\" type=\"float\"/>"
        "<param name=\"Y\" type=\"float\"/><param name=\"Z\" type=\"float\"/>"
        "</accessor></technique_common></source>",
        file);

  for (set = 0u; set < colorSetCount; set++) {
    fprintf(file,
            "<source id=\"color-%u\"><float_array id=\"color-%u-array\" count=\"12\">"
            "1 0 0 1 0 1 0 1 0 0 1 1"
            "</float_array><technique_common><accessor source=\"#color-%u-array\" "
            "count=\"3\" stride=\"4\"><param name=\"R\" type=\"float\"/>"
            "<param name=\"G\" type=\"float\"/><param name=\"B\" type=\"float\"/>"
            "<param name=\"A\" type=\"float\"/></accessor></technique_common></source>",
            set,
            set,
            set);
  }

  fputs("<vertices id=\"vertices\"><input semantic=\"POSITION\" "
        "source=\"#positions\"/></vertices><triangles count=\"1\">"
        "<input semantic=\"VERTEX\" source=\"#vertices\" offset=\"0\"/>",
        file);
  for (set = 0u; set < colorSetCount; set++) {
    fprintf(file,
            "<input semantic=\"COLOR\" source=\"#color-%u\" offset=\"%u\" set=\"%u\"/>",
            set,
            set + 1u,
            set);
  }

  fputs("<p>", file);
  for (vertex = 0u; vertex < 3u; vertex++) {
    uint32_t input;

    for (input = 0u; input <= colorSetCount; input++)
      fprintf(file, "%s%u", vertex || input ? " " : "", vertex);
  }
  fputs("</p></triangles></mesh></geometry></library_geometries>"
        "<library_visual_scenes><visual_scene id=\"Scene\">"
        "<node id=\"base00\" name=\"base00\"><node id=\"body_a800\" name=\"body_a800\">"
        "<instance_geometry url=\"#beamng-mesh\"/></node></node>"
        "</visual_scene></library_visual_scenes>"
        "<scene><instance_visual_scene url=\"#Scene\"/></scene></COLLADA>",
        file);

  return fclose(file) == 0;
}

static
uint32_t
ak_test_primitive_color_input_count(AkDoc *doc) {
  AkGeometry      *geom;
  AkMesh          *mesh;
  AkMeshPrimitive *prim;
  AkInput         *input;
  uint32_t         count;

  geom = doc ? doc->lib.geometries.first : NULL;
  if (!geom || !geom->gdata || geom->gdata->type != AK_GEOMETRY_MESH)
    return 0u;
  mesh = ak_objGet(geom->gdata);
  prim = mesh ? mesh->primitive : NULL;
  if (!prim)
    return 0u;

  count = 0u;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_COLOR)
      count++;
  }
  return count;
}

TEST_IMPL(dae_export_many_vertex_color_sets) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc       = NULL;
  roundTrip = NULL;
  tmpBase   = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-many-colors-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/beamng_colors.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/beamng_colors.dae", outDir);

  ASSERT(ak_test_write_dae_many_color_sets(daePath, 40u));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(ak_test_primitive_color_input_count(doc) == 40u);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "semantic=\"COLOR\""));
  ASSERT(ak_test_file_contains(outDae, "set=\"39\""));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_AUTO) == AK_OK && roundTrip);
  ASSERT(ak_test_primitive_color_input_count(roundTrip) == 40u);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_brep_smoke) {
  AkDoc        *doc;
  AkDoc        *roundTrip;
  AkBoundryRep *brep;
  char          dirTemplate[PATH_MAX];
  char         *tmpdir;
  char          daePath[PATH_MAX];
  char          outDir[PATH_MAX];
  char          outDae[PATH_MAX];
  const char   *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-export-brep-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/brep.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/brep.dae", outDir);

  ASSERT(ak_test_write_dae_brep_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.geometries.first != NULL);
  ASSERT(doc->lib.geometries.first->gdata != NULL);
  ASSERT(doc->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);
  brep = ak_objGet(doc->lib.geometries.first->gdata);
  ASSERT(brep != NULL);
  ASSERT(brep->vertices != NULL);
  ASSERT(brep->vertices->input != NULL);
  ASSERT(brep->vertices->input->accessor != NULL);
  ASSERT(brep->edges != NULL);
  ASSERT(brep->edges->input != NULL);
  ASSERT(brep->edges->input->semanticRaw != NULL);
  ASSERT(strcmp(brep->edges->input->semanticRaw, "VERTEX") == 0);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(
    outDae,
    "<COLLADA xmlns=\"http://www.collada.org/2008/03/COLLADASchema\" version=\"1.5.0\""));
  ASSERT(ak_test_file_contains(outDae, "<edges"));
  ASSERT(ak_test_file_contains(outDae, "<input semantic=\"VERTEX\" source=\"#geom_0_brep_vertices\""));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.geometries.first != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);
  brep = ak_objGet(roundTrip->lib.geometries.first->gdata);
  ASSERT(brep != NULL);
  ASSERT(brep->edges != NULL);
  ASSERT(brep->edges->input != NULL);
  ASSERT(brep->edges->input->semanticRaw != NULL);
  ASSERT(strcmp(brep->edges->input->semanticRaw, "VERTEX") == 0);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_spline_roundtrip) {
  AkDoc       *doc;
  AkDoc       *roundTrip;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-export-spline-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/spline.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/spline.dae", outDir);

  ASSERT(ak_test_write_dae_spline_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.geometries.first != NULL);
  ASSERT(doc->lib.geometries.first->gdata != NULL);
  ASSERT(doc->lib.geometries.first->gdata->type == AK_GEOMETRY_SPLINE);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "version=\"1.4.1\""));
  ASSERT(ak_test_file_contains(outDae, "<spline"));
  ASSERT(ak_test_file_contains(outDae, "<control_vertices"));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.geometries.first != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata->type == AK_GEOMETRY_SPLINE);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_brep_nurbs_roundtrip) {
  AkHeap            *heap;
  AkDoc             *doc;
  AkDoc             *roundTrip;
  AkBoundryRep      *brep;
  AkCurve           *curve;
  AkNurbs           *nurbs;
  AkSurface         *brepSurface;
  AkNurbsSurface    *nurbsSurface;
  AkSweptSurface    *sweptSurface;
  AkLine            *line;
  AkImage           *image;
  AkImageSource     *source;
  AkMaterial        *mat;
  AkMaterialSurface *surface;
  AkMaterialInput   *baseColor;
  AkTextureRef      *texref;
  AkTexture         *texture;
  AkSampler         *sampler;
  char               dirTemplate[PATH_MAX];
  char              *tmpdir;
  char               daePath[PATH_MAX];
  char               outDir[PATH_MAX];
  char               outDae[PATH_MAX];
  const char        *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-export-brep-nurbs-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/brep_nurbs.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/brep_nurbs.dae", outDir);

  ASSERT(ak_test_write_dae_brep_nurbs_minimal(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);
  ASSERT(doc->lib.geometries.first != NULL);
  ASSERT(doc->lib.geometries.first->gdata != NULL);
  ASSERT(doc->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);

  heap         = ak_heap_getheap(doc);
  brep         = ak_objGet(doc->lib.geometries.first->gdata);
  image        = ak_heap_calloc(heap, doc, sizeof(*image));
  source       = ak_heap_calloc(heap, image, sizeof(*source));
  mat          = ak_heap_calloc(heap, doc, sizeof(*mat));
  surface      = ak_heap_calloc(heap, mat, sizeof(*surface));
  baseColor = ak_test_material_input(heap, surface);
  texref       = ak_heap_calloc(heap, baseColor, sizeof(*texref));
  texture      = ak_heap_calloc(heap, doc, sizeof(*texture));
  sampler      = ak_heap_calloc(heap, texture, sizeof(*sampler));
  ASSERT(image != NULL);
  ASSERT(source != NULL);
  ASSERT(mat != NULL);
  ASSERT(surface != NULL);
  ASSERT(baseColor != NULL);
  ASSERT(texref != NULL);
  ASSERT(texture != NULL);
  ASSERT(sampler != NULL);
  ASSERT(brep != NULL);
  ASSERT(brep->curves != NULL);
  ASSERT(brep->curves->curve != NULL);
  ASSERT(brep->curves->curve->curve != NULL);
  ASSERT(brep->curves->curve->curve->type == AK_CURVE_NURBS);
  ASSERT(brep->surfaces != NULL);
  ASSERT(brep->surfaces->surface != NULL);

  source->type     = AK_IMAGE_SOURCE_URI;
  source->uri      = "data:image/png;base64,AA==";
  image->source    = source;
  texture->image   = image;
  texture->sampler = sampler;
  texref->texture  = texture;
  texref->slot     = 0;
  baseColor->source    = AK_MATERIAL_INPUT_TEXTURE;
  baseColor->valueType = AK_MATERIAL_VALUE_COLOR;
  baseColor->texture   = texref;
  surface->type        = AK_MATERIAL_TYPE_LAMBERT;
  surface->baseColor   = baseColor;
  mat->surface         = surface;

  doc->lib.images.first    = image;
  doc->lib.images.last     = image;
  doc->lib.images.count    = 1;
  doc->lib.materials.first = mat;
  doc->lib.materials.last  = mat;
  doc->lib.materials.count = 1;

  curve                   = brep->curves->curve;
  nurbs                   = ak_objGet(curve->curve);
  nurbsSurface            = NULL;
  sweptSurface            = NULL;
  for (brepSurface = brep->surfaces->surface;
       brepSurface;
       brepSurface = brepSurface->next) {
    if (!brepSurface->surface)
      continue;
    if (brepSurface->surface->type == AK_SURFACE_NURBS_SURFACE)
      nurbsSurface = ak_objGet(brepSurface->surface);
    else if (brepSurface->surface->type == AK_SURFACE_SWEPT_SURFACE)
      sweptSurface = ak_objGet(brepSurface->surface);
  }
  line                    = sweptSurface && sweptSurface->curve
                              && sweptSurface->curve->curve
                            ? ak_objGet(sweptSurface->curve->curve)
                            : NULL;
  ASSERT(nurbs != NULL);
  ASSERT(nurbs->cverts != NULL);
  ASSERT(nurbsSurface != NULL);
  ASSERT(nurbsSurface->cverts != NULL);
  ASSERT(sweptSurface != NULL);
  ASSERT(line != NULL);

  brep->curves->extra         = ak_test_dae_extra(heap, brep->curves, "brep-curves-extra");
  nurbs->extra                = ak_test_dae_extra(heap, nurbs, "brep-nurbs-extra");
  nurbs->cverts->extra        = ak_test_dae_extra(heap, nurbs->cverts, "brep-nurbs-cverts-extra");
  brep->surfaces->extra       = ak_test_dae_extra(heap, brep->surfaces, "brep-surfaces-extra");
  nurbsSurface->extra         = ak_test_dae_extra(heap, nurbsSurface, "brep-nurbs-surface-extra");
  nurbsSurface->cverts->extra = ak_test_dae_extra(heap, nurbsSurface->cverts, "brep-surface-cverts-extra");
  sweptSurface->extra         = ak_test_dae_extra(heap, sweptSurface, "brep-swept-extra");
  line->extra                 = ak_test_dae_extra(heap, line, "brep-line-extra");
  ASSERT(brep->curves->extra != NULL);
  ASSERT(nurbs->extra != NULL);
  ASSERT(nurbs->cverts->extra != NULL);
  ASSERT(brep->surfaces->extra != NULL);
  ASSERT(nurbsSurface->extra != NULL);
  ASSERT(nurbsSurface->cverts->extra != NULL);
  ASSERT(sweptSurface->extra != NULL);
  ASSERT(line->extra != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(
    outDae,
    "<COLLADA xmlns=\"http://www.collada.org/2008/03/COLLADASchema\" version=\"1.5.0\""));
  ASSERT(ak_test_file_contains(outDae,
                               "<init_from><ref>data:image/png;base64,AA==</ref></init_from>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<sampler2D><instance_image url=\"#image_0\"/>"));
  ASSERT(!ak_test_file_contains(outDae, "<source>surface_0</source>"));
  ASSERT(ak_test_file_contains(outDae, "<nurbs"));
  ASSERT(ak_test_file_contains(outDae, "<nurbs_surface"));
  ASSERT(ak_test_file_contains(outDae, "<swept_surface>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-curves-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-nurbs-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-nurbs-cverts-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-surfaces-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-nurbs-surface-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-surface-cverts-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-swept-extra</note>"));
  ASSERT(ak_test_file_contains(outDae, "<note>brep-line-extra</note>"));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.geometries.first != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata != NULL);
  ASSERT(roundTrip->lib.geometries.first->gdata->type == AK_GEOMETRY_BREP);
  ASSERT(roundTrip->lib.images.count == 1);
  ASSERT(roundTrip->lib.images.first != NULL);
  ASSERT(roundTrip->lib.images.first->source != NULL);
  ASSERT(roundTrip->lib.images.first->source->type == AK_IMAGE_SOURCE_URI);
  ASSERT(roundTrip->lib.materials.count == 1);
  ASSERT(roundTrip->lib.materials.first != NULL);
  ASSERT(roundTrip->lib.materials.first->surface != NULL);
  ASSERT(roundTrip->lib.materials.first->surface->baseColor != NULL);
  ASSERT(roundTrip->lib.materials.first->surface->baseColor->texture != NULL);
  ASSERT(roundTrip->lib.materials.first->surface->baseColor->texture->texture != NULL);
  ASSERT(roundTrip->lib.materials.first->surface->baseColor->texture->texture->image
         == roundTrip->lib.images.first);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_vertex_input_offset_preserved) {
  AkDoc             *doc;
  AkGeometry        *geom;
  AkMesh            *mesh;
  AkMeshPrimitive   *prim;
  AkInput           *input;
  AkInput           *texcoord;
  float              pos[9];
  char               dirTemplate[PATH_MAX];
  char              *tmpdir;
  char               daePath[PATH_MAX];
  char               outDir[PATH_MAX];
  char               outDae[PATH_MAX];
  const char        *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-vertex-offset-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/vertex_offset.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/vertex_offset.dae", outDir);

  ASSERT(ak_test_write_dae_vertex_offset_nonzero(daePath));
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
  ASSERT(ak_accessorAsFloat(prim->pos->accessor, pos, 9) == 9);
  ASSERT(pos[0] == 0.0f && pos[1] == 0.0f && pos[2] == 0.0f);
  ASSERT(pos[3] == 1.0f && pos[4] == 0.0f && pos[5] == 0.0f);
  ASSERT(pos[6] == 0.0f && pos[7] == 1.0f && pos[8] == 0.0f);

  texcoord = NULL;
  for (input = prim->input; input; input = input->next) {
    if (input->semantic == AK_INPUT_TEXCOORD) {
      texcoord = input;
      break;
    }
  }
  ASSERT(texcoord != NULL);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<input semantic=\"VERTEX\""));
  ASSERT(ak_test_file_contains(outDae, "<input semantic=\"TEXCOORD\""));

  unlink(outDae);
  rmdir(outDir);
  ak_free(doc);
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
  ASSERT(inst->sourceUrl != NULL);
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

TEST_IMPL(dae_external_instance_node_resolves_and_exports) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root;
  AkNode         *shared;
  AkInstanceNode *inst;
  char            dirTemplate[PATH_MAX];
  char           *tmpdir;
  char            daePath[PATH_MAX];
  char            libPath[PATH_MAX];
  char            outDir[PATH_MAX];
  char            outDae[PATH_MAX];
  const char     *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-external-instance-node-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/main.dae", tmpdir);
  snprintf(libPath, sizeof(libPath), "%s/external_nodes.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/main.dae", outDir);

  ASSERT(ak_test_write_dae_external_node_lib(libPath));
  ASSERT(ak_test_write_dae_external_node_main(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  ASSERT(scene->node != NULL);

  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->node != NULL);

  inst = root->node;
  ASSERT(inst->owner == root);
  ASSERT(inst->sourceUrl != NULL);

  shared = ak_instanceNodeTarget(inst);
  ASSERT(shared != NULL);
  ASSERT(shared == inst->target);
  ASSERT(shared->name && strcmp(shared->name, "ExternalShared") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_images>"));
  ASSERT(ak_test_file_contains(outDae, "<library_cameras>"));
  ASSERT(ak_test_file_contains(outDae, "<library_geometries>"));
  ASSERT(ak_test_file_contains(outDae, "<library_materials>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<init_from>data:image/png;base64,QUJD</init_from>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<texture texture=\"sampler_0\" texcoord=\"UV0\"/>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<bind_vertex_input semantic=\"UV0\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>"));
  ASSERT(ak_test_file_contains(outDae, "<instance_camera url=\"#camera_0\""));
  ASSERT(ak_test_file_contains(outDae, "<instance_geometry url=\"#geom_0\""));
  ASSERT(ak_test_file_contains(outDae, "target=\"#material_0\""));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ak_free(roundTrip);

  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  unlink(libPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_external_library_node_ref_resolves_and_exports) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root;
  AkNode         *wrapper;
  AkNode         *shared;
  AkNode         *node;
  AkInstanceNode *rootInst;
  AkInstanceNode *wrappedInst;
  char            dirTemplate[PATH_MAX];
  char           *tmpdir;
  char            daePath[PATH_MAX];
  char            libPath[PATH_MAX];
  char            outDir[PATH_MAX];
  char            outDae[PATH_MAX];
  const char     *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  wrapper = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-external-library-node-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/main.dae", tmpdir);
  snprintf(libPath, sizeof(libPath), "%s/external_nodes.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/main.dae", outDir);

  ASSERT(ak_test_write_dae_external_node_lib(libPath));
  ASSERT(ak_test_write_dae_external_node_wrapped_main(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  for (node = doc->lib.nodes.first; node; node = node->docNext) {
    if (node->name && strcmp(node->name, "Wrapper") == 0) {
      wrapper = node;
      break;
    }
  }

  ASSERT(wrapper != NULL);
  ASSERT(wrapper->node != NULL);

  scene = doc->scene;
  ASSERT(scene != NULL);
  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->node != NULL);

  rootInst = root->node;
  ASSERT(rootInst->owner == root);
  ASSERT(ak_instanceNodeTarget(rootInst) == wrapper);

  wrappedInst = wrapper->node;
  ASSERT(wrappedInst->owner == wrapper);
  ASSERT(wrappedInst->sourceUrl != NULL);

  shared = ak_instanceNodeTarget(wrappedInst);
  ASSERT(shared != NULL);
  ASSERT(shared == wrappedInst->target);
  ASSERT(shared->name && strcmp(shared->name, "ExternalShared") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_images>"));
  ASSERT(ak_test_file_contains(outDae, "<library_cameras>"));
  ASSERT(ak_test_file_contains(outDae, "<library_geometries>"));
  ASSERT(ak_test_file_contains(outDae, "<library_materials>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<init_from>data:image/png;base64,QUJD</init_from>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<texture texture=\"sampler_0\" texcoord=\"UV0\"/>"));
  ASSERT(ak_test_file_contains(outDae,
                               "<bind_vertex_input semantic=\"UV0\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>"));
  ASSERT(ak_test_file_contains(outDae, "<instance_camera url=\"#camera_0\""));
  ASSERT(ak_test_file_contains(outDae, "<instance_geometry url=\"#geom_0\""));
  ASSERT(ak_test_file_contains(outDae, "target=\"#material_0\""));
  ASSERT(!ak_test_file_contains(outDae, "external_nodes.dae"));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ak_free(roundTrip);

  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  unlink(libPath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_export_skips_unresolved_node_instance_url_ptr) {
  AkDoc          *doc;
  AkDoc          *roundTrip;
  AkScene        *scene;
  AkNode         *root;
  AkInstanceNode *inst;
  struct stat     stDae;
  char            dirTemplate[PATH_MAX];
  char           *tmpdir;
  char            daePath[PATH_MAX];
  char            outDir[PATH_MAX];
  char            outDae[PATH_MAX];
  const char     *tmpBase;

  doc = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";

  snprintf(dirTemplate,
           sizeof(dirTemplate),
           "%s/assetkit-dae-missing-external-node-XXXXXX",
           tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);

  snprintf(daePath, sizeof(daePath), "%s/main.dae", tmpdir);
  snprintf(outDir, sizeof(outDir), "%s/out", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/main.dae", outDir);

  ASSERT(ak_test_write_dae_missing_external_node_main(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_AUTO) == AK_OK && doc);

  scene = doc->scene;
  ASSERT(scene != NULL);
  root = ak_sceneFindRoot(scene, "Root");
  ASSERT(root != NULL);
  ASSERT(root->node != NULL);

  inst = root->node;
  ASSERT(inst->sourceUrl != NULL);
  ASSERT(ak_instanceNodeTarget(inst) == NULL);
  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(stat(outDae, &stDae) == 0);
  ASSERT(stDae.st_size > 0);
  ASSERT(!ak_test_file_contains(outDae, "<instance_node"));

  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(roundTrip != NULL);

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);

  TEST_SUCCESS
}

TEST_IMPL(dae_camera_light_extra_preserve_opt) {
  AkDoc      *doc;
  AkDoc      *docWithExtras;
  AkCamera   *camera;
  AkLight    *light;
  AkSpotLight *spot;
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
  ASSERT(doc->lib.lights.first->data != NULL);
  ASSERT(doc->lib.lights.first->data->type == AK_LIGHT_TYPE_SPOT);
  spot = (AkSpotLight *)doc->lib.lights.first->data;
  ASSERT(fabsf(spot->base.intensity - 3.0f) < 0.001f);
  ASSERT(fabsf(spot->innerConeAngle - 0.2617994f) < 0.001f);
  ASSERT(fabsf(spot->outerConeAngle - 0.6981317f) < 0.001f);
  ASSERT(spot->base.range > 0.0f);
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
