all: create 

include ../compiler.make

create: 
	$(GCC) createdisks.cpp -o createdisks $(STXXL_OBJ) 

clean:
	rm -f *.o 
