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
#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

#include "poker.h"

/*~~ Main program ~~~~~~~~~~~~~~~~~~~~~*/

#define MAXBATCHES 100000
#define HANDSPERBATCH 1000 // one-hundred million hands maximum
#define MAXTIME 5.0

int main(int argc, char ** argv) {
  uint32_t i, j, k, cs[7];
  int64_t score;
  double t_elapsed;
  clock_t ticks1, ticks2;
  ticks1 = clock();
  deck * d = newdeck();
  for (k=0; k<MAXBATCHES; k++) {
    for (i=0; i<HANDSPERBATCH; i++) {
      initdeck(d, 52);
      for (j=0; j<7; j++) cs[j] = draw(d);
      sort(cs);
      score = eval7(cs);
    }
    ticks2 = clock();
    t_elapsed = ((double)(ticks2-ticks1))/CLOCKS_PER_SEC;
    if (t_elapsed >= MAXTIME) break;
  }
  fprintf(stdout, "hands:%d\n", (k+1)*HANDSPERBATCH);
  fprintf(stdout, "time:%.3f\n", t_elapsed);
  fprintf(stdout, "perf:%.3f\n", (k+1)*HANDSPERBATCH/t_elapsed/1000000);
  return 0;
}
