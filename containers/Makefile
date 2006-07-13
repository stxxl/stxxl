all: vector stack mstack many p_queue queue ext_merger bench_pqueue copy_file testmap test_map_random

include ../make.settings

DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)

BERKELEY_DB_COMPILE_OPTIONS = -I/usr/local/BerkeleyDB.4.4/include
BERKELEY_DB_LINK_OPTIONS = -L/usr/local/BerkeleyDB.4.4/lib/ -ldb_cxx
QUANTIFY = # quantify #-best-effort

berkeley_db_benchmark: $(DEPENDENCIES) berkeley_db_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) $(BERKELEY_DB_COMPILE_OPTIONS) -c berkeley_db_benchmark.cpp -DNDEBUG
	$(QUANTIFY) $(LINKER) berkeley_db_benchmark.$(OBJEXT) $(OUT)berkeley_db_benchmark.$(EXEEXT) $(BERKELEY_DB_LINK_OPTIONS) $(STXXL_LINKER_OPTIONS)


stack_benchmark: $(DEPENDENCIES) stack_benchmark.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stack_benchmark.cpp -DNDEBUG
	$(LINKER) stack_benchmark.$(OBJEXT) $(OUT)stack_benchmark.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

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
	g++  -I/home/dementiev/projects/tpie_091905/include -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O3  -c -o tpie_stack_benchmark.o tpie_stack_benchmark.cpp -DNDEBUG
	g++  -I/home/dementiev/projects/tpie_091905/include -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O3 tpie_stack_benchmark.o -L/home/dementiev/projects/tpie_091905/lib -ltpie -lm  -o tpie_stack_benchmark.bin

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

queue: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c testqueue.cpp
	$(LINKER) testqueue.$(OBJEXT) $(OUT)testqueue.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


testmap: testmap.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c testmap.cpp
	$(LINKER)  testmap.$(OBJEXT) $(OUT)testmap.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_random: test_map_random.cpp map_test_handlers.h $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map_random.cpp 
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
	$(RM) *.$(OBJEXT) bench_pqueue.$(EXEEXT) testmap.$(EXEEXT) test_map_random.$(EXEEXT)
	$(RM) ext_merger.$(EXEEXT) p_queue.$(EXEEXT) many_stacks.$(EXEEXT)
	$(RM) test_mstack.$(EXEEXT) test_stack.$(EXEEXT) test.$(EXEEXT)
	$(RM) testqueue.$(EXEEXT) testmap.$(EXEEXT) copy_file.$(EXEEXT)
