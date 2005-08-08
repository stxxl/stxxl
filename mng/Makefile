all_tests: main streams write_pool prefetch_pool

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES)

lib: mng.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mng.cpp
	

main: test.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test.cpp -DSTXXL_VERBOSE_LEVEL=2 
	$(LINKER) test.$(OBJEXT) $(OUT)test.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

streams: test_streams.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_streams.cpp
	$(LINKER) test_streams.$(OBJEXT) $(OUT)test_streams.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

write_pool: write_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c write_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) write_pool.$(OBJEXT) $(OUT)write_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

prefetch_pool: prefetch_pool.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c prefetch_pool.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER) prefetch_pool.$(OBJEXT) $(OUT)prefetch_pool.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) prefetch_pool.$(OBJEXT) write_pool.$(OBJEXT)
	$(RM) test_streams.$(OBJEXT) test.$(OBJEXT)
	$(RM) prefetch_pool.$(EXEEXT) write_pool.$(EXEEXT)
	$(RM) test_streams.$(EXEEXT) test.$(EXEEXT)
