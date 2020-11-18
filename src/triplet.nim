import tables
import strutils
import os
import system

if os.paramCount() < 1:
  echo("Use: ./" & os.paramStr(0) & " <input-file>")
  system.quit(-1)


# let data = readFile("gutenberg-utf-8.txt")
let data = readFile(paramStr(1))
# let data = readFile("small.txt")

var normText:string = ""
var isSpace = false

for c in data:
  if strutils.isAlphaNumeric(c):
    normText.add(strutils.toLowerAscii(c))
    isSpace = false
  elif strutils.isSpaceAscii(c) and (not isSpace):
    normText.add(' ')
    isSpace = true
  elif not isSpace:
    normText.add(' ')
    isSpace = true

var total = tables.initCountTable[string]()

var w1, w2, w3 : string
var nwords = 0

for word in normText.split():
  if word == "":
    continue

  case nwords mod 3:
  of 0:
    w1 = word
  of 1:
    w2 = word
  of 2:
    w3 = word
  else:
    echo "modulo error"

  nwords += 1
  if nwords <= 2:
    continue

  case nwords mod 3:
  of 0:
    total.inc (w1 & " " & w2 & " " & w3)
  of 1:
    total.inc (w2 & " " & w3 & " " & w1)
  of 2:
    total.inc (w3 & " " & w1 & " " & w2)
  else:
    echo "modulo error"


var counter = 0

total.sort()
for item in total.pairs():
  if counter >= 3:
    break
  echo item[0], " - ", item[1]
  counter += 1

# echo total
# total.pairs() --> take(10).foreach(echo it[0] & " " & $it[1])
# echo GC_getStatistics()