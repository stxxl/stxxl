all: asch scan genfile sortfile ksort stable sort 

include ../make.settings



DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)


lib: async_schedule.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c async_schedule.cpp

manyunits:
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c munits1.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c munits2.cpp
	$(LINKER) munits1.$(OBJEXT) munits2.$(OBJEXT) $(OUT)munits.$(EXEEXT) $(STXXL_LINKER_OPTIONS)



main: test_ksort.cpp $(DEPENDENCIES) # takes long to compile 
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ksort.cpp
	$(LINKER)  test_ksort.$(OBJEXT) $(OUT)test_ksort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

mains: test_sort.cpp # takes long to compile
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_sort.cpp -DSTXXL_VERBOSE_LEVEL=1
	$(LINKER)  test_sort.$(OBJEXT) $(OUT)test_sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)
	
asch: test_asch.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_asch.cpp
	$(LINKER)  test_asch.$(OBJEXT) $(OUT)test_asch.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

scan: test_scan.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_scan.cpp
	$(LINKER)  test_scan.$(OBJEXT) $(OUT)test_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

genfile: gen_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c gen_file.cpp
	$(LINKER)  gen_file.$(OBJEXT) $(OUT)gen_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

sortfile: sort_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sort_file.cpp
	$(LINKER)  sort_file.$(OBJEXT) $(OUT)sort_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

ksort: ksort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c ksort.cpp -DSTXXL_VERBOSE_LEVEL=1 -DSTXXL_CHECK_ORDER_IN_SORTS
	$(LINKER)  ksort.$(OBJEXT) $(OUT)ksort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

stable: stable_ksort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stable_ksort.cpp -DCOUNT_WAIT_TIME
	$(LINKER)  stable_ksort.$(OBJEXT) $(OUT)stable_ksort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

sort: sort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sort.cpp -DSTXXL_VERBOSE_LEVEL=0
	$(LINKER)  sort.$(OBJEXT) $(OUT)sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT) sort.$(EXEEXT) stable_ksort.$(EXEEXT) 
	$(RM) ksort.$(EXEEXT) sort_file.$(EXEEXT) gen_file.$(EXEEXT) 
	$(RM) test_scan.$(EXEEXT) test_asch.$(EXEEXT) test_ksort.$(EXEEXT) 
	$(RM) test_sort.$(EXEEXT) munits.$(EXEEXT)

