all_tests: main streams 

include ../compiler.make

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES)


main: test.cpp $(DEPENDENCIES)
	$(GCC) test.cpp -o mngtest $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=2 

streams: test_streams.cpp $(DEPENDENCIES)
	$(GCC) test_streams.cpp -o test_streams $(STXXL_LIB)

write_pool: write_pool.cpp $(DEPENDENCIES)
	$(GCC) write_pool.cpp -o test_write_pool $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=2

prefetch_pool: prefetch_pool.cpp $(DEPENDENCIES)
	$(GCC) prefetch_pool.cpp -o test_prefetch_pool $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=2

test1: test_1.cpp $(DEPENDENCIES)
	$(GCC) test_1.cpp -o test_1 $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=0

fragmalloc: fragmalloc.cpp
	$(GCC) fragmalloc.cpp -o fragmalloc $(STXXL_LIB) -DSTXXL_VERBOSE_LEVEL=0 -ggdb

clean:
	rm -f *~ fragmalloc test_1 test_prefetch_pool test_write_pool test_streams mngtest
