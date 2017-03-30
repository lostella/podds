# Podds

`podds` is a command line tool to compute the probability of winning a poker (texas hold-em), given the number of players and your available cards.

## Usage

To compile the program simply navigate to its directory and type `make`.

To run the program invoke

```
  ./podds <n> <h1> <h2> [<t1> ... <t5>]
```

Here `<n>` is the **total** number of players, `<h1>` and `<h2>` are the hole cards (mandatory arguments), while `<t1>` to `<t5>` are the community cards (optional, you can specify 0 to 5 of them).

Each card is represented by a **case sensitive** 2-characters string, according to the following convention:

* `2` to `9` = cards from 2 to 9
* `T` = 10
* `J`, `Q`, `K`, `A` = jack, queen, king, ace
* `h`, `d`, `c`, `s` = hearts, diamonds, clubs, spades

Examples: the ace of spades is `As`, the 10 of diamonds is `Td` and the 4 of clubs is `4c`.
