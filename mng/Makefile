include Makefile.common
include ../make.settings

tests: $(TESTS)

lib: $(LIB_SRC)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mng.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES)


test_mng: test_mng.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mng.cpp -DSTXXL_VERBOSE_LEVEL=2 
	$(LINKER) test_mng.$(OBJEXT) $(OUT)test_mng.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_mng1: test_mng1.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mng1.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_mng1.$(OBJEXT) $(OUT)test_mng1.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_streams: test_streams.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_streams.cpp
	$(LINKER) test_streams.$(OBJEXT) $(OUT)test_streams.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_write_pool: test_write_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_write_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_write_pool.$(OBJEXT) $(OUT)test_write_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_prefetch_pool: test_prefetch_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_prefetch_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) test_prefetch_pool.$(OBJEXT) $(OUT)test_prefetch_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

unittest: unittest.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(CPPUNIT_COMPILER_OPTIONS) -c unittest.cpp
	$(LINKER) unittest.$(OBJEXT) $(OUT)unittest.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT)
	$(RM) test_mng.$(EXEEXT) test_mng1.$(EXEEXT)
	$(RM) test_prefetch_pool.$(EXEEXT) test_write_pool.$(EXEEXT)
	$(RM) test_streams.$(EXEEXT) unittest.$(EXEEXT)
	
