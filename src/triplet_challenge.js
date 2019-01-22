const fs = require('fs');

const MAX_WORD_SIZE = 1024;

const HASH_TABLE_PRIMES = [
  389, 769, 1543, 3079, 6151, 12289, 24593, 49157, 98317, 196613,
  786433, 1572869, 3145739, 6291469, 12582917, 50331653, 100663319,
  201326611
];

// -----------------------------------------------------------------------------
// HashTableCounter
//
// Custom hash table
// -----------------------------------------------------------------------------
class HashTableCounter {
  constructor (size) {
    let optimalSize = 1.25*size;
    for (let i = 0; i < HASH_TABLE_PRIMES.length; i++) {
      if (HASH_TABLE_PRIMES[i] > optimalSize) {
        optimalSize = HASH_TABLE_PRIMES[i]
        break;
      }
    }

    this.counters = new Uint32Array(optimalSize);
    this.triplets = new Array(optimalSize);
    this.hashTableSize = optimalSize;
  }

  fnv(word, result) {
    const PRIME = 16777619;
    for (let i = 0; i < word.length; i++) {
      result ^= word[i];
      result *= PRIME;
    }

    return (result < 0 ? -result : result);
  }

  addTriplet (triplet) {
    const hash2 = this.fnv(triplet[2], 0);
    const hash1 = this.fnv(triplet[1], hash2);
    const hash0 = this.fnv(triplet[1], hash1);
    const offset = hash0 % this.hashTableSize;

    while (
      this.triplets[offset] !== undefined
      && !this.tripletEquals (this.triplets[offset], triplet)
    ) {
      offset++;
      if (offset > this.hashTableSize) {
        offset = 0;
      }
    }
    this.triplets [offset] = triplet;
    this.counters [ offset ] ++;
  }

  tripletEquals (a, b) {
    for (let i = 0; i < 3; i++) {
      if (a[i] != b[i])
        return false;
    }

    return true;
  }
};

// -----------------------------------------------------------------------------
// getSanitizedWordsGenerator
//
// Returns all words from the file once sanitized
// -----------------------------------------------------------------------------
function *getSanitizedWordsGenerator (raw) {
  const char_a = 'a'.charCodeAt(0);
  const char_z = 'z'.charCodeAt(0);

  const char_A = 'A'.charCodeAt(0);
  const char_Z = 'Z'.charCodeAt(0);

  const char_0 = '0'.charCodeAt(0);
  const char_9 = '9'.charCodeAt(0);

  const sanitizedWord = Buffer.alloc(MAX_WORD_SIZE, ' ', 'ascii');

  let words = [];
  let lastChar = 0;
  for (let i = 0, j = 0; i < raw.length; i++) {
    let char = raw[i];

    if (char >= char_a && char <= char_z) {
      sanitizedWord[j++] = char;
    }
    else if (char >= char_A && char <= char_Z) {
      sanitizedWord[j++] = char + (char_a - char_A);
    }
    else if (char >= char_0 && char <= char_9) {
      sanitizedWord[j++] = char;
    }
    else {
      if (j) {
        const wordBytes = sanitizedWord.slice(0, j);
        yield wordBytes;
      }

      j = 0; // restart
    }
  }
}

// -----------------------------------------------------------------------------
// readSanitizedWords
//
// Read file, sanitizes it, and returns all input words
// -----------------------------------------------------------------------------
function readSanitizedWords (file) {
  const allWordsGenerator = getSanitizedWordsGenerator (fs.readFileSync (file));
  return allWordsGenerator;
}

// -----------------------------------------------------------------------------
// getTripletsGenerator
//
// Yields all triplets using a generator
// -----------------------------------------------------------------------------
function *getTripletsGenerator (allWordsGenerator) {
  const triplet = [];
  for (const wordBytes of allWordsGenerator) {
    triplet.push (new Buffer(wordBytes));
    if (triplet.length == 3) {
      yield triplet;
      triplet.shift();
    }
  }
}


// -----------------------------------------------------------------------------
// findTriplets
//
// Counts all triplets
// -----------------------------------------------------------------------------
function findTriplets (allWordsGenerator) {
  const tripletsByWordLength = new Array(MAX_WORD_SIZE);
  const tripletsCount = {};

  const tripletsCounter = new HashTableCounter(50000); // FIXME!

  for (const triplet of getTripletsGenerator (allWordsGenerator)) {
    tripletsCounter.addTriplet (triplet);
  }

  // Print triplets with highest count
  let top1 = { c: 0, v : ''};
  let top2 = { c: 0, v : ''};
  let top3 = { c: 0, v : ''};

  for (let i = 0; i < tripletsCounter.hashTableSize; i++) {
    const triplet = tripletsCounter.triplets[i];
    const count = tripletsCounter.counters[i];

    if (count >= top1.c)  {
      top3 = top2;
      top2 = top1;
      top1 = { c: count, v : triplet};
    }

    else if (count >= top2.c)  {
      top3 = top2;
      top2 = { c: count, v : triplet};
    }

    else if (count >= top3.c)  {
      top3 = { c: count, v : triplet};
    }
  }

  console.log (top1.v.join (' '), '-', top1.c);
  console.log (top2.v.join (' '), '-', top2.c);
  console.log (top3.v.join (' '), '-', top3.c);
}


// -----------------------------------------------------------------------------
// Main method
// -----------------------------------------------------------------------------
function main () {
  if (process.argv.length <= 2) {
    console.log ("Use: node " + process.argv[1] + " <file>")
    process.exit(-1);
  }

  fileToProcess = process.argv[2];

  const allWords = readSanitizedWords (fileToProcess);
  findTriplets (allWords);
}


// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------
main ();