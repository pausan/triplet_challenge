#!/usr/bin/python3
import sys

from triplet_utils import *

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