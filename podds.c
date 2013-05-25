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
  the twenty-one combinations of five cards drawn from seven. the eval5 function
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
#include <stdio.h>
#include <unistd.h>

#include "poker.h"

/* total number of games for the simulation */
#define MAXGAMES        200000

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
  int cs[7], myas[7], cs0, cs1, result, result1, i, j, k;
  int mywins = 0, mydraws = 0;
  deck * d = newdeck();
  for (i=0; i<kc; i++) {
    pick(d, as[i]);
    myas[i] = as[i];
  }
  for (i=0; i<GAMESPERTHREAD; i++) {
    long long score;
    initdeck(d, 52-kc);
    for (j=0; j<2*(np-1); j++) ohs[j] = draw(d);
    for (j=kc; j<7; j++) myas[j] = draw(d);
    for (j=0; j<7; j++) cs[j] = myas[j];
    sort(cs);
    score = eval7(cs);
    result = WIN;
    for (j=0; j<np-1; j++) {
      cs[0] = ohs[2*j];
      cs[1] = ohs[2*j+1];
      for (k=2; k<7; k++) cs[k] = myas[k];
      sort(cs);
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
//#ifdef MAIN
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
  printf("wins:%.3f\n", ((float)wins)/NUMGAMES);
  printf("draws:%.3f\n", ((float)draws)/NUMGAMES);
  // clear all
  pthread_mutex_destroy(&tlock);
  return 0;
}
//#endif

/*~~ Hand recognition test ~~~~~~~~~~~~*//*
int main(int argc, char ** argv) {
  int cs1[] = {14, 2, 21, 7, 35, 18, 19};
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
#ifdef GENERATOR
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
#endif

/*~~ Games test ~~~~~~~~~~~~~~~~~~~~~~~*//*
#ifdef TEST
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
#endif

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
