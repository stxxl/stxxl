include Makefile.common
include ../make.settings

tests: $(TESTS) $(TESTS_MAP)
	cd btree
	$(MAKE) tests
	cd ..
#tests: $(TESTS_NON_MSVC)

NDEBUG		 = #-DNDEBUG

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)


stack_benchmark: stack_benchmark.$(EXEEXT)
stack_benchmark.$(EXEEXT): stack_benchmark.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stack_benchmark.cpp $(NDEBUG)
	$(LINKER) stack_benchmark.$(OBJEXT) $(OUT)stack_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_iterators: test_iterators.$(EXEEXT)
test_iterators.$(EXEEXT): test_iterators.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_iterators.cpp
	$(LINKER) test_iterators.$(OBJEXT) $(OUT)test_iterators.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_deque: test_deque.$(EXEEXT)
test_deque.$(EXEEXT): test_deque.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_deque.cpp
	$(LINKER) test_deque.$(OBJEXT) $(OUT)test_deque.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

pq_benchmark: pq_benchmark.$(EXEEXT)
pq_benchmark.$(EXEEXT):  pq_benchmark.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c pq_benchmark.cpp $(NDEBUG)
	$(LINKER) pq_benchmark.$(OBJEXT) $(OUT)pq_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

write_vector: write_vector.$(EXEEXT)
write_vector.$(EXEEXT): write_vector.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c write_vector.cpp
	$(LINKER) write_vector.$(OBJEXT) $(OUT)write_vector.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	
write_vector2: write_vector2.$(EXEEXT)
write_vector2.$(EXEEXT): write_vector2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c write_vector2.cpp
	$(LINKER) write_vector2.$(OBJEXT) $(OUT)write_vector2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_queue: test_queue.$(EXEEXT)
test_queue.$(EXEEXT): test_queue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_queue.cpp
	$(LINKER) test_queue.$(OBJEXT) $(OUT)test_queue.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_queue: test_queue2.$(EXEEXT)
test_queue.$(EXEEXT): test_queue2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_queue2.cpp
	$(LINKER) test_queue2.$(OBJEXT) $(OUT)test_queue2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map: test_map.$(EXEEXT)
test_map.$(EXEEXT): test_map.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map.cpp
	$(LINKER)  test_map.$(OBJEXT) $(OUT)test_map.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_random: test_map_random.$(EXEEXT)
test_map_random.$(EXEEXT): test_map_random.cpp map_test_handlers.h $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map_random.cpp 
	$(LINKER) test_map_random.$(OBJEXT) $(OUT)test_map_random.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

copy_file: copy_file.$(EXEEXT)
copy_file.$(EXEEXT): copy_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c copy_file.cpp
	$(LINKER) copy_file.$(OBJEXT) $(OUT)copy_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	

test_vector: test_vector.$(EXEEXT)
test_vector.$(EXEEXT): test_vector.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_vector.cpp
	$(LINKER)  test_vector.$(OBJEXT) $(OUT)test_vector.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_vector_export: test_vector_export.$(EXEEXT)
test_vector_export.$(EXEEXT): test_vector_export.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_vector_export.cpp
	$(LINKER)  test_vector_export.$(OBJEXT) $(OUT)test_vector_export.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_stack: test_stack.$(EXEEXT)
test_stack.$(EXEEXT): test_stack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stack.cpp
	$(LINKER)  test_stack.$(OBJEXT) $(OUT)test_stack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_migr_stack: test_migr_stack.$(EXEEXT)
test_migr_stack.$(EXEEXT): test_migr_stack.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_migr_stack.cpp
	$(LINKER)  test_migr_stack.$(OBJEXT) $(OUT)test_migr_stack.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_many_stacks: test_many_stacks.$(EXEEXT)
test_many_stacks.$(EXEEXT): test_many_stacks.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_many_stacks.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_many_stacks.$(OBJEXT) $(OUT)test_many_stacks.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_pqueue: test_pqueue.$(EXEEXT)
test_pqueue.$(EXEEXT): test_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_pqueue.cpp
	$(LINKER)  test_pqueue.$(OBJEXT) $(OUT)test_pqueue.$(EXEEXT) $(STXXL_LINKER_OPTIONS) 

test_ext_merger: test_ext_merger.$(EXEEXT)
test_ext_merger.$(EXEEXT): test_ext_merger.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ext_merger.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_ext_merger.$(OBJEXT) $(OUT)test_ext_merger.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_ext_merger2: test_ext_merger2.$(EXEEXT)
test_ext_merger2.$(EXEEXT): test_ext_merger2.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ext_merger2.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_ext_merger2.$(OBJEXT) $(OUT)test_ext_merger2.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

bench_pqueue: bench_pqueue.$(EXEEXT)
bench_pqueue.$(EXEEXT): bench_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c bench_pqueue.cpp -DSTXXL_VERBOSE_LEVEL=0
	$(LINKER)  bench_pqueue.$(OBJEXT) $(OUT)bench_pqueue.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)

monotonic_pq: monotonic_pq.$(EXEEXT)
monotonic_pq.$(EXEEXT): monotonic_pq.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c monotonic_pq.cpp
	$(LINKER)  monotonic_pq.$(OBJEXT) $(OUT)monotonic_pq.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)

benchmark_naive_matrix: benchmark_naive_matrix.$(EXEEXT)
benchmark_naive_matrix.$(EXEEXT): benchmark_naive_matrix.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c benchmark_naive_matrix.cpp -DSTXXL_VERBOSE_LEVEL=0
	$(LINKER) benchmark_naive_matrix.$(OBJEXT) $(OUT)benchmark_naive_matrix.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_matrix: test_matrix.$(EXEEXT)
test_matrix.$(EXEEXT): test_matrix.cpp $(DEPENDENCIES)
        $(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_matrix.cpp -DSTXXL_VERBOSE_LEVEL=0
        $(LINKER)  test_matrix.$(OBJEXT) $(OUT)test_matrix.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

matrix_benchmark: matrix_benchmark.$(EXEEXT)
matrix_benchmark.$(EXEEXT): matrix_benchmark.cpp $(DEPENDENCIES)
        $(COMPILER) $(STXXL_COMPILER_OPTIONS) -c matrix_benchmark.cpp -DSTXXL_VERBOSE_LEVEL=0
        $(LINKER)  matrix_benchmark.$(OBJEXT) $(OUT)matrix_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_vector_sizes: test_vector_sizes.$(EXEEXT)
test_vector_sizes.$(EXEEXT): test_vector_sizes.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_vector_sizes.cpp
	$(LINKER) test_vector_sizes.$(OBJEXT) $(OUT)test_vector_sizes.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) $(CLEAN_TARGETS)
	cd btree
	$(MAKE) clean
	cd ..
