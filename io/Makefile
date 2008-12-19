include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: ufs_file_base.$(OBJEXT) syscall_file.$(OBJEXT) wincall_file.$(OBJEXT) wfs_file_base.$(OBJEXT) \
iostats.$(OBJEXT) \
mmap_file.$(OBJEXT) simdisk_file.$(OBJEXT) boostfd_file.$(OBJEXT) mem_file.$(OBJEXT) \
fileperblock_file.$(OBJEXT) request.$(OBJEXT) request_waiters_impl_basic.$(OBJEXT) \
request_state_impl_basic.$(OBJEXT) request_impl_basic.$(OBJEXT) \
file_request_basic.$(OBJEXT) request_queue_impl_qwqr.$(OBJEXT) \



ufs_file_base.$(OBJEXT): ufs_file_base.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file_base.cpp

syscall_file.$(OBJEXT): syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp

wincall_file.$(OBJEXT): wincall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wincall_file.cpp

wfs_file_base.$(OBJEXT): wfs_file_base.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wfs_file_base.cpp

iostats.$(OBJEXT): iostats.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iostats.cpp

mmap_file.$(OBJEXT): mmap_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mmap_file.cpp

simdisk_file.$(OBJEXT): simdisk_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c simdisk_file.cpp

boostfd_file.$(OBJEXT): boostfd_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c boostfd_file.cpp

mem_file.$(OBJEXT): mem_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mem_file.cpp

fileperblock_file.$(OBJEXT): fileperblock_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c fileperblock_file.cpp

request.$(OBJEXT): request.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request.cpp

request_waiters_impl_basic.$(OBJEXT): request_waiters_impl_basic.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_waiters_impl_basic.cpp

request_state_impl_basic.$(OBJEXT): request_state_impl_basic.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_state_impl_basic.cpp

request_impl_basic.$(OBJEXT): request_impl_basic.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_impl_basic.cpp

request_impl_qwqr.$(OBJEXT): request_impl_qwqr.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_impl_qwqr.cpp

file_request_basic.$(OBJEXT): file_request_basic.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c file_request_basic.cpp

request_queue_impl_qwqr.$(OBJEXT): request_queue_impl_qwqr.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_queue_impl_qwqr.cpp


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
