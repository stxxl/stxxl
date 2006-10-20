all: test_vector test_stack test_migr_stack test_many_stacks test_pqueue test_queue test_ext_merger bench_pqueue copy_file test_map test_map_random test_deque

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)

BERKELEY_DB_COMPILE_OPTIONS = -I/usr/local/BerkeleyDB.4.4/include
BERKELEY_DB_LINK_OPTIONS = -L/usr/local/BerkeleyDB.4.4/lib/ -ldb_cxx
QUANTIFY = # quantify #-best-effort

TPIE_COMPILER_OPTIONS = -I/home/dementiev/projects/tpie_091905/include -DHAVE_CONFIG_H 
TPIE_LINKER_OPTIONS = -L/home/dementiev/projects/tpie_091905/lib -ltpie -lm

berkeley_db_benchmark: $(DEPENDENCIES) berkeley_db_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(BERKELEY_DB_COMPILE_OPTIONS) $(TPIE_COMPILER_OPTIONS) -c berkeley_db_benchmark.cpp -DNDEBUG
	$(LINKER) berkeley_db_benchmark.$(OBJEXT) $(OUT)berkeley_db_benchmark.$(EXEEXT) $(BERKELEY_DB_LINK_OPTIONS) $(STXXL_LINKER_OPTIONS) $(TPIE_LINKER_OPTIONS) 


stack_benchmark: $(DEPENDENCIES) stack_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stack_benchmark.cpp -DNDEBUG
	$(LINKER) stack_benchmark.$(OBJEXT) $(OUT)stack_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_deque: test_deque.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_deque.cpp
	$(LINKER) test_deque.$(OBJEXT) $(OUT)test_deque.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

pq_benchmark: $(DEPENDENCIES) pq_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c pq_benchmark.cpp -DNDEBUG
	$(LINKER) pq_benchmark.$(OBJEXT) $(OUT)pq_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


leda_sm_pq_benchmark: $(DEPENDENCIES) leda_sm_pq_benchmark.cpp
	/usr/local/bin/g++-2.95.3 -I/software/LEDA/4.2.1//incl -I/home/dementiev/projects/LEDA-SM-1.3/incl  -fpic -D_LINUX -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -O3 -c leda_sm_pq_benchmark.cpp
	/usr/local/bin/g++-2.95.3 -o leda_sm_pq_benchmark.bin leda_sm_pq_benchmark.o -L/software/LEDA/4.2.1/ \
 -L/home/dementiev/projects/LEDA-SM-1.3 -L/usr/X11R6/lib \
 -Xlinker -rpath -Xlinker /software/LEDA/4.2.1/ -Xlinker -rpath -Xlinker /home/dementiev/projects/LEDA-SM-1.3 -Xlinker -rpath -Xlinker /usr/X11R6/lib \
 -lW -lP -lG -lL -lX11 -lm  -lLEDA-SM


tpie_stack_benchmark: $(DEPENDENCIES) tpie_stack_benchmark.cpp
	$(COMPILER)  $(TPIE_COMPILER_OPTIONS) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O3  -c -o tpie_stack_benchmark.o tpie_stack_benchmark.cpp -DNDEBUG
	$(LINKER) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O3 tpie_stack_benchmark.o $(TPIE_LINKER_OPTIONS)  -o tpie_stack_benchmark.bin

leda_sm_stack_benchmark: $(DEPENDENCIES) leda_sm_stack_benchmark.cpp
	/usr/local/bin/g++-2.95.3 -I/software/LEDA/4.2.1//incl -I/home/dementiev/projects/LEDA-SM-1.3/incl  -fpic -D_LINUX -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -O3 -c leda_sm_stack_benchmark.cpp
	/usr/local/bin/g++-2.95.3 -o leda_sm_stack_benchmark.bin leda_sm_stack_benchmark.o -L/software/LEDA/4.2.1/ \
 -L/home/dementiev/projects/LEDA-SM-1.3 -L/usr/X11R6/lib \
 -Xlinker -rpath -Xlinker /software/LEDA/4.2.1/ -Xlinker -rpath -Xlinker /home/dementiev/projects/LEDA-SM-1.3 -Xlinker -rpath -Xlinker /usr/X11R6/lib \
 -lW -lP -lG -lL -lX11 -lm  -lLEDA-SM

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

bench_pqueue: bench_pqueue.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c bench_pqueue.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  bench_pqueue.$(OBJEXT) $(OUT)bench_pqueue.$(EXEEXT)  $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) bench_pqueue.$(EXEEXT) test_map.$(EXEEXT) test_map_random.$(EXEEXT)
	$(RM) test_ext_merger.$(EXEEXT) test_pqueue.$(EXEEXT) test_many_stacks.$(EXEEXT)
	$(RM) test_migr_stack.$(EXEEXT) test_stack.$(EXEEXT) test_vector.$(EXEEXT)
	$(RM) test_queue.$(EXEEXT) copy_file.$(EXEEXT) test_deque.$(EXEEXT)
