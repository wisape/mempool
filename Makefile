CC=gcc -Wall -c
LD=gcc

%.o: %.c
	 $(CC) $^ -o $@

mempool_test: mempool_test.o mempool.o
	 $(LD) $^ -o $@

clean:
	rm *.o
	rm mempool_test
