include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: ufs_file.$(OBJEXT) syscall_file.$(OBJEXT) wincall_file.$(OBJEXT) wfs_file.$(OBJEXT) iostats.$(OBJEXT) \
diskqueue.$(OBJEXT) mmap_file.$(OBJEXT) simdisk_file.$(OBJEXT) boostfd_file.$(OBJEXT) mem_file.$(OBJEXT)


ufs_file.$(OBJEXT): ufs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file.cpp

syscall_file.$(OBJEXT): syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp

wincall_file.$(OBJEXT): wincall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wincall_file.cpp

wfs_file.$(OBJEXT): wfs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wfs_file.cpp

iostats.$(OBJEXT): iostats.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iostats.cpp

diskqueue.$(OBJEXT): diskqueue.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c diskqueue.cpp

mmap_file.$(OBJEXT): mmap_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mmap_file.cpp

simdisk_file.$(OBJEXT): simdisk_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c simdisk_file.cpp

boostfd_file.$(OBJEXT): boostfd_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c boostfd_file.cpp

mem_file.$(OBJEXT): mem_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mem_file.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


test_io: test_io.$(EXEEXT)
test_io.$(EXEEXT): test_io.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_io.cpp
	$(LINKER) test_io.$(OBJEXT) $(OUT)test_io.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

benchmark_disks: benchmark_disks.$(EXEEXT)
benchmark_disks.$(EXEEXT): benchmark_disks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_disks.cpp  
	$(LINKER) benchmark_disks.$(OBJEXT) $(OUT)benchmark_disks.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 
	
flushbuffers: flushbuffers.$(EXEEXT)
flushbuffers.$(EXEEXT): flushbuffers.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c flushbuffers.cpp
	$(LINKER) flushbuffers.$(OBJEXT) $(OUT)flushbuffers.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

unittest: unittest.$(EXEEXT)
unittest.$(EXEEXT): unittest.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(CPPUNIT_COMPILER_OPTIONS) -c unittest.cpp
	$(LINKER) unittest.$(OBJEXT) $(OUT)unittest.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)

sd_test: sd_test.$(EXEEXT)
sd_test.$(EXEEXT): sd_test.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sd_test.cpp
	$(LINKER) sd_test.$(OBJEXT) $(OUT)sd_test.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
