// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLET_STRING_HASH_H_
#define _TRIPLET_STRING_HASH_H_

#include <stdint.h>

// -----------------------------------------------------------------------------
// fnvHash32v
//
// Simple, yet powerful algorithm to avoid collisions on english words based
// on FNV 1a. The last 'v' is because I've introduced a variation, since the
// result is initialized using first 4 words.
//
// Although as any algorithm, never collision free.
//
// Based on FNV/1a algorithm from:
//  http://isthe.com/chongo/tech/comp/fnv/
//
// See:
//   https://softwareengineering.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed
// -----------------------------------------------------------------------------
inline uint32_t fnvHash32v(const uint8_t *data, size_t n) {
  const uint32_t PRIME = 16777619;
  uint32_t result = 0;
  size_t i = 0;

  // Hopefully this should affect word collisions and give us some
  // speed boost as well (NOTE: I've not verified this!)
  if (n >= 4) {
    result = ((uint32_t *)data)[0];
    i = 4;
  }

  // continue iterating through the rest of the string
  for( ; i < n; i++) {
    result ^= data[i];
    result *= PRIME;
  }

  return result;
}


// -----------------------------------------------------------------------------
// TripletStringHash / TripletStringHashNode
//
// Tripled String Hash has been made only for this challenge, and assumes
// certain things:
//
//   - strings will point to read-only memory
//   - strings won't be modified
//   - hash tables will only insert items
//
// Observation:
//   - Most triplets will only happen once, so we can use a small hash list
//    and move most likely items to the top, to prevent comparisons
//
// Hash table is formed of:
//   - Items which contain the string, the length and how many we found
//   - The hash is saved to avoid rehashing, even if it consumes a little bit more
//   - A linked list of items that have same hash
// -----------------------------------------------------------------------------
typedef struct _TripletStringHashNode {
  const char *str;    // triplet string
  size_t      len;    // str len in bytes
  uint32_t    count;  // how many items we found
  // uint32_t    hash;   // 32-bit hash, to prevent computing hash again
  //                       (since we won't rehash, we don't need this)

  // inline linked list for collisions
  struct _TripletStringHashNode *nextCollision;
} TripletStringHashNode;

typedef struct _TripletStringHash {
  uint32_t slotsAllocated;
  uint32_t slotsUsageCount;

  // TODO: we can use **nodes to avoid allocating all memory at once
  TripletStringHashNode *nodes;
} TripletStringHash;

TripletStringHash *tshInit (uint32_t desiredSize);
void tshAdd (TripletStringHash *tsh, const char *word, size_t len, uint32_t hash);
void tshFree (TripletStringHash *tsh);

#endif /* _TRIPLET_STRING_HASH_H_ */
