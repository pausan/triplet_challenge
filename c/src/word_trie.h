// -----------------------------------------------------------------------------
// Author: Pau Sanchez (contact@pausanchez.com)
// License: MIT/GPLv3
// -----------------------------------------------------------------------------
#ifndef _WORD_TRIE_H_
#define _WORD_TRIE_H_

#include <stdint.h>
#include <stdlib.h>

// 26 for alphabet ('a' to 'z'), plus 10 for digits ('0' to '9'), plus space
#define WORD_TRIE_ALPHABET_SIZE (26+10+1)

typedef struct _triplet_trie_t {
  struct _triplet_trie_t *children37[WORD_TRIE_ALPHABET_SIZE];
  uint32_t index;
  uint32_t count;

  //const char *ptr;
  //const char *end;

  // NOTE: any trie can be a node and can have more children
  // Use this structure to avoid creating more children
  // than required
  // struct {
  //   const char     *ptr;
  //   const char     *end;
  //   uint32_t  count;
  // } node;
} WordTrie;

WordTrie *trieInit ();
void trieFree (WordTrie *trie);

uint32_t trieAdd (WordTrie *trie, const char *start, const char *end);
uint32_t trieAddWithLen (WordTrie *trie, const char *start, size_t len);

void triePrint (WordTrie *trie);



// FIXME! check with a conversion lookup table, which is simpler, no idea if faster
// values from 0 to 36 included
#define CHAR_TO_INDEX(x)  __CHAR_TO_INDEX_ALNUM( (x) )
#define INDEX_TO_CHAR(x)  __INDEX_TO_CHAR_ALNUM( (x) )

#define __CHAR_TO_INDEX_ALNUM(x)  \
  (                               \
    (x >= 'a' && x <= 'z')        \
    ? (x - 'a')                   \
    : (('z'-'a' + 1) + (x - '0')) \
  )

#define __INDEX_TO_CHAR_ALNUM(x)  \
  (                               \
    (x <= 25)                     \
    ? ('a' + x)                   \
    : ('0' + (x - 26))            \
  )

#define __CHAR_TO_INDEX_ALNUMSPACE(x)  \
  (                                    \
    (x >= 'a')                         \
    ? (x - 'a')                        \
    : (                                \
      (x >= '0')                       \
      ? (('z'-'a' + 1) + (x - '0'))    \
      : 36                             \
    )                                  \
  )

#define __INDEX_TO_CHAR_ALNUMSPACE(x)  \
  (                               \
    (x <= 25)                     \
    ? ('a' + x)                   \
    : (                           \
        (x != 36)                 \
        ? ('0' + (x - 26))        \
        : ' '                     \
    )                             \
  )


#endif /* _WORD_TRIE_H_ */
