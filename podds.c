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

  *** UTILIZZO
  parametri: <numero giocatori> <carta 1> <carta 2> [<carta 3> ... <carta 7>]
  dove le prime due carte sono la mano, le altre (opzionali) sono quelle in tavola
  le carte sono indicate da un indice che varia da 0 a 51, e sono raggruppate
  per seme. poiche' nel texas hold 'em i semi sono totalmente indifferenti
  si ha:
    seme 1: 0-12
    seme 2: 13-25
    seme 3: 26-38
    seme 4: 29-51
  a numero maggiore corrisponde carta di valore maggiore, quindi ad esempio
  i numeri 0, 13, 26, 29 corrispondono ai "due", mentre 12, 25, 38, 51 sono
  gli "assi".
  
  *** MAIN
  il programma prende atto della configurazione di carte note (mano+tavola)
  e le rimuove dal mazzo. dopodiche' esegue un numero prefissato di partite casuali
  durante le quali:
  - distribuisce casualmente le carte agli altri giocatori;
  - estrae le carte che rimangono da mettere in tavola;
  - verifica se si e' vinto oppure no.
  alla fine della simulazione la percentuale di partite casuali vinte rispetto
  al totale di quelle giocate costituisce	la probabilita' di vittoria della
  configurazione. nota: le carte estratte (essenzialmente sono numeri interi
  da 0 a 51) vengono preventivamente convertite in un formato piu' conveniente
  per la valutazione delle mani.
  
  *** EVAL7
  per stabilire quale giocatore vince si assegna, tramite la funzione eval7,
  un punteggio intero alle 7 carte (mano+tavola) di ciascun giocatore;
  questo punteggio, per le regole del gioco, sara' il massimo punteggio ottenibile
  prendendo 5 carte dalle 7 utilizzabili: la matrice combs contiene 21 righe
  ognuna delle quali costituisce una differente combinazione degli indici 0,...,6
  presi cinque a cinque. la funzione eval7 utilizza una alla volta queste
  serie di indici per costruire una combinazione di 5 carte, e poi richiama la
  funzione eval5 per valutarla.
  
  *** EVAL5
  questa funzione deve associare ad ogni
  combinazione cs di cinque carte un valore intero (a 32 bit) piu' alto per le
  combinazioni piu' forti, piu' basso per le combinazioni piu' deboli.
  la funzione procede confrontando le carte prima con i punti piu' alti e poi
  con quelli piu' bassi, in modo, ad esempio, da fermarsi non appena si sia
  verificato che c'e' un poker d'assi tra le cinque carte: sara' inutile andare
  a verificare se si trovi anche un tris o una doppia coppia.

*************************************************************************/

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>

/* numero di partite da giocare per la simulazione montecarlo */
#define TOTAL		100000

#define SUITSHIFT	18

/* maschere per manipolare opportunamente le bitmap utilizzate */
#define SUITMASK	0x003C0000 // 3932160
#define BITRANKMASK	0x0003FFF0 // 262128
#define RANKMASK	0x0000000F // 15
#define STRAIGHTMASK	0x000001F0 // 496

typedef struct {
	int c[52];
	int n;
} deck;

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

int randint(int hi) {
	return (int)((float)(rand())/RAND_MAX*hi);
}

int suit(int i) {
	return i/13;
}

int rank(int i) {
	return i%13;
}

int bitcard(int i) {
	int r = rank(i);
	return (1<<(SUITSHIFT+suit(i))) | (1<<(r+5)) | (r == 12 ? 1<<4 : 0) | r;
}

deck * newdeck() {
	int i;
	deck * d = (deck *)malloc(sizeof(deck));
	d->n = 52;
	for (i=0; i<52; i++) d->c[i] = i;
	return d;
}

void initdeck(deck * d, int n) {
	d->n = n;
}

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

void pick(deck * d, int c) {
	int i, m;
	for (i=0; i<d->n; i++)
		if (d->c[i] == c) {
			d->c[i] = d->c[d->n - 1];
			d->c[d->n - 1] = c;
			d->n--;
			break;
		}
}

int flush(int cs[]) {
	int i;
	for (i=1; i<5; i++) if ((cs[i-1] & SUITMASK) != (cs[i] & SUITMASK)) return 0;
	return 1;
}

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
	for (i=1; i<5; i++)
		for (j=0; j<5-i; j++)
			if ((cs[j] & RANKMASK) < (cs[j+1] & RANKMASK)) {
				m = cs[j];
				cs[j] = cs[j+1];
				cs[j+1] = m;
			}
	for (i=0; i<5; i++)
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
	for (i=1; i<5; i++)
		for (j=0; j<5-i; j++)
			if (cv[j] < cv[j+1]) {
				m = cv[j];
				cv[j] = cv[j+1];
				cv[j+1] = m;
				m = rv[j];
				rv[j] = rv[j+1];
				rv[j+1] = m;
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

int eval7(int cs[]) {
	int i, ds[5], max = -1, v;
	for (i = 0; i<21; i++) {
		ds[0] = cs[combs[i][0]]; //
		ds[1] = cs[combs[i][1]]; //
		ds[2] = cs[combs[i][2]]; // tutto cio' e' ottimizzabile
		ds[3] = cs[combs[i][3]]; //
		ds[4] = cs[combs[i][4]]; //
		v = eval5(ds);
		if (v > max) max = v;
	}
	return max;
}

int better(int cs[], int s) {
	int i, ds[5], v;
	for (i = 0; i<21; i++) {
		ds[0] = cs[combs[i][0]]; //
		ds[1] = cs[combs[i][1]]; //
		ds[2] = cs[combs[i][2]]; // tutto cio' e' ottimizzabile
		ds[3] = cs[combs[i][3]]; //
		ds[4] = cs[combs[i][4]]; //
		v = eval5(ds);
		if (v > s) return 1;
	}
	return 0;
}

int main(int argc, char ** argv) {
	int cs[7], as[7], i, j, np = atoi(argv[1]), cs0, cs1;
	int * ohs = (int *)malloc(2*(np-1)*sizeof(int));
	int wins = 0, wininc;
	for (i=0; i<argc-2; i++) as[i] = atoi(argv[i+2]);
	for (i=2; i<argc-2; i++) cs[i] = bitcard(as[i]);
	srand(time(NULL));
	deck * d = newdeck();
	cs0 = bitcard(as[0]);
	cs1 = bitcard(as[1]);
	for (i=0; i<argc-2; i++) pick(d, as[i]);
	for (i=0; i<TOTAL; i++) {
		int score;
		initdeck(d, 52-argc+2);
		cs[0] = cs0;
		cs[1] = cs1;
		for (j=0; j<2*(np-1); j++) ohs[j] = bitcard(draw(d));
		for (j=argc-2; j<7; j++) cs[j] = bitcard(draw(d));
		score = eval7(cs);
		wininc = 1;
		for (j=0; j<np-1; j++) {
			cs[0] = ohs[2*j];
			cs[1] = ohs[2*j+1];
			if (better(cs, score)) {
				wininc = 0;
				break;
			}
		}
		if (wininc) wins++;
	}
	printf("%.3f\n", (float)(wins)/TOTAL);
	free(ohs);
	free(d);
	return 0;
}
