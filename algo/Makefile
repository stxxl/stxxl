all: asch scan genfile sortfile ksort stable sort 

include ../compiler.make


STXXL_LIB += -DSTXXL_CHECK_ORDER_IN_SORTS

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)

main: test_ksort.cpp $(DEPENDENCIES)
	$(GCC) test_ksort.cpp -o test_ksort $(STXXL_LIB)

main_opt: test_ksort.cpp $(DEPENDENCIES)
	$(GCC) test_ksort.cpp -o test_ksort_op $(STXXL_LIB) # -DSORT_OPT_PREFETCHING

mains: test_sort.cpp
	$(GCC) test_sort.cpp -o test_sort $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=1
	

asch: test_asch.cpp $(DEPENDENCIES)
	$(GCC) test_asch.cpp -o test_asch $(STXXL_LIB)

scan: test_scan.cpp $(DEPENDENCIES)
	$(GCC) test_scan.cpp -o test_scan $(STXXL_LIB)

genfile: gen_file.cpp $(DEPENDENCIES)
	$(GCC) gen_file.cpp -o gen_file $(STXXL_LIB)

sortfile: sort_file.cpp $(DEPENDENCIES)
	$(GCC) sort_file.cpp -o sort_file $(STXXL_LIB)

ksort: ksort.cpp $(DEPENDENCIES)
	$(GCC) ksort.cpp -o ksort $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=1 -DSTXXL_CHECK_ORDER_IN_SORTS

stable: stable_ksort.cpp $(DEPENDENCIES)
	$(GCC) stable_ksort.cpp -o stable_ksort $(STXXL_LIB) -DCOUNT_WAIT_TIME

sort: sort.cpp $(DEPENDENCIES)
	$(GCC) sort.cpp -o ssort $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=1

clean:
	rm -f *.o ssort stable_ksort ksort sort_file gen_file test_scan test_asch test_ksort_op test_ksort
