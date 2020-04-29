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

#define SPACE_CHAR ' '

// -----------------------------------------------------------------------------
// use buildLookupTables to generate g_charLookup and g_isCharLookup
//
// These two tables are used to sanitize words in a quick way, without using
// any branches, which basically speeds things up 2x for that part of the
// challenge.
// -----------------------------------------------------------------------------
void buildLookupTables() {
  uint8_t lookup[256], isCharLookup[256];
  for (int c = 0; c < 256; c++) {
    isCharLookup[c] = 1;

    // skip valid chars (lowercase / digits)
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
      lookup[c] = c;
    }

    // uppercase
    else if (c >= 'A' && c <= 'Z') {
      lookup[c] = (c - 'A') + 'a';
    }

    // one space
    else {
      lookup[c] = SPACE_CHAR;
      isCharLookup[c] = 0;
    }
  }

  printf ("uint8_t g_charLookup[256] = {\n  ");
  for (int c = 0; c < 256; c++) {
    printf ("'%c'%c", lookup[c], c == 255 ? ' ' : ',');
    if (c % 16 == 15)
      printf ("\n  ");
  }
  printf ("\r};\n");

  printf ("uint8_t g_isCharLookup[256] = {\n  ");
  for (int c = 0; c < 256; c++) {
    printf ("%d%c", isCharLookup[c], c == 255 ? ' ' : ',');
    if (c % 16 == 15)
      printf ("\n  ");
  }
  printf ("\r};\n");
}

// g_charLookup contains all alphanumeric characters transformed to
// lowercase or space char
static uint8_t g_charLookup[256] = {
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  '0','1','2','3','4','5','6','7','8','9',' ',' ',' ',' ',' ',' ',
  ' ','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
  'p','q','r','s','t','u','v','w','x','y','z',' ',' ',' ',' ',' ',
  ' ','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
  'p','q','r','s','t','u','v','w','x','y','z',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',
  ' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' '
};

// g_charLookup contains 1 if given char index is a character or
// o if it is a space
static uint8_t g_isCharLookup[256] = {
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
// -----------------------------------------------------------------------------


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

  // no need to check for endptr since we know last character is ':' on purpose
  while (*ptr != SPACE_CHAR)
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
      tripletResult->triplet[i].str.len,
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
  size_t newLen = sanitizeTripletsInput ((uint8_t*)buffer, len, &wordCount);

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
  size_t newLen = sanitizeTripletsInput ((uint8_t*)buffer, len, &wordCount);

  if (buffer == NULL || wordCount < 3)
    return;

  // iterate triplets
  const char *endPtr = buffer + newLen;

  uint32_t *words = (uint32_t *)malloc(wordCount * sizeof(uint32_t));
  uint16_t *wordsLen = (uint16_t *)malloc(wordCount * sizeof(uint16_t));
  uint32_t  wordIndex = 0;

#define MAX_BUCKETS  512
  uint32_t stringsOnBucketX[MAX_BUCKETS];
  memset (&stringsOnBucketX[0], 0, sizeof(stringsOnBucketX));

  // first word!
  words[wordIndex] = 0;
  wordIndex++;

  // find the beginning of each word
  const char *ptr = buffer+1;
  while (ptr < endPtr) {
    int isSpace = (*ptr == SPACE_CHAR);

    words[wordIndex] = (uint32_t)(ptr - buffer) + 1;
    wordIndex += isSpace;

    ptr++;
  }

  // find out the length of each triplet once we identified the beginning
  // of each word
  uint32_t ntriplets = wordIndex;
  for (uint32_t i = 3; i < ntriplets; i++) {
    uint16_t len = words[i] - words[i-3] - 1;
    wordsLen[i-3] = len;
    stringsOnBucketX[len] ++;
  }

  // last three items don't have enough words to form a triplet
  ntriplets -= 3;

  // printf ("%.*s\n", (uint32_t)(endPtr - buffer + 1), buffer);
  // for (uint32_t i = 0; i < ntriplets; i++) {
  //   printf ("%04x:%3d: %.*s\n", words[i], wordsLen[i], wordsLen[i], buffer+words[i]);
  // }
  //exit(0);

  // print items of length X
  // for (uint32_t i = 0; i < MAX_BUCKETS; i++) {
  //   if (!stringsOnBucketX[i])
  //     continue;
  //   printf ("# where len=%d => %d\n", i, stringsOnBucketX[i]);
  // }

  TripletResult winningTriplet, partialResult;
  memset (&winningTriplet, 0, sizeof(winningTriplet));

  uint32_t maxStringsInABucket = 0;

  // do a single string allocation for the total capacity
  uint32_t *allStrings = (uint32_t*)malloc (ntriplets * sizeof (uint32_t));
  uint32_t  allocationIndex = 0;

  // build a list sorted by items of length X
  FixedLenStringArray fixedLenStrings[MAX_BUCKETS];
  memset(&fixedLenStrings[0], 0, sizeof(fixedLenStrings));

  // We process some buckets in order to find the winning triplets for these
  // blocks.
  for (uint32_t i = 0; i < MAX_BUCKETS; i++)
  {
    fixedLenStrings[i].capacity = stringsOnBucketX[i];
    fixedLenStrings[i].len      = i;
    //fixedLenStrings[i].count    = 0;
    //fixedLenStrings[i].stringOffsets = NULL;

    if (fixedLenStrings[i].capacity) {
      fixedLenStrings[i].stringOffsets = &allStrings[allocationIndex];
      allocationIndex += fixedLenStrings[i].capacity;

      if (fixedLenStrings[i].capacity >= maxStringsInABucket)
        maxStringsInABucket = fixedLenStrings[i].capacity;
    }
  }

  // add all strings to its proper bucket of length _i_
  for (uint32_t i = 0; i < ntriplets; i++)
  {
    uint32_t bucket = wordsLen[i];

    fixedLenStrings[bucket].stringOffsets[
      fixedLenStrings[bucket].count++
    ] = words[i];
  }

  // we allocate the max required amount for any of the lists only once
  uint32_t *hashes = (uint32_t*)malloc (maxStringsInABucket * sizeof (uint32_t));

  // by definition of a triplet, a triplet should contain at least 5 characters
  // (three words of 1 letter + 2 spaces), thus, we can use the first 5 positions
  // to reorder

  // find the top 4 most promising lists to explore (not necesarily the top-5
  // by score)
  uint32_t bestIndex[4], bestScore[4];
  memset (&bestIndex, 0, sizeof(bestIndex));
  memset (&bestScore, 0, sizeof(bestScore));
  for (uint32_t i = 0; i < MAX_BUCKETS; i++) {
    if (stringsOnBucketX[i] > bestScore[0]) {
      for (uint32_t j = 3; j != 0; j--) {
        bestIndex[j] = bestIndex[j-1];
        bestScore[j] = bestScore[j-1];
      }
      bestIndex[0] = i;
      bestScore[0] = stringsOnBucketX[i];
    }
  }

  for (uint32_t j = 0; j < 4; j++) {
    memcpy(&fixedLenStrings[j], &fixedLenStrings[bestIndex[j]], sizeof(fixedLenStrings[0]));
    memset(&fixedLenStrings[bestIndex[j]], 0, sizeof(fixedLenStrings[0]));
  }

  // at this point we have a list of strings of the same size
  for (uint32_t bucketIndex = 0; bucketIndex < MAX_BUCKETS; bucketIndex++)
  {
    if (
      // equivalent to fixedStringArrayOfLenI->capacity
      (!stringsOnBucketX[bucketIndex])
      || (stringsOnBucketX[bucketIndex] <= winningTriplet.triplet[2].count)
    ) {
      continue;
    }

    FixedLenStringArray  *fixedStringArrayOfLenI = &fixedLenStrings[bucketIndex];
    // memset (&partialResult, 0, sizeof(partialResult));
    // findBestFixedLenghtStringTriplets (&partialResult, fixedStringArrayOfLenI);
    // printf ("-- hash:\n");
    // printTriplet(&partialResult);

    memset (&partialResult, 0, sizeof(partialResult));
    findBestFixedLenghtStringTripletsByBounding (
      buffer,
      &partialResult,
      fixedStringArrayOfLenI,
      hashes,
      winningTriplet.triplet[2].count
    );

    mergeTriplets (&winningTriplet, &partialResult);
  }

  // print winning triplet
  printTriplet(&winningTriplet);

  // program is going to die, why spend cycles freeing memory anyway?
  // free(hashes);
  // free(allStrings);
  // free(words);
}


// -----------------------------------------------------------------------------
// findBestFixedLenghtStringTripletsByBounding
//
// Finds the three triplets that appear the more times.
//
// This algorithm  computes a hash of all triplets and creates a histogram of
// all items whose hash module suits position X, then we remove all items whose
// count is lower than highestThirdCount.
//
// Said another way, we get the hash of all the words, count how many hashes
// collide in bucket X, and remove all strings whose buckets count, even
// considering collisions, are not going to make it to the top 3.
//
// We know that, even considering collisions when inserting in the histogram,
// if histogram[i] < highestThirdCount, then there would be less words
// and they won't make it even to the third position.
//
// In the end, this reduces the size of the problem in order to use the
// hash table in the end to find the winning triplets.
// -----------------------------------------------------------------------------
void findBestFixedLenghtStringTripletsByBounding (
  const char *buffer,
  TripletResult *tripletResult,
  FixedLenStringArray *fixedLenStrings,
  uint32_t *hashes,
  uint32_t highestThirdCount
)
{
  uint32_t *stringOffsets = fixedLenStrings->stringOffsets;
  size_t    stringLen = fixedLenStrings->len;
  const uint_fast32_t orgStringsCount = fixedLenStrings->count;

#define BUCKETS_SIZE (4*1024)
#define BUCKETS_MODULE(x)   ((x) & (BUCKETS_SIZE-1))

  uint32_t buckets[BUCKETS_SIZE];
  memset (&buckets[0], 0, sizeof(buckets));

  // compute all hashes in order to discard all collisions whose
  for (size_t i = 0; i < orgStringsCount; i++) {
    uint32_t tripletHash = fnvHash32v16 ((const uint16_t *)(buffer + stringOffsets[i]), stringLen);
    buckets[BUCKETS_MODULE(tripletHash)] ++;
    hashes[i] = tripletHash;
  }

  uint_fast32_t newStringsCount = 0;
  for (size_t i = 0; i < orgStringsCount; i++) {
    if (buckets[BUCKETS_MODULE(hashes[i])] > highestThirdCount) {
      // since we know newStringsCount is less/eq to i, we can do:
      stringOffsets [ newStringsCount ] = stringOffsets[i];
      hashes  [ newStringsCount ] = hashes[i];
      newStringsCount++;
    }
  }

  // nothing to do
  if (newStringsCount == 0)
    return;

  fixedLenStrings->count = newStringsCount;

  // use a hash table for all triplets of length N
  TripletStringHash *tsh = tshInit (1.2*newStringsCount);

  for (uint32_t i = 0; i < newStringsCount; i++) {
    tshAdd (tsh, buffer + stringOffsets[i], stringLen, hashes[i]);
  }

  tshGetThreeTripletsWithHighestCount (tsh, tripletResult);
  tshFree(tsh);
}

// -----------------------------------------------------------------------------
// findBestFixedLenghtStringTriplets
//
// Finds the three triplets that appear the more times.
// -----------------------------------------------------------------------------
void findBestFixedLenghtStringTriplets (
  const char *buffer,
  TripletResult *tripletResult,
  const FixedLenStringArray *fixedLenStrings
)
{
  const uint32_t *stringOffsets = fixedLenStrings->stringOffsets;
  size_t          stringLen = fixedLenStrings->len;

  // use a hash table for all triplets of length N
  TripletStringHash *tsh = tshInit (1.2*fixedLenStrings->count);

  for (uint32_t j = 0; j < fixedLenStrings->count; j++) {
    uint32_t tripletHash = fnvHash32v ((const uint8_t *)(buffer + stringOffsets[j]), stringLen);
    tshAdd (tsh, buffer + stringOffsets[j], stringLen, tripletHash);
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
//
// As a side note, this implementation avoids branches by using pregenerated
// lookup tables. We save some few milliseconds by having it pregenerated.
// -----------------------------------------------------------------------------
size_t sanitizeTripletsInput (uint8_t *buffer, size_t len, uint32_t *wordCount) {
  uint8_t *readPtr = buffer, *writePtr = buffer, *endPtr = (buffer + len);

  uint8_t last = 0;
  *wordCount = 0;

  // prepare buffer in one pass to lowercase words, remove punctuation chars
  // and allow only one space between words
  while (readPtr < endPtr) {
    uint8_t c = *readPtr++;

    *writePtr = g_charLookup[c];
    writePtr += g_isCharLookup[c];

    uint8_t oneSpaceForNextWord = (!g_isCharLookup[c] && last != g_isCharLookup[c]);
    writePtr += oneSpaceForNextWord;
    *wordCount += oneSpaceForNextWord;
    last = g_isCharLookup[c];
  }

  // NOTE: worst case scenario is output = input, in that case it is fine to
  //       write '\0' since we saved one extra char (find: extra-char!)
  //       Also, there is no need to make sure there is a space at the end
  //       since the last '\0' works same way for counting words.
  *writePtr = '\0';

  // printf ("%.*s\n", 100, buffer);
  // exit(1);

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

  if (optimization == HASH_TABLE_RADIX)
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
