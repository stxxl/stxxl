all: main

include ../compiler.make

main: test diskqueue 
	$(GCC) test.o diskqueue.o -o main -lpthread

test:
	$(GCC) -c test.cpp

diskqueue:
	$(GCC) -c diskqueue.cpp

disktestmain:
	$(GCC) -c disktest.cpp

flushbuffers:
	$(GCC) flushbuffers.cpp -o flush $(STXXL_OBJ) 

dsktest: disktestmain
	$(GCC) disktest.o -o disktest $(STXXL_OBJ) 

clean:
	rm -f *.o test disktest
