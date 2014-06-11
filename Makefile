all:	podds

podds:	podds.c poker.c
	gcc -O3 -pthread -o podds podds.c poker.c

clean:
	rm podds
