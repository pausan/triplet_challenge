use std::{
  fs,
  env,
  collections::HashMap,
  vec::Vec,
};

#[derive(Clone)]
struct Triplet {
  triplet : String,
  count : u32
}

impl Triplet {
  fn new() -> Triplet {
    Triplet { triplet : "".to_string(), count : 0 }
  }
}

#[derive(Clone)]
struct Top3Triplets {
  gold   : Triplet,
  silver : Triplet,
  bronze : Triplet
}

impl Top3Triplets {
  fn show(&self) {
    // print winning triplets
    println!("{} - {}", self.gold.triplet, self.gold.count);
    println!("{} - {}", self.silver.triplet, self.silver.count);
    println!("{} - {}", self.bronze.triplet, self.bronze.count);
  }

  fn merge(&mut self, other : &Top3Triplets) {
    for triplet in &[&other.gold, &other.silver, &other.bronze] {
      if triplet.count >= self.gold.count {
        self.bronze = self.silver.clone();
        self.silver = self.gold.clone();

        self.gold.count = triplet.count;
        self.gold.triplet = triplet.triplet.clone();
      }
      else if triplet.count >= self.silver.count {
        self.bronze = self.silver.clone();

        self.silver.count = triplet.count;
        self.silver.triplet = triplet.triplet.clone();
      }
      else if triplet.count >= self.bronze.count {
        self.bronze.count = triplet.count;
        self.bronze.triplet = triplet.triplet.clone();
      }
    }
  }
}

#[inline]
fn bucketize(bytes : &[u8]) -> u32 {
  const PRIME : u32 = 16777619;

  let len : u32 = bytes.len() as u32;
  let mut result : u32 = len ^ PRIME;

  result = result.wrapping_mul (bytes[0] as u32);
  result = result ^ (bytes[((len-1)/2) as usize] as u32);
  result = result.wrapping_mul (bytes[(len-1) as usize] as u32);

  result
}

fn find_top3triplets_from_hashmap (triplets_hash : &HashMap<&str, u32>) -> Top3Triplets {
  let mut winners : Top3Triplets = Top3Triplets {
    gold : Triplet::new(),
    silver : Triplet::new(),
    bronze : Triplet::new()
  };

  for (triplet, count) in triplets_hash.iter() {
    if (*count) >= winners.gold.count {
      winners.bronze = winners.silver.clone();
      winners.silver = winners.gold.clone();

      winners.gold.count = *count;
      winners.gold.triplet = triplet.to_string();
    }
    else if (*count) >= winners.silver.count {
      winners.bronze = winners.silver.clone();

      winners.silver.count = *count;
      winners.silver.triplet = triplet.to_string();
    }
    else if (*count) >= winners.bronze.count {
      winners.bronze.count = *count;
      winners.bronze.triplet = triplet.to_string();
    }
  }

  winners
}

fn find_top3_triplets (triplets : &Vec<&str>) -> Top3Triplets {
  let mut triplets_hash : HashMap<&str, u32> = HashMap::default();
  for triplet in triplets {
    if let Some(x) = triplets_hash.get_mut(triplet) {
      *x += 1;
    }
    else {
      triplets_hash.insert (triplet, 1);
    }
  }

  find_top3triplets_from_hashmap(&triplets_hash)
}


fn process_string_triplets (s : &str, _nwords: u32) {
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

  const NUM_BUCKETS : usize = (1 << 13);
  const BUCKETS_MASK : u32 = (NUM_BUCKETS - 1) as u32;
  let mut triplet_buckets : Vec<Vec<&str>> = vec![
    Vec::new();
    NUM_BUCKETS
  ];

  for word in whitespace_iter {
    w3len = word.len();
    end += 1 + w3len;

    // NOTE: I don't understand why start+1 and end+1
    let triplet = &s[start+1..end+1];
    //let bucket : u32 = triplet.len() as u32;
    let bucket : u32 = bucketize(triplet.as_bytes()) & BUCKETS_MASK;

    triplet_buckets[bucket as usize].push(triplet);

    start += w1len + 1;
    w1len = w2len;
    w2len = w3len;
  }

  // sort buckets by len to explore most promising first
  triplet_buckets.sort_unstable_by(|a, b| b.len().cmp(&a.len()));

  // initialize top triplets from most promising bucket (the one with more items)
  let mut top3triplets : Top3Triplets = find_top3_triplets(
    &triplet_buckets[0]
  );

  // explore the rest of buckets
  for i in 1..triplet_buckets.len() {
    // has our bronze winner more items than this bucket? then why explore it?
    if (triplet_buckets[i].len() as u32) < top3triplets.bronze.count {
      continue;
    }

    let winner_for_bucket : Top3Triplets = find_top3_triplets(
      &triplet_buckets[i]
    );

    top3triplets.merge (&winner_for_bucket);
  }

  top3triplets.show();
}

fn sanitize_words (contents : String) -> (String, u32) {
  let mut out_str : String = String::with_capacity(contents.len());
  let mut last_added : bool = true;
  let mut nwords : u32 = 0;

  for c in contents.chars() {
    if c.is_alphanumeric() {
      if !last_added {
        out_str.push (' ');
        last_added = true;
        nwords += 1;
      }

      if c >= 'A' && c <= 'Z' {
        out_str.push (
          ((c as u8) - b'A' + b'a') as char
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

  return (out_str, nwords);
}

fn process_file_triplets (file_name : &String) {
  let file_contents : String = fs::read_to_string(file_name).expect("Expecting a readable file");
  let (out_str, nwords) = sanitize_words (file_contents);
  process_string_triplets (&out_str, nwords);
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
