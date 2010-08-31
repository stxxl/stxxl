include ../Makefile.common
include ../../make.settings

tests: $(TESTS_HASH_MAP)


STXXL_COMPILER_OPTIONS = $(STXXL_COMPILER_OPTIONS) -DSTXXL_VERBOSE_LEVEL=0

TESTS_EXECUTABLES = test_block_cache.$(EXEEXT) test_hash_map.$(EXEEXT) test_iterators.$(EXEEXT) test_reader_writer.$(EXEEXT)

test_block_cache: test_block_cache.$(EXEEXT)
test_block_cache.$(EXEEXT): $(DEPENDENCIES) test_block_cache.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_block_cache.cpp
	$(LINKER) test_block_cache.$(OBJEXT) $(OUT)test_block_cache.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_hash_map: test_hash_map.$(EXEEXT)
test_hash_map.$(EXEEXT): $(DEPENDENCIES) test_hash_map.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_hash_map.cpp
	$(LINKER) test_hash_map.$(OBJEXT) $(OUT)test_hash_map.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_iterators: test_iterators.$(EXEEXT)
test_iterators.$(EXEEXT): $(DEPENDENCIES) test_iterators.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_iterators.cpp
	$(LINKER) test_iterators.$(OBJEXT) $(OUT)test_iterators.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_reader_writer: test_reader_writer.$(EXEEXT)
test_reader_writer.$(EXEEXT): $(DEPENDENCIES) test_reader_writer.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_reader_writer.cpp
	$(LINKER) test_reader_writer.$(OBJEXT) $(OUT)test_reader_writer.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) $(TESTS_EXECUTABLES)

	
