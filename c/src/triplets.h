// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLETS_H_
#define _TRIPLETS_H_

#define debug(...)
// #define debug(...)  printf (__VA_ARGS__);

#include "triplet_string_hash.h"

typedef enum {
  HASH_TABLE_SPEED = 0,
  HASH_TABLE_SPACE,
  HASH_TABLE_RADIX
} TripletsOptimization;

#pragma pack(push,2)
typedef struct {
  uint32_t offset;
  uint16_t len;
} StringOffset;

typedef struct {
  const char *start;
  uint16_t len;
} StringPtr;

// contains an array of strings, all of them of length "len"
typedef struct {
  uint32_t *stringOffsets;
  uint32_t  count;
  uint32_t  capacity;
  uint16_t  len;
} FixedLenStringArray;

typedef struct {
  struct {
    StringPtr str;
    uint32_t  count;
  } triplet[3];
} TripletResult;
#pragma pack(pop)

size_t sanitizeTripletsInput (uint8_t *buffer, size_t len, uint32_t *wordCount);
void countTripletsWithHashTable (char *buffer, size_t len, TripletsOptimization op);
void countTripletsWithSplittedHashTable (char *buffer, size_t len);

void findBestFixedLenghtStringTriplets (
  const char *buffer,
  TripletResult *result,
  const FixedLenStringArray *fixedLenStrings
);

void findBestFixedLenghtStringTripletsByBounding (
  const char *buffer,
  TripletResult *tripletResult,
  FixedLenStringArray *fixedLenStrings,
  uint32_t *hashes,
  uint32_t highestThirdCount
);

void mergeTriplets (TripletResult *winning, const TripletResult *other);

void tshGetThreeTripletsWithHighestCount (const TripletStringHash *tsh, TripletResult *result);
void printTriplet (const TripletResult *triplet);
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh);

int processTripletsFromFile (const char *fileName, TripletsOptimization op);

#endif /* _TRIPLETS_H_ */
