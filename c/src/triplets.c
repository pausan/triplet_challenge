// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>

#include "triplet_string_hash.h"
#include "triplets.h"

#define IS_LOWER_ALPHANUM(x)   (     \
  ( ((x) >= 'a') && ((x) <='z') ) || \
  ( ((x) >= '0') && ((x) <='9') )    \
)

// -----------------------------------------------------------------------------
// consumeWord
//
// Takes input pointer and iterates through all alphanumeric character plus one.
//
// This extra character makes it position at the end space.
// -----------------------------------------------------------------------------
inline const char *consumeWord (const char *ptr, const char *endPtr) {
  // are we done?
  if ((ptr + 1) >= endPtr)
    return NULL;

  while (IS_LOWER_ALPHANUM (*ptr) && ptr < endPtr)
    ptr++;

  // skip last space (or '\0' if EOF)
  return (ptr+1);
}

// -----------------------------------------------------------------------------
// printThreeTripletsWithHighestCount
//
// Print the three triplets with highest count.
// -----------------------------------------------------------------------------
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh) {
  // TODO: can be refactored using loops and arrays to print N highest items

  // (hashNode1, hashCount1) - highest number
  // (hashNode2, hashCount2) - second highest number
  // (hashNode3, hashCount3) - third highest number
  const TripletStringHashNode *hashNode1 = NULL, *hashNode2 = NULL, *hashNode3 = NULL;
  uint32_t hashCount1 = 0, hashCount2 = 0, hashCount3 = 0;

  for (size_t i = 0; i < tsh->slotsAllocated; i++) {
    const TripletStringHashNode *tshNodePtr = &tsh->nodes[i];

    // iterate through all items on the linked list for this hash slot
    while (tshNodePtr != NULL) {
      uint32_t tshNodeCount = tshNodePtr->count;

      // new highest count
      if (tshNodeCount >= hashCount1) {
        hashCount3 = hashCount2;
        hashCount2 = hashCount1;
        hashCount1 = tshNodeCount;

        hashNode3 = hashNode2;
        hashNode2 = hashNode1;
        hashNode1 = tshNodePtr;
      }

      // second higest count
      else if (tshNodeCount >= hashCount2) {
        hashCount3 = hashCount2;
        hashCount2 = tshNodeCount;

        hashNode3 = hashNode2;
        hashNode2 = tshNodePtr;
      }

      // third highest count
      else if (tshNodeCount >= hashCount3) {
        hashCount3 = tshNodeCount;
        hashNode3  = tshNodePtr;
      }

      // if it has items on the linked list, iterate throught them
      tshNodePtr = tshNodePtr->nextCollision;
    }
  }

  if (hashNode1 != NULL)
    printf (" >> %.*s - %u\n", (int)hashNode1->len, hashNode1->str, hashNode1->count);

  if (hashNode2 != NULL)
    printf (" >> %.*s - %u\n", (int)hashNode2->len, hashNode2->str, hashNode2->count);

  if (hashNode3 != NULL)
    printf (" >> %.*s - %u\n", (int)hashNode3->len, hashNode3->str, hashNode3->count);
}

// -----------------------------------------------------------------------------
// countTripletsFromMemory
//
// Count and print top triplets from given memory buffer
//
// IMPORTANT: this method assumes it can manipulate and overwrite memory
//            completely
// -----------------------------------------------------------------------------
void countTripletsFromMemory (
  char *buffer,
  size_t len,
  TripletsOptimization optimization
)
{
  size_t newLen = sanitizeTripletsInput (buffer, len);

  if (buffer == NULL)
    return;

  // iterate triplets
  const char *ptr = buffer,
             *endPtr = buffer + newLen;

  const char *word1 = buffer,
             *word2 = NULL,
             *word3 = NULL,
             *word4 = NULL;

  // find first three words, please note each word starts the next character
  // where previous ends (+1 because we skip only one space)
  word2 = consumeWord(word1, endPtr);
  if (word2 == NULL)
    return;

  word3 = consumeWord(word2, endPtr);
  if (word3 == NULL)
    return;

  word4 = consumeWord(word3, endPtr);
  if (word4 == NULL || (word4 >= endPtr)) {
    printf ("Only one triplet\n");
    printf (" - %s", buffer);
    return;
  }

#define ROL32(x, bits) ( (((uint32_t)x) << bits) | (((uint32_t)x) >> (32 - bits)) )

  // reduce computing by reusing hashes as we move along instead of rehashing
  // on every word that we advance
  uint32_t hash1, hash2, hash3;
  hash1 = fnvHash32v ((const uint8_t *)word1, (size_t)(word2 - word1 - 1));
  hash2 = fnvHash32v ((const uint8_t *)word2, (size_t)(word3 - word2 - 1));
  hash3 = fnvHash32v ((const uint8_t *)word3, (size_t)(word4 - word3 - 1));

  // Let's assume every word is around 6 chars and every triplet around 15.
  // Let's assume every triplet appears mostly once.
  // For space let's say we want to allocate 100x less, but for speed let's be
  // generious with the space, like 1 slot per item approx.
  TripletStringHash *tsh = tshInit (
    (optimization == OPTIMIZE_SPACE) ? newLen / 1500 : (newLen / 15)
  );

  while ((ptr < endPtr) && (word4 != NULL)) {
    uint32_t tripletHash = ROL32(hash1, 16) ^ ROL32(hash2, 8) ^ hash3;
    //uint32_t tripletHash = fnvHash32v((const uint8_t *)word1, (size_t)(word4 - word1 - 1));

    tshAdd (tsh, word1, (size_t)(word4 - word1 - 1), tripletHash);

    //printf ("%.*s\n", (int)(word4 - word1 - 1), word1);
    //printf ("Triplet[%08x]: -%.*s-\n", tripletHash, (int)(word4 - word1 - 1), word1);

    // hashIncrement (word1, word2, word3, word4);

    // NOTE: this can be implemented using a static char[4] and cyclic index,
    //       but implemented this way for clarity
    word1 = word2;
    word2 = word3;
    word3 = word4;
    word4 = consumeWord(word3+1, endPtr);

    if (word4 != NULL) {
      hash1 = hash2;
      hash2 = hash3;
      hash3 = fnvHash32v ((const uint8_t *)word3, (size_t)(word4 - word3 - 1));
    }

    ptr++;
  }

  printThreeTripletsWithHighestCount (tsh);

  tshFree (tsh);

  // printf ("From %ld to %ld\n", len, newLen);
  // printf ("%ld: --%.*s--\n", newLen, newLen, buffer);
}

// -----------------------------------------------------------------------------
// sanitizeTripletsInput
//
// Sanitize triplets input so that lowercases all words, removes punctuation
// characters and allows only one space to separate alphanumeric words.
//
// Multiple spaces and newlines will be considered as one space only.
//
// Please note that '10asdf' and 'asdf10' will be considered a word, but
// thngs like 'asdf-10' or 'asdf.asdf' will be splitted using a space.
// -----------------------------------------------------------------------------
size_t sanitizeTripletsInput (char *buffer, size_t len) {
  char *readPtr = buffer, *writePtr = buffer, *endPtr = (buffer + len);

  register uint_fast8_t lastIsSpace = 1;

  // prepare buffer in one pass to lowercase words, remove punctuation chars
  // and allow only one space between words
  while (readPtr < endPtr) {
    register char c = *readPtr;

    // skip valid chars
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      *writePtr = c;
      writePtr ++;

      lastIsSpace = 0;
    }

    // lowercase
    else if (c >= 'A' && c <= 'Z') {
      *writePtr = (c - 'A') + 'a';
      writePtr++;

      lastIsSpace = 0;
    }

    // one space
    else {
      if (!lastIsSpace) {
        *writePtr = ' ';
        writePtr++;
        lastIsSpace = 1;
      }
      else {
        // already overwritten a space (lastIsSpace = 1) so do nothing!
      }
    }

    readPtr++;
  }

  // NOTE: worst case scenario is output = input, in that case it is fine to
  //       write '\0' since we saved one extra char (find: extra-char!)
  //       Also, there is no need to make sure there is a space at the end
  //       since the last '\0' works same way for counting words.
  *writePtr = '\0';

  return (size_t)(writePtr - buffer + 1);
}

// -----------------------------------------------------------------------------
// processTripletsFromFile
//
// Counts triplets from given file. It uses a read-only memory mapped file for
// faster/easier counting.
//
// Params:
//   - file: input file to read (read-only)
//
// Return:
//   0 if ok and other negative number in case of error
// -----------------------------------------------------------------------------
int processTripletsFromFile (
  const char *fileName,
  TripletsOptimization optimization
) {
  int result = 0;
  int fd = -1;
  char *addr = NULL;
  struct stat sb;

  printf ("Processing %s...\n", fileName);

  if ((fd = open(fileName, O_RDONLY)) == -1) {
    printf ("ERROR: Cannot open file: %s\n", fileName);
    result = -1;
    goto cleanup;
  }

  if (fstat(fd, &sb) == -1) {
    printf ("ERROR: cannot query file size for: %s\n", fileName);
    result = -2;
    goto cleanup;
  }

  addr = mmap(
    NULL,                       // let the kernel allocate the space
    sb.st_size + 1,             // length: read the whole file (see: extra-char!)
    PROT_READ | PROT_WRITE,     // allow overwritting memory
    MAP_PRIVATE,                // don't flush to disk
    fd,                         // file to map
    0                           // no offset, read the whole file
  );
  if (addr == MAP_FAILED) {
    printf ("ERROR: Could not mmap file: %s\n", fileName);
    result = -3;
    goto cleanup;
  }

  countTripletsFromMemory (addr, sb.st_size, optimization);

  cleanup:
    if (fd != -1)
      close(fd);

    if (addr != MAP_FAILED)
      munmap(addr, sb.st_size);

  return result;
}
