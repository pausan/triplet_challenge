// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "word_trie.h"

static WordTrie *_trie_pool = NULL;
static uint32_t _trie_pool_index = 0;
static uint32_t _trie_pool_max = 1000 * 1000;
static uint32_t _trie_count = 0;

// -----------------------------------------------------------------------------
// trieInit
// -----------------------------------------------------------------------------
WordTrie *trieInit () {

  // NOTE: as a trick, we might create a big pool of tries preinitialized
  //       so that we just increment variables everytime we have to create
  //       a new child, and we don't lose time
  if (_trie_pool == NULL) {
    _trie_pool = (WordTrie*)calloc (_trie_pool_max, sizeof (WordTrie));
    _trie_pool_index = 0;
  }

  // create fist trie... since we know it will grow, we assign a set of children
  // already
  WordTrie *trie = (WordTrie*)calloc (1, sizeof (WordTrie));

  return trie;
}

// -----------------------------------------------------------------------------
// trieFree
// -----------------------------------------------------------------------------
void trieFree (WordTrie *trie) {
  trie++; // ignore warning

  // FIXME! properly free memory here
  free (_trie_pool);
}

// -----------------------------------------------------------------------------
// trieAdd
//
// Adds a new element to the trie, if that element exists, it increments its
// count value (only for leafs)
// -----------------------------------------------------------------------------
uint32_t trieAdd (WordTrie *trie, const char *start, const char *end) {
  //printf ("Trie add: %.*s -------------\n", (int)(end - start + 1), start);
  if (trie == NULL)
    return -1;

  //printf ("word = %.*s\n", (int)(end - start + 1), start);

  // first iterate through the tree & add all missing chars
  for ( ; start < end; start++) {
    const uint8_t c = (char)CHAR_TO_INDEX(*start);
    //printf ("ptr = %c %d\n", *start, (int)c);

    // let's always expand (TODO: it can be optimized by setting substring and stopping)
    if (trie->children37[c] == NULL) {
      // printf ("Expanding trie! (pool index = %d -> %d)\n", _trie_pool_index, _trie_pool_index + WORD_TRIE_ALPHABET_SIZE);
      trie->children37[c] = &_trie_pool[_trie_pool_index++];

      //printf ("_trie_pool_index = %d %d\n", _trie_pool_index, _trie_pool_max);
      // FIXME! check _trie_pool_index against max reserved items
    }

    trie = trie->children37[c];
  }

  // finally assign a new index to this word (if no index given)
  if (trie->index == 0) {
    trie->index = (++_trie_count); // start in 1
    // printf ("Setting Index: %d\n", trie->index);
  }

  trie->count++;

  return trie->index;
}

// -----------------------------------------------------------------------------
// trieAddWithLen
//
// Like trieAdd but using length instead of pointers.
// -----------------------------------------------------------------------------
uint32_t trieAddWithLen (WordTrie *trie, const char *start, size_t len) {
  return trieAdd (trie, start, (const char*)start + len);
}

// -----------------------------------------------------------------------------
// _triePrintRecursively
//
// Prints trie recursively
// -----------------------------------------------------------------------------
static void _triePrintRecursively (WordTrie *trie, char *trieWord, int level) {
  if ((trie == NULL) || (trie->children37 == NULL))
    return;

  for (size_t i = 0; i < WORD_TRIE_ALPHABET_SIZE; i++)
  {
    WordTrie *child = trie->children37[i];
    if (child == NULL)
      continue;

    trieWord[level]   = INDEX_TO_CHAR(i);
    if (child->index > 0)
      printf ("-%s- (id = %d, count = %d)\n", trieWord, child->index, child->count);

    if (child) {
      _triePrintRecursively (child, trieWord, level + 1);
      trieWord[level+1] = '\0';
    }
  }
}

// -----------------------------------------------------------------------------
// triePrint
//
// Prints all tries
// -----------------------------------------------------------------------------
void triePrint (WordTrie *trie) {
  // a word can be 64k long max (or core dump!)
  static char _trieWord [65536];
  memset (&_trieWord[0], 0, sizeof(_trieWord));

  printf ("\n\n");
  printf ("Printing trie!\n");

  _triePrintRecursively (trie, _trieWord, 0);
}


/*
  WordTrie *trie = trieInit ();
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
  trieFree(trie);
  exit(-1);
*/