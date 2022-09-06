use itertools::Itertools;
use rand::Rng;

fn rank(card: u8) -> u8 {
    card % 13
}

fn suit(card: u8) -> u8 {
    card / 13
}

struct Deck {
    cards: Vec<u8>,
}

impl Deck {
    fn new() -> Self {
        Self {
            cards: (0..52).collect::<Vec<u8>>(),
        }
    }

    fn pick_random(&mut self) -> Result<u8, ()> {
        if self.cards.len() == 0 {
            return Err(());
        }
        let index = rand::thread_rng().gen_range(0..self.cards.len());
        let card = self.cards[index];
        self.cards.remove(index);
        Ok(card)
    }

    fn pick(&mut self, card: u8) -> Result<u8, u8> {
        match self.cards.iter().position(|&c| c == card) {
            Some(pos) => {
                self.cards.remove(pos);
                Ok(card)
            }
            None => Err(card),
        }
    }
}

struct Player {
    hand: Vec<u8>,
}

impl Player {
    fn new() -> Self {
        Player { hand: vec![] }
    }

    fn give(&mut self, card: u8) -> Result<u8, u8> {
        if self.hand.len() < 2 {
            self.hand.push(card);
            return Ok(card);
        }
        return Err(card);
    }
}

pub struct Game {
    player: Player,
    opponents: Vec<Player>,
    public: Vec<u8>,
    deck: Deck,
}

impl Game {
    pub fn new(num_players: u8) -> Self {
        let mut opponents = vec![];
        for _ in 0..num_players - 1 {
            opponents.push(Player::new())
        }
        Self {
            player: Player::new(),
            opponents,
            public: vec![],
            deck: Deck::new(),
        }
    }

    pub fn deal(&mut self, card: u8) -> Result<u8, u8> {
        self.deck.pick(card)?;
        match self.player.give(card) {
            Ok(c) => Ok(c),
            Err(c) => {
                if self.public.len() == 5 {
                    return Err(card);
                }
                self.public.push(card);
                return Ok(card);
            }
        }
    }
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
const HC: u8 = 3;
const PAIR: u8 = 4;
const TWOPAIRS: u8 = 5;
const TOAK: u8 = 6;
const STRAIGHT: u8 = 7;
const FLUSH: u8 = 8;
const FULLHOUSE: u8 = 9;
const FOAK: u8 = 10;
const STRFLUSH: u8 = 11;

fn eval5(cs: Vec<&u8>) -> i64 {
    assert!(cs.len() == 5);
    let mut s: [i64; 5] = [-1, -1, -1, -1, -1];
    let mut count: usize = 1;
    let mut straight: u32 = 1;
    let mut flush: u32 = 1;
    let mut s0: u8 = suit(*cs[0]);
    let mut r0: u8 = rank(*cs[0]);
    s[0] = (r0 as i64) << 16;
    for i in 1..5 {
        s[0] |= (rank(*cs[i]) as i64) << (4 - i) * 4;
        if straight > 0
            && rank(*cs[i - 1]) - rank(*cs[i]) != 1
            && !(i == 1 && r0 == 12 && rank(*cs[1]) == 3)
        {
            straight = 0;
        }
        if flush == 1 && suit(*cs[i]) != s0 {
            flush = 0;
        }
        if rank(*cs[i]) == rank(*cs[i - 1]) {
            count += 1;
        }
        if i == 4 || rank(*cs[i]) != rank(*cs[i - 1]) {
            if count == 2 && s[2] > -1 {
                s[1] = rank(*cs[i - 1]) as i64;
            } else if count == 2 {
                s[2] = rank(*cs[i - 1]) as i64;
            } else if count > 2 {
                s[count] = rank(*cs[i - 1]) as i64;
            }
            count = 1;
        }
    }
    // straight flush then straight
    if straight > 0 {
        if r0 == 12 && rank(*cs[1]) == 3 {
            r0 = 3;
        }
        if flush > 0 {
            return (r0 as i64) << SFLUSH_SHIFT;
        }
        return (r0 as i64) << STRAIGHT_SHIFT;
    }
    // flush
    if flush > 0 {
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

pub fn eval(cards: &mut [u8; 7]) -> Option<i64> {
    cards.sort_by_key(|&a| u8::MAX - rank(a));
    cards.iter().combinations(5).map(eval5).max()
}

pub fn comp(cards: &mut [u8; 7], score: i64) -> u8 {
    cards.sort_by_key(|&a| u8::MAX - rank(a));
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

    #[test]
    fn pick_random_works() {
        let mut deck = Deck::new();
        for _ in 0..52 {
            deck.pick_random();
        }
    }

    #[test]
    fn pick_works() {
        let mut deck = Deck::new();
        for card in [13, 42, 17, 31, 50] {
            assert_eq!(deck.pick(card), Ok(card))
        }
        for card in [13, 42, 17, 31, 50] {
            assert_eq!(deck.pick(card), Err(card))
        }
    }

    #[test]
    fn give_works() {
        let mut player = Player::new();
        for card in [13, 42] {
            assert_eq!(player.give(card), Ok(card))
        }
        assert_eq!(player.give(17), Err(17))
    }

    #[test]
    fn deal_works() {
        let mut game = Game::new(3);
        for card in [13, 42, 17, 31, 50] {
            assert_eq!(game.deal(card), Ok(card))
        }
        for card in [42, 31] {
            assert_eq!(game.deal(card), Err(card))
        }
        for card in [6, 9] {
            assert_eq!(game.deal(card), Ok(card))
        }
        for card in [19, 37] {
            assert_eq!(game.deal(card), Err(card))
        }
    }

    #[test]
    fn eval5_straight_flush() {
        let mut cards: [u8; 5] = [0, 1, 2, 3, 4];
        cards.sort_by_key(|&a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(17592186044416)
        )
    }

    #[test]
    fn eval5_straight() {
        let mut cards: [u8; 5] = [13, 14, 15, 3, 4];
        cards.sort_by_key(|&a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(17179869184)
        )
    }

    #[test]
    fn eval5_flush() {
        let mut cards: [u8; 5] = [0, 2, 4, 6, 8];
        cards.sort_by_key(|&a| u8::MAX - rank(a));
        assert_eq!(
            cards.iter().combinations(5).map(eval5).max(),
            Some(68720026656)
        )
    }

    #[test]
    fn eval_works() {
        let mut cards: [u8; 7] = [13, 19, 21, 42, 49, 30, 27];
        assert_eq!(eval(&mut cards), Some(689731))
    }
}
