all: vector stack mstack

include ../compiler.make

vector: test.cpp 
	$(GCC) test.cpp -o test_vector $(STXXL_OBJ) 

stack: test_stack.cpp
	$(GCC) test_stack.cpp -o test_stack $(STXXL_OBJ) -DSTXXL_VERBOSE_LEVEL=2

mstack: test_mstack.cpp
	$(GCC) test_mstack.cpp -o test_mstack $(STXXL_OBJ)

clean:
	rm -f *.o sort 
