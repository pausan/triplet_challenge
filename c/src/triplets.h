// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLETS_H_
#define _TRIPLETS_H_

#include "triplet_string_hash.h"

typedef enum {
  OPTIMIZE_SPEED = 0,
  OPTIMIZE_SPACE = 1
} TripletsOptimization;

size_t sanitizeTripletsInput (char *buffer, size_t len);
void countTripletsFromMemory (char *buffer, size_t len, TripletsOptimization op);
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh);
int processTripletsFromFile (const char *fileName, TripletsOptimization op);

#endif /* _TRIPLETS_H_ */
