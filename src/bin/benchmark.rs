use podds::{Card, rank, eval5};
use std::time::{Duration, Instant};
use std::str::FromStr;
use itertools::Itertools;

fn main() {
    let mut cards: [Card; 5] = [
        Card::from_str("2H").unwrap(),
        Card::from_str("3H").unwrap(),
        Card::from_str("4H").unwrap(),
        Card::from_str("5H").unwrap(),
        Card::from_str("6H").unwrap(),
    ];
    cards.sort_by_key(|a| u8::MAX - rank(a));
    let start = Instant::now();
    let val = cards.iter().combinations(5).map(eval5).max();
    let duration = start.elapsed();
    println!("Time elapsed is: {:?}", duration);
}
