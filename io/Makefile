include Makefile.common
include ../make.settings

tools: $(UTIL_BINARIES)

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: ufs_file_base.$(OBJEXT) syscall_file.$(OBJEXT) wincall_file.$(OBJEXT) wfs_file_base.$(OBJEXT) \
iostats.$(OBJEXT) \
mmap_file.$(OBJEXT) simdisk_file.$(OBJEXT) boostfd_file.$(OBJEXT) mem_file.$(OBJEXT) \
fileperblock_file.$(OBJEXT) request.$(OBJEXT) request_with_waiters.$(OBJEXT) \
request_state_impl_basic.$(OBJEXT) serving_request.$(OBJEXT) \
disk_queued_file.$(OBJEXT) request_queue_impl_qwqr.$(OBJEXT) request_queue_impl_1q.$(OBJEXT) \
request_queue_impl_worker.$(OBJEXT) wbtl_file.$(OBJEXT) create_file.$(OBJEXT)



ufs_file_base.$(OBJEXT): ufs_file_base.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file_base.cpp

syscall_file.$(OBJEXT): syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp

wbtl_file.$(OBJEXT): wbtl_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wbtl_file.cpp

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

request_with_waiters.$(OBJEXT): request_with_waiters.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_with_waiters.cpp

request_state_impl_basic.$(OBJEXT): request_state_impl_basic.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_state_impl_basic.cpp

serving_request.$(OBJEXT): serving_request.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c serving_request.cpp

disk_queued_file.$(OBJEXT): disk_queued_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c disk_queued_file.cpp

request_queue_impl_qwqr.$(OBJEXT): request_queue_impl_qwqr.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_queue_impl_qwqr.cpp

request_queue_impl_1q.$(OBJEXT): request_queue_impl_1q.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_queue_impl_1q.cpp

request_queue_impl_worker.$(OBJEXT): request_queue_impl_worker.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c request_queue_impl_worker.cpp

create_file.$(OBJEXT): create_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c create_file.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


test_io: test_io.$(EXEEXT)
test_io.$(EXEEXT): test_io.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_io.cpp
	$(LINKER) test_io.$(OBJEXT) $(OUT)test_io.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_io_sizes: test_io_sizes.$(EXEEXT)
test_io_sizes.$(EXEEXT): test_io_sizes.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_io_sizes.cpp
	$(LINKER) test_io_sizes.$(OBJEXT) $(OUT)test_io_sizes.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_cancel: test_cancel.$(EXEEXT)
test_cancel.$(EXEEXT): test_cancel.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_cancel.cpp
	$(LINKER) test_cancel.$(OBJEXT) $(OUT)test_cancel.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

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

gen_file: gen_file.$(EXEEXT)
gen_file.$(EXEEXT): gen_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c gen_file.cpp
	$(LINKER) gen_file.$(OBJEXT) $(OUT)gen_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)

benchmark_disk_and_flash: benchmark_disk_and_flash.$(EXEEXT)
benchmark_disk_and_flash.$(EXEEXT): benchmark_disk_and_flash.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_disk_and_flash.cpp
	$(LINKER) benchmark_disk_and_flash.$(OBJEXT) $(OUT)benchmark_disk_and_flash.$(EXEEXT) $(STXXL_LINKER_OPTIONS) $(CPPUNIT_LINKER_OPTIONS)

benchmark_configured_disks: benchmark_configured_disks.$(EXEEXT)
benchmark_configured_disks.$(EXEEXT): benchmark_configured_disks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_configured_disks.cpp
	$(LINKER) benchmark_configured_disks.$(OBJEXT) $(OUT)benchmark_configured_disks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

benchmark_random_block_access: benchmark_random_block_access.$(EXEEXT)
benchmark_random_block_access.$(EXEEXT): benchmark_random_block_access.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_random_block_access.cpp
	$(LINKER) benchmark_random_block_access.$(OBJEXT) $(OUT)benchmark_random_block_access.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

iobench_scatter_in_place: iobench_scatter_in_place.$(EXEEXT)
iobench_scatter_in_place.$(EXEEXT): iobench_scatter_in_place.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iobench_scatter_in_place.cpp
	$(LINKER) iobench_scatter_in_place.$(OBJEXT) $(OUT)iobench_scatter_in_place.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

verify_disks: verify_disks.$(EXEEXT)
verify_disks.$(EXEEXT): verify_disks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c verify_disks.cpp  
	$(LINKER) verify_disks.$(OBJEXT) $(OUT)verify_disks.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 


clean:
	$(RM) $(CLEAN_TARGETS)
