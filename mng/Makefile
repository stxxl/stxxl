include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: mng.$(OBJEXT)


mng.$(OBJEXT): mng.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mng.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES)


test_mng: test_mng.$(EXEEXT)
test_mng.$(EXEEXT): test_mng.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mng.cpp -DSTXXL_VERBOSE_LEVEL=2 
	$(LINKER) test_mng.$(OBJEXT) $(OUT)test_mng.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_mng1: test_mng1.$(EXEEXT)
test_mng1.$(EXEEXT): test_mng1.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mng1.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_mng1.$(OBJEXT) $(OUT)test_mng1.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_buf_streams: test_buf_streams.$(EXEEXT)
test_buf_streams.$(EXEEXT): test_buf_streams.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_buf_streams.cpp
	$(LINKER) test_buf_streams.$(OBJEXT) $(OUT)test_buf_streams.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_block_alloc_strategy: test_block_alloc_strategy.$(EXEEXT)
test_block_alloc_strategy.$(EXEEXT): test_block_alloc_strategy.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_block_alloc_strategy.cpp
	$(LINKER) test_block_alloc_strategy.$(OBJEXT) $(OUT)test_block_alloc_strategy.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_write_pool: test_write_pool.$(EXEEXT)
test_write_pool.$(EXEEXT): test_write_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_write_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_write_pool.$(OBJEXT) $(OUT)test_write_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_prefetch_pool: test_prefetch_pool.$(EXEEXT)
test_prefetch_pool.$(EXEEXT): test_prefetch_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_prefetch_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_prefetch_pool.$(OBJEXT) $(OUT)test_prefetch_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_mng_recursive_alloc: test_mng_recursive_alloc.$(EXEEXT)
test_mng_recursive_alloc.$(EXEEXT): test_mng_recursive_alloc.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mng_recursive_alloc.cpp -DSTXXL_VERBOSE_LEVEL=3
	$(LINKER) test_mng_recursive_alloc.$(OBJEXT) $(OUT)test_mng_recursive_alloc.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

unittest: unittest.$(EXEEXT) $(DEPENDENCIES)
unittest.$(EXEEXT): unittest.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(CPPUNIT_COMPILER_OPTIONS) -c unittest.cpp
	$(LINKER) unittest.$(OBJEXT) $(OUT)unittest.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
	
