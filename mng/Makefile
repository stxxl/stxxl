all: main

include ../compiler.make

main:
	$(GCC) test.cpp -o mngtest $(STXXL_OBJ) 

streams:
	$(GCC) test_streams.cpp -o test_streams $(STXXL_OBJ)
