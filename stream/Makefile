all: test 

include ../compiler.make

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) \
               $(CONTAINER_FILES) $(ALGO_FILES) $(STREAM_FILES)

stream: stream.h stream.cpp
	$(GCC) stream.cpp -o stream $(STXXL_LIB) -g -DSTXXL_VERBOSE_LEVEL=1


test:	stream.h test_stream.cpp
	$(GCC) test_stream.cpp -o test $(STXXL_LIB) -g -DSTXXL_VERBOSE_LEVEL=1
