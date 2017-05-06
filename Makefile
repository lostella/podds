all:	podds podds-perftest

podds:	podds.c poker.c xorshift.c
	gcc -O3 -pthread -o podds podds.c poker.c xorshift.c

podds-perftest:	podds-perftest.c poker.c xorshift.c
	gcc -O3 -o podds-perftest podds-perftest.c poker.c xorshift.c

clean:
	rm podds podds-perftest
