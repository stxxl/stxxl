all: vector stack mstack

include ../compiler.make

vector: 
	$(GCC) test.cpp -o test_vector $(STXXL_OBJ) 


stack:
	$(GCC) test_stack.cpp -o test_stack $(STXXL_OBJ)

mstack:
	$(GCC) test_mstack.cpp -o test_mstack $(STXXL_OBJ)

clean:
	rm -f *.o sort 
