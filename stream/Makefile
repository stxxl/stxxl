include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) \
               $(CONTAINER_FILES) $(ALGO_FILES) $(STREAM_FILES)


test_stream1: test_stream1.$(EXEEXT)
test_stream1.$(EXEEXT): test_stream1.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stream1.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_stream1.$(OBJEXT) $(OUT)test_stream1.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_sorted_runs: test_sorted_runs.$(EXEEXT)
test_sorted_runs.$(EXEEXT): test_sorted_runs.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_sorted_runs.cpp
	$(LINKER)  test_sorted_runs.$(OBJEXT) $(OUT)test_sorted_runs.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_push_sort: test_push_sort.$(EXEEXT)
test_push_sort.$(EXEEXT): test_push_sort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_push_sort.cpp
	$(LINKER)  test_push_sort.$(OBJEXT) $(OUT)test_push_sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_stream: test_stream.$(EXEEXT)
test_stream.$(EXEEXT): test_stream.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stream.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_stream.$(OBJEXT) $(OUT)test_stream.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_materialize: test_materialize.$(EXEEXT)
test_materialize.$(EXEEXT): test_materialize.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_materialize.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_materialize.$(OBJEXT) $(OUT)test_materialize.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_transpose: test_transpose.$(EXEEXT)
test_transpose.$(EXEEXT): test_transpose.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_transpose.cpp
	$(LINKER) test_transpose.$(OBJEXT) $(OUT)test_transpose.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_loop: test_loop.$(EXEEXT)
test_loop.$(EXEEXT): test_loop.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_loop.cpp
	$(LINKER) test_loop.$(OBJEXT) $(OUT)test_loop.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) $(CLEAN_TARGETS)
