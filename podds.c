/**************************************************************************

  podds - Texas Hold 'Em Poker odds calculator
  Copyright (C) 2011  Lorenzo Stella
  
  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************

  *** USAGE
  parameters: <number of players> <card 1> <card 2> [<card 3> ... <card 7>]
  where the first two cards are the player's hand, while the others (optional)
  are those on the table.
  cards are natural numbers in the range [0, 51], grouped by suit. since in texas
  hold 'em poker suits are no different from each other, one thus has:
    suit 1: 0-12
    suit 2: 13-25
    suit 3: 26-38
    suit 4: 29-51
  bigger numbers corresponds to higher cards, so for instance cards 0, 13, 26, 29
  are the "twos", while 12, 25, 38, 51 are the "aces".
  the rank of a card k is obviously given by k%13;
  its suit is obviously given by k/13 (euclidean division).
  the probabilities of winning and drawing are printed (three significant digits) to standard output
  with a newline character '\n' inbetween.
  
  the lack of usability of this tool and its nice performances makes it particularly
  useful for embedding it in much more usable programs.
    
  *** MAIN PROGRAM
  the main program allocates a number of threads equal to the number of cores. each one of them
  starts by removing the known cards from the deck; afterwards it plays a certain amount of random
  games during which:
  - cards for every other player are drawn;
  - remaining cards on the table are drawn;
  - checks whether or not the player has won the game.
  in order to accomplish this very last phase the cards that were drawn are
  converted to a format which is more suitable for the computation: see BITCARD section.
  the amount of won games over the total number of games played is an approximation
  of the probability of winning given the known cards.
  
  *** EVAL7 & EVAL5
  in order to determine which player wins the game a score is assigned to the
  seven cards (hand+table) of each player. this score is the maximum score of
  the twenty-one combinations of five cards obtainable from seven. the eval5 function
  calculates the score for each of these twenty-one combinations. it should give a higher
  integer score for a stronger combination, so higher scores are matched first: there's no
  need to look for three-of-a-kind when we've already found a full-house or a straight or
  whatever.
  
  *** BITCARD
  [i'm working on it]

*************************************************************************/

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

/* total number of games for the simulation */
#define MAXGAMES      1000000

/* bitmasks and shifts to manipulate cards properly */
#define SUITSHIFT     18
#define SUITMASK      0x003C0000 // 3932160
#define BITRANKMASK   0x0003FFF0 // 262128
#define RANKMASK      0x0000000F // 15
#define STRAIGHTMASK  0x000001F0 // 496

/* some constants */
#define LOSS          0
#define DRAW          1
#define WIN           2

typedef struct {
  int c[52];
  int n;
} deck;

/* the 21 combinations C(7,5) */
const int combs[][5] =
  {{0, 1, 2, 3, 4},
   {0, 1, 2, 3, 5},
   {0, 1, 2, 3, 6},
   {0, 1, 2, 4, 5},
   {0, 1, 2, 4, 6},
   {0, 1, 2, 5, 6},
   {0, 1, 3, 4, 5},
   {0, 1, 3, 4, 6},
   {0, 1, 3, 5, 6},
   {0, 1, 4, 5, 6},
   {0, 2, 3, 4, 5},
   {0, 2, 3, 4, 6},
   {0, 2, 3, 5, 6},
   {0, 2, 4, 5, 6},
   {0, 3, 4, 5, 6},
   {1, 2, 3, 4, 5},
   {1, 2, 3, 4, 6},
   {1, 2, 3, 5, 6},
   {1, 2, 4, 5, 6},
   {1, 3, 4, 5, 6},
   {2, 3, 4, 5, 6}};

/* utility function that gives a random integer in the range [0, hi-1] */
int randint(int hi) {
  return (int)((float)(rand())/RAND_MAX*hi);
}

int suit(int i) {
  return i/13;
}

int rank(int i) {
  return i%13;
}

/* convert a card from integer-form to bitmap-form */
int bitcard(int i) {
  int r = rank(i);
  return (1<<(SUITSHIFT+suit(i))) | (1<<(r+5)) | (r == 12 ? 1<<4 : 0) | r;
}

/* allocates a new deck */
deck * newdeck() {
  int i;
  deck * d = (deck *)malloc(sizeof(deck));
  d->n = 52;
  for (i=0; i<52; i++) d->c[i] = i;
  return d;
}

/* (re)initialize a given deck to a given number n of available cards
    keeping the first 52-n drawn cards unavailable */
void initdeck(deck * d, int n) {
  d->n = n;
}

/* return a random card from deck d and make it unavailable for the future */
int draw(deck * d) {
  if (d->n > 0) {
    int j = randint(d->n), k;
    d->n--;
    k = d->c[j];
    d->c[j] = d->c[d->n];
    d->c[d->n] = k;
    return k;
  }
  return -1;
}

/* return a CHOSEN card from deck d and make it unavailable for the future */
void pick(deck * d, int c) {
  int i, m;
  for (i=0; i<d->n; i++) {
    if (d->c[i] == c) {
      d->c[i] = d->c[d->n - 1];
      d->c[d->n - 1] = c;
      d->n--;
      break;
    }
  }
}

/* check for a flush (cs[] must have length >= 5) */
int flush(int cs[]) {
  int i;
  for (i=1; i<5; i++) if ((cs[i-1] & SUITMASK) != (cs[i] & SUITMASK)) return 0;
  return 1;
}

/* check for a straight (cs[] must have length >= 5) */
int straight(int cs[]) {
  int i, disj = cs[0];
  for (i=1; i<5; i++) disj |= cs[i];
  for (i=0; i<10; i++) if ((disj & (STRAIGHTMASK<<i))>>i == STRAIGHTMASK) break;
  if (i > 9) return 0;
  else return i+1;
}

int eval5(int cs[]) {
  int i, j, rv[] = {-1,-1,-1,-1,-1}, cv[] = {0,0,0,0,0};
  int m, s = straight(cs), f = flush(cs), v = 0;
  if (s) {
    if (f) return (8<<24) | (s<<20);
    else return (4<<24) | (s<<20);
  } else if (f) v = 5<<24;
  for (i=1; i<5; i++) {
    for (j=0; j<5-i; j++) {
      if ((cs[j] & RANKMASK) < (cs[j+1] & RANKMASK)) {
        m = cs[j];
        cs[j] = cs[j+1];
        cs[j+1] = m;
      }
    }
  }
  for (i=0; i<5; i++) {
    for (j=0; j<5; j++) {
      if (rv[j] == (cs[i] & RANKMASK)) {
        cv[j]++;
        break;
      }
      if (rv[j] == -1) {
        rv[j] = cs[i] & RANKMASK;
        cv[j]++;
        break;
      }
    }
  }
  for (i=1; i<5; i++) {
    for (j=0; j<5-i; j++) {
      if (cv[j] < cv[j+1]) {
        m = cv[j];
        cv[j] = cv[j+1];
        cv[j+1] = m;
        m = rv[j];
        rv[j] = rv[j+1];
        rv[j+1] = m;
      }
    }
  }
  if (cv[0] == 2 && cv[1] == 1) v = 1<<24;
  else if (cv[0] == 2 && cv[1] == 2) v = 2<<24;
  else if (cv[0] == 3 && cv[1] == 1) v = 3<<24;
  else if (cv[0] == 3 && cv[1] == 2) v = 6<<24;
  else if (cv[0] == 4) v = 7<<24;
  for (i=0; i<5; i++) {
    if (cv[i] == 0) break;
    v |= rv[i] << 4*(4-i);
  }
  return v;
}

/* find the most valuable five-cards combination out of the seven given cards and return its score */
int eval7(int cs[]) {
  int i, ds[5], max = -1, v;
  for (i = 0; i<21; i++) {
    ds[0] = cs[combs[i][0]]; //
    ds[1] = cs[combs[i][1]]; //
    ds[2] = cs[combs[i][2]]; // TODO: optimize
    ds[3] = cs[combs[i][3]]; //
    ds[4] = cs[combs[i][4]]; //
    v = eval5(ds);
    if (v > max) max = v;
  }
  return max;
}

/* check if there's a five-cards combination among cards cs[] with score higher than s */
int comp7(int cs[], int s) {
  int i, ds[5], v, result = WIN;
  for (i = 0; i<21; i++) {
    ds[0] = cs[combs[i][0]]; //
    ds[1] = cs[combs[i][1]]; //
    ds[2] = cs[combs[i][2]]; // TODO: optimize
    ds[3] = cs[combs[i][3]]; //
    ds[4] = cs[combs[i][4]]; //
    v = eval5(ds);
    if (v > s) return LOSS;
    if (v == s) result = DRAW;
  }
  return result;
}

/*~~ Global (shared) data ~~~~~~~~~~~~~*/
int wins = 0, draws = 0;
int np, kc, as[7];

/*~~ Threading ~~~~~~~~~~~~~~~~~~~~*/
#include <pthread.h>
int NUMTHREADS, NUMGAMES, GAMESPERTHREAD;
pthread_t * tpool;
pthread_mutex_t tlock;

void * simulator(void * v) {
  int * ohs = (int *)malloc(2*(np-1)*sizeof(int));
  int cs[7], cs0, cs1, result, result1, i, j;
  int mywins = 0, mydraws = 0;
  deck * d = newdeck();
  for (i=0; i<kc; i++) pick(d, as[i]);
  for (i=2; i<kc; i++) cs[i] = bitcard(as[i]);
  cs0 = bitcard(as[0]);
  cs1 = bitcard(as[1]);
  for (i=0; i<GAMESPERTHREAD; i++) {
    int score;
    initdeck(d, 52-kc);
    cs[0] = cs0;
    cs[1] = cs1;
    for (j=0; j<2*(np-1); j++) ohs[j] = bitcard(draw(d));
    for (j=kc; j<7; j++) cs[j] = bitcard(draw(d));
    score = eval7(cs);
    result = WIN;
    for (j=0; j<np-1; j++) {
      cs[0] = ohs[2*j];
      cs[1] = ohs[2*j+1];
      result1 = comp7(cs, score);
      if (result1 < result) result = result1;
      if (result == LOSS) break;
    }
    if (result == WIN) mywins++;
    else if (result == DRAW) mydraws++;
  }
  pthread_mutex_lock(&tlock);
  wins += mywins;
  draws += mydraws;
  pthread_mutex_unlock(&tlock);
  free(ohs);
  free(d);
  return NULL;
}

/*~~ Main program ~~~~~~~~~~~~~~~~~~~~~*/
int main(int argc, char ** argv) {
  int i, cs0, cs1;
  if (argc < 4) {
  	fprintf(stderr, "incorrect number of arguments\n");
  	fprintf(stderr, "required: <#players> <card1> <card2>\n");
  	return 1;
  }
  NUMTHREADS = sysconf(_SC_NPROCESSORS_ONLN);
  GAMESPERTHREAD = MAXGAMES/NUMTHREADS;
  NUMGAMES = GAMESPERTHREAD*NUMTHREADS;
  printf("cores:%d\n", NUMTHREADS);
  printf("games:%d\n", NUMGAMES);
  // read the arguments and create the known cards
  np = atoi(argv[1]);
  kc = argc-2;
  for (i=0; i<kc; i++) as[i] = atoi(argv[i+2]);
  // initialize the rng seed and the mutex
  srand(time(NULL));
  pthread_mutex_init(&tlock, NULL);
  pthread_mutex_unlock(&tlock);
  // run the simulation threads
  tpool = (pthread_t *)malloc(NUMTHREADS*sizeof(pthread_t));
  for (i=0; i<NUMTHREADS; i++) {
    pthread_create(&tpool[i], NULL, simulator, NULL);
  }
  // wait for the threads to finish
  for (i=0; i<NUMTHREADS; i++) {
    pthread_join(tpool[i], NULL);
  }
  // show the results
  printf("wins:%.3f\n", (float)(wins)/NUMGAMES);
  printf("draws:%.3f\n", (float)(draws)/NUMGAMES);
  // clear all
  pthread_mutex_destroy(&tlock);
  return 0;
}

/*~~ Games test ~~~~~~~~~~~~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int i, j, c[9], h1[7], h2[7], s1, s2;
  while (scanf("%d %d %d %d %d %d %d %d %d ", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8]) == 9) {
    h1[0] = bitcard(c[0]);
    h1[1] = bitcard(c[1]);
    h2[0] = bitcard(c[2]);
    h2[1] = bitcard(c[3]);
    for (i=2; i<9; i++) {
      h1[i] = h2[i] = bitcard(c[i]);
    }
    s1 = eval7(h1);
    s2 = eval7(h2);
    if (s1 > s2) printf("1\n");
    else if (s1 < s2) printf("2\n");
    else printf("0\n");
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
