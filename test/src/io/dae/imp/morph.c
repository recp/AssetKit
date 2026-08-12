/*
 * Copyright (C) 2020 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 */

#include "../../../test_export_common.h"

static
bool
ak_test_write_dae_vertices_only_morph(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs(
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
    "version=\"1.4.1\">"
    "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>"
    "<library_geometries>"
    "<geometry id=\"base\"><mesh>"
    "<source id=\"base-pos\"><float_array id=\"base-pos-array\" count=\"12\">"
    "-1 -1 0 1 -1 0 1 1 0 -1 1 0</float_array><technique_common>"
    "<accessor source=\"#base-pos-array\" count=\"4\" stride=\"3\">"
    "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
    "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
    "<vertices id=\"base-vertices\"><input semantic=\"POSITION\" "
    "source=\"#base-pos\"/></vertices>"
    "<polylist count=\"1\"><input semantic=\"VERTEX\" "
    "source=\"#base-vertices\" offset=\"0\"/><vcount>4</vcount>"
    "<p>0 1 2 3</p></polylist></mesh></geometry>"
    "<geometry id=\"raise\"><mesh>"
    "<source id=\"raise-pos\"><float_array id=\"raise-pos-array\" count=\"12\">"
    "-1 -1 0 1 -1 0 1 1 2 -1 1 2</float_array><technique_common>"
    "<accessor source=\"#raise-pos-array\" count=\"4\" stride=\"3\">"
    "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
    "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
    "<vertices id=\"raise-vertices\"><input semantic=\"POSITION\" "
    "source=\"#raise-pos\"/></vertices>"
    "</mesh></geometry></library_geometries>"
    "<library_controllers><controller id=\"morph\"><morph source=\"#base\" "
    "method=\"NORMALIZED\">"
    "<source id=\"targets\"><IDREF_array id=\"targets-array\" count=\"1\">"
    "raise</IDREF_array><technique_common><accessor source=\"#targets-array\" "
    "count=\"1\" stride=\"1\"><param name=\"MORPH_TARGET\" type=\"IDREF\"/>"
    "</accessor></technique_common></source>"
    "<source id=\"weights\"><float_array id=\"weights-array\" count=\"1\">"
    "0.5</float_array><technique_common><accessor source=\"#weights-array\" "
    "count=\"1\" stride=\"1\"><param name=\"MORPH_WEIGHT\" type=\"float\"/>"
    "</accessor></technique_common></source>"
    "<targets><input semantic=\"MORPH_TARGET\" source=\"#targets\"/>"
    "<input semantic=\"MORPH_WEIGHT\" source=\"#weights\"/></targets>"
    "</morph></controller></library_controllers>"
    "<library_visual_scenes><visual_scene id=\"Scene\"><node id=\"mesh\">"
    "<instance_controller url=\"#morph\"/></node></visual_scene>"
    "</library_visual_scenes><scene><instance_visual_scene url=\"#Scene\"/>"
    "</scene></COLLADA>",
    file);

  return fclose(file) == 0;
}

static
bool
ak_test_write_dae_shared_morph_animation(const char *path) {
  FILE *file;

  file = fopen(path, "wb");
  if (!file)
    return false;

  fputs(
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
    "version=\"1.4.1\">"
    "<asset><unit name=\"meter\" meter=\"1\"/><up_axis>Y_UP</up_axis></asset>"
    "<library_geometries>"
    "<geometry id=\"base\"><mesh>"
    "<source id=\"base-pos\"><float_array id=\"base-pos-array\" count=\"9\">"
    "0 0 0 1 0 0 0 1 0</float_array><technique_common>"
    "<accessor source=\"#base-pos-array\" count=\"3\" stride=\"3\">"
    "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
    "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
    "<vertices id=\"base-vertices\"><input semantic=\"POSITION\" "
    "source=\"#base-pos\"/></vertices><triangles count=\"1\">"
    "<input semantic=\"VERTEX\" source=\"#base-vertices\" offset=\"0\"/>"
    "<p>0 1 2</p></triangles></mesh></geometry>"
    "<geometry id=\"target\"><mesh>"
    "<source id=\"target-pos\"><float_array id=\"target-pos-array\" count=\"9\">"
    "0 0 1 1 0 1 0 1 1</float_array><technique_common>"
    "<accessor source=\"#target-pos-array\" count=\"3\" stride=\"3\">"
    "<param name=\"X\" type=\"float\"/><param name=\"Y\" type=\"float\"/>"
    "<param name=\"Z\" type=\"float\"/></accessor></technique_common></source>"
    "<vertices id=\"target-vertices\"><input semantic=\"POSITION\" "
    "source=\"#target-pos\"/></vertices></mesh></geometry>"
    "</library_geometries>"
    "<library_controllers><controller id=\"shared-morph\">"
    "<morph source=\"#base\" method=\"RELATIVE\">"
    "<source id=\"morph-targets\"><IDREF_array id=\"morph-targets-array\" "
    "count=\"1\">target</IDREF_array><technique_common>"
    "<accessor source=\"#morph-targets-array\" count=\"1\" stride=\"1\">"
    "<param name=\"MORPH_TARGET\" type=\"IDREF\"/>"
    "</accessor></technique_common></source>"
    "<source id=\"morph-weights\"><float_array id=\"morph-weights-array\" "
    "count=\"1\">0.25</float_array><technique_common>"
    "<accessor source=\"#morph-weights-array\" count=\"1\" stride=\"1\">"
    "<param name=\"MORPH_WEIGHT\" type=\"float\"/>"
    "</accessor></technique_common></source>"
    "<targets><input semantic=\"MORPH_TARGET\" source=\"#morph-targets\"/>"
    "<input semantic=\"MORPH_WEIGHT\" source=\"#morph-weights\"/>"
    "</targets></morph></controller></library_controllers>"
    "<library_animations><animation id=\"weight-animation\">"
    "<source id=\"weight-time\"><float_array id=\"weight-time-array\" "
    "count=\"2\">0 1</float_array><technique_common>"
    "<accessor source=\"#weight-time-array\" count=\"2\" stride=\"1\">"
    "<param name=\"TIME\" type=\"float\"/>"
    "</accessor></technique_common></source>"
    "<source id=\"weight-value\"><float_array id=\"weight-value-array\" "
    "count=\"2\">0.25 0.75</float_array><technique_common>"
    "<accessor source=\"#weight-value-array\" count=\"2\" stride=\"1\">"
    "<param name=\"VALUE\" type=\"float\"/>"
    "</accessor></technique_common></source>"
    "<sampler id=\"weight-sampler\"><input semantic=\"INPUT\" "
    "source=\"#weight-time\"/><input semantic=\"OUTPUT\" "
    "source=\"#weight-value\"/></sampler>"
    "<channel source=\"#weight-sampler\" target=\"morph-weights(0)\"/>"
    "</animation></library_animations>"
    "<library_animation_clips><animation_clip id=\"weight-clip\" name=\"Weight\">"
    "<instance_animation url=\"#weight-animation\"/>"
    "</animation_clip></library_animation_clips>"
    "<library_visual_scenes><visual_scene id=\"Scene\">"
    "<node id=\"mesh-a\"><instance_controller url=\"#shared-morph\"/></node>"
    "<node id=\"mesh-b\"><instance_controller url=\"#shared-morph\"/></node>"
    "</visual_scene></library_visual_scenes>"
    "<scene><instance_visual_scene url=\"#Scene\"/></scene></COLLADA>",
    file);

  return fclose(file) == 0;
}

static
bool
ak_test_rewrite_dae_morph_on_morph(const char *path) {
  static const char closeControllers[] = "</library_controllers>";
  static const char innerInstance[]    = "url=\"#shared-morph\"";
  static const char outerInstance[]    = "url=\"#nested-morph\"";
  static const char outerController[] =
    "<controller id=\"nested-morph\"><morph source=\"#shared-morph\" "
    "method=\"RELATIVE\">"
    "<source id=\"nested-targets\"><IDREF_array id=\"nested-targets-array\" "
    "count=\"1\">target</IDREF_array><technique_common>"
    "<accessor source=\"#nested-targets-array\" count=\"1\" stride=\"1\">"
    "<param name=\"MORPH_TARGET\" type=\"IDREF\"/>"
    "</accessor></technique_common></source>"
    "<source id=\"nested-weights\"><float_array id=\"nested-weights-array\" "
    "count=\"1\">0.5</float_array><technique_common>"
    "<accessor source=\"#nested-weights-array\" count=\"1\" stride=\"1\">"
    "<param name=\"MORPH_WEIGHT\" type=\"float\"/>"
    "</accessor></technique_common></source>"
    "<targets><input semantic=\"MORPH_TARGET\" source=\"#nested-targets\"/>"
    "<input semantic=\"MORPH_WEIGHT\" source=\"#nested-weights\"/>"
    "</targets></morph></controller>";
  FILE *file;
  char *source, *rewritten, *controllersAt, *instanceAt;
  long  fileSize;
  size_t prefix, suffix, rewrittenSize;
  bool ok;

  file = fopen(path, "rb");
  if (!file)
    return false;
  if (fseek(file, 0, SEEK_END) != 0
      || (fileSize = ftell(file)) < 0
      || fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }

  source = malloc((size_t)fileSize + 1u);
  if (!source) {
    fclose(file);
    return false;
  }
  ok = fread(source, 1, (size_t)fileSize, file) == (size_t)fileSize;
  if (fclose(file) != 0)
    ok = false;
  source[fileSize] = '\0';
  if (!ok
      || !(controllersAt = strstr(source, closeControllers))
      || !(instanceAt = strstr(source, innerInstance))) {
    free(source);
    return false;
  }

  memcpy(instanceAt, outerInstance, sizeof(outerInstance) - 1u);
  prefix        = (size_t)(controllersAt - source);
  suffix        = (size_t)fileSize - prefix;
  rewrittenSize = (size_t)fileSize + sizeof(outerController) - 1u;
  rewritten     = malloc(rewrittenSize);
  if (!rewritten) {
    free(source);
    return false;
  }
  memcpy(rewritten, source, prefix);
  memcpy(rewritten + prefix, outerController, sizeof(outerController) - 1u);
  memcpy(rewritten + prefix + sizeof(outerController) - 1u,
         controllersAt,
         suffix);

  file = fopen(path, "wb");
  ok = file
       && fwrite(rewritten, 1, rewrittenSize, file) == rewrittenSize;
  if (file && fclose(file) != 0)
    ok = false;
  free(rewritten);
  free(source);
  return ok;
}

static
AkGeometry *
ak_test_morph_base_geometry(AkDoc *doc, AkMorph *morph) {
  AkGeometry *targetGeom;
  AkGeometry *geom;

  targetGeom = morph && morph->target && morph->target->target
                 ? ak_objGetTarget(morph->target->target)
                 : NULL;
  for (geom = doc ? doc->lib.geometries.first : NULL; geom; geom = geom->next) {
    AkMesh *mesh;

    if (geom == targetGeom || !geom->gdata
        || geom->gdata->type != AK_GEOMETRY_MESH)
      continue;
    mesh = ak_objGet(geom->gdata);
    if (mesh && mesh->primitive)
      return geom;
  }
  return NULL;
}

static
AkMorphInspectMorphable *
ak_test_morphable_at(AkMorphInspectTargetView *target, uint32_t index) {
  AkMorphInspectMorphable *morphable;
  uint32_t i;

  morphable = target ? target->morphable : NULL;
  for (i = 0; morphable && i < index; i++)
    morphable = morphable->next;
  return morphable;
}

TEST_IMPL(dae_morph_vertices_only_target_roundtrip) {
  AkDoc                    *doc, *roundTrip;
  AkMorph                  *morph;
  AkGeometry               *baseGeom, *targetGeom;
  AkMesh                   *baseMesh, *targetMesh;
  AkMorphInspectView       *view;
  AkMorphInspectMorphable  *inspected;
  AkInputSemantic           desired[1] = {AK_INPUT_POSITION};
  uintptr_t                 oldCoord, oldTriangulate, oldNormals, oldIndices;
  char                      dirTemplate[PATH_MAX];
  char                     *tmpdir;
  char                      daePath[PATH_MAX];
  char                      outDir[PATH_MAX];
  char                      outDae[PATH_MAX];
  const char               *tmpBase;

  doc = roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";
  ASSERT(ak_test_path_join(dirTemplate,
                           sizeof(dirTemplate),
                           tmpBase,
                           "assetkit-dae-morph-vertices-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "morph.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "morph.dae"));
  ASSERT(ak_test_write_dae_vertices_only_morph(daePath));

  oldCoord       = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  oldTriangulate = ak_opt_get(AK_OPT_TRIANGULATE);
  oldNormals     = ak_opt_get(AK_OPT_GEN_NORMALS_IF_NEEDED);
  oldIndices     = ak_opt_get(AK_OPT_INDICES_DEFAULT);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  ak_opt_set(AK_OPT_TRIANGULATE, true);
  ak_opt_set(AK_OPT_GEN_NORMALS_IF_NEEDED, true);
  ak_opt_set(AK_OPT_INDICES_DEFAULT, false);

  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);
  ASSERT(doc->lib.morphs.count == 1 && doc->lib.morphs.first);
  morph = doc->lib.morphs.first;
  ASSERT(morph->targetCount == 1 && morph->target);
  ASSERT(morph->defaultWeights && morph->defaultWeights->count == 1);
  ASSERT(fabsf(morph->defaultWeights->items[0] - 0.5f) < 1e-6f);

  targetGeom = ak_objGetTarget(morph->target->target);
  targetMesh = targetGeom && targetGeom->gdata
                 ? ak_objGet(targetGeom->gdata)
                 : NULL;
  ASSERT(targetMesh && !targetMesh->primitive && targetMesh->vertices);
  ASSERT(targetMesh->vertices->input);
  ASSERT(targetMesh->vertices->input->semantic == AK_INPUT_POSITION);
  ASSERT(targetMesh->vertices->input->accessor->count == 4);

  baseGeom = ak_test_morph_base_geometry(doc, morph);
  baseMesh = baseGeom ? ak_objGet(baseGeom->gdata) : NULL;
  ASSERT(baseMesh && baseMesh->primitive && baseMesh->primitive->pos);
  ASSERT(baseMesh->primitive->pos->accessor->count > 4);
  ASSERT(ak_morphInspect(baseGeom, morph, desired, 1, false, true) == AK_OK);
  view = morph->inspectResult;
  ASSERT(view && view->nTargets == 1 && view->targets);
  ASSERT(view->targets->nTargets == baseMesh->primitiveCount);
  inspected = ak_test_morphable_at(view->targets, 0);
  ASSERT(inspected && inspected->input && inspected->input->input);
  ASSERT(inspected->vertexCount == baseMesh->primitive->pos->accessor->count);
  ASSERT(inspected->input->input->accessor->count == inspected->vertexCount);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<geometry id=\"morph_0_target_0\""));
  ASSERT(ak_test_file_contains(outDae, ">morph_0_target_0</IDREF_array>"));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->lib.morphs.count == 1 && roundTrip->lib.morphs.first);
  morph = roundTrip->lib.morphs.first;
  baseGeom = ak_test_morph_base_geometry(roundTrip, morph);
  baseMesh = baseGeom ? ak_objGet(baseGeom->gdata) : NULL;
  ASSERT(baseMesh && baseMesh->primitive && baseMesh->primitive->pos);
  ASSERT(ak_morphInspect(baseGeom, morph, desired, 1, false, true) == AK_OK);
  view = morph->inspectResult;
  inspected = view && view->targets
                ? ak_test_morphable_at(view->targets, 0)
                : NULL;
  ASSERT(view && view->nTargets == 1 && view->targets->nTargets == 1);
  ASSERT(inspected && inspected->input && inspected->input->input);
  ASSERT(inspected->vertexCount == baseMesh->primitive->pos->accessor->count);
  ASSERT(inspected->input->input->accessor->count == inspected->vertexCount);

  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, oldCoord);
  ak_opt_set(AK_OPT_TRIANGULATE, oldTriangulate);
  ak_opt_set(AK_OPT_GEN_NORMALS_IF_NEEDED, oldNormals);
  ak_opt_set(AK_OPT_INDICES_DEFAULT, oldIndices);
  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);
  TEST_SUCCESS
}

TEST_IMPL(dae_shared_morph_channel_resolves_all_instances) {
  AkDoc            *doc, *roundTrip;
  AkAnimation      *animation;
  AkChannel        *channel;
  AkResolvedTarget  original, resolved[2];
  AkContext         context;
  AkFileType        originalType;
  char              dirTemplate[PATH_MAX];
  char             *tmpdir;
  char              daePath[PATH_MAX];
  char              outDir[PATH_MAX];
  char              outDae[PATH_MAX];
  const char       *tmpBase;
  size_t            count;

  doc = roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";
  ASSERT(ak_test_path_join(dirTemplate, sizeof(dirTemplate), tmpBase,
                           "assetkit-dae-morph-fanout-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "fanout.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "fanout.dae"));
  ASSERT(ak_test_write_dae_shared_morph_animation(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  animation = doc->lib.animations.first;
  channel   = animation ? animation->channel : NULL;
  ASSERT(animation && channel && channel->targetType == AK_TARGET_WEIGHTS);
  ASSERT(doc->animationClips.count == 1);
  ASSERT(ak_animationClipContainsAnimation(doc->animationClips.first,
                                           animation));
  context          = AkContextZeroed();
  context.doc      = doc;
  original         = ak_channelTarget(&context, channel);
  ASSERT(original.target && original.isPartial && original.off == 0);
  count = ak_channelResolvedTargets(&context, channel, NULL, 0);
  ASSERT(count == 2);
  ASSERT(ak_channelResolvedTargets(&context, channel, resolved, 2) == 2);
  ASSERT(resolved[0].target == original.target);
  ASSERT(resolved[1].target && resolved[1].target != resolved[0].target);
  ASSERT(ak_typeid(resolved[0].target) == AKT_MORPH_INST);
  ASSERT(ak_typeid(resolved[1].target) == AKT_MORPH_INST);
  ASSERT(((AkInstanceMorph *)resolved[0].target)->morph
         == ((AkInstanceMorph *)resolved[1].target)->morph);
  ASSERT(resolved[0].off == 0 && resolved[1].off == 0);
  ASSERT(resolved[0].isPartial && resolved[1].isPartial);

  /* glTF weight channels are node-local even when two instances happen to
   * share an AkMorph object. The document provenance is the boundary. */
  originalType    = doc->inf->ftype;
  doc->inf->ftype = AK_FILE_TYPE_GLTF;
  ASSERT(ak_channelResolvedTargets(&context, channel, resolved, 2) == 1);
  ASSERT(resolved[0].target == original.target);
  doc->inf->ftype = originalType;

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  animation   = roundTrip->lib.animations.first;
  channel     = animation ? animation->channel : NULL;
  context.doc = roundTrip;
  ASSERT(channel && ak_channelResolvedTargets(&context, channel, resolved, 2) == 2);
  ASSERT(resolved[0].target != resolved[1].target);
  ASSERT(roundTrip->animationClips.count == 1);
  ASSERT(ak_animationClipContainsAnimation(roundTrip->animationClips.first,
                                           animation));

  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);
  TEST_SUCCESS
}

TEST_IMPL(dae_morph_on_morph_instance_fails_closed) {
  AkDoc       *doc;
  AkNode      *unsupportedNode, *supportedNode;
  AkTreeNode  *marker;
  char         dirTemplate[PATH_MAX];
  char        *tmpdir;
  char         daePath[PATH_MAX];
  char         outDir[PATH_MAX];
  char         outDae[PATH_MAX];
  const char  *tmpBase;

  doc = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";
  ASSERT(ak_test_path_join(dirTemplate, sizeof(dirTemplate), tmpBase,
                           "assetkit-dae-morph-chain-XXXXXX"));
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  ASSERT(ak_test_path_join(daePath, sizeof(daePath), tmpdir, "chain.dae"));
  ASSERT(ak_test_path_join(outDir, sizeof(outDir), tmpdir, "out"));
  ASSERT(ak_test_path_join(outDae, sizeof(outDae), outDir, "chain.dae"));
  ASSERT(ak_test_write_dae_shared_morph_animation(daePath));
  ASSERT(ak_test_rewrite_dae_morph_on_morph(daePath));
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);

  unsupportedNode = ak_getObjectById(doc, "mesh-a");
  supportedNode   = ak_getObjectById(doc, "mesh-b");
  ASSERT(unsupportedNode && supportedNode);
  ASSERT(unsupportedNode->geometry == NULL);
  ASSERT(supportedNode->geometry && supportedNode->geometry->morpher);
  marker = unsupportedNode->extra ? unsupportedNode->extra->chld : NULL;
  ASSERT(marker && marker->name && marker->val);
  ASSERT(strcmp(marker->name,
                "assetkit_unsupported_controller_chain") == 0);
  ASSERT(strcmp(marker->val, "nested-morph") == 0);

  ASSERT(ak_export(doc, outDir, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae,
    "<assetkit_unsupported_controller_chain>nested-morph"
    "</assetkit_unsupported_controller_chain>"));

  ak_free(doc);
  unlink(outDae);
  rmdir(outDir);
  unlink(daePath);
  rmdir(tmpdir);
  TEST_SUCCESS
}

TEST_IMPL(morph_inspect_preserves_missing_primitive_slots) {
  static const float basePositions[9] = {
    0, 0, 0, 1, 0, 0, 0, 1, 0
  };
  static const float targetPositions[9] = {
    0, 0, 1, 1, 0, 1, 0, 1, 2
  };
  AkHeap                   *heap;
  AkDoc                    *doc;
  AkGeometry               *baseGeom, *targetGeom;
  AkMesh                   *baseMesh, *targetMesh;
  AkMeshPrimitive          *baseFirst, *targetValid;
  AkTriangles              *baseSecond, *targetMissing;
  AkMorph                  *morph;
  AkMorphTarget            *target;
  AkObject                 *wrap;
  AkMorphInspectView       *view;
  AkMorphInspectMorphable  *first, *second;
  AkInputSemantic           desired[1] = {AK_INPUT_POSITION};

  heap = ak_heap_new(NULL, NULL, NULL);
  doc  = ak_heap_calloc(heap, NULL, sizeof(*doc));
  ak_heap_setdata(heap, doc);

  baseGeom  = ak_test_make_triangle_geom(heap, doc, basePositions);
  targetGeom = ak_test_make_triangle_geom(heap, doc, targetPositions);
  ASSERT(baseGeom && targetGeom);
  baseMesh   = ak_objGet(baseGeom->gdata);
  targetMesh = ak_objGet(targetGeom->gdata);
  baseFirst  = baseMesh->primitive;
  targetValid = targetMesh->primitive;

  baseSecond = ak_heap_calloc(heap, baseGeom->gdata, sizeof(*baseSecond));
  baseSecond->base.mesh  = baseMesh;
  baseSecond->base.pos   = baseFirst->pos;
  baseSecond->base.input = baseFirst->input;
  baseSecond->base.type  = AK_PRIMITIVE_TRIANGLES;
  baseSecond->mode       = AK_TRIANGLES;
  baseFirst->next        = &baseSecond->base;
  baseMesh->primitiveCount = 2;

  targetMissing = ak_heap_calloc(heap,
                                 targetGeom->gdata,
                                 sizeof(*targetMissing));
  targetMissing->base.mesh = targetMesh;
  targetMissing->base.type = AK_PRIMITIVE_TRIANGLES;
  targetMissing->mode      = AK_TRIANGLES;
  targetMissing->base.next = targetValid;
  targetMesh->primitive    = &targetMissing->base;
  targetMesh->primitiveCount = 2;

  morph  = ak_heap_calloc(heap, doc, sizeof(*morph));
  target = ak_heap_calloc(heap, morph, sizeof(*target));
  wrap   = ak_objAlloc(heap,
                       target,
                       sizeof(AkGeometry *),
                       AK_MORPHABLE_GEOMETRY,
                       true);
  ak_objGetTarget(wrap) = targetGeom;
  target->target         = wrap;
  target->primitiveCount = 2;
  morph->target          = target;
  morph->targetCount     = 1;
  morph->method          = AK_MORPH_METHOD_NORMALIZED;

  ASSERT(ak_morphInspect(baseGeom, morph, desired, 1, false, true) == AK_OK);
  view = morph->inspectResult;
  ASSERT(view && view->nTargets == 1 && view->targets);
  ASSERT(view->base && view->base->nTargets == 2);
  ASSERT(view->targets->nTargets == 2);
  first  = ak_test_morphable_at(view->targets, 0);
  second = ak_test_morphable_at(view->targets, 1);
  ASSERT(first && second && first->next == second);
  ASSERT(first->vertexCount == 3 && first->input == NULL);
  ASSERT(second->vertexCount == 3 && second->input);
  ASSERT(second->input->input
         && second->input->input->semantic == AK_INPUT_POSITION);
  ASSERT(view->targets->interleaveBufferSize == sizeof(targetPositions));

  ak_free(doc);
  TEST_SUCCESS
}
