CC=g++
CFLAGS=-std=c++17 -Wall -Werror -Wextra


metric: metrics.a main.o metrics.hpp
	$(CC) $(CFLAGS) main.o metrics.a -o metric.exe

metrics.a: metrics.o metrics.hpp
	ar crs metrics.a metrics.o
	ranlib metrics.a

metrics.o: metrics.cpp metrics.hpp
	$(CC) -c $(CFLAGS) metrics.cpp -o metrics.o 

main.o: main.cpp metrics.hpp	
	$(CC) -c $(CFLAGS) main.cpp -o main.o


clean:
	rm -rf *.o *.out