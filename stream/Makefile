all: lazy

include ../compiler.make


stream: stream.h stream.cpp
	$(GCC) stream.cpp -o stream $(STXXL_LIB) -g -DSTXXL_VERBOSE_LEVEL=1


