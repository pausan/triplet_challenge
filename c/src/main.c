// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "triplets.h"

#define CLOCKS_PER_MILLIS   (CLOCKS_PER_SEC / 1000)

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
int main (int argc, char *argv[]) {
  clock_t start_t = clock(), end_t;
  TripletsOptimization optimizationType = OPTIMIZE_SPEED;
  int result = 0;

  if (argc < 2) {
    printf ("Use: %s <input_file> [speed|space]\n", argv[0]);
    return -1;
  }

  // adjust for speed/space
  if (argc >= 3) {
    if (strcmp(argv[2], "speed") == 0) {
      optimizationType = OPTIMIZE_SPEED;
    }
    else if (strcmp(argv[2], "space") == 0) {
      optimizationType = OPTIMIZE_SPACE;
    }
    else {
      printf ("ERROR: unknown third parameter... ignoring %s", argv[2]);
    }
  }

  result = processTripletsFromFile (argv[1], optimizationType);

  end_t = clock();
  printf ("Took %ld ms\n", ((end_t - start_t) / CLOCKS_PER_MILLIS) );

  return result;
}