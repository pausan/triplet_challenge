// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include "triplet_string_hash.h"

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


// Hash Table primes selected for speed (not space)
// See: https://planetmath.org/goodhashtableprimes
#define HT_PRIMES_SIZE  11
static const uint32_t HT_PRIMES[HT_PRIMES_SIZE] = {
  12289,
  24593,
  196613,
  786433,
  1572869,
  3145739,
  6291469,
  12582917,
  50331653,
  100663319,
  201326611
};

// -----------------------------------------------------------------------------
// tshInit
//
// Initializes a triplet string hash
// -----------------------------------------------------------------------------
TripletStringHash *tshInit (uint32_t desiredSize) {
  uint32_t i;

  TripletStringHash *tsh = (TripletStringHash*)malloc (sizeof (TripletStringHash));
  if (tsh == NULL)
    return tsh;

  tsh->slotsAllocated = 0;
  tsh->slotsUsageCount = 0;
  tsh->nodes = NULL;

  // find which space will suit the hash table best
  for ( i = 0; i < HT_PRIMES_SIZE; i++) {
    if (desiredSize < HT_PRIMES[i]) {
      tsh->slotsAllocated = HT_PRIMES[i];
      break;
    }
  }

  tsh->nodes = (TripletStringHashNode*)malloc (
    tsh->slotsAllocated * sizeof (TripletStringHashNode)
  );
  memset (&tsh->nodes[0], 0, tsh->slotsAllocated * sizeof (TripletStringHashNode));

  return tsh;
}

// -----------------------------------------------------------------------------
// tshAdd
//
// Insert an item into a TripletStringHash (or increment count if already exists)
// -----------------------------------------------------------------------------
void tshAdd (TripletStringHash *tsh, const char *word, size_t len) {
  uint32_t tripletHash = fnvHash32v((const uint8_t *)word, len);

  TripletStringHashNode *node = &(tsh->nodes[tripletHash % tsh->slotsAllocated]);

  // EMPTY_CASE: inserting on an empty slot (easy peasy)
  if (node->str == NULL) {
    node->str = word;
    node->len = len;
    // node->hash = tripletHash;
    node->count = 1;

    tsh->slotsUsageCount ++;
    return;
  }

  // SAME_STRING: increment and we are done
  if ((node->len == len) && (memcmp (&node->str[0], word, len) == 0)) {
    node->count ++;
    return;
  }

  // COLLISION_FOUND: well, there are several scenarios here...
  //   printf(
  //     "Collision found! %08x vs %08x // -%.*s- with -%.*s-\n",
  //     tripletHash,
  //     node->hash,
  //     (int)len, word,
  //     (int)node->len, node->str
  //   );

  // COLLISION_FOUND_EXISTS: check if word is already inserted as collision
  TripletStringHashNode *collision = node->nextCollision,
                        *prev = node;

  while (collision != NULL) {
    // if same string, increment and we are done
    if ((collision->len == len) && (memcmp (&collision->str[0], word, len) == 0)) {
      // TODO: in case this count is bigger than prev->count plus some K,
      //       we can swap items, so that the most likely to happen is found earlier
      //
      //       We can just keep swapping and it will eventually get to the top
      //       of the list.
      collision->count ++;

      // not a lot of gain in practice...
      /* if ((prev->count + 3) < collision->count) {
        TripletStringHashNode temp;
        TripletStringHashNode *nextCollision = collision->nextCollision;
        memcpy (&temp, prev, sizeof(TripletStringHashNode));
        memcpy (prev, collision, sizeof(TripletStringHashNode));
        memcpy (collision, &temp, sizeof(TripletStringHashNode));

        // update pointers to the right values
        prev->nextCollision = collision;
        collision->nextCollision = nextCollision;
      }*/

      return;
    }

    // otherwise let's continue iterating
    prev      = collision;
    collision = collision->nextCollision;
  }

  // REHASH? Let's rehash if the hash map is kind of full
  /* if (tsh->slotsUsageCount >= (0.8 * tsh->slotsAllocated)) {
    // TODO:
    //   tshRehash (fsh, fsh->slotsAllocated + 1);
    printf ("CRITICAL ERROR: REHASH NOT IMPLEMENTED!\n");
    exit(-1);
  } */

  // COLLISION_NOT_EXISTS: now let's insert at the end of the linked list
  TripletStringHashNode *newCollision = (TripletStringHashNode*)malloc (sizeof (TripletStringHashNode));
  memset (newCollision, 0, sizeof (TripletStringHashNode));

  prev->nextCollision = newCollision;

  newCollision->str = word;
  newCollision->len = len;
  // newCollision->hash = tripletHash;
  newCollision->count = 1;

  tsh->slotsUsageCount ++;

}

// -----------------------------------------------------------------------------
// tshFree
//
// Free a TripletStringHash initialized with tshInit
// -----------------------------------------------------------------------------
void tshFree (TripletStringHash *tsh) {
  if (tsh != NULL) {
    // FIXME! iterate on all nodes to free extra items created on linked lists

    if (tsh->nodes != NULL)
      free (tsh->nodes);

    free(tsh);
  }
}

// TODO: tshRehash (hash)
//   - create bigger hash
//   - iterate all items & re-add them