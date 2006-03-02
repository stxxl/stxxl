all: vector stack mstack many p_queue queue ext_merger bench_pqueue copy_file

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)


queue: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c testqueue.cpp
	$(LINKER) testqueue.$(OBJEXT) $(OUT)testqueue.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


testmap: testmap.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c testmap.cpp
	$(LINKER)  testmap.$(OBJEXT) $(OUT)testmap.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_random: test_map_random.cpp map_test_handlers.h $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map_random.cpp -O3 
	$(LINKER) test_map_random.$(OBJEXT) $(OUT)test_map_random.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

copy_file: copy_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c copy_file.cpp	
	$(LINKER) copy_file.$(OBJEXT) $(OUT)copy_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	

vector: test.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test.cpp
	$(LINKER)  test.$(OBJEXT) $(OUT)test.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

stack: test_stack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stack.cpp
	$(LINKER)  test_stack.$(OBJEXT) $(OUT)test_stack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

mstack: test_mstack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_mstack.cpp
	$(LINKER)  test_mstack.$(OBJEXT) $(OUT)test_mstack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


many: many_stacks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c many_stacks.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  many_stacks.$(OBJEXT) $(OUT)many_stacks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

p_queue: p_queue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c p_queue.cpp
	$(LINKER)  p_queue.$(OBJEXT) $(OUT)p_queue.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

ext_merger: ext_merger.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ext_merger.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER)  ext_merger.$(OBJEXT) $(OUT)ext_merger.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

bench_pqueue: bench_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c bench_pqueue.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  bench_pqueue.$(OBJEXT) $(OUT)bench_pqueue.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) bench_pqueue.$(EXEEXT)
	$(RM) ext_merger.$(EXEEXT) p_queue.$(EXEEXT) many_stacks.$(EXEEXT)
	$(RM) test_mstack.$(EXEEXT) test_stack.$(EXEEXT) test.$(EXEEXT)
	$(RM) testqueue.$(EXEEXT) testmap.$(EXEEXT) copy_file.$(EXEEXT)
