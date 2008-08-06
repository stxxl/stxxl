include Makefile.common
include ../make.settings

tests: $(TESTS)
	cd btree
	$(MAKE) tests
	cd ..
#tests: $(TESTS_NON_MSVC)

NDEBUG		 = #-DNDEBUG

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)


stack_benchmark: $(DEPENDENCIES) stack_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stack_benchmark.cpp $(NDEBUG)
	$(LINKER) stack_benchmark.$(OBJEXT) $(OUT)stack_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_iterators: test_iterators.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_iterators.cpp
	$(LINKER) test_iterators.$(OBJEXT) $(OUT)test_iterators.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_deque: test_deque.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_deque.cpp
	$(LINKER) test_deque.$(OBJEXT) $(OUT)test_deque.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

pq_benchmark: $(DEPENDENCIES) pq_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c pq_benchmark.cpp $(NDEBUG)
	$(LINKER) pq_benchmark.$(OBJEXT) $(OUT)pq_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

write_vector: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c write_vector.cpp
	$(LINKER) write_vector.$(OBJEXT) $(OUT)write_vector.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	
write_vector2: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c write_vector2.cpp
	$(LINKER) write_vector2.$(OBJEXT) $(OUT)write_vector2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_queue: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_queue.cpp
	$(LINKER) test_queue.$(OBJEXT) $(OUT)test_queue.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


test_map: test_map.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map.cpp
	$(LINKER)  test_map.$(OBJEXT) $(OUT)test_map.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_random: test_map_random.cpp map_test_handlers.h $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map_random.cpp 
	$(LINKER) test_map_random.$(OBJEXT) $(OUT)test_map_random.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

copy_file: copy_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c copy_file.cpp
	$(LINKER) copy_file.$(OBJEXT) $(OUT)copy_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	

test_vector: test_vector.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_vector.cpp
	$(LINKER)  test_vector.$(OBJEXT) $(OUT)test_vector.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_stack: test_stack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stack.cpp
	$(LINKER)  test_stack.$(OBJEXT) $(OUT)test_stack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_migr_stack: test_migr_stack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_migr_stack.cpp
	$(LINKER)  test_migr_stack.$(OBJEXT) $(OUT)test_migr_stack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


test_many_stacks: test_many_stacks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_many_stacks.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_many_stacks.$(OBJEXT) $(OUT)test_many_stacks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_pqueue: test_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_pqueue.cpp
	$(LINKER)  test_pqueue.$(OBJEXT) $(OUT)test_pqueue.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 

test_ext_merger: test_ext_merger.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ext_merger.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER)  test_ext_merger.$(OBJEXT) $(OUT)test_ext_merger.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_ext_merger2: test_ext_merger2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ext_merger2.cpp -DSTXXL_VERBOSE_LEVEL=2
	$(LINKER)  test_ext_merger2.$(OBJEXT) $(OUT)test_ext_merger2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

bench_pqueue: bench_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c bench_pqueue.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  bench_pqueue.$(OBJEXT) $(OUT)bench_pqueue.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)

monotonic_pq: monotonic_pq.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c monotonic_pq.cpp
	$(LINKER)  monotonic_pq.$(OBJEXT) $(OUT)monotonic_pq.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
	cd btree
	$(MAKE) clean
	cd ..
