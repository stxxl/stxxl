all: test_stream test_stream1 test_sorted_runs test_push_sort 

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) \
               $(CONTAINER_FILES) $(ALGO_FILES) $(STREAM_FILES)

test_stream1: stream.h test_stream1.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stream1.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_stream1.$(OBJEXT) $(OUT)test_stream1.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_sorted_runs: stream.h test_sorted_runs.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_sorted_runs.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_sorted_runs.$(OBJEXT) $(OUT)test_sorted_runs.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_push_sort: stream.h test_push_sort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_push_sort.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_push_sort.$(OBJEXT) $(OUT)test_push_sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	
test_stream:	stream.h test_stream.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stream.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_stream.$(OBJEXT) $(OUT)test_stream.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) test_stream.$(EXEEXT) test_sorted_runs.$(EXEEXT)
	$(RM) test_push_sort.$(EXEEXT) test_stream1.$(EXEEXT)
