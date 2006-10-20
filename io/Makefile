all_tests: test_io benchmark_disks 

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


lib: diskqueue.cpp wfs_file.cpp ufs_file.cpp syscall_file.cpp wincall_file.cpp iostats.cpp mmap_file.cpp simdisk_file.cpp boostfd_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wincall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wfs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iostats.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c diskqueue.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mmap_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c simdisk_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c boostfd_file.cpp

test_io: test_io.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_io.cpp
	$(LINKER) test_io.$(OBJEXT) $(OUT)test_io.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

benchmark_disks: benchmark_disks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_disks.cpp  
	$(LINKER) benchmark_disks.$(OBJEXT) $(OUT)benchmark_disks.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 
	

flushbuffers: flushbuffers.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c flushbuffers.cpp
	$(LINKER) flushbuffers.$(OBJEXT) $(OUT)flushbuffers.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) *.$(LIBEXT) benchmark_disks.$(EXEEXT)
	$(RM) flushbuffers.$(EXEEXT) test_io.$(EXEEXT)

