all: main

include ../compiler.make

main: 
	$(GCC) test_ksort.cpp -o test_ksort $(STXXL_OBJ)

main_opt:
	$(GCC) test_ksort.cpp -o test_ksort_op $(STXXL_OBJ) -DSORT_OPT_PREFETCHING

asch:
	$(GCC) test_asch.cpp -o test_asch $(STXXL_OBJ)

scan:
	$(GCC) test_scan.cpp -o test_scan $(STXXL_OBJ)

genfile:
	$(GCC) gen_file.cpp -o gen_file $(STXXL_OBJ)

sortfile:
	$(GCC) sort_file.cpp -o sort_file $(STXXL_OBJ)

ksort: ksort.cpp ksort.h
	$(GCC) ksort.cpp -o ksort $(STXXL_OBJ) -DSTXXL_VERBOSE_LEVEL=1

stable:
	$(GCC) stable_ksort.cpp -o stable_ksort $(STXXL_OBJ) -DCOUNT_WAIT_TIME

sort:
	$(GCC) sort.cpp -o ssort $(STXXL_OBJ)

clean:
	rm -f *.o sort 
