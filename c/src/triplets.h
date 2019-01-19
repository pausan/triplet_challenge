// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLETS_H_
#define _TRIPLETS_H_

#include "triplet_string_hash.h"

typedef enum {
  HASH_TABLE_SPEED = 0,
  HASH_TABLE_SPACE,
  HASH_TABLE_SPLITTED
} TripletsOptimization;

typedef struct {
  const char *start;
  size_t      len;
} StringPtr;

// contains an array of strings, all of them of length "len"
typedef struct {
  const char **strings;
  uint32_t  count;
  uint32_t  capacity;
  uint32_t  len;
} FixedLenStringArray;

typedef struct {
  struct {
    StringPtr str;
    uint32_t  count;
  } triplet[3];
} TripletResult;

size_t sanitizeTripletsInput (char *buffer, size_t len, uint32_t *wordCount);
void countTripletsWithHashTable (char *buffer, size_t len, TripletsOptimization op);
void countTripletsWithSplittedHashTable (char *buffer, size_t len);

void mergeTriplets (TripletResult *winning, const TripletResult *other);

void tshGetThreeTripletsWithHighestCount (const TripletStringHash *tsh, TripletResult *result);
void printTriplet (const TripletResult *triplet);
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh);

int processTripletsFromFile (const char *fileName, TripletsOptimization op);

#endif /* _TRIPLETS_H_ */
