const fs = require('fs');

const MAX_WORD_SIZE = 1024;

// -----------------------------------------------------------------------------
// allTripletsGenerator
//
// Returns a generator with all the words from the file once sanitized
// -----------------------------------------------------------------------------
function *allTripletsGenerator (raw) {
  const char_a = 'a'.charCodeAt(0);
  const char_z = 'z'.charCodeAt(0);

  const char_A = 'A'.charCodeAt(0);
  const char_Z = 'Z'.charCodeAt(0);

  const char_0 = '0'.charCodeAt(0);
  const char_9 = '9'.charCodeAt(0);

  const sanitizedWord = Buffer.alloc(raw.length, ' ', 'ascii');

  let lastChar = 0;

  let w1start = 0, w2start = 0, w3start = 0;

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
      if (j != w3start) {
        const tripletBytes = sanitizedWord.slice(w1start, j);

        j++;
        if (w3start > 0)
          yield tripletBytes.toString();

        w1start = w2start;
        w2start = w3start;
        w3start = j;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// findTriplets
//
// Counts all triplets
// -----------------------------------------------------------------------------
function findTriplets (file) {
  const tripletsByWordLength = new Array(MAX_WORD_SIZE);
  const triplets = allTripletsGenerator (fs.readFileSync (file));

  const tripletsCount = new Map();
  for (const triplet of triplets) {
    if (!tripletsCount.has (triplet)) {
      tripletsCount.set (triplet, 1);
    }
    else {
      tripletsCount.set (triplet, tripletsCount.get (triplet) + 1);
    }
  }

  // Print triplets with highest count
  let top1 = { c: 0, v : ''};
  let top2 = { c: 0, v : ''};
  let top3 = { c: 0, v : ''};
  for (var [triplet, count] of tripletsCount) {
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

  console.log (top1.v, '-', top1.c);
  console.log (top2.v, '-', top2.c);
  console.log (top3.v, '-', top3.c);
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

  findTriplets (fileToProcess);
}


// -----------------------------------------------------------------------------
// Entry point
// -----------------------------------------------------------------------------
main ();