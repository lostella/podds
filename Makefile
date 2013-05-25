all:	podds pwins

podds:	podds.c poker.c
	gcc -O3 -pthread -o podds podds.c poker.c

pwins:	pwins.c poker.c
	gcc -O3 -o pwins pwins.c poker.c

clean:
	rm podds pwins
