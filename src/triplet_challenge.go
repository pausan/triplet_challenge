package main

import (
  "os"
  "fmt"
  "io/ioutil"
)

type Triplet struct {
  triplet  string
  count    int
}

// -----------------------------------------------------------------------------
// sanitizeData
// -----------------------------------------------------------------------------
func sanitizeData (data []byte) []byte {
  var lastChar byte = 0
  var j int = 0

  for i := 0; i < len(data); i++ {
    char := data[i]

    if ((char >= 'a') && (char <= 'z')) {
      // ignore
    } else if ((char >= 'A') && (char <= 'Z')) {
      char = char - 'A' + 'a'
    } else if ((char >= '0') && (char <= '9')) {
      // ignore
    } else {
      char = 32
      if (lastChar == char) {
        continue
      }
    }

    data[j] = char
    j++
    lastChar = char
  }

  return data[0:j]
}

// -----------------------------------------------------------------------------
// findWinningTriplets
// -----------------------------------------------------------------------------
func findWinningTriplets (counter map[string]int) []Triplet {
  t1, t2, t3 := "", "", ""
  c1, c2, c3 :=  0,  0,  0

  for triplet, count := range counter {
    if (count > c1) {
      c3 = c2
      t3 = t2

      c2 = c1
      t2 = t1

      c1 = count
      t1 = triplet
    } else if (count > c2) {
      c3 = c2
      t3 = t2

      c2 = count
      t2 = triplet
    } else if (count > c3) {
      c3 = count
      t3 = triplet
    }
  }

  return []Triplet {
    Triplet{t1, c1},
    Triplet{t2, c2},
    Triplet{t3, c3},
  }
}

// -----------------------------------------------------------------------------
// fnvHash32
// Compute a string hash
// -----------------------------------------------------------------------------
func fnvHash32(data []byte) uint32 {
  const PRIME = 16777619
  var result uint32 = 0

  // continue iterating through the rest of the string
  for i := 0; i < len(data); i++ {
    result ^= uint32(data[i])
    result *= PRIME
  }

  return result
}

// -----------------------------------------------------------------------------
// findHighestTripletsWithBounding
// -----------------------------------------------------------------------------
func findHighestTripletsWithBounding(tripletsOfLenghtX [][]byte, threshold int) []Triplet {
  var counter map[ string ]int = make(map[string]int)

  var hashes []uint32 = make ([]uint32, len(tripletsOfLenghtX))
  var collisionMap []int = make ([]int, 10 + len(tripletsOfLenghtX) / 8)
  var collisionMapLen uint32 = uint32(len(collisionMap))

  for i := 0; i < len(tripletsOfLenghtX); i++ {
    hash := fnvHash32 (tripletsOfLenghtX[i]) % collisionMapLen
    collisionMap[hash] ++
    hashes[i] = hash
  }

  newLen := 0
  for i := 0; i < len(tripletsOfLenghtX); i++ {
    if collisionMap [hashes[i]] <= threshold {
      continue
    }

    tripletsOfLenghtX[newLen] = tripletsOfLenghtX[i]
    newLen++
  }

  tripletsOfLenghtX = tripletsOfLenghtX[0:newLen]

  for i := 0; i < len(tripletsOfLenghtX); i++ {
    triplet := string(tripletsOfLenghtX[i])
    counter [triplet]++
  }

  return findWinningTriplets(counter)
}

// -----------------------------------------------------------------------------
// mergeTriplets
// -----------------------------------------------------------------------------
func mergeTriplets (winning []Triplet, partial []Triplet) {
  for i := 0; i < len(winning); i++ {
    if partial[i].count > winning[0].count {
      winning[1] = winning[0]
      winning[2] = winning[1]
      winning[0] = partial[i]
    } else if partial[i].count > winning[1].count {
      winning[2] = winning[1]
      winning[1] = partial[i]
    } else if partial[i].count > winning[2].count {
      winning[2] = partial[i]
    }
  }
}

// -----------------------------------------------------------------------------
// getTripletsPerLength
// -----------------------------------------------------------------------------
func getTripletsPerLength (sanitized []byte) [][][]byte {

  var tripletsPerLength [][][]byte = make([][][]byte, 128)

  off1, off2, off3 := 0, 0, 0
  for off:= 0; off < len(sanitized); off++ {
    if (sanitized[off] != 32) {
      continue
    }

    if off2 != 0 {
      triplet := sanitized[off3 : off]
      tripletsPerLength[len(triplet)] = append (tripletsPerLength[len(triplet)], triplet)
    }

    off3 = off2
    off2 = off1
    off1 = off + 1
  }

  return tripletsPerLength
}


// -----------------------------------------------------------------------------
// findTriplets
// -----------------------------------------------------------------------------
func findTriplets (data []byte) {
  var sanitized []byte = sanitizeData (data)

  tripletsPerLength := getTripletsPerLength(sanitized)

  // iterate on each bucket of size X
  var winningTriplets = []Triplet {
    Triplet{"", 0},
    Triplet{"", 0},
    Triplet{"", 0},
  }

  for i := 0; i < len(tripletsPerLength); i++ {
    if len(tripletsPerLength[i]) == 0 {
      continue
    }

    threshold := winningTriplets[2].count
    partialTriplets := findHighestTripletsWithBounding (tripletsPerLength[i], threshold)
    mergeTriplets(winningTriplets, partialTriplets)
  }

  for i := 0; i < len(winningTriplets); i++ {
    fmt.Printf ("%s - %d\n", winningTriplets[i].triplet, winningTriplets[i].count)
  }
}

// -----------------------------------------------------------------------------
// main
// -----------------------------------------------------------------------------
func main() {
  if len(os.Args) < 2 {
    panic ("Invalid parameters")
  }

  data, err := ioutil.ReadFile (os.Args[1])
  if err != nil {
    panic ("Cannot open file")
  }

  findTriplets(data)
}
