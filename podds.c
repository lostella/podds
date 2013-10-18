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
