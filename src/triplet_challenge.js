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
          yield tripletBytes;

        w1start = w2start;
        w2start = w3start;
        w3start = j;
      }
    }
  }
}

// -----------------------------------------------------------------------------
// fnv
//
// compute hash on given byteBuffer
// -----------------------------------------------------------------------------
function fnv(byteBuffer) {
  const PRIME = 16777619;
  let result = 0;
  for (let i = 0; i < byteBuffer.length; i++) {
    result ^= byteBuffer[i];
    result *= PRIME;
  }
  return (result < 0 ? -result : result);
}

// -----------------------------------------------------------------------------
// findHighestCountTripletsForArray
// -----------------------------------------------------------------------------
function findHighestCountTripletsForArray (triplets, countThreshold) {
  // first find all items that have collisions
  const collisionCounter = new Uint32Array(triplets.length / 8);
  const collisionLen = collisionCounter.length;

  for (let i = 0; i < triplets.length; i++) {
    const hash = fnv (triplets[i]) % collisionLen;
    collisionCounter[hash] ++;
  }

  let tripletsSubset = [];

  // now recompute again, but remove all those items that have collided less than
  // countThreshold
  for (let i = 0; i < triplets.length; i++) {
    const hash = fnv (triplets[i]) % collisionLen;
    if (collisionCounter[hash] >= countThreshold) {
      tripletsSubset.push (triplets[i].toString());
    }
  }

  const tripletsCount = new Map();

  for (let i = 0; i < tripletsSubset.length; i++) {
    const triplet = tripletsSubset[i];
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

  return [
    { v : top1.v, c : top1.c },
    { v : top2.v, c : top2.c },
    { v : top3.v, c : top3.c }
  ];
}

// -----------------------------------------------------------------------------
// mergeTriplets
// -----------------------------------------------------------------------------
function mergeTriplets (winningTriplet, partialTriplet) {
  for (let i = 0; i < partialTriplet.length; i++) {
    const count = partialTriplet[i].c;
    const triplet = partialTriplet[i].v;

    if (count >= winningTriplet[0].c)  {
      winningTriplet[2] = winningTriplet[1];
      winningTriplet[1] = winningTriplet[0];
      winningTriplet[0] = { c: count, v : triplet};
    }

    else if (count >= winningTriplet[1].c)  {
      winningTriplet[2] = winningTriplet[1];
      winningTriplet[1] = { c: count, v : triplet};
    }

    else if (count >= winningTriplet[2].c)  {
      winningTriplet[2] = { c: count, v : triplet};
    }
  }

  return winningTriplet;
}

// -----------------------------------------------------------------------------
// findTriplets
//
// Counts all triplets
// -----------------------------------------------------------------------------
function findTriplets (file) {
  const tripletsByWordLength = new Array(MAX_WORD_SIZE);
  const triplets = allTripletsGenerator (fs.readFileSync (file));

  // initialize all lengths with an empty array
  for (let i = 0; i < tripletsByWordLength.length; i++)
    tripletsByWordLength[i] = [];

  // insert each triplet to the bucket with their length
  for (const triplet of triplets) {
    tripletsByWordLength[triplet.length].push (triplet);
  }

  // solve the problem for a subarray
  let winningTriplet = [ { v : '', c: 0} , { v : '', c : 0}, { v : '', c: 0} ];

  for (let i = 0; i < tripletsByWordLength.length; i++) {
    if (tripletsByWordLength[i].length == 0)
      continue;

    let countThreshold = winningTriplet[2].c;
    let partialTriplet = findHighestCountTripletsForArray (tripletsByWordLength[i], countThreshold);

    // merge triplets
    winningTriplet = mergeTriplets (winningTriplet, partialTriplet);
  }

  console.log (winningTriplet[0].v.toString(), '-', winningTriplet[0].c);
  console.log (winningTriplet[1].v.toString(), '-', winningTriplet[1].c);
  console.log (winningTriplet[2].v.toString(), '-', winningTriplet[2].c);
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
/*a = Buffer.from([1, 2, 3, 4, 5, 6])
b = [];

b.push (a.slice(1, 3))
b.push (a.slice(2, 4))
console.log (b);*/