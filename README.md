# Podds

`podds` is a command line tool to compute the probability of winning a (Texas hold 'em) poker round, given the number of players and the available cards.
Probabilities are estimated by sampling random games given the available information.

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

**Examples:** the ace of spades is `As`, the 10 of diamonds is `Td` and the 4 of clubs is `4c`.

The results are written onto `stdout`, and error messages (such as when arguments are not correctly formatted) are written to `stderr`. This way `podds` can be easily used from a higher-level interface. The output has the form: `<key>:<value>` and is supposed to be self-descriptive.Typical command-line interactions look like the following:

```
bash$ ./podds 5 Ts 9d Js 8h 3c
cores:4
games:200000
win:0.315
draw:0.031
pair:0.367
two-pairs:0.083
three-of-a-kind:0.014
straight:0.314
flush:0.000
full-house:0.000
four-of-a-kind:0.000
straight-flush:0.000
```

```
bash$ ./podds 4 Kd Jd Qs Tc Qd
cores:4
games:200000
win:0.406
draw:0.045
pair:0.291
two-pairs:0.290
three-of-a-kind:0.051
straight:0.301
flush:0.039
full-house:0.025
four-of-a-kind:0.001
straight-flush:0.002
```

## TODO

* Implement some options (e.g.: how many cores to use, how many games to simulate).
* Find a smart way to determine the default number of games to be simulated (e.g.: bounding the probability that results are inaccurate above some threshold).
* Implement interactive mode (where inputs are taken from `stdin`).
* Store results in a table (at least for short combos).
* Make available as a library (shared object).
