/*
This file is part of podds.

podds is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

podds is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with podds.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <math.h>

#include "poker.h"

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
  // set flags for straight, flush, n-of-a-kind
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
  // straight flush then straight
  if (straight) {
    if (r0 == 12 && rank(cs[1]) == 3) r0 = 3;
    if (flush) return (long long)r0 << SFLUSH_SHIFT;
    return (long long)r0 << STRAIGHT_SHIFT;
  }
  // flush
  if (flush) {
    return  (long long)1 << FLUSH_SHIFT |
            (long long)s[0] << HC_SHIFT;
  }
  // builds bitmask for n-of-a-kind and fullhouse
  if (s[4] > -1) {
    return ((long long)s[4]+1) << FOAK_SHIFT | s[0] << HC_SHIFT;
  }
  if (s[3] > -1) {
    // fullhouse
    if (s[2] > -1)
      return (((long long)s[3]+1) << TOAK_SHIFT) | (((long long)s[2]+1) << PAIR2_SHIFT) | (long long)1 << FULL_SHIFT;
    // three-of-a-kind
    return ((long long)s[3]+1) << TOAK_SHIFT | s[0] << HC_SHIFT;
  }
  if (s[2] > -1) {
    // two pairs
    if (s[1] > -1)
      return (((long long)s[2]+1) << PAIR2_SHIFT) | (((long long)s[1]+1) << PAIR1_SHIFT) | s[0] << HC_SHIFT;
    // two-of-a-kind
    return ((long long)s[2]+1) << PAIR1_SHIFT | s[0] << HC_SHIFT;
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

