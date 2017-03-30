/*
podds - Texas Hold 'Em Poker odds calculator
Copyright (C) 2011-2013  Lorenzo Stella

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
*/

#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <unistd.h>

#include "poker.h"

/* total number of games for the simulation */
#define MAXGAMES        200000

/*~~ Argument parsing ~~~~~~~~~~~~~~~~~*/

#define SYMBOL_TEN    84 // 'T'
#define SYMBOL_JACK   74 // 'J'
#define SYMBOL_QUEEN  81 // 'Q'
#define SYMBOL_KING   75 // 'K'
#define SYMBOL_ACE    65 // 'A'

#define SYMBOL_HEARTS     104 // 'h'
#define SYMBOL_DIAMONDS   100 // 'd'
#define SYMBOL_CLUBS      99  // 'c'
#define SYMBOL_SPADES     115 // 's'

int char2rank(char c) {
  // 50 = '2', 57 = '9'
  if (c >= 50 && c <= 57) return c - 50;
  else if (c == SYMBOL_TEN) return 8;
  else if (c == SYMBOL_JACK) return 9;
  else if (c == SYMBOL_QUEEN) return 10;
  else if (c == SYMBOL_KING) return 11;
  else if (c == SYMBOL_ACE) return 12;
  return -1;
}

int char2suit(char c) {
  if (c == SYMBOL_HEARTS) return 0;
  else if (c == SYMBOL_DIAMONDS) return 1;
  else if (c == SYMBOL_CLUBS) return 2;
  else if (c == SYMBOL_SPADES) return 3;
  return -1;
}

int string2index(char * str) {
  int r, s;
  r = char2rank(str[0]);
  s = char2suit(str[1]);
  if (r < 0 || s < 0) return -1;
  return s*13 + r;
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
  for (i=0; i<kc; i++) {
    as[i] = string2index(argv[i+2]);
    if (as[i] < 0) {
      fprintf(stderr, "wrong card identifier: %s\n", argv[i+2]);
      return 1;
    }
  }
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
