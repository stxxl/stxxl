all_tests: lib main disktest flushbuffers

include ../compiler.make

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


lib: diskqueue
	ar cr libstxxl.a diskqueue.o

main: test lib 
	$(GCC) test.o -o main $(STXXL_LIB) 

test: test.cpp $(DEPENDENCIES)
	$(GCC) -c test.cpp

diskqueue: diskqueue.cpp $(DEPENDENCIES)
	$(GCC) -c diskqueue.cpp

disktest: disktest.cpp $(DEPENDENCIES)
	$(GCC) disktest.cpp -o disktest $(STXXL_LIB)

flushbuffers: flushbuffers.cpp $(DEPENDENCIES)
	$(GCC) flushbuffers.cpp -o flush $(STXXL_LIB) 

clean:
	rm -f *.o *.a main disktest flush 

