/*
 * Copyright (C) 2026 Recep Aslantas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 */

#include "../../../test_export_common.h"

static bool
ak_test_write_component_animation(FILE       *file,
                                  const char *id,
                                  const char *target,
                                  const char *values) {
  return fprintf(file,
                 "<animation id=\"%s\" name=\"%s\">"
                 "<source id=\"%s-input\"><float_array id=\"%s-input-array\" "
                 "count=\"3\">0 0.5 1</float_array><technique_common>"
                 "<accessor source=\"#%s-input-array\" count=\"3\" stride=\"1\">"
                 "<param name=\"TIME\" type=\"float\"/></accessor>"
                 "</technique_common></source>"
                 "<source id=\"%s-output\"><float_array id=\"%s-output-array\" "
                 "count=\"3\">%s</float_array><technique_common>"
                 "<accessor source=\"#%s-output-array\" count=\"3\" stride=\"1\">"
                 "<param name=\"ANGLE\" type=\"float\"/></accessor>"
                 "</technique_common></source>"
                 "<source id=\"%s-interp\"><Name_array id=\"%s-interp-array\" "
                 "count=\"3\">LINEAR LINEAR LINEAR</Name_array>"
                 "<technique_common><accessor source=\"#%s-interp-array\" "
                 "count=\"3\" stride=\"1\"><param name=\"INTERPOLATION\" "
                 "type=\"name\"/></accessor></technique_common></source>"
                 "<sampler id=\"%s-sampler\"><input semantic=\"INPUT\" "
                 "source=\"#%s-input\"/><input semantic=\"OUTPUT\" "
                 "source=\"#%s-output\"/><input semantic=\"INTERPOLATION\" "
                 "source=\"#%s-interp\"/></sampler><channel source=\"#%s-sampler\" "
                 "target=\"%s\"/></animation>",
                 id, id,
                 id, id, id,
                 id, id, values, id,
                 id, id, id,
                 id, id, id, id, id, target) > 0;
}

static bool
ak_test_write_dae_animation_clips(const char *path) {
  FILE *file;
  bool  ok;
  uint32_t i;

  file = fopen(path, "wb");
  if (!file)
    return false;

  ok = fputs("<?xml version=\"1.0\" encoding=\"utf-8\"?>"
             "<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" "
             "version=\"1.4.1\"><asset><unit name=\"meter\" meter=\"1\"/>"
             "<up_axis>Y_UP</up_axis></asset>"
             "<library_animation_clips>"
             "<animation_clip id=\"broken\" name=\"Broken\">"
             "<instance_animation url=\"#x\"/>"
             "<instance_animation url=\"#missing\"/></animation_clip>"
             "<animation_clip id=\"xy-clip\" name=\"XY Clip\" "
             "start=\"0.25\" end=\"0.75\">"
             "<instance_animation url=\"#x\"/>"
             "<instance_animation url=\"#y\"/></animation_clip>"
             "<animation_clip id=\"z-clip\" name=\"Z Clip\">"
             "<instance_animation url=\"#z\"/></animation_clip>"
             "<animation_clip id=\"wide-clip\" name=\"Wide Clip\">"
             "<instance_animation url=\"#wide-leaf\"/></animation_clip>"
             "</library_animation_clips><library_animations>", file) >= 0;
  ok = ok && ak_test_write_component_animation(file, "x", "root/rx.ANGLE", "0 30 0");
  ok = ok && ak_test_write_component_animation(file, "y", "root/ry.ANGLE", "0 -40 0");
  ok = ok && ak_test_write_component_animation(file, "z", "root/rz.ANGLE", "0 60 0");
  ok = ok && ak_test_write_component_animation(file, "u", "root/rz.ANGLE", "0 -25 0");
  /* Parsed DAE libraries are linked in reverse, so these empty entries appear
   * before the meaningful implicit member.  The bake walk must not mistake
   * its historical 256-channel capacity for a total-animation limit. */
  for (i = 0; ok && i < 300; i++)
    ok = fprintf(file, "<animation id=\"empty-%u\"/>", i) > 0;
  ok = ok && fputs("<animation id=\"wide-root\">", file) >= 0;
  for (i = 0; ok && i < 300; i++)
    ok = fprintf(file, "<animation id=\"wide-empty-%u\"/>", i) > 0;
  ok = ok && ak_test_write_component_animation(file,
                                                "wide-leaf",
                                                "root/rx.ANGLE",
                                                "0 15 0");
  ok = ok && fputs("</animation>", file) >= 0;
  ok = ok && fputs("</library_animations><library_visual_scenes>"
                   "<visual_scene id=\"Scene\"><node id=\"root\" name=\"Root\" "
                   "type=\"JOINT\"><rotate sid=\"rx\">1 0 0 0</rotate>"
                   "<rotate sid=\"ry\">0 1 0 0</rotate>"
                   "<rotate sid=\"rz\">0 0 1 0</rotate></node></visual_scene>"
                   "</library_visual_scenes><scene><instance_visual_scene "
                   "url=\"#Scene\"/></scene></COLLADA>", file) >= 0;

  return fclose(file) == 0 && ok;
}

static const float*
ak_test_baked_matrix_at(AkBakedAnimation *baked, float time) {
  uint32_t i;

  if (!baked)
    return NULL;
  for (i = 0; i < baked->count; i++) {
    if (fabsf(baked->times[i] - time) < 0.00001f)
      return baked->matrices + (size_t)i * 16u;
  }
  return NULL;
}

static bool
ak_test_matrix16_near(const float *actual, const float expected[16]) {
  uint32_t i;

  if (!actual)
    return false;
  for (i = 0; i < 16; i++) {
    if (fabsf(actual[i] - expected[i]) > 0.0002f)
      return false;
  }
  return true;
}

TEST_IMPL(dae_animation_clips_parse_and_bake_separately) {
  static const float expectedXY[16] = {
     0.76604444f, -0.32139380f,  0.55667040f, 0.0f,
     0.0f,         0.86602540f,  0.5f,        0.0f,
    -0.64278761f, -0.38302222f,  0.66341395f, 0.0f,
     0.0f,         0.0f,         0.0f,        1.0f
  };
  static const float expectedUnclipped[16] = {
     0.90630779f, -0.42261826f, 0.0f, 0.0f,
     0.42261826f,  0.90630779f, 0.0f, 0.0f,
     0.0f,         0.0f,        1.0f, 0.0f,
     0.0f,         0.0f,        0.0f, 1.0f
  };
  AkDoc            *doc;
  AkDoc            *roundTrip;
  AkAnimationClip  *xy, *z, *wide;
  AkAnimation      *animation, *wideRoot, *wideLeaf;
  AkNode           *root;
  AkBakedAnimation *xyBake, *zBake, *implicitBake;
  char              dirTemplate[PATH_MAX];
  char             *tmpdir;
  char              daePath[PATH_MAX];
  char              outDae[PATH_MAX];
  const char       *tmpBase;
  uintptr_t         previousCoordCvtType;
  float             start, end;

  doc       = NULL;
  roundTrip = NULL;
  tmpBase = getenv("TMPDIR");
  if (!tmpBase || !tmpBase[0])
    tmpBase = "/tmp";
  snprintf(dirTemplate, sizeof(dirTemplate),
           "%s/assetkit-dae-animation-clips-XXXXXX", tmpBase);
  tmpdir = mkdtemp(dirTemplate);
  ASSERT(tmpdir != NULL);
  snprintf(daePath, sizeof(daePath), "%s/clips.dae", tmpdir);
  snprintf(outDae, sizeof(outDae), "%s/clips-roundtrip.dae", tmpdir);
  ASSERT(ak_test_write_dae_animation_clips(daePath));

  previousCoordCvtType = ak_opt_get(AK_OPT_COORD_CONVERT_TYPE);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, AK_COORD_CVT_DISABLED);
  ASSERT(ak_load(&doc, daePath, AK_FILE_TYPE_DAE) == AK_OK && doc);
  ak_opt_set(AK_OPT_COORD_CONVERT_TYPE, previousCoordCvtType);

  ASSERT(doc->animationClips.count == 3);
  xy = doc->animationClips.first;
  z  = xy ? xy->next : NULL;
  wide = z ? z->next : NULL;
  ASSERT(xy && z && wide && wide->next == NULL);
  ASSERT(xy->name && strcmp(xy->name, "XY Clip") == 0);
  ASSERT(z->name && strcmp(z->name, "Z Clip") == 0);
  ASSERT(xy->memberCount == 2 && xy->members && xy->lastMember);
  ASSERT(xy->members->next == xy->lastMember);
  ASSERT(xy->members->animation != NULL);
  ASSERT(xy->lastMember->animation != NULL);
  ASSERT(strcmp(ak_getId(xy->members->animation), "x") == 0);
  ASSERT(strcmp(ak_getId(xy->lastMember->animation), "y") == 0);
  ASSERT(ak_animationClipTimeRange(xy, &start, &end));
  ASSERT(fabsf(start - 0.25f) < 0.00001f);
  ASSERT(fabsf(end - 0.75f) < 0.00001f);

  /* The referenced leaf sits after 300 sibling animations. This proves DAE
   * URL fixup, public membership, and derived range walks no longer truncate
   * at the historical fixed 256-entry stack. */
  wideRoot = ak_getObjectById(doc, "wide-root");
  wideLeaf = ak_getObjectById(doc, "wide-leaf");
  ASSERT(wideRoot && wideLeaf && wide->members);
  ASSERT(wide->members->animation == wideLeaf);
  ASSERT(ak_animationTimeRange(wideRoot, &start, &end));
  ASSERT(fabsf(start) < 0.00001f && fabsf(end - 1.0f) < 0.00001f);
  wide->members->animation = wideRoot;
  ASSERT(ak_animationClipContainsAnimation(wide, wideLeaf));
  wide->members->animation = wideLeaf;

  animation = ak_getObjectById(doc, "u");
  ASSERT(animation != NULL);
  ASSERT(!ak_animationIsClipped(doc, animation));

  root = ak_sceneFindRoot(doc->scene, "Root");
  ASSERT(root != NULL);
  xyBake       = ak_nodeBakeAnimationForClip(doc, root, xy);
  zBake        = ak_nodeBakeAnimationForClip(doc, root, z);
  implicitBake = ak_nodeBakeUnclippedAnimation(doc, root);
  ASSERT(xyBake && zBake && implicitBake);
  ASSERT(fabsf(xyBake->times[0] - 0.25f) < 0.00001f);
  ASSERT(fabsf(xyBake->times[xyBake->count - 1] - 0.75f) < 0.00001f);
  ASSERT(ak_test_matrix16_near(ak_test_baked_matrix_at(xyBake, 0.5f), expectedXY));
  ASSERT(ak_test_matrix16_near(ak_test_baked_matrix_at(implicitBake, 0.5f),
                               expectedUnclipped));
  ASSERT(!ak_test_matrix16_near(ak_test_baked_matrix_at(zBake, 0.5f),
                                expectedUnclipped));

  ASSERT(ak_exportFile(doc, outDae, AK_FILE_TYPE_DAE) == AK_OK);
  ASSERT(ak_test_file_contains(outDae, "<library_animation_clips>"));
  ASSERT(ak_test_file_contains(outDae, "name=\"XY Clip\" start=\"0.25\" end=\"0.75\""));
  ASSERT(ak_load(&roundTrip, outDae, AK_FILE_TYPE_DAE) == AK_OK && roundTrip);
  ASSERT(roundTrip->animationClips.count == 3);
  xy = roundTrip->animationClips.first;
  ASSERT(xy && xy->memberCount == 2 && xy->members && xy->lastMember);
  ASSERT(xy->members->next == xy->lastMember);
  ASSERT(xy->members->animation != NULL && xy->lastMember->animation != NULL);
  ASSERT(ak_animationClipTimeRange(xy, &start, &end));
  ASSERT(fabsf(start - 0.25f) < 0.00001f && fabsf(end - 0.75f) < 0.00001f);

  ak_free(xyBake);
  ak_free(zBake);
  ak_free(implicitBake);
  ak_free(roundTrip);
  ak_free(doc);
  unlink(outDae);
  unlink(daePath);
  rmdir(tmpdir);
  TEST_SUCCESS
}
