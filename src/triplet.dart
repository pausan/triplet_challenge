import 'dart:typed_data';
import 'dart:io';
import 'dart:collection';

void main(List<String> args) {
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

  Map<String, int> map = new Map<String, int>();

  List<String> words = buffer.toString().split(' ');
  for (int i = 2; i < words.length; i++) {
    String triplet = "${words[i - 2]} ${words[i - 1]} ${words[i]}";
    if (!map.containsKey(triplet))
      map[triplet] = 1;
    else
      map[triplet]++;
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
