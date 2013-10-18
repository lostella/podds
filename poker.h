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

int randint(int hi);
int suit(int i);
int rank(int i);
deck * newdeck();
void initdeck(deck * d, int n);
int draw(deck * d);
void pick(deck * d, int c);
void sort(int cs[]);
int hand(long long s);
long long eval5(int cs[]);
long long eval7(int cs[]);
int comp7(int cs[], long long s);

