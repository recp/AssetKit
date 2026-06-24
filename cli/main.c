/*
 * Copyright (C) 2026 Recep Aslantas
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
#include <ak/version.h>

#include <ctype.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef AK_BUILD_EXPORTERS
#  define AK_BUILD_EXPORTERS 0
#endif

#ifndef AK_CLI_STATIC_ASSETKIT
#  define AK_CLI_STATIC_ASSETKIT 0
#endif

#define AK_CLI_HOME   "https://github.com/recp/assetkit"
#define AK_CLI_ISSUES "https://github.com/recp/assetkit/issues"

#if AK_CLI_STATIC_ASSETKIT
void ak__init(void);
void ak__cleanup(void);
#endif

typedef enum AkCliAction {
  AK_CLI_ACTION_NONE = 0,
  AK_CLI_ACTION_CONVERT,
  AK_CLI_ACTION_INSPECT
} AkCliAction;

typedef enum AkCliReportFormat {
  AK_CLI_REPORT_AUTO = 0,
  AK_CLI_REPORT_TEXT,
  AK_CLI_REPORT_JSON
} AkCliReportFormat;

typedef struct AkCliConvertOptions {
  const char *input;
  const char *output;
  const char *format;
  const char *assetVersion;
  int         forceBinary;
  int         forceAscii;
} AkCliConvertOptions;

typedef struct AkCliInspectOptions {
  const char        *input;
  const char        *output;
  AkCliReportFormat reportFormat;
} AkCliInspectOptions;

static int
ak_cli_streq(const char *a, const char *b) {
  return a && b && strcmp(a, b) == 0;
}

static int
ak_cli_strieq(const char *a, const char *b) {
  unsigned char ca;
  unsigned char cb;

  if (!a || !b)
    return 0;

  while (*a && *b) {
    ca = (unsigned char)*a++;
    cb = (unsigned char)*b++;
    if (tolower(ca) != tolower(cb))
      return 0;
  }

  return *a == '\0' && *b == '\0';
}

static const char*
ak_cli_arg_value(const char *arg, const char *name) {
  size_t len;

  if (!arg || !name)
    return NULL;

  len = strlen(name);
  if (strncmp(arg, name, len) == 0 && arg[len] == '=')
    return arg + len + 1u;

  return NULL;
}

static void
ak_cli_print_usage(FILE *out) {
  fprintf(out,
          "AssetKit %s\n"
          "\n"
          "Fast 3D asset inspection and conversion tool.\n"
          "Reads and writes scene, mesh, material, animation and print formats.\n"
          "\n"
          "Home:   " AK_CLI_HOME "\n"
          "Issues: " AK_CLI_ISSUES "\n"
          "\n"
          "Usage:\n"
          "  assetkit --convert <input> <output> [options]\n"
          "  assetkit --inspect <input> [output] [options]\n"
          "\n"
          "Aliases:\n"
          "  -c, --convert    Convert an asset\n"
          "  -i, --inspect    Inspect an asset\n"
          "  -h, --help       Show help\n"
          "      --version    Show AssetKit version\n"
          "\n"
          "Convert options:\n"
          "      --format <fmt>          Use when output has no extension, or force this format\n"
          "      --bin                   Binary output when supported: stl, ply, glb\n"
          "      --ascii                 Text output when supported: stl, ply, gltf\n"
          "      --asset-version <ver>   Output asset version: auto, 1.4.1, 1.5.0, 2.0, 2.1\n"
          "\n"
          "Inspect options:\n"
          "      --json                  JSON report\n"
          "      --text                  Text report\n"
          "  -o, --output <path>         Write report to file\n\n",
          AK_VERSION_STRING);
}

static void
ak_cli_print_version(FILE *out) {
  fprintf(out,
          " AssetKit %s\n\n"
          " Home:   " AK_CLI_HOME "\n"
          " Issues: " AK_CLI_ISSUES "\n",
          AK_VERSION_STRING);
}

static const char*
ak_cli_file_name(const char *path) {
  const char *slash;
  const char *backslash;

  if (!path)
    return NULL;

  slash     = strrchr(path, '/');
  backslash = strrchr(path, '\\');
  if (!slash || (backslash && backslash > slash))
    slash = backslash;

  return slash ? slash + 1 : path;
}

static const char*
ak_cli_extension(const char *path) {
  const char *base;
  const char *dot;

  base = ak_cli_file_name(path);
  if (!base)
    return NULL;

  dot = strrchr(base, '.');
  if (!dot || dot == base || dot[1] == '\0')
    return NULL;

  return dot + 1;
}

static AkFileType
ak_cli_file_type_from_name(const char *name) {
  if (!name || !*name)
    return AK_FILE_TYPE_AUTO;

  if (ak_cli_strieq(name, "dae") || ak_cli_strieq(name, "collada"))
    return AK_FILE_TYPE_DAE;
  if (ak_cli_strieq(name, "gltf"))
    return AK_FILE_TYPE_GLTF;
  if (ak_cli_strieq(name, "glb"))
    return AK_FILE_TYPE_GLB;
  if (ak_cli_strieq(name, "obj") || ak_cli_strieq(name, "wavefront"))
    return AK_FILE_TYPE_OBJ;
  if (ak_cli_strieq(name, "stl"))
    return AK_FILE_TYPE_STL;
  if (ak_cli_strieq(name, "ply"))
    return AK_FILE_TYPE_PLY;
  if (ak_cli_strieq(name, "3mf"))
    return AK_FILE_TYPE_3MF;

  return AK_FILE_TYPE_AUTO;
}

static const char*
ak_cli_file_type_name(AkFileType type) {
  switch (type) {
    case AK_FILE_TYPE_DAE:
      return "dae";
    case AK_FILE_TYPE_GLTF:
      return "gltf";
    case AK_FILE_TYPE_GLB:
      return "glb";
    case AK_FILE_TYPE_OBJ:
      return "obj";
    case AK_FILE_TYPE_STL:
      return "stl";
    case AK_FILE_TYPE_PLY:
      return "ply";
    case AK_FILE_TYPE_3MF:
      return "3mf";
    case AK_FILE_TYPE_X3D:
      return "x3d";
    case AK_FILE_TYPE_USD:
      return "usd";
    case AK_FILE_TYPE_ALEMBIC:
      return "abc";
    case AK_FILE_TYPE_AUTO:
    default:
      return "auto";
  }
}

static int
ak_cli_parse_asset_version(AkFileType type,
                           const char *version,
                           uintptr_t *option,
                           uintptr_t *value) {
  if (!version || !*version || ak_cli_strieq(version, "auto")) {
    if (type == AK_FILE_TYPE_DAE) {
      *option = AK_OPT_DAE_EXPORT_VERSION;
      *value  = AK_DAE_EXPORT_VERSION_AUTO;
    } else if (type == AK_FILE_TYPE_GLTF || type == AK_FILE_TYPE_GLB) {
      *option = AK_OPT_GLTF_EXPORT_VERSION;
      *value  = AK_GLTF_EXPORT_VERSION_AUTO;
    } else {
      *option = (uintptr_t)-1;
      *value  = 0;
    }
    return 1;
  }

  if (type == AK_FILE_TYPE_DAE) {
    *option = AK_OPT_DAE_EXPORT_VERSION;
    if (ak_cli_streq(version, "1.4")
        || ak_cli_streq(version, "1.4.0")
        || ak_cli_streq(version, "1.4.1")) {
      *value = AK_DAE_EXPORT_VERSION_1_4;
      return 1;
    }
    if (ak_cli_streq(version, "1.5") || ak_cli_streq(version, "1.5.0")) {
      *value = AK_DAE_EXPORT_VERSION_1_5;
      return 1;
    }
  } else if (type == AK_FILE_TYPE_GLTF || type == AK_FILE_TYPE_GLB) {
    *option = AK_OPT_GLTF_EXPORT_VERSION;
    if (ak_cli_streq(version, "2.0")) {
      *value = AK_GLTF_EXPORT_VERSION_2_0;
      return 1;
    }
    if (ak_cli_streq(version, "2.1")) {
      *value = AK_GLTF_EXPORT_VERSION_2_1;
      return 1;
    }
  }

  return 0;
}

static int
ak_cli_apply_convert_options(AkFileType type,
                             const AkCliConvertOptions *opts,
                             uintptr_t *saved_dae_version,
                             uintptr_t *saved_gltf_version,
                             uintptr_t *saved_stl_format,
                             uintptr_t *saved_ply_format,
                             uintptr_t *saved_authoring_tool) {
  uintptr_t option;
  uintptr_t value;

  *saved_dae_version    = ak_opt_get(AK_OPT_DAE_EXPORT_VERSION);
  *saved_gltf_version   = ak_opt_get(AK_OPT_GLTF_EXPORT_VERSION);
  *saved_stl_format     = ak_opt_get(AK_OPT_STL_EXPORT_FORMAT);
  *saved_ply_format     = ak_opt_get(AK_OPT_PLY_EXPORT_FORMAT);
  *saved_authoring_tool = ak_opt_get(AK_OPT_EXPORT_AUTHORING_TOOL);

  ak_opt_set(AK_OPT_EXPORT_AUTHORING_TOOL, (uintptr_t)AK_AUTHORING_TOOL);

  if (opts->forceBinary && opts->forceAscii) {
    fprintf(stderr, "error: --bin and --ascii cannot be used together\n");
    return 0;
  }

  if (opts->assetVersion) {
    if (!ak_cli_parse_asset_version(type, opts->assetVersion, &option, &value)) {
      fprintf(stderr,
              "error: unsupported asset version '%s' for %s export\n",
              opts->assetVersion,
              ak_cli_file_type_name(type));
      return 0;
    }
    if (option != (uintptr_t)-1)
      ak_opt_set((AkOption)option, value);
  } else {
    if (type == AK_FILE_TYPE_DAE)
      ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, AK_DAE_EXPORT_VERSION_AUTO);
    else if (type == AK_FILE_TYPE_GLTF || type == AK_FILE_TYPE_GLB)
      ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, AK_GLTF_EXPORT_VERSION_AUTO);
  }

  if (opts->forceBinary) {
    if (type == AK_FILE_TYPE_STL)
      ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, AK_STL_EXPORT_BINARY);
    else if (type == AK_FILE_TYPE_PLY)
      ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_BINARY_LITTLE);
    else if (type != AK_FILE_TYPE_GLB) {
      fprintf(stderr,
              "error: --bin is not supported for %s export\n",
              ak_cli_file_type_name(type));
      return 0;
    }
  }

  if (opts->forceAscii) {
    if (type == AK_FILE_TYPE_STL)
      ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, AK_STL_EXPORT_ASCII);
    else if (type == AK_FILE_TYPE_PLY)
      ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, AK_PLY_EXPORT_ASCII);
    else if (type != AK_FILE_TYPE_GLTF) {
      fprintf(stderr,
              "error: --ascii is not supported for %s export\n",
              ak_cli_file_type_name(type));
      return 0;
    }
  }

  return 1;
}

static void
ak_cli_restore_convert_options(uintptr_t saved_dae_version,
                               uintptr_t saved_gltf_version,
                               uintptr_t saved_stl_format,
                               uintptr_t saved_ply_format,
                               uintptr_t saved_authoring_tool) {
  ak_opt_set(AK_OPT_DAE_EXPORT_VERSION, saved_dae_version);
  ak_opt_set(AK_OPT_GLTF_EXPORT_VERSION, saved_gltf_version);
  ak_opt_set(AK_OPT_STL_EXPORT_FORMAT, saved_stl_format);
  ak_opt_set(AK_OPT_PLY_EXPORT_FORMAT, saved_ply_format);
  ak_opt_set(AK_OPT_EXPORT_AUTHORING_TOOL, saved_authoring_tool);
}

static int
ak_cli_convert(const AkCliConvertOptions *opts) {
  AkFileType  type;
  AkResult    result;
  uintptr_t   saved_dae_version;
  uintptr_t   saved_gltf_version;
  uintptr_t   saved_stl_format;
  uintptr_t   saved_ply_format;
  uintptr_t   saved_authoring_tool;

  if (!opts || !opts->input || !opts->output) {
    ak_cli_print_usage(stderr);
    return 2;
  }

  if (!AK_BUILD_EXPORTERS) {
    fprintf(stderr, "error: AssetKit was built without exporter support\n");
    return 1;
  }

  type = opts->format
         ? ak_cli_file_type_from_name(opts->format)
         : ak_cli_file_type_from_name(ak_cli_extension(opts->output));
  if (type == AK_FILE_TYPE_AUTO) {
    fprintf(stderr,
            "error: cannot infer output format; pass --format <fmt>\n");
    return 2;
  }

  if (!ak_cli_apply_convert_options(type,
                                    opts,
                                    &saved_dae_version,
                                    &saved_gltf_version,
                                    &saved_stl_format,
                                    &saved_ply_format,
                                    &saved_authoring_tool)) {
    return 2;
  }

  result = ak_convert(opts->input, opts->output, type);

  ak_cli_restore_convert_options(saved_dae_version,
                                 saved_gltf_version,
                                 saved_stl_format,
                                 saved_ply_format,
                                 saved_authoring_tool);

  if (result != AK_OK) {
    fprintf(stderr, "error: convert failed (result=%d)\n", result);
    return 1;
  }

  return 0;
}

static void
ak_cli_json_string(FILE *out, const char *value) {
  const unsigned char *p;

  fputc('"', out);
  if (value) {
    for (p = (const unsigned char *)value; *p; p++) {
      switch (*p) {
        case '"':
          fputs("\\\"", out);
          break;
        case '\\':
          fputs("\\\\", out);
          break;
        case '\b':
          fputs("\\b", out);
          break;
        case '\f':
          fputs("\\f", out);
          break;
        case '\n':
          fputs("\\n", out);
          break;
        case '\r':
          fputs("\\r", out);
          break;
        case '\t':
          fputs("\\t", out);
          break;
        default:
          if (*p < 0x20u)
            fprintf(out, "\\u%04x", (unsigned)*p);
          else
            fputc((int)*p, out);
          break;
      }
    }
  }
  fputc('"', out);
}

static void
ak_cli_print_feature_names(FILE *out, AkPrintFeatureFlags features) {
  int first;

  first = 1;
#define AK_CLI_PRINT_FEATURE(flag, name)                                      \
  do {                                                                        \
    if (features & (flag)) {                                                  \
      fprintf(out, "%s%s", first ? "" : ", ", (name));                      \
      first = 0;                                                              \
    }                                                                         \
  } while (0)

  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_CORE, "core");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_MATERIALS, "materials");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_PRODUCTION, "production");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_SLICE, "slice");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_BEAM_LATTICE, "beam_lattice");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_BOOLEAN, "boolean");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_DISPLACEMENT, "displacement");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_VOLUMETRIC, "volumetric");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_SECURE_CONTENT, "secure_content");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_PACKAGE, "package");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_THUMBNAIL, "thumbnail");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_TEXTURES, "textures");
  AK_CLI_PRINT_FEATURE(AK_PRINT_FEATURE_UNKNOWN, "unknown");

#undef AK_CLI_PRINT_FEATURE

  if (first)
    fputs("none", out);
}

static void
ak_cli_print_validation_names(FILE *out, AkPrintValidationFlags flags) {
  int first;

  first = 1;
#define AK_CLI_PRINT_VALIDATION(flag, name)                                   \
  do {                                                                        \
    if (flags & (flag)) {                                                     \
      fprintf(out, "%s%s", first ? "" : ", ", (name));                      \
      first = 0;                                                              \
    }                                                                         \
  } while (0)

  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_NON_MANIFOLD, "non_manifold");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_DEGENERATE_TRIANGLES, "degenerate_triangles");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_OPEN_BOUNDARY, "open_boundary");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_NEGATIVE_SCALE, "negative_scale");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_UNSUPPORTED_FEATURE, "unsupported_feature");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_LOSSY_IMPORT, "lossy_import");
  AK_CLI_PRINT_VALIDATION(AK_PRINT_VALIDATION_LOSSY_EXPORT, "lossy_export");

#undef AK_CLI_PRINT_VALIDATION

  if (first)
    fputs("none", out);
}

static void
ak_cli_inspect_text(FILE *out, const char *path, const AkDoc *doc) {
  const AkPrintDocument *print;

  print = doc ? ak_printDocument((AkDoc *)doc) : NULL;

  fprintf(out, "file: %s\n", path ? path : "(none)");
  fprintf(out, "format: %s\n",
          doc && doc->inf ? ak_cli_file_type_name(doc->inf->ftype) : "unknown");
  fprintf(out, "load_ms: %.3f\n", doc ? doc->loadMillis : 0.0f);
  fprintf(out, "scenes: %u\n", doc ? doc->lib.scenes.count : 0u);
  fprintf(out, "nodes: %u\n", doc ? doc->lib.nodes.count : 0u);
  fprintf(out, "geometries: %u\n", doc ? doc->lib.geometries.count : 0u);
  fprintf(out, "materials: %u\n", doc ? doc->lib.materials.count : 0u);
  fprintf(out, "material_variants: %u\n", doc ? doc->materialVariantCount : 0u);
  fprintf(out, "material_property_sets: %u\n",
          doc ? doc->materialProperties.count : 0u);
  fprintf(out, "images: %u\n", doc ? doc->lib.images.count : 0u);
  fprintf(out, "textures: %u\n", doc ? doc->lib.textures.count : 0u);
  fprintf(out, "samplers: %u\n", doc ? doc->lib.samplers.count : 0u);
  fprintf(out, "cameras: %u\n", doc ? doc->lib.cameras.count : 0u);
  fprintf(out, "lights: %u\n", doc ? doc->lib.lights.count : 0u);
  fprintf(out, "animations: %u\n", doc ? doc->lib.animations.count : 0u);
  fprintf(out, "skins: %u\n", doc ? doc->lib.skins.count : 0u);
  fprintf(out, "morphs: %u\n", doc ? doc->lib.morphs.count : 0u);
  fprintf(out, "buffers: %u\n", doc ? doc->lib.buffers.count : 0u);
  fprintf(out, "accessors: %u\n", doc ? doc->lib.accessors.count : 0u);

  if (print) {
    fputs("print_features: ", out);
    ak_cli_print_feature_names(out, print->features);
    fputc('\n', out);
    fputs("print_required_features: ", out);
    ak_cli_print_feature_names(out, print->requiredFeatures);
    fputc('\n', out);
    fputs("print_unsupported_features: ", out);
    ak_cli_print_feature_names(out, print->unsupportedFeatures);
    fputc('\n', out);
    fputs("print_validation: ", out);
    ak_cli_print_validation_names(out, print->validationFlags);
    fputc('\n', out);
    fprintf(out, "print_package_parts: %u\n", print->packagePartCount);
    fprintf(out, "print_build_items: %u\n", print->buildItemCount);
    fprintf(out, "print_objects: %u\n", print->objectCount);
    fprintf(out, "print_mesh_objects: %u\n", print->meshObjectCount);
    fprintf(out, "print_component_objects: %u\n", print->componentObjectCount);
    fprintf(out, "print_slice_stacks: %u\n", print->sliceStackCount);
    fprintf(out, "print_beam_lattices: %u\n", print->beamLatticeCount);
    fprintf(out, "print_boolean_shapes: %u\n", print->booleanShapeCount);
    fprintf(out, "print_displacement_meshes: %u\n", print->displacementMeshCount);
    fprintf(out, "print_volumetric_meshes: %u\n", print->volumetricMeshCount);
  }
}

static void
ak_cli_inspect_json(FILE *out, const char *path, const AkDoc *doc) {
  const AkPrintDocument *print;

  print = doc ? ak_printDocument((AkDoc *)doc) : NULL;

  fputs("{\n", out);
  fputs("  \"file\": ", out);
  ak_cli_json_string(out, path ? path : "");
  fputs(",\n", out);
  fprintf(out,
          "  \"format\": \"%s\",\n"
          "  \"load_ms\": %.3f,\n"
          "  \"scenes\": %u,\n"
          "  \"nodes\": %u,\n"
          "  \"geometries\": %u,\n"
          "  \"materials\": %u,\n"
          "  \"material_variants\": %u,\n"
          "  \"material_property_sets\": %u,\n"
          "  \"images\": %u,\n"
          "  \"textures\": %u,\n"
          "  \"samplers\": %u,\n"
          "  \"cameras\": %u,\n"
          "  \"lights\": %u,\n"
          "  \"animations\": %u,\n"
          "  \"skins\": %u,\n"
          "  \"morphs\": %u,\n"
          "  \"buffers\": %u,\n"
          "  \"accessors\": %u",
          doc && doc->inf ? ak_cli_file_type_name(doc->inf->ftype) : "unknown",
          doc ? doc->loadMillis : 0.0f,
          doc ? doc->lib.scenes.count : 0u,
          doc ? doc->lib.nodes.count : 0u,
          doc ? doc->lib.geometries.count : 0u,
          doc ? doc->lib.materials.count : 0u,
          doc ? doc->materialVariantCount : 0u,
          doc ? doc->materialProperties.count : 0u,
          doc ? doc->lib.images.count : 0u,
          doc ? doc->lib.textures.count : 0u,
          doc ? doc->lib.samplers.count : 0u,
          doc ? doc->lib.cameras.count : 0u,
          doc ? doc->lib.lights.count : 0u,
          doc ? doc->lib.animations.count : 0u,
          doc ? doc->lib.skins.count : 0u,
          doc ? doc->lib.morphs.count : 0u,
          doc ? doc->lib.buffers.count : 0u,
          doc ? doc->lib.accessors.count : 0u);

  if (print) {
    fprintf(out,
            ",\n"
            "  \"print\": {\n"
            "    \"features\": %u,\n"
            "    \"required_features\": %u,\n"
            "    \"unsupported_features\": %u,\n"
            "    \"validation_flags\": %u,\n"
            "    \"package_parts\": %u,\n"
            "    \"build_items\": %u,\n"
            "    \"objects\": %u,\n"
            "    \"mesh_objects\": %u,\n"
            "    \"component_objects\": %u,\n"
            "    \"slice_stacks\": %u,\n"
            "    \"beam_lattices\": %u,\n"
            "    \"boolean_shapes\": %u,\n"
            "    \"displacement_meshes\": %u,\n"
            "    \"volumetric_meshes\": %u\n"
            "  }",
            print->features,
            print->requiredFeatures,
            print->unsupportedFeatures,
            print->validationFlags,
            print->packagePartCount,
            print->buildItemCount,
            print->objectCount,
            print->meshObjectCount,
            print->componentObjectCount,
            print->sliceStackCount,
            print->beamLatticeCount,
            print->booleanShapeCount,
            print->displacementMeshCount,
            print->volumetricMeshCount);
  }

  fputs("\n}\n", out);
}

static int
ak_cli_inspect(const AkCliInspectOptions *opts) {
  AkCliReportFormat format;
  AkDoc            *doc;
  AkResult          result;
  FILE             *out;

  if (!opts || !opts->input) {
    ak_cli_print_usage(stderr);
    return 2;
  }

  format = opts->reportFormat;
  if (format == AK_CLI_REPORT_AUTO) {
    const char *ext = ak_cli_extension(opts->output);
    format = ext && ak_cli_strieq(ext, "json")
             ? AK_CLI_REPORT_JSON
             : AK_CLI_REPORT_TEXT;
  }

  doc = NULL;
  result = ak_load(&doc, opts->input, AK_FILE_TYPE_AUTO);
  if (result != AK_OK || !doc) {
    fprintf(stderr, "error: failed to load %s (result=%d)\n", opts->input, result);
    return 1;
  }

  out = stdout;
  if (opts->output) {
    out = fopen(opts->output, "wb");
    if (!out) {
      fprintf(stderr, "error: could not open %s for writing\n", opts->output);
      ak_free(doc);
      return 1;
    }
  }

  if (format == AK_CLI_REPORT_JSON)
    ak_cli_inspect_json(out, opts->input, doc);
  else
    ak_cli_inspect_text(out, opts->input, doc);

  if (out != stdout)
    fclose(out);
  ak_free(doc);

  return 0;
}

static int
ak_cli_parse(int argc,
             char **argv,
             AkCliAction *action,
             AkCliConvertOptions *convert,
             AkCliInspectOptions *inspect) {
  int i;

  *action = AK_CLI_ACTION_NONE;
  memset(convert, 0, sizeof(*convert));
  memset(inspect, 0, sizeof(*inspect));
  convert->assetVersion = "auto";
  inspect->reportFormat = AK_CLI_REPORT_AUTO;

  for (i = 1; i < argc; i++) {
    const char *arg;
    const char *value;

    arg = argv[i];

    if (ak_cli_streq(arg, "-h") || ak_cli_streq(arg, "--help") || ak_cli_streq(arg, "help")) {
      ak_cli_print_usage(stdout);
      return 1;
    }

    if (ak_cli_streq(arg, "--version")) {
      ak_cli_print_version(stdout);
      return 1;
    }

    if (ak_cli_streq(arg, "-c") || ak_cli_streq(arg, "--convert") || ak_cli_streq(arg, "convert")) {
      if (*action != AK_CLI_ACTION_NONE) {
        fprintf(stderr, "error: multiple actions passed\n");
        return -1;
      }
      *action = AK_CLI_ACTION_CONVERT;
      continue;
    }

    if (ak_cli_streq(arg, "-i") || ak_cli_streq(arg, "--inspect") || ak_cli_streq(arg, "inspect")) {
      if (*action != AK_CLI_ACTION_NONE) {
        fprintf(stderr, "error: multiple actions passed\n");
        return -1;
      }
      *action = AK_CLI_ACTION_INSPECT;
      continue;
    }

    if ((value = ak_cli_arg_value(arg, "--format")) != NULL) {
      convert->format = value;
      continue;
    }
    if (ak_cli_streq(arg, "--format")) {
      if (++i >= argc) {
        fprintf(stderr, "error: --format needs a value\n");
        return -1;
      }
      convert->format = argv[i];
      continue;
    }

    if (ak_cli_streq(arg, "--bin")) {
      convert->forceBinary = 1;
      continue;
    }

    if (ak_cli_streq(arg, "--ascii")) {
      convert->forceAscii = 1;
      continue;
    }

    if ((value = ak_cli_arg_value(arg, "--asset-version")) != NULL) {
      convert->assetVersion = value;
      continue;
    }
    if (ak_cli_streq(arg, "--asset-version")) {
      if (++i >= argc) {
        fprintf(stderr, "error: --asset-version needs a value\n");
        return -1;
      }
      convert->assetVersion = argv[i];
      continue;
    }

    if (ak_cli_streq(arg, "--json")) {
      if (inspect->reportFormat == AK_CLI_REPORT_TEXT) {
        fprintf(stderr, "error: --json and --text cannot be used together\n");
        return -1;
      }
      inspect->reportFormat = AK_CLI_REPORT_JSON;
      continue;
    }

    if (ak_cli_streq(arg, "--text")) {
      if (inspect->reportFormat == AK_CLI_REPORT_JSON) {
        fprintf(stderr, "error: --json and --text cannot be used together\n");
        return -1;
      }
      inspect->reportFormat = AK_CLI_REPORT_TEXT;
      continue;
    }

    if ((value = ak_cli_arg_value(arg, "--output")) != NULL) {
      inspect->output = value;
      continue;
    }
    if (ak_cli_streq(arg, "-o") || ak_cli_streq(arg, "--output")) {
      if (++i >= argc) {
        fprintf(stderr, "error: %s needs a value\n", arg);
        return -1;
      }
      inspect->output = argv[i];
      continue;
    }

    if (*action == AK_CLI_ACTION_CONVERT) {
      if (!convert->input)
        convert->input = arg;
      else if (!convert->output)
        convert->output = arg;
      else {
        fprintf(stderr, "error: unexpected argument: %s\n", arg);
        return -1;
      }
    } else if (*action == AK_CLI_ACTION_INSPECT) {
      if (!inspect->input)
        inspect->input = arg;
      else if (!inspect->output)
        inspect->output = arg;
      else {
        fprintf(stderr, "error: unexpected argument: %s\n", arg);
        return -1;
      }
    } else {
      fprintf(stderr, "error: action must be -c/--convert or -i/--inspect\n");
      return -1;
    }
  }

  if (*action == AK_CLI_ACTION_NONE) {
    fprintf(stderr, "error: action must be -c/--convert or -i/--inspect\n");
    return -1;
  }

  return 0;
}

int
main(int argc, char **argv) {
  AkCliAction         action;
  AkCliConvertOptions convert;
  AkCliInspectOptions inspect;
  int                 parsed;

#if AK_CLI_STATIC_ASSETKIT
  ak__init();
  atexit(ak__cleanup);
#endif

  if (argc <= 1) {
    ak_cli_print_usage(stderr);
    return 2;
  }

  parsed = ak_cli_parse(argc, argv, &action, &convert, &inspect);
  if (parsed > 0)
    return 0;
  if (parsed < 0) {
    ak_cli_print_usage(stderr);
    return 2;
  }

  switch (action) {
    case AK_CLI_ACTION_CONVERT:
      return ak_cli_convert(&convert);
    case AK_CLI_ACTION_INSPECT:
      return ak_cli_inspect(&inspect);
    case AK_CLI_ACTION_NONE:
    default:
      ak_cli_print_usage(stderr);
      return 2;
  }
}
