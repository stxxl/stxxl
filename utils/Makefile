all: create 

include ../compiler.make

create: 
	$(GCC) createdisks.cpp -o createdisks $(STXXL_OBJ) 

off:
	g++ -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE off_t_size.cpp -o off_t_size 

clean:
	rm -f *.o 
