all:ashell

ashell: ashell.o
	g++ ashell.o -o ashell

ashell.o: ashell.cpp
	g++ -c ashell.cpp

clean:
	rm -rf *.o ashell
