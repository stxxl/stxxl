all: vector stack mstack

include ../compiler.make

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)

vector: test.cpp $(DEPENDENCIES)
	$(GCC) test.cpp -o test_vector $(STXXL_OBJ) 

stack: test_stack.cpp $(DEPENDENCIES)
	$(GCC) test_stack.cpp -o test_stack $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=2

mstack: test_mstack.cpp $(DEPENDENCIES)
	$(GCC) test_mstack.cpp -o test_mstack $(STXXL_LIB)


many: many_stacks.cpp $(DEPENDENCIES)
	$(GCC) many_stacks.cpp -o many_stacks $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=1

clean:
	rm -f *.o sort 
