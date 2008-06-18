include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_EXTRA)

lib: $(LIB_SRC)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wincall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wfs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iostats.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c diskqueue.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mmap_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c simdisk_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c boostfd_file.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


test_io: test_io.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_io.cpp
	$(LINKER) test_io.$(OBJEXT) $(OUT)test_io.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

benchmark_disks: benchmark_disks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_disks.cpp  
	$(LINKER) benchmark_disks.$(OBJEXT) $(OUT)benchmark_disks.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 
	

flushbuffers: flushbuffers.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c flushbuffers.cpp
	$(LINKER) flushbuffers.$(OBJEXT) $(OUT)flushbuffers.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

unittest: unittest.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(CPPUNIT_COMPILER_OPTIONS) -c unittest.cpp
	$(LINKER) unittest.$(OBJEXT) $(OUT)unittest.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)

sd_test: sd_test.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sd_test.cpp
	$(LINKER) sd_test.$(OBJEXT) $(OUT)sd_test.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(LIBEXT) benchmark_disks.$(EXEEXT)
	$(RM) flushbuffers.$(EXEEXT) test_io.$(EXEEXT) unittest.$(EXEEXT)

