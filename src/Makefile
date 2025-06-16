CC=g++
CFLAGS=-std=c++17 -Wall -Werror -Wextra


metric: metrics.o main.o
	$(CC) $(CFLAGS) metrics.o main.o -o a.out

metrics: metrics.cpp metrics.hpp
	$(CC) -c $(CFLAGS) metrics.cpp -o metrics.o

main.o: main.cpp metrics.hpp
	$(CC) -c $(CFLAGS) main.cpp -o main.o


clean:
	rm -rf *.o *.out