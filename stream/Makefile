all: test stream sruns push_sort 

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) \
               $(CONTAINER_FILES) $(ALGO_FILES) $(STREAM_FILES)

stream: stream.h stream.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stream.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  stream.$(OBJEXT) $(OUT)stream.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

sruns: stream.h sorted_runs.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sorted_runs.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  sorted_runs.$(OBJEXT) $(OUT)sorted_runs.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

push_sort: stream.h push_sort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c push_sort.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  push_sort.$(OBJEXT) $(OUT)push_sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	
test:	stream.h test_stream.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stream.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_stream.$(OBJEXT) $(OUT)test_stream.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) stream.$(EXEEXT) sorted_runs.$(EXEEXT)
	$(RM) push_sort.$(EXEEXT) test.$(EXEEXT) test_stream.$(EXEEXT)
