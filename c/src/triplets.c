// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#include <pthread.h>
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
// tshGetThreeTripletsWithHighestCount
//
// Returns the three triplets with highest count in result
// -----------------------------------------------------------------------------
void tshGetThreeTripletsWithHighestCount (
  const TripletStringHash *tsh,
  TripletResult *result
)
{
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

  if (hashNode1 != NULL) {
    result->triplet[0].str.start = hashNode1->str;
    result->triplet[0].str.len   = hashNode1->len;
    result->triplet[0].count     = hashNode1->count;
  }

  if (hashNode2 != NULL) {
    result->triplet[1].str.start = hashNode2->str;
    result->triplet[1].str.len   = hashNode2->len;
    result->triplet[1].count     = hashNode2->count;
  }

  if (hashNode3 != NULL) {
    result->triplet[2].str.start = hashNode3->str;
    result->triplet[2].str.len   = hashNode3->len;
    result->triplet[2].count     = hashNode3->count;
  }
}

// -----------------------------------------------------------------------------
// printTriplet
//
// Find triplet with highest count on given hash table and print them
// -----------------------------------------------------------------------------
void printTriplet (const TripletResult *tripletResult) {
  for (int i = 0; i < 3; i++) {
    if (tripletResult->triplet[i].count == 0)
      continue;

    printf (
      "%.*s - %u\n",
      (int)tripletResult->triplet[i].str.len,
      tripletResult->triplet[i].str.start,
      tripletResult->triplet[i].count
    );
  }
}

// -----------------------------------------------------------------------------
// printThreeTripletsWithHighestCount
//
// Find triplet with highest count on given hash table and print them
// -----------------------------------------------------------------------------
void printThreeTripletsWithHighestCount (const TripletStringHash *tsh) {
  TripletResult tripletResult;
  memset (&tripletResult, 0, sizeof(TripletResult));

  tshGetThreeTripletsWithHighestCount (tsh, &tripletResult);
  printTriplet (&tripletResult);
}

// -----------------------------------------------------------------------------
// countTripletsWithHashTable
//
// Count and print top triplets from given memory buffer using a hash table.
//
// IMPORTANT: this method assumes it can manipulate and overwrite memory
//            completely
// -----------------------------------------------------------------------------
void countTripletsWithHashTable (
  char *buffer,
  size_t len,
  TripletsOptimization optimization
)
{
  uint32_t wordCount = 0;
  size_t newLen = sanitizeTripletsInput (buffer, len, &wordCount);

  if (buffer == NULL)
    return;

  // iterate triplets
  const char *endPtr = buffer + newLen;

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

  // For speed, let's allocate a big buffer
  // For space let's say we want to allocate 100x less, but for speed let's be
  // generious with the space, like 1 slot per item approx.
  TripletStringHash *tsh = tshInit (
    (optimization == HASH_TABLE_SPACE) ? wordCount / 10 : (wordCount / 3)
  );

  while (word4 != NULL) {
    uint32_t tripletHash = ROL32(hash1, 16) ^ ROL32(hash2, 8) ^ hash3;
    // uint32_t tripletHash = fnvHash32v((const uint8_t *)word1, (size_t)(word4 - word1 - 1));
    // printf ("tripletHash: %d\n", tripletHash);
    tshAdd (tsh, word1, (size_t)(word4 - word1 - 1), tripletHash);

    //printf ("%.*s\n", (int)(word4 - word1 - 1), word1);
    //printf ("Triplet[%08x]: -%.*s-\n", tripletHash, (int)(word4 - word1 - 1), word1);

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
  }

  printThreeTripletsWithHighestCount (tsh);

  /*
  int numCollisions = 0;
  for (size_t i = 0; i < tsh->slotsAllocated; i++) {
    const TripletStringHashNode *tshNodePtr = &tsh->nodes[i];

    // iterate through all items on the linked list for this hash slot
    while (tshNodePtr != NULL) {
      if (tshNodePtr->nextCollision != NULL)
        numCollisions++;

      // if it has items on the linked list, iterate throught them
      tshNodePtr = tshNodePtr->nextCollision;
      if (tshNodePtr) {
        printf ("count: %d\n", tshNodePtr->count);
      }
    }
  }

  printf ("numCollisions: %d\n", numCollisions);
  */

  tshFree (tsh);

  // printf ("From %ld to %ld\n", len, newLen);
  // printf ("%ld: --%.*s--\n", newLen, newLen, buffer);
}

// -----------------------------------------------------------------------------
// countTripletsWithSplittedHashTable
//
// Count and print top triplets from given memory buffer using a hash table.
//
// This algorithm divides the triplets in a list of lists containing triplets
// of the same length, so that hashes are computed from those words with same
// lenght.
//
// For each of those hashes, we get the winning triplet and then we merge
// with previous winning triplet. That way we end up with the highest winning
// triplet.
//
// There are two main advantages of this approach. First it runs faster because
// it is less likely to have collisions, plus it also consumes less memory
// since it works one list at a time.
//
// IMPORTANT: this method assumes it can manipulate and overwrite memory
//            completely
// -----------------------------------------------------------------------------
void countTripletsWithSplittedHashTable (
  char *buffer,
  size_t len
)
{
  uint32_t wordCount = 0;
  size_t newLen = sanitizeTripletsInput (buffer, len, &wordCount);

  if (buffer == NULL)
    return;

  // iterate triplets
  const char *endPtr = buffer + newLen;

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

  StringPtr *stringPtrList = calloc(wordCount, sizeof(StringPtr));
  uint32_t   stringPtrIndex = 0;

  while (word4 != NULL) {
    const char *nextWord = consumeWord(word4+1, endPtr);
    if (nextWord == NULL)
      break;

    stringPtrList[stringPtrIndex].start = word1;
    stringPtrList[stringPtrIndex].len   = (int)(word4 - word1 - 1);
    stringPtrIndex++;

    // NOTE: this can be implemented using a static char[4] and cyclic index,
    //       but implemented this way for clarity
    word1 = word2;
    word2 = word3;
    word3 = word4;
    word4 = nextWord;
  }

// FIXME! if any string is bigger than this value, program will crash
#define MAX_BUCKETS 64

  uint32_t stringPtrCount = stringPtrIndex;

  uint32_t stringsOfLengthXCount[MAX_BUCKETS];
  memset (&stringsOfLengthXCount[0], 0, sizeof(stringsOfLengthXCount));

  // find out how many items of lenght X we have
  for (uint32_t i = 0; i < stringPtrCount; i++)
  {
    StringPtr *stringPtr = &stringPtrList[i];
    uint32_t   bucket    = stringPtr->len;
    stringsOfLengthXCount[bucket] ++;
  }

/*
  // print items of length X
  for (uint32_t i = 0; i < MAX_BUCKETS; i++) {
    if (stringsOfLengthXCount[i] > 0)
      printf ("# where len=%d => %d\n", i, stringsOfLengthXCount[i]);
  }*/

  // build a list sorted by items of length X
  FixedLenStringArray fixedLenStrings[MAX_BUCKETS];

  for (uint32_t i = 0; i < MAX_BUCKETS; i++)
  {
    fixedLenStrings[i].capacity = stringsOfLengthXCount[i];
    fixedLenStrings[i].len      = i;
    fixedLenStrings[i].count    = 0;
    fixedLenStrings[i].strings  = (const char**)calloc (fixedLenStrings[i].capacity, sizeof (const char*));
  }

  // add all strings to its proper bucket of length _i_
  for (uint32_t i = 0; i < stringPtrCount; i++)
  {
    StringPtr *stringPtr = &stringPtrList[i];
    uint32_t   bucket = stringPtr->len;

    fixedLenStrings[bucket].strings[
      fixedLenStrings[bucket].count++
    ] = stringPtr->start;
  }

  TripletResult winningTriplet;
  memset (&winningTriplet, 0, sizeof(TripletResult));

  // at this point we have a list of strings of the same size
  for (uint32_t bucketIndex = 0; bucketIndex < MAX_BUCKETS; bucketIndex++)
  {
    FixedLenStringArray  *fixedStringArrayOfLenI = &fixedLenStrings[bucketIndex];

    // if array contains no strings, or the strings it contains is less than
    // the lower count of the triplets, we can just ignore that list
    if (
      (fixedStringArrayOfLenI->count == 0)
      || (fixedStringArrayOfLenI->count < winningTriplet.triplet[2].count)
    ) {
      continue;
    }

    TripletResult tripletResult;
    memset (&tripletResult, 0, sizeof(TripletResult));
    findBestFixedLenghtStringTriplets(&tripletResult, fixedStringArrayOfLenI);

    mergeTriplets (&winningTriplet, &tripletResult);
  }

  // print winning triplet
  printTriplet(&winningTriplet);

  free(stringPtrList);
}

// -----------------------------------------------------------------------------
// findBestFixedLenghtStringTriplets
//
// Finds the three triplets that appear the more times.
// -----------------------------------------------------------------------------
void findBestFixedLenghtStringTriplets (
  TripletResult *tripletResult,
  const FixedLenStringArray *fixedLenStrings
)
{
  const char **strings = fixedLenStrings->strings;
  size_t       stringLen = fixedLenStrings->len;

  // use a hash table for all triplets of length N
  TripletStringHash *tsh = tshInit (1.2*fixedLenStrings->count);

  for (uint32_t j = 0; j < fixedLenStrings->count; j++) {
    uint32_t tripletHash = fnvHash32v ((const uint8_t *)strings[j], stringLen);
    tshAdd (tsh, strings[j], stringLen, tripletHash);
  }

  tshGetThreeTripletsWithHighestCount (tsh, tripletResult);

  tshFree(tsh);
}

// -----------------------------------------------------------------------------
// mergeTriplets
//
// Merge a triplet with the winning triplet so that we end up with the
// three most repeated words in the _winning_ variable.
// -----------------------------------------------------------------------------
void mergeTriplets (TripletResult *winning, const TripletResult *other) {

  size_t SIZEOF_TRIPLET = sizeof(winning->triplet[0]);
  //

  for (int otherI = 0; otherI < 3; otherI++) {

    // new highest count
    if (other->triplet[otherI].count >= winning->triplet[0].count) {
      memcpy (&winning->triplet[2], &winning->triplet[1],    SIZEOF_TRIPLET);
      memcpy (&winning->triplet[1], &winning->triplet[0],    SIZEOF_TRIPLET);
      memcpy (&winning->triplet[0], &other->triplet[otherI], SIZEOF_TRIPLET);
    }

    // second higest count
    else if (other->triplet[otherI].count >= winning->triplet[1].count) {
      memcpy (&winning->triplet[2], &winning->triplet[1],    SIZEOF_TRIPLET);
      memcpy (&winning->triplet[1], &other->triplet[otherI], SIZEOF_TRIPLET);
    }

    // third highest count
    else if (other->triplet[otherI].count >= winning->triplet[2].count) {
      memcpy (&winning->triplet[2], &other->triplet[otherI], SIZEOF_TRIPLET);
    }
  }
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
size_t sanitizeTripletsInput (char *buffer, size_t len, uint32_t *wordCount) {
  char *readPtr = buffer, *writePtr = buffer, *endPtr = (buffer + len);
  register uint_fast8_t lastIsSpace = 1;

  *wordCount = 0;

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
        (*wordCount) ++;
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

  debug ("Processing %s...\n", fileName);

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

  if (optimization == HASH_TABLE_SPLITTED)
    countTripletsWithSplittedHashTable (addr, sb.st_size);
  else
    countTripletsWithHashTable (addr, sb.st_size, optimization);

  cleanup:
    if (fd != -1)
      close(fd);

    if (addr != MAP_FAILED)
      munmap(addr, sb.st_size);

  return result;
}
