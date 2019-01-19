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
#include "word_trie.h"
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
      " >> %.*s - %u\n",
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
// countTripletsFromMemoryUsingHashTable
//
// Count and print top triplets from given memory buffer using a hash table.
//
// IMPORTANT: this method assumes it can manipulate and overwrite memory
//            completely
// -----------------------------------------------------------------------------
void countTripletsFromMemoryUsingHashTable (
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
    (optimization == OPTIMIZE_SPACE) ? wordCount / 10 : (wordCount / 3)
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

int compareUInt64(const void * a, const void * b)
{
  if ( *((uint64_t*)a) > *((uint64_t*)b)) {
    return 1;
  }
  else if ( *((uint64_t*)a) == *((uint64_t*)b)) {
    return 0;
  }

  return -1;
}

int compareStringPtrs(const void * a, const void * b)
{
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
  int minlen = MIN(((StringPtr*)a)->len, ((StringPtr*)b)->len);
  int result = memcmp ( ((StringPtr*)a)->start, ((StringPtr*)b)->start, minlen);

  if (result == 0) {
    if (((StringPtr*)a)->len < ((StringPtr*)b)->len)
      return -1;
    else if (((StringPtr*)a)->len == ((StringPtr*)b)->len)
      return 0;
    return -1;
  }

  return result;
}


int compareStringPtrs2(const void * a, const void * b)
{
  if (((StringPtr*)a)->len < ((StringPtr*)b)->len)
    return -1;
  else if (((StringPtr*)a)->len > ((StringPtr*)b)->len)
    return 1;

  return 0;
  //return memcmp ( ((StringPtr*)a)->start, ((StringPtr*)b)->start, ((StringPtr*)a)->len);
}

// -----------------------------------------------------------------------------
// countTripletsFromMemoryUsingTries
//
// Count and print top triplets from given memory buffer using a hash table.
//
// IMPORTANT: this method assumes it can manipulate and overwrite memory
//            completely
// -----------------------------------------------------------------------------
void countTripletsFromMemoryUsingTries (
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

  WordTrie *trie = trieInit ();
/*
  trieAddWithLen (trie, "a", 1);
  trieAddWithLen (trie, "a", 1);
  trieAddWithLen (trie, "az", 2);
  trieAddWithLen (trie, "a0", 2);
  trieAddWithLen (trie, "a9", 2);
  trieAddWithLen (trie, "a ", 2);
  trieAddWithLen (trie, "az09", 4);
  trieAddWithLen (trie, "az09", 4);
  trieAddWithLen (trie, "az09end", 7);
  trieAddWithLen (trie, "az09end", 7);
  trieAddWithLen (trie, "az09endo", 8);

  char c;
  c = 'a'; printf ("%c - %d - %c\n", c, CHAR_TO_INDEX(c), INDEX_TO_CHAR(CHAR_TO_INDEX(c)));
  c = 'z'; printf ("%c - %d - %c\n", c, CHAR_TO_INDEX(c), INDEX_TO_CHAR(CHAR_TO_INDEX(c)));
  c = '0'; printf ("%c - %d - %c\n", c, CHAR_TO_INDEX(c), INDEX_TO_CHAR(CHAR_TO_INDEX(c)));
  c = '9'; printf ("%c - %d - %c\n", c, CHAR_TO_INDEX(c), INDEX_TO_CHAR(CHAR_TO_INDEX(c)));
  c = ' '; printf ("%c - %d - %c\n", c, CHAR_TO_INDEX(c), INDEX_TO_CHAR(CHAR_TO_INDEX(c)));

  printf ("----------------------------------------------------------------\n");
  triePrint (trie);

  exit(-1);

  uint32_t idx1, idx2, idx3;
  idx1 = trieAdd (trie, word1, word2 - 1);
  idx2 = trieAdd (trie, word2, word3 - 1);
  idx3 = trieAdd (trie, word3, word4 - 1);
  const uint32_t MAX_WORDS = 64*1024; // 64k words max
*/
  // NOTE: trieAdd using full words ~= 460ms
  // NOTE: trieAdd using triplet-generated words ~= 260ms
  // NOTE: trieAdd using 64-bit ids ~= 45ms
  // NOTE: trieAdd with qsort ~= 100ms
  //

  //uint64_t *idList = calloc(wordCount, sizeof(uint64_t));
  //uint64_t idListIndex = 0;
  StringPtr *stringPtrList = calloc(wordCount, sizeof(StringPtr));
  uint32_t   stringPtrIndex = 0;

/*  if (wordCount >= nhashMax) {
    printf ("CRITICAL ERROR: program not prepared to handle this...increase nhashMax with a higher prime than %d\n", wordCount);
    return;
  }*/

  uint32_t lastId = 0, maxId = 0;
  while (word4 != NULL) {
    const char *nextWord = consumeWord(word4+1, endPtr);
    if (nextWord == NULL)
      break;

    // uint32_t tripletHash = fnvHash32v((const uint8_t *)word1, (size_t)(word4 - word1 - 1));
    // printf ("tripletHash: %d\n", tripletHash);
    //printf ("%.*s\n", (int)(nextWord - word4 - 1), word4);
    /*lastId = trieAdd (trie, word4, nextWord - 1);
    if (maxId < lastId)
      maxId = lastId;
    */

    //lastId = trieAdd (trie, word1, word4 - 1);
    stringPtrList[stringPtrIndex].start = word1;
    stringPtrList[stringPtrIndex].len   = (int)(word4 - word1 - 1);
    stringPtrIndex++;
    //printf ("Triplet[%08x]: -%.*s-\n", lastId, (int)(word4 - word1 - 1), word1);

    // NOTE: this can be implemented using a static char[4] and cyclic index,
    //       but implemented this way for clarity
    word1 = word2;
    word2 = word3;
    word3 = word4;
    word4 = nextWord;

    /*
    idx1 = idx2;
    idx2 = idx3;
    idx3 = lastId;

    // NOTE: this id is guaranteed to be unique
    uint64_t id = idx1 + MAX_WORDS*idx2 + (MAX_WORDS*MAX_WORDS*idx3);
    idList[idListIndex++] = id;

    uint32_t tripletHash = fnvHash32v((const uint8_t *)&id, 6) % nhashMax;
    uint32_t nhashPos    = tripletHash % nhashMax;
    //uint32_t nhashPos = (idx1 + 1543*idx2 + 12289*idx3) % nhashMax;

    while (
      !( (nhash[nhashPos].count == 0) || (nhash[nhashPos].id == id) )
    ) {
      // printf ("nhash[%d]=(%8lu, %d)\n", nhashPos, nhash[nhashPos].id, nhash[nhashPos].count);
      //nhashPos = (nhashPos+1) % nhashMax;;
      nhashPos++;
      if (nhashPos == nhashMax)
        nhashPos = 0;
    }

    nhash[nhashPos].id = id;
    nhash[nhashPos].count++;
    */

    //printf ("Combination: %d, %d, %d => %lu %x\n", idx1, idx2, idx3, id, tripletHash & 0xFFFF);
  }

// FIXME! if any string is bigger than this value, program will crash
#define STRING_OF_MAX_LEN  100

  uint32_t stringPtrCount = stringPtrIndex;

  uint32_t stringsOfLengthXCount[STRING_OF_MAX_LEN];
  memset (&stringsOfLengthXCount[0], 0, sizeof(stringsOfLengthXCount));

  // find out how many items of lenght X we have
  for (uint32_t i = 0; i < stringPtrCount; i++) {
    StringPtr *stringPtr = &stringPtrList[i];
    stringsOfLengthXCount[stringPtr->len] ++;
  }

  /*
  // print items of length X
  for (uint32_t i = 0; i < STRING_OF_MAX_LEN; i++) {
    if (stringsOfLengthXCount[i] > 0)
      printf ("# where len=%d => %d\n", i, stringsOfLengthXCount[i]);
  }*/

  // contains a list of strings, all of them of length "len"
  typedef struct {
    const char **strings;
    uint32_t  count;
    uint32_t  capacity;
    uint32_t  len;
  } FixedLenStringArray;

  // build a list sorted by items of length X
  FixedLenStringArray fixedLenStrings[STRING_OF_MAX_LEN];

  for (uint32_t i = 0; i < STRING_OF_MAX_LEN; i++) {
    fixedLenStrings[i].capacity = stringsOfLengthXCount[i];
    fixedLenStrings[i].len      = i;
    fixedLenStrings[i].count    = 0;
    fixedLenStrings[i].strings  = (const char**)calloc (fixedLenStrings[i].capacity, sizeof (const char*));
  }

  // add all strings to its proper bucket of length _i_
  for (uint32_t i = 0; i < stringPtrCount; i++) {
    StringPtr *stringPtr = &stringPtrList[i];
    fixedLenStrings[stringPtr->len].strings[
      fixedLenStrings[stringPtr->len].count++
    ] = stringPtr->start;
  }

  TripletResult winningTriplet;
  memset (&winningTriplet, 0, sizeof(TripletResult));

  // at this point we have a list of strings of the same size
  for (uint32_t i = 0; i < STRING_OF_MAX_LEN; i++) {
    FixedLenStringArray  *fixedStringArrayOfLenI = &fixedLenStrings[i];
    const char          **strings = fixedStringArrayOfLenI->strings;
    size_t                stringLen = fixedStringArrayOfLenI->len; // should be = i

    if (fixedStringArrayOfLenI->count == 0) // FIXME! if len < x
      continue;

    // use a hash table for all triplets of length N
    TripletStringHash *tsh = tshInit (1.2*fixedStringArrayOfLenI->count);

    for (uint32_t j = 0; j < fixedStringArrayOfLenI->count; j++) {
      uint32_t tripletHash = fnvHash32v ((const uint8_t *)strings[j], stringLen);
      tshAdd (tsh, strings[j], stringLen, tripletHash);
    }

    TripletResult tripletResult;
    memset (&tripletResult, 0, sizeof(TripletResult));
    tshGetThreeTripletsWithHighestCount (tsh, &tripletResult);
    mergeTriplets (&winningTriplet, &tripletResult);

    tshFree(tsh);
  }


  printTriplet(&winningTriplet);


  //StringPtr *stringPtrOrderedByLen = calloc(stringPtrCount, sizeof(StringPtr));

  // order by length
  /*qsort(&stringPtrList[0], stringPtrIndex, sizeof(StringPtr), compareStringPtrs2);
  printf ("%d\n", stringPtrIndex);
  for (uint32_t i = 0; i < stringPtrIndex; i++) {
    printf ("%.*s\n", stringPtrList[i].len, stringPtrList[i].start);
  }*/
  // qsort(&idList[0], idListIndex, sizeof(uint64_t), compareUInt64);

  //reduceTrieToWordsThatAppearMoreThanOnce (trie);

  // Trie of tries
  /*printf ("nwords = %d, maxid = %d\n", lastId, maxId);
  printf ("----------------------------------------------------------------\n");
  triePrint (trie);*/

  trieFree (trie);
  free(stringPtrList);
}

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

  if (optimization == USE_TRIES)
    countTripletsFromMemoryUsingTries (addr, sb.st_size);
  else
    countTripletsFromMemoryUsingHashTable (addr, sb.st_size, optimization);

  cleanup:
    if (fd != -1)
      close(fd);

    if (addr != MAP_FAILED)
      munmap(addr, sb.st_size);

  return result;
}
