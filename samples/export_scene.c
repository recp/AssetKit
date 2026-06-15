/*
 * Minimal import -> export -> reload sample.
 *
 * Usage:
 *   sample_export_scene model.gltf out-dir gltf
 *   sample_export_scene model.gltf out-dir glb
 *   sample_export_scene model.gltf out-dir obj
 *   sample_export_scene model.gltf out-dir stl-ascii
 *   sample_export_scene model.gltf out-dir ply-ascii
 *   sample_export_scene model.gltf out-dir 3mf
 *
 * The exporter options are global AssetKit options. The sample snapshots and
 * restores the ones it touches so embedding applications can use the same
 * pattern around a single export call.
 */

#include "sample_common.h"

#include <ak/options.h>

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#  include <direct.h>
#  define sample_mkdir(path) _mkdir(path)
#else
#  include <sys/stat.h>
#  include <sys/types.h>
#  define sample_mkdir(path) mkdir(path, 0775)
#endif

#define SAMPLE_PATH_CAP 4096u

typedef struct SampleExportFormat {
  const char *name;
  const char *ext;
  AkFileType  file_type;
} SampleExportFormat;

typedef struct SampleExportOptions {
  uintptr_t dae_index_mode;
  uintptr_t dae_version;
  uintptr_t stl_format;
  uintptr_t ply_format;
  uintptr_t ply_normals;
  uintptr_t ply_uv;
  uintptr_t ply_color_mode;
  uintptr_t ply_triangulated;
  uintptr_t authoring_tool;
} SampleExportOptions;

static int
sample_find_format(const char *name, SampleExportFormat *out) {
  static const SampleExportFormat formats[] = {
    {"dae",       "dae",  AK_FILE_TYPE_DAE},
    {"dae-single", "dae", AK_FILE_TYPE_DAE},
    {"gltf",      "gltf", AK_FILE_TYPE_GLTF},
    {"glb",       "glb",  AK_FILE_TYPE_GLB},
    {"obj",       "obj",  AK_FILE_TYPE_OBJ},
    {"stl",       "stl",  AK_FILE_TYPE_STL},
    {"stl-ascii", "stl",  AK_FILE_TYPE_STL},
    {"ply",       "ply",  AK_FILE_TYPE_PLY},
    {"ply-ascii", "ply",  AK_FILE_TYPE_PLY},
    {"3mf",       "3mf",  AK_FILE_TYPE_3MF}
  };
  size_t i;

  for (i = 0; i < sizeof(formats) / sizeof(formats[0]); i++) {
    if (strcmp(name, formats[i].name) == 0) {
      *out = formats[i];
      return 1;
    }
  }

  return 0;
}

static void
sample_save_export_options(SampleExportOptions *saved) {
  saved->dae_index_mode  = ak_opt_get(AK_OPT_DAE_EXPORT_INDEX_MODE);
  saved->dae_version     = ak_opt_get(AK_OPT_DAE_EXPORT_VERSION);
  saved->stl_format      = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);
  saved->ply_format      = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  saved->ply_normals     = ak_opt_get(AK_OPT_PLY_EXPORT_NORMALS);
  saved->ply_uv          = ak_opt_get(AK_OPT_PLY_EXPORT_UV);
  saved->ply_color_mode  = ak_opt_get(AK_OPT_PLY_EXPORT_COLOR_MODE);
  saved->ply_triangulated = ak_opt_get(AK_OPT_PLY_EXPORT_TRIANGULATED);
  saved->authoring_tool  = ak_opt_get(AK_OPT_EXPORT_AUTHORING_TOOL);
}

static void
sample_restore_export_options(const SampleExportOptions *saved) {
  ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, saved->dae_index_mode);
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, saved->dae_version);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, saved->stl_format);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, saved->ply_format);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, saved->ply_normals);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, saved->ply_uv);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, saved->ply_color_mode);
  ak_opt_set(AK_OPT_PLY_EXPORT_TRIANGULATED, saved->ply_triangulated);
  ak_opt_set(AK_OPT_EXPORT_AUTHORING_TOOL, saved->authoring_tool);
}

static void
sample_apply_export_options(const char *format_name) {
  ak_opt_set(AK_OPT_EXPORT_AUTHORING_TOOL, (uintptr_t)"sample_export_scene");

  if (strcmp(format_name, "dae-single") == 0) {
    ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, AK_DAE_EXPORT_INDEX_SINGLE);
    ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, AK_DAE_EXPORT_VERSION_AUTO);
  } else {
    ak_opt_set(AK_OPT_DAE_EXPORT_INDEX_MODE, AK_DAE_EXPORT_INDEX_MULTI);
    ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, AK_DAE_EXPORT_VERSION_AUTO);
  }

  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT,
             strcmp(format_name, "stl-ascii") == 0
             ? AK_STL_EXPORT_ASCII
             : AK_STL_EXPORT_BINARY);

  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT,
             strcmp(format_name, "ply-ascii") == 0
             ? AK_PLY_EXPORT_ASCII
             : AK_PLY_EXPORT_BINARY_LITTLE);
  ak_opt_set(AK_OPT_PLY_EXPORT_NORMALS, true);
  ak_opt_set(AK_OPT_PLY_EXPORT_UV, true);
  ak_opt_set(AK_OPT_PLY_EXPORT_COLOR_MODE, AK_PLY_EXPORT_COLOR_SRGB);
  ak_opt_set(AK_OPT_PLY_EXPORT_TRIANGULATED, false);
}

static const char *
sample_basename(const char *path) {
  const char *slash;
  const char *backslash;

  slash = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (!slash || (backslash && backslash > slash))
    slash = backslash;
  return slash ? slash + 1 : path;
}

static int
sample_expected_output_path(char *out,
                            size_t out_cap,
                            const char *out_dir,
                            const char *input_path,
                            const char *ext) {
  const char *base;
  const char *dot;
  size_t stem_len;
  size_t dir_len;
  int written;

  base = sample_basename(input_path);
  dot = strrchr(base, '.');
  stem_len = dot && dot > base ? (size_t)(dot - base) : strlen(base);
  dir_len = strlen(out_dir);

  written = snprintf(out,
                     out_cap,
                     "%s%s%.*s.%s",
                     out_dir,
                     dir_len && (out_dir[dir_len - 1u] == '/' || out_dir[dir_len - 1u] == '\\')
                     ? ""
                     : "/",
                     (int)stem_len,
                     base,
                     ext);
  return written > 0 && (size_t)written < out_cap;
}

static void
sample_print_doc_summary(const char *label, const AkDoc *doc) {
  printf("%s: scenes=%u geometries=%u materials=%u images=%u animations=%u skins=%u morphs=%u\n",
         label,
         doc->lib.scenes.count,
         doc->lib.geometries.count,
         doc->lib.materials.count,
         doc->lib.images.count,
         doc->lib.animations.count,
         doc->lib.skins.count,
         doc->lib.morphs.count);
}

static int
sample_ensure_dir(const char *path) {
  if (sample_mkdir(path) == 0 || errno == EEXIST)
    return 1;

  fprintf(stderr, "could not create output directory %s (errno=%d)\n", path, errno);
  return 0;
}

static void
sample_usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s input-model output-dir format\n"
          "formats: dae, dae-single, gltf, glb, obj, stl, stl-ascii, ply, ply-ascii, 3mf\n",
          argv0);
}

int
main(int argc, char **argv) {
  SampleExportFormat format;
  SampleExportOptions saved;
  AkDoc *doc;
  AkDoc *round_trip;
  AkResult export_result;
  char expected[SAMPLE_PATH_CAP];

  if (argc != 4) {
    sample_usage(argv[0]);
    return 2;
  }

  if (!sample_find_format(argv[3], &format)) {
    sample_usage(argv[0]);
    return 2;
  }

  if (!sample_ensure_dir(argv[2]))
    return 1;

  if (!sample_expected_output_path(expected, sizeof(expected), argv[2], argv[1], format.ext)) {
    fprintf(stderr, "expected output path is too long\n");
    return 1;
  }

  if (!sample_load_doc(&doc, argv[1]))
    return 1;

  sample_print_doc_summary("loaded", doc);

  sample_save_export_options(&saved);
  sample_apply_export_options(argv[3]);
  export_result = ak_export(doc, argv[2], format.file_type);
  sample_restore_export_options(&saved);

  printf("export: format=%s result=%d expected_main=%s\n",
         argv[3],
         export_result,
         expected);
  if (export_result != AK_OK) {
    ak_free(doc);
    return 1;
  }

  round_trip = NULL;
  if (ak_load(&round_trip, expected, AK_FILE_TYPE_AUTO) == AK_OK && round_trip) {
    sample_print_doc_summary("roundtrip", round_trip);
    ak_free(round_trip);
  } else {
    printf("roundtrip: skipped or failed for %s\n", expected);
  }

  ak_free(doc);
  return 0;
}
