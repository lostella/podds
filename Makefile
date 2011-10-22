podds:	podds.c
	gcc -O3 -pthread -o podds podds.c

clean:
	rm podds
