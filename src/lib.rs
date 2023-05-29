use std::str::FromStr;

use itertools::Itertools;

#[derive(Copy, Clone, Debug, PartialEq)]
enum Rank {
    Two,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace,
}

#[derive(Copy, Clone, Debug, PartialEq)]
enum Suit {
    Clubs,
    Diamonds,
    Hearts,
    Spades,
}

impl FromStr for Rank {
    type Err = ();
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "2" => Ok(Rank::Two),
            "3" => Ok(Rank::Three),
            "4" => Ok(Rank::Four),
            "5" => Ok(Rank::Five),
            "6" => Ok(Rank::Six),
            "7" => Ok(Rank::Seven),
            "8" => Ok(Rank::Eight),
            "9" => Ok(Rank::Nine),
            "T" => Ok(Rank::Ten),
            "J" => Ok(Rank::Jack),
            "Q" => Ok(Rank::Queen),
            "K" => Ok(Rank::King),
            "A" => Ok(Rank::Ace),
            _ => Err(()),
        }
    }
}

impl FromStr for Suit {
    type Err = ();
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "C" => Ok(Suit::Clubs),
            "D" => Ok(Suit::Diamonds),
            "H" => Ok(Suit::Hearts),
            "S" => Ok(Suit::Spades),
            _ => Err(()),
        }
    }
}

#[derive(Debug, PartialEq)]
pub struct Card {
    rank: Rank,
    suit: Suit,
}

impl FromStr for Card {
    type Err = ();
    fn from_str(s: &str) -> Result<Self, Self::Err> {
        let rank = s[0..1].parse()?;
        let suit = s[1..2].parse()?;
        Ok(Card { rank, suit })
    }
}

pub fn rank(card: &Card) -> u8 {
    card.rank as u8
}

pub fn suit(card: &Card) -> u8 {
    card.suit as u8
}

const SFLUSH_SHIFT: u8 = 42;
const FOAK_SHIFT: u8 = 38;
const FULL_SHIFT: u8 = 37;
const FLUSH_SHIFT: u8 = 36;
const STRAIGHT_SHIFT: u8 = 32;
const TOAK_SHIFT: u8 = 28;
const PAIR2_SHIFT: u8 = 24;
const PAIR1_SHIFT: u8 = 20;
const HC_SHIFT: u8 = 0;

const LOSS: u8 = 0;
const DRAW: u8 = 1;
const WIN: u8 = 2;

pub fn eval5(cs: Vec<&Card>) -> i64 {
    assert!(cs.len() == 5);
    let mut s: [i64; 5] = [-1, -1, -1, -1, -1];
    let mut count: usize = 1;
    let mut straight = true;
    let mut flush = true;
    let s0: u8 = suit(cs[0]);
    let mut r0: u8 = rank(cs[0]);
    s[0] = (r0 as i64) << 16;
    for i in 1..5 {
        s[0] |= (rank(cs[i]) as i64) << ((4 - i) * 4);
        if straight
            && rank(cs[i - 1]) - rank(cs[i]) != 1
            && !(i == 1 && r0 == 12 && rank(cs[1]) == 3)
        {
            straight = false;
        }
        if flush && suit(cs[i]) != s0 {
            flush = false;
        }
        if rank(cs[i]) == rank(cs[i - 1]) {
            count += 1;
        }
        if i == 4 || rank(cs[i]) != rank(cs[i - 1]) {
            if count == 2 && s[2] > -1 {
                s[1] = rank(cs[i - 1]) as i64;
            } else if count == 2 {
                s[2] = rank(cs[i - 1]) as i64;
            } else if count > 2 {
                s[count] = rank(cs[i - 1]) as i64;
            }
            count = 1;
        }
    }
    // straight flush then straight
    if straight {
        if r0 == 12 && rank(cs[1]) == 3 {
            r0 = 3;
        }
        if flush {
            return (r0 as i64) << SFLUSH_SHIFT;
        }
        return (r0 as i64) << STRAIGHT_SHIFT;
    }
    // flush
    if flush {
        return (1 << FLUSH_SHIFT) | (s[0] << HC_SHIFT);
    }
    // builds bitmask for n-of-a-kind and fullhouse
    if s[4] > -1 {
        return ((s[4] + 1) << FOAK_SHIFT) | (s[0] << HC_SHIFT);
    }
    if s[3] > -1 {
        // fullhouse
        if s[2] > -1 {
            return ((s[3] + 1) << TOAK_SHIFT) | ((s[2] + 1) << PAIR2_SHIFT) | (1 << FULL_SHIFT);
        }
        // three-of-a-kind
        return ((s[3] + 1) << TOAK_SHIFT) | (s[0] << HC_SHIFT);
    }
    if s[2] > -1 {
        // two pairs
        if s[1] > -1 {
            return ((s[2] + 1) << PAIR2_SHIFT) | ((s[1] + 1) << PAIR1_SHIFT) | (s[0] << HC_SHIFT);
        }
        // two-of-a-kind
        return ((s[2] + 1) << PAIR1_SHIFT) | (s[0] << HC_SHIFT);
    }
    s[0] << HC_SHIFT
}

pub fn eval(cards: &mut [Card; 7]) -> Option<i64> {
    cards.sort_by_key(|a| u8::MAX - rank(a));
    cards.iter().combinations(5).map(eval5).max()
}

pub fn comp(cards: &mut [Card; 7], score: i64) -> u8 {
    cards.sort_by_key(|a| u8::MAX - rank(a));
    let mut result: u8 = WIN;
    let mut v: i64;
    for comb5 in cards.iter().combinations(5) {
        v = eval5(comb5);
        if v > score {
            return LOSS;
        }
        if v == score {
            result = DRAW;
        }
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use tests::Bencher;

    #[test]
    fn card_works() {
        let card = Card{rank: Rank::Seven, suit: Suit::Diamonds};
        assert_eq!(card.rank as u8, 5);
        assert_eq!(card.suit as u8, 1)
    }

    #[test]
    fn card_from_str_works() {
        let res = Card::from_str("AS");
        assert_eq!(res, Ok(Card{rank: Rank::Ace, suit: Suit::Spades}));

        let res = Card::from_str("JD");
        assert_eq!(res, Ok(Card{rank: Rank::Jack, suit: Suit::Diamonds}));

        let res = Card::from_str("4H");
        assert_eq!(res, Ok(Card{rank: Rank::Four, suit: Suit::Hearts}));

        let res = Card::from_str("TC");
        assert_eq!(res, Ok(Card{rank: Rank::Ten, suit: Suit::Clubs}));

        let res = Card::from_str("2s");
        assert_eq!(res, Err(()));

        let res = Card::from_str("1D");
        assert_eq!(res, Err(()));

        let res = Card::from_str("9y");
        assert_eq!(res, Err(()))
    }

    #[test]
    fn eval5_straight_flush() {
        let mut cards: [Card; 5] = [
            Card::from_str("2H").unwrap(),
            Card::from_str("3H").unwrap(),
            Card::from_str("4H").unwrap(),
            Card::from_str("5H").unwrap(),
            Card::from_str("6H").unwrap(),
        ];
        cards.sort_by_key(|a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(17592186044416)
        )
    }

    #[test]
    fn eval5_straight() {
        let mut cards: [Card; 5] = [
            Card::from_str("2D").unwrap(),
            Card::from_str("3D").unwrap(),
            Card::from_str("4D").unwrap(),
            Card::from_str("5H").unwrap(),
            Card::from_str("6H").unwrap(),
        ];
        cards.sort_by_key(|a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(17179869184)
        )
    }

    #[test]
    fn eval5_flush() {
        let mut cards: [Card; 5] = [
            Card::from_str("2H").unwrap(),
            Card::from_str("4H").unwrap(),
            Card::from_str("6H").unwrap(),
            Card::from_str("8H").unwrap(),
            Card::from_str("TH").unwrap(),
        ];
        cards.sort_by_key(|a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(68720026656)
        )
    }

    #[test]
    fn eval_works() {
        let mut cards: [Card; 7] = [
            Card::from_str("2D").unwrap(),
            Card::from_str("8D").unwrap(),
            Card::from_str("TD").unwrap(),
            Card::from_str("5S").unwrap(),
            Card::from_str("QS").unwrap(),
            Card::from_str("6C").unwrap(),
            Card::from_str("3C").unwrap(),
        ];
        assert_eq!(eval(&mut cards), Some(689731))
    }
}
