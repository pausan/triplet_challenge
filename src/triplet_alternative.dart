import 'dart:typed_data';
import 'dart:io';
import 'dart:collection';

Iterable<int> wordIndexes(String text) sync* {
  for (int i = 0; i < text.length; i++) {
    if (text[i] == ' ')
      yield i;
  }

  yield text.length;
}


void main() {
  if (args.length < 1) {
    print ("Use: triplet <file>");
    return;
  }

  Uint8List contents = (new File(args[0])).readAsBytesSync();

  final int a = 'a'.codeUnitAt(0);
  final int z = 'z'.codeUnitAt(0);
  final int A = 'A'.codeUnitAt(0);
  final int Z = 'Z'.codeUnitAt(0);

  StringBuffer buffer = new StringBuffer();
  bool space = false;

  for (int i = 0; i < contents.length; i++) {
    var c = contents[i];
    if (c >= a && c <= z) {
      if (space) buffer.writeCharCode(32);

      space = false;
      buffer.writeCharCode(c);
    } else if (c >= A && c <= Z) {
      if (space) buffer.writeCharCode(32);

      space = false;
      buffer.writeCharCode(c - A + a);
    } else if (!space) {
      space = true;
    }
  }

  HashMap<String, int> map = new HashMap<String, int>();

  String text = buffer.toString();
  int w1start = 0, w2start = 0, w3start = 0;
  int wcount = 0;
  for (int i in wordIndexes(text)) {
    if (wcount >= 2) {
      String triplet = text.substring(w1start, i);
      if (!map.containsKey(triplet))
        map[triplet] = 1;
      else
        map[triplet]++;
    }

    w1start = w2start;
    w2start = w3start;
    w3start = i + 1;
    wcount++;
  }


  int w1count = 0, w2count = 0, w3count = 0;
  String w1, w2, w3;

  map.forEach((triplet, count) {
    if (count > w1count) {
      w3 = w2;
      w3count = w2count;

      w2 = w1;
      w2count = w1count;

      w1count = count;
      w1 = triplet;
    } else if (count > w2count) {
      w3 = w2;
      w3count = w2count;

      w2count = count;
      w2 = triplet;
    } else if (count > w3count) {
      w3count = count;
      w3 = triplet;
    }
  });

  print("${w1} - ${w1count}");
  print("${w2} - ${w2count}");
  print("${w3} - ${w3count}");
}
