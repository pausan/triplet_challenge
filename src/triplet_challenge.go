package main

import (
  "os"
  "fmt"
  "io/ioutil"
)

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
// countTriplets
// -----------------------------------------------------------------------------
func countTriplets (sanitized []byte) map[string]int {
  var counter map[ string ]int = make(map[string]int)

  off1, off2, off3 := 0, 0, 0
  for off:= 0; off < len(sanitized); off++ {
    if (sanitized[off] != 32) {
      continue
    }

    if off2 != 0 {
      triplet := string(sanitized[off3 : off])
      counter[triplet]++
    }

    off3 = off2
    off2 = off1
    off1 = off + 1
  }

  return counter
}


func printHighestThree (counter map[string]int) {
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

  fmt.Printf ("%s - %d\n", t1, int(c1))
  fmt.Printf ("%s - %d\n", t2, int(c2))
  fmt.Printf ("%s - %d\n", t3, int(c3))
}

// -----------------------------------------------------------------------------
// findTriplets
// -----------------------------------------------------------------------------
func findTriplets (data []byte) {
  var sanitized []byte = sanitizeData (data)

  counter := countTriplets(sanitized)
  printHighestThree (counter)
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
