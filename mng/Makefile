all_tests: main streams 

include ../compiler.make

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES)


main: test.cpp $(DEPENDENCIES)
	$(GCC) test.cpp -o mngtest $(STXXL_LIB) -DSTXXL_VERBOSE_ 

streams: test_streams.cpp $(DEPENDENCIES)
	$(GCC) test_streams.cpp -o test_streams $(STXXL_LIB)


