// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _TRIPLET_STRING_HASH_H_
#define _TRIPLET_STRING_HASH_H_

#include <stdint.h>

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
void tshAdd (TripletStringHash *tsh, const char *word, size_t len);
void tshFree (TripletStringHash *tsh);

#endif /* _TRIPLET_STRING_HASH_H_ */
