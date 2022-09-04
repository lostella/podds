use rand::Rng;
use itertools::Itertools;

fn rank(card: u8) -> u8 {
    card % 13
}

fn suit(card: u8) -> u8 {
    card / 13
}

struct Deck {
    cards: Vec<u8>
}

impl Deck {
    fn new() -> Self {
        Self {
            cards: (0..52).collect::<Vec<u8>>()
        }
    }

    fn pick_random(&mut self) -> Result<u8, ()> {
        if self.cards.len() == 0 {
            return Err(())
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
            None => Err(card)
        }
    }
}

struct Player {
    hand: Vec<u8>
}

impl Player {
    fn new() -> Self {
        Player {
            hand: vec![]
        }
    }

    fn give(&mut self, card: u8) -> Result<u8, u8> {
        if self.hand.len() < 2 {
            self.hand.push(card);
            return Ok(card)
        }
        return Err(card)
    }
}

pub struct Game {
    player: Player,
    opponents: Vec<Player>,
    public: Vec<u8>,
    deck: Deck
}

impl Game {
    pub fn new(num_players: u8) -> Self {
        let mut opponents = vec![];
        for _ in 0..num_players-1 {
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
                    return Err(card)
                }
                self.public.push(card);
                return Ok(card)
            }
        }
    }
}

fn eval5(cards: Vec<&u8>) -> u32 {
    assert!(cards.len() == 5);
    0
}

pub fn eval(cards: &mut [u8; 7]) -> Option<u32> {
    cards.sort_by_key(|&a| u8::MAX - rank(a));
    cards.iter().combinations(5).map(eval5).max()
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
    fn eval_works() {
        let mut cards: [u8; 7] = [13, 19, 21, 42, 49, 30, 27];
        assert_eq!(eval(&mut cards), Some(0))
    }    
}
