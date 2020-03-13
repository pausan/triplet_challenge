use std::collections::HashMap;
use std::{
  fs,
  env
};

fn process_string_triplets (s : &str) {
  let mut triplets : HashMap<&str, u32> = HashMap::new();
  let mut whitespace_iter = s.split_whitespace();

  let mut word;
  let mut w1len;
  let mut w2len;
  let mut w3len;
  let mut start;
  let mut end;

  // first word
  word = whitespace_iter.next();
  w1len = word.unwrap().len();

  // second word
  word = whitespace_iter.next();
  w2len = word.unwrap().len();

  start = 0;
  end = w1len + 1 + w2len;

  for word in whitespace_iter {
    w3len = word.len();
    end += 1 + w3len;

    // NOTE: I don't understand why start+1 and end+1
    let triplet = &s[start+1..end+1];
    if let Some(x) = triplets.get_mut(triplet) {
      *x += 1;
    }
    else {
      triplets.insert (triplet, 1);
    }

    start += w1len + 1;
    w1len = w2len;
    w2len = w3len;
  }

  // find top-3 triplets

  let mut win_count: [u32; 3] = [0; 3];
  let mut winning: [&str; 3] = [""; 3];

  for (triplet, count) in triplets.iter() {
    if (*count) >= win_count[0] {
      win_count[2] = win_count[1];
      win_count[1] = win_count[0];
      win_count[0] = *count;

      winning[2] = winning[1];
      winning[1] = winning[0];
      winning[0] = triplet;
    }
    else if (*count) >= win_count[1] {
      win_count[2] = win_count[1];
      win_count[1] = *count;

      winning[2] = winning[1];
      winning[1] = triplet;
    }
    else if (*count) >= win_count[2] {
      win_count[2] = *count;
      winning[2] = triplet;
    }
  }

  // solution
  for i in 0..3 {
    println!("{} - {}", winning[i], win_count[i]);
  }
}

fn sanitize_words (contents : String) -> String {
  let mut out_str : String = String::with_capacity(contents.len());
  let mut last_added = true;

  for c in contents.chars() {
    if c.is_alphanumeric() {
      if !last_added {
        out_str.push (' ');
        last_added = true;
      }

      if c >= 'A' && c <= 'Z' {
        out_str.push (
          ((c as u8) - ('A' as u8) + ('a' as u8)) as char
        );
      }
      else {
        out_str.push(c);
      }
    }
    else {
      last_added = false;
    }
  }

  out_str
}

fn process_file_triplets (file_name : &String) {
  let file_contents : String = fs::read_to_string(file_name).expect("Expecting a readable file");
  let out_str : String = sanitize_words (file_contents);
  process_string_triplets (&out_str);
}

fn main(){
  let args: Vec<String> = env::args().collect();

  match args.len() {
    1 => { help(); },
    _ => { process_file_triplets (&args[1]); }
  }
}

fn help() {
  print!("usage:\ntriplet_challenge <file>");
}
