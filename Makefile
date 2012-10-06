podds:	podds.c
	gcc -DMAIN -O3 -pthread -o podds podds.c
	
generator:	podds.c
	gcc -DGENERATOR -O3 -pthread -o podds podds.c

test:	podds.c
	gcc -DTEST -O3 -pthread -o podds podds.c

clean:
	rm podds
