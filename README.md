# Podds

`podds` is a command line tool to compute the probability of winning a poker (texas hold-em), given the number of players and your available cards.

## Usage

To compile the program simply navigate to its directory and type `make`.

All cards are indicated with integer numbers ranging from 0 to 51 included. Numbers 0-12 correspond to 2, 3, ..., Queen, King, Ace of the first seed, numbers 13-25 correspond to 2, 3, ..., Queen, King, Ace of the second seed, and so on.

* `./podds <n> <h1> <h2>` where `<n>` is the number of players in the game, and `<h1> <h2>` is the player's hand.

* `./podds <n> <h1> <h2> <t1> ... <t5>` where `<t1> ... <t5>`, not necessarily all provided, are the cards revealed face up on the table (flop, turn, river).
