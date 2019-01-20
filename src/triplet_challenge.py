#!/usr/bin/python3
import re
import os
import sys
import io
from collections import Counter

def getLookupTable ():
  lookup = [32]*256
  for i in range(ord('a'), ord('z')):
    lookup[i] = i

  for i in range(ord('0'), ord('9')):
    lookup[i] = i

  for i in range(ord('A'), ord('Z')):
    lookup[i] = i + ord('a') - ord('A')

  return lookup

def readAllWords (file):
  """ Reads all words from file using ioWriter, and uses '\n' as delimiter
  so that we can skip lines
  """
  ioWriter = io.StringIO()

  lookup = getLookupTable()
  transTable = str.maketrans ( { i : lookup[i] for i in range(0, 256) })

  wordRegex = re.compile(r'\w+')

  word = []
  with open(file, 'rt') as f:
    data = f.read().translate(transTable)

    # TODO: explore other options of returning words
    for s in data.split():
      if s:
        yield s

def getTriplets(words):
  triplet = []
  for word in words:
    triplet.append(word)
    if len(triplet) == 3:
      yield ' '.join(triplet)
      triplet = triplet[1:]

def printTopTriplets(triplets):
  topCount    = 3*[0]
  topTriplets = 3*['']

  for triplet, count in triplets.items():
    if count >= topCount[0]:
      topCount[2] = topCount[1]
      topCount[1] = topCount[0]
      topCount[0] = count

      topTriplets[2] = topTriplets[1]
      topTriplets[1] = topTriplets[0]
      topTriplets[0] = triplet

    elif count >= topCount[1]:
      topCount[2] = topCount[1]
      topCount[1] = count

      topTriplets[2] = topTriplets[1]
      topTriplets[1] = triplet

    elif count >= topCount[2]:
      topCount[2]    = count
      topTriplets[2] = triplet

  for i in range(3):
    print ('%s - %d' % (topTriplets[i], topCount[i]))
  return

def main ():
  if len(sys.argv) < 2:
    print ("Use: python %s <file>" % sys.argv[0])
    return -1

  words = readAllWords (sys.argv[1])

  triplets = Counter()
  for triplet in getTriplets(words):
    triplets[triplet] += 1

  printTopTriplets (triplets)
  return 0

if __name__ == '__main__':
  sys.exit (main())