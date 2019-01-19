// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLETS_H_
#define _TRIPLETS_H_

#include "triplet_string_hash.h"

typedef enum {
  OPTIMIZE_SPEED = 0,
  OPTIMIZE_SPACE = 1,
  USE_TRIES      = 2
} TripletsOptimization;

typedef struct {
  const char *start;
  size_t      len;
} StringPtr;

typedef struct {
  struct {
    StringPtr str;
    uint32_t  count;
  } triplet[3];
} TripletResult;

size_t sanitizeTripletsInput (char *buffer, size_t len, uint32_t *wordCount);
void countTripletsFromMemoryUsingHashTable (char *buffer, size_t len, TripletsOptimization op);
void countTripletsFromMemoryUsingTries (char *buffer, size_t len);

void mergeTriplets (TripletResult *winning, const TripletResult *other);

void tshGetThreeTripletsWithHighestCount (const TripletStringHash *tsh, TripletResult *result);
void printTriplet (const TripletResult *triplet);
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh);

int processTripletsFromFile (const char *fileName, TripletsOptimization op);

#endif /* _TRIPLETS_H_ */
