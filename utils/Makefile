all: create 

include ../compiler.make


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)

create: createdisks.cpp $(DEPENDENCIES)
	$(GCC) createdisks.cpp -o createdisks $(STXXL_LIB) -DNDEBUG 

memory: testMemory.cpp
	$(GCC) testMemory.cpp -o memory $(STXXL_LIB) 

off:
	g++ -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE off_t_size.cpp -o off_t_size 

clean:
	rm -f *.o 
