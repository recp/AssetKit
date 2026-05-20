/*
 * Small manual benchmark/regression helper for importer hot paths.
 *
 * Build with:
 *   cmake -S . -B build -DAK_BUILD_TOOLS=ON
 *   cmake --build build --target assetkit_import_stats
 *
 * It prints one TSV row per input file with timing and index-buffer shape.
 * This intentionally stays out of the normal test target because corpus paths
 * are machine-local and benchmark numbers are not stable enough for pass/fail.
 */

#include <ak/assetkit.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ImportStats {
  uint64_t primitives;
  uint64_t points;
  uint64_t lines;
  uint64_t triangles;
  uint64_t polygons;
  uint64_t owned;
  uint64_t accessor;
  uint64_t u8;
  uint64_t u16;
  uint64_t u32;
  uint64_t ownedBytes;
  uint64_t indexCount;
} ImportStats;

static double
stats_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static int
stats_cmp_double(const void *a, const void *b) {
  double da;
  double db;

  da = *(const double *)a;
  db = *(const double *)b;
  return (da > db) - (da < db);
}

static void
stats_collect(AkDoc *doc, ImportStats *stats) {
  AkGeometry *geom;

  memset(stats, 0, sizeof(*stats));

  for (geom = ak_libFirstGeom(doc); geom; geom = (AkGeometry *)geom->base.next) {
    AkMesh *mesh;
    AkMeshPrimitive *prim;

    if (!geom->gdata)
      continue;

    mesh = ak_objGet(geom->gdata);
    if (!mesh)
      continue;

    for (prim = mesh->primitive; prim; prim = prim->next) {
      AkTypeId componentType;
      size_t count;

      componentType = ak_meshPrimitiveIndexComponentType(prim);
      count         = ak_meshPrimitiveIndexCount(prim);

      stats->primitives++;
      stats->indexCount += count;

      switch (prim->type) {
        case AK_PRIMITIVE_POINTS:    stats->points++;    break;
        case AK_PRIMITIVE_LINES:     stats->lines++;     break;
        case AK_PRIMITIVE_TRIANGLES: stats->triangles++; break;
        case AK_PRIMITIVE_POLYGONS:  stats->polygons++;  break;
        default:                                      break;
      }

      if (prim->indices) {
        stats->owned++;
        stats->ownedBytes += count * ak_indexComponentSize(componentType);
      }

      if (prim->indexAccessor)
        stats->accessor++;

      switch (componentType) {
        case AKT_UBYTE:  stats->u8++;  break;
        case AKT_USHORT: stats->u16++; break;
        case AKT_UINT:   stats->u32++; break;
        default:                    break;
      }
    }
  }
}

static bool
stats_load_once(const char *path, ImportStats *stats, double *elapsedMs) {
  AkDoc *doc;
  AkResult result;
  double start;

  doc   = NULL;
  start = stats_now_ms();
  result = ak_load(&doc, path, AK_FILE_TYPE_AUTO);
  if (elapsedMs)
    *elapsedMs = stats_now_ms() - start;

  if (result != AK_OK || !doc)
    return false;

  if (stats)
    stats_collect(doc, stats);

  ak_free(doc);
  return true;
}

static const char *
stats_base_name(const char *path) {
  const char *last;

  last = strrchr(path, '/');
  return last ? last + 1 : path;
}

static bool
stats_bench_path(const char *path, int iterations, int warmup) {
  double *times;
  double sum;
  double minv;
  double maxv;
  ImportStats stats;
  int i;

  for (i = 0; i < warmup; i++) {
    if (!stats_load_once(path, NULL, NULL)) {
      fprintf(stderr, "load failed during warmup: %s\n", path);
      return false;
    }
  }

  times = calloc((size_t)iterations, sizeof(*times));
  if (!times)
    return false;

  sum  = 0.0;
  minv = 1.0e100;
  maxv = 0.0;

  for (i = 0; i < iterations; i++) {
    if (!stats_load_once(path, NULL, &times[i])) {
      fprintf(stderr, "load failed: %s\n", path);
      free(times);
      return false;
    }
    sum += times[i];
    if (times[i] < minv)
      minv = times[i];
    if (times[i] > maxv)
      maxv = times[i];
  }

  qsort(times, (size_t)iterations, sizeof(*times), stats_cmp_double);

  if (!stats_load_once(path, &stats, NULL)) {
    fprintf(stderr, "load failed during stats: %s\n", path);
    free(times);
    return false;
  }

  printf("%s\t%d\t%.3f\t%.3f\t%.3f\t%.3f\t%" PRIu64 "\t%" PRIu64
         "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
         "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
         "\t%" PRIu64 "\t%" PRIu64 "\t%s\n",
         stats_base_name(path),
         iterations,
         minv,
         sum / (double)iterations,
         times[iterations / 2],
         maxv,
         stats.primitives,
         stats.points,
         stats.lines,
         stats.triangles,
         stats.polygons,
         stats.owned,
         stats.accessor,
         stats.u8,
         stats.u16,
         stats.u32,
         stats.indexCount,
         stats.ownedBytes,
         path);

  free(times);
  return true;
}

int
main(int argc, char **argv) {
  int iterations;
  int warmup;
  int firstPath;
  int i;
  bool ok;

  iterations = 7;
  warmup     = 2;
  firstPath  = 1;
  ok         = true;

  while (firstPath < argc) {
    if (strcmp(argv[firstPath], "-n") == 0 && firstPath + 1 < argc) {
      iterations = atoi(argv[firstPath + 1]);
      firstPath += 2;
    } else if (strcmp(argv[firstPath], "-w") == 0 && firstPath + 1 < argc) {
      warmup = atoi(argv[firstPath + 1]);
      firstPath += 2;
    } else {
      break;
    }
  }

  if (iterations <= 0 || warmup < 0 || firstPath >= argc) {
    fprintf(stderr, "usage: %s [-n iterations] [-w warmup] file...\n", argv[0]);
    return 2;
  }

  printf("file\titers\tmin_ms\tavg_ms\tmedian_ms\tmax_ms\tprims\tpoints"
         "\tlines\ttriangles\tpolygons\towned\taccessor\tu8\tu16\tu32"
         "\tindices\towned_bytes\tpath\n");

  for (i = firstPath; i < argc; i++)
    ok &= stats_bench_path(argv[i], iterations, warmup);

  return ok ? 0 : 1;
}
