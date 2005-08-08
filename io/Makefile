all_tests: main disktest flushbuffers

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES)


lib: diskqueue.cpp wfs_file.cpp ufs_file.cpp syscall_file.cpp wincall_file.cpp iostats.cpp mmap_file.cpp simdisk_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ufs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c syscall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wincall_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c wfs_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c iostats.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c diskqueue.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c mmap_file.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c simdisk_file.cpp

main: test
	$(LINKER) test.$(OBJEXT) $(OUT)test.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test: test.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test.cpp


disktest: disktest.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c disktest.cpp  
	$(LINKER) disktest.$(OBJEXT) $(OUT)disktest.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 
	

flushbuffers: flushbuffers.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c flushbuffers.cpp
	$(LINKER) flushbuffers.$(OBJEXT) $(OUT)flushbuffers.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) *.$(LIBEXT) disktest.$(EXEEXT)
	$(RM) flushbuffers.$(EXEEXT) test.$(EXEEXT)

