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
  its suit is obviously given by k/13 (euclidean division, with remainder).
  the probabilities of winning and drawing are printed (three significant digits) to standard output,
  along with some other infos, all separated by a newline character '\n'.
  
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
  
  *** COMP7
  once the first player's hand has been evaluated, there's no need to *exactly* calculate
  the score for every other player. instead we just need to compare every combination of
  5 cards taken from 7 with the score of the first player, until we find one that beats it,
  ignoring any other combination (or even other players).

*************************************************************************/

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <unistd.h>

/* total number of games for the simulation */
#define MAXGAMES        1000000

/* shifts to build up scores properly */
#define SFLUSH_SHIFT    42
#define FOAK_SHIFT      38
#define FULL_SHIFT      37
#define FLUSH_SHIFT     36
#define STRAIGHT_SHIFT  32
#define TOAK_SHIFT      28
#define PAIR2_SHIFT     24
#define PAIR1_SHIFT     20
#define HC_SHIFT        0

/* some constants */
#define LOSS            0
#define DRAW            1
#define WIN             2
#define HC              3
#define PAIR            4
#define TWOPAIRS        5
#define TOAK            6
#define STRAIGHT        7
#define FLUSH           8
#define FULLHOUSE       9
#define FOAK            10
#define STRFLUSH        11

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

/* sort the given 7 cards by rank (decreasing order) */
void sort(int cs[]) {
  int i, j, imax, rmax, temp;
  for (i=0; i<6; i++) {
    rmax = rank(cs[i]); imax = i;
    for (j=i+1; j<7; j++) {
      if (rank(cs[j]) > rmax) {
        rmax = rank(cs[j]);
        imax = j;
      }
    }
    if (imax > i) {
      temp = cs[imax];
      cs[imax] = cs[i];
      cs[i] = temp;
    }
  }
}

/* returns the type of hand given a score (may be useful for debugging purposes) */
int hand(long long s) {
  if (s >= ((long long)1)<<SFLUSH_SHIFT) return STRFLUSH;
  if (s >= ((long long)1)<<FOAK_SHIFT) return FOAK;
  if (s >= ((long long)1)<<FULL_SHIFT) return FULLHOUSE;
  if (s >= ((long long)1)<<FLUSH_SHIFT) return FLUSH;
  if (s >= ((long long)1)<<STRAIGHT_SHIFT) return STRAIGHT;
  if (s >= ((long long)1)<<TOAK_SHIFT) return TOAK;
  if (s >= ((long long)1)<<PAIR2_SHIFT) return TWOPAIRS;
  if (s >= ((long long)1)<<PAIR1_SHIFT) return PAIR;
  return HC;
}

/* calculates a score for the 5-card combo */
/* TODO: PROVE CORRECT */
/* TODO: OPTIMIZE */
long long eval5(int cs[]) {
  int s[] = {-1,-1,-1,-1,-1}, i;
  int count = 1, straight = 1, flush = 1;
  int s0 = suit(cs[0]), r0 = rank(cs[0]);
  s[0] = r0<<16;
  for (i=1; i<5; i++) {
    s[0] |= rank(cs[i])<<(4-i)*4;
  	if (straight && rank(cs[i-1])-rank(cs[i]) != 1 && !(i == 1 && r0 == 12 && rank(cs[1]) == 3)) {
      straight = 0;
    }
  	if (flush == 1 && suit(cs[i]) != s0)
      flush = 0;
    if (rank(cs[i]) == rank(cs[i-1]))
      count++;
    if (i == 4 || rank(cs[i]) != rank(cs[i-1])) {
      if (count == 2 && s[2] > -1) s[1] = rank(cs[i-1]);
      else if (count == 2) s[2] = rank(cs[i-1]);
      else if (count > 2) s[count] = rank(cs[i-1]);
      count = 1;
    }
  }
  if (straight) {
    if (r0 == 12 && rank(cs[1]) == 3) r0 = 3;
    if (flush) return (long long)r0 << SFLUSH_SHIFT;
    return (long long)r0 << STRAIGHT_SHIFT;
  }
  if (flush) {
    return  (long long)1 << FLUSH_SHIFT |
            (long long)s[0] << HC_SHIFT;
  }
  if (s[4] > -1) {
  	return ((long long)s[4]+1) << FOAK_SHIFT | s[0];
  }
  if (s[3] > -1) {
  	if (s[2] > -1)
  	  return (((long long)s[3]+1) << TOAK_SHIFT) | (((long long)s[2]+1) << PAIR2_SHIFT) | (long long)1 << FULL_SHIFT;
  	return ((long long)s[3]+1) << TOAK_SHIFT | s[0];
  }
  if (s[2] > -1) {
    if (s[1] > -1)
      return (((long long)s[2]+1) << PAIR2_SHIFT) | (((long long)s[1]+1) << PAIR1_SHIFT) | s[0];
    return ((long long)s[2]+1) << PAIR1_SHIFT | s[0];
  }
  return (long long)s[0] << HC_SHIFT;
}

/* find the most valuable five-cards combination out of the seven given cards and return its score */
long long eval7(int cs[]) {
  int i, ds[5];
  long long max = -1, v;
  for (i = 0; i<21; i++) {
    ds[0] = cs[combs[i][0]];
    ds[1] = cs[combs[i][1]];
    ds[2] = cs[combs[i][2]];
    ds[3] = cs[combs[i][3]];
    ds[4] = cs[combs[i][4]];
    v = eval5(ds);
    if (v > max) max = v;
    //printf(" ---> %Ld\n", v);
  }
  return max;
}

/* check if there's a five-cards combination among cards cs[] with score higher than s */
int comp7(int cs[], long long s) {
  int i, ds[5], result = WIN;
  long long v;
  for (i = 0; i<21; i++) {
    ds[0] = cs[combs[i][0]];
    ds[1] = cs[combs[i][1]];
    ds[2] = cs[combs[i][2]];
    ds[3] = cs[combs[i][3]];
    ds[4] = cs[combs[i][4]];
    v = eval5(ds);
    if (v > s) return LOSS;
    if (v == s) result = DRAW;
  }
  return result;
}

/*~~ Global (shared) data ~~~~~~~~~~~~~*/
int wins = 0, draws = 0;
int np, kc, as[7];

/*~~ Threading ~~~~~~~~~~~~~~~~~~~~~~~~*/
#include <pthread.h>
int NUMTHREADS, NUMGAMES, GAMESPERTHREAD;
pthread_t * tpool;
pthread_mutex_t tlock;

void * simulator(void * v) {
  int * ohs = (int *)malloc(2*(np-1)*sizeof(int));
  int cs[7], cs0, cs1, result, result1, i, j, k;
  int mywins = 0, mydraws = 0;
  deck * d = newdeck();
  for (i=0; i<kc; i++) pick(d, as[i]);
  for (i=0; i<GAMESPERTHREAD; i++) {
    long long score;
    initdeck(d, 52-kc);
    for (j=0; j<2*(np-1); j++) ohs[j] = draw(d);
    for (j=kc; j<7; j++) as[j] = draw(d);
    for (j=0; j<7; j++) cs[j] = as[j];
    sort(cs);
    score = eval7(cs);
    result = WIN;
    for (j=0; j<np-1; j++) {
      cs[0] = ohs[2*j];
      cs[1] = ohs[2*j+1];
      for (k=2; k<7; k++) cs[k] = as[k];
      sort(cs);
      result1 = comp7(cs, score);
      if (result1 < result) result = result1;
      if (result == LOSS) break;
    }
    if (result == WIN) mywins++;
    else if (result == DRAW) {/*
      printf("%d(%d) ", rank(as[0]), suit(as[0]));
      printf("%d(%d) ", rank(as[1]), suit(as[1]));
      printf("%d(%d) ", rank(as[2]), suit(as[2]));
      printf("%d(%d) ", rank(as[3]), suit(as[3]));
      printf("%d(%d) ", rank(as[4]), suit(as[4]));
      printf("%d(%d) ", rank(as[5]), suit(as[5]));
      printf("%d(%d)\n", rank(as[6]), suit(as[6]));*/
      mydraws++;
    }
  }
  pthread_mutex_lock(&tlock);
  wins += mywins;
  draws += mydraws;
  pthread_mutex_unlock(&tlock);
  free(ohs);
  free(d);
  return NULL;
}

/*~~ Main program ~~~~~~~~~~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int i, cs0, cs1;
  if (argc < 4) {
  	fprintf(stderr, "incorrect number of arguments\n");
  	fprintf(stderr, "required: <#players> <card1> <card2>\n");
  	return 1;
  }
  NUMTHREADS = 1;//sysconf(_SC_NPROCESSORS_ONLN);
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
  printf("wins:%.3f\n", ((float)wins)/NUMGAMES);
  printf("draws:%.3f\n", ((float)draws)/NUMGAMES);
  // clear all
  pthread_mutex_destroy(&tlock);
  return 0;
}

/*~~ Hand recognition test ~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int cs1[] = {14, 2, 21, 20, 35, 18, 19};
  long long s1;
  sort(cs1);
  printf("player 1: %d %d %d %d %d %d %d\n", rank(cs1[0]), rank(cs1[1]), rank(cs1[2]), rank(cs1[3]), rank(cs1[4]), rank(cs1[5]), rank(cs1[6]));
  s1 = eval7(cs1);
  printf("%d\n", hand(s1));
  return 0;
}

/*~~ Eval test ~~~~~~~~~~~~~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int cs1[] = {14, 2, 21, 20, 35, 18, 19};
  int cs2[] = {44, 1, 21, 20, 35, 18, 19};
  long long s1, s2;
  int res;
  sort(cs1);
  sort(cs2);
  printf("player 1: %d %d %d %d %d %d %d\n", rank(cs1[0]), rank(cs1[1]), rank(cs1[2]), rank(cs1[3]), rank(cs1[4]), rank(cs1[5]), rank(cs1[6]));
  s1 = eval7(cs1);
  printf("score=%Ld\n",s1);
  printf("player 2: %d %d %d %d %d %d %d\n", rank(cs2[0]), rank(cs2[1]), rank(cs2[2]), rank(cs2[3]), rank(cs2[4]), rank(cs2[5]), rank(cs2[6]));
  s2 = eval7(cs2);
  printf("score=%Ld\n",s2);
  if (s1 < s2)
    printf("player 2 wins\n");
  if (s1 == s2)
    printf("draw\n");
  if (s1 > s2)
    printf("player 1 wins\n");
  //printf("score = %Ld\n", s);
}

/*~~ Games generator ~~~~~~~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int ng = atoi(argv[1]); // number of pairs
  int i, j;
  deck * d = newdeck();
  for (i=0; i<ng; i++) {
    initdeck(d, 52);
    for (j=0; j<9; j++)
      printf("%d ", draw(d));
    printf("\n");
  }
  return 0;
}

/*~~ Games test ~~~~~~~~~~~~~~~~~~~~~~~*/
int main(int argc, char ** argv) {
  int i, j, c[9], h1[7], h2[7];
  long long s1, s2;
  while (scanf("%d %d %d %d %d %d %d %d %d ", &c[0], &c[1], &c[2], &c[3], &c[4], &c[5], &c[6], &c[7], &c[8]) == 9) {
    h1[0] = c[0];
    h1[1] = c[1];
    h2[0] = c[2];
    h2[1] = c[3];
    for (i=2; i<7; i++) {
      h1[i] = c[i+2];
      h2[i] = c[i+2];
    }
    sort(h1);
    sort(h2);
    s1 = eval7(h1);
    s2 = eval7(h2);
    if (s1 > s2) printf("1\n");
    else if (s1 < s2) printf("2\n");
    else printf("0\n");
  }
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
