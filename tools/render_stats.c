/*
 * Manual benchmark for the optional static render-batch view.
 *
 * Build with:
 *   cmake -S . -B build -DAK_BUILD_TOOLS=ON
 *   cmake --build build --target assetkit_render_stats
 */

#include <ak/assetkit.h>

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static
double
render_stats_now_ms(void) {
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static
int
render_stats_compare(const void *left, const void *right) {
  double a, b;

  a = *(const double *)left;
  b = *(const double *)right;
  return (a > b) - (a < b);
}

int
main(int argc, char **argv) {
  const char *path;
  double     *times;
  int         iterations, i;
  uint32_t    groups, batches, included, skipped;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "usage: %s file [iterations]\n", argv[0]);
    return 2;
  }

  path       = argv[1];
  iterations = argc == 3 ? atoi(argv[2]) : 7;
  if (iterations <= 0)
    return 2;

  times = calloc((size_t)iterations, sizeof(*times));
  if (!times)
    return 1;

  groups = batches = included = skipped = 0;
  for (i = 0; i < iterations; i++) {
    AkSceneRenderData *renderData;
    AkDoc             *doc;
    AkResult           result;
    double             start;

    doc = NULL;
    if (ak_load(&doc, path, AK_FILE_TYPE_AUTO) != AK_OK || !doc || !doc->scene) {
      fprintf(stderr, "load failed: %s\n", path);
      free(times);
      return 1;
    }

    renderData = NULL;
    start      = render_stats_now_ms();
    result     = ak_sceneBuildRenderData(doc->scene, &renderData);
    times[i]   = render_stats_now_ms() - start;
    if (result != AK_OK || !renderData) {
      fprintf(stderr, "render batch build failed: %d\n", result);
      ak_free(doc);
      free(times);
      return 1;
    }

    groups  = renderData->groupCount;
    batches = 0;
    for (uint32_t groupIndex = 0;
         groupIndex < renderData->groupCount;
         groupIndex++)
      batches += renderData->groups[groupIndex].batchCount;
    included = renderData->includedPrimitiveCount;
    skipped  = renderData->skippedPrimitiveCount;
    ak_sceneRenderDataFree(renderData);
    ak_free(doc);
  }

  qsort(times, (size_t)iterations, sizeof(*times), render_stats_compare);
  printf("render_ms=%.3f groups=%u batches=%u included=%u skipped=%u path=%s\n",
         times[iterations / 2],
         groups,
         batches,
         included,
         skipped,
         path);
  free(times);
  return 0;
}
