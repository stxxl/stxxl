all: thr 


include ../compiler.make

threads:
	$(GCC) threads.cpp -o threads -lpthread 

dev:
	$(GCC) dev.cpp -o dev


thr:
	$(GCC) test_thr.cpp -o test_thr $(STXXL_OBJ) 


blk:
	$(GCC) block.cpp -o block $(STXXL_OBJ)


stack_band: stack_band.cpp
	$(GCC) stack_band.cpp -o stack_band $(STXXL_OBJ)

clean:
	rm -f *.o threads dev
