include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_LONG_COMPILE)
#tests: $(TESTS_NON_MSVC)

lib: async_schedule.$(OBJEXT)


async_schedule.$(OBJEXT): async_schedule.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c async_schedule.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)


test_sort: test_sort.$(EXEEXT)
test_sort.$(EXEEXT): test_sort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_sort.cpp -DSTXXL_VERBOSE_LEVEL=0
	$(LINKER)  test_sort.$(OBJEXT) $(OUT)test_sort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_sort_all_parameters: test_sort_all_parameters.$(EXEEXT)
test_sort_all_parameters.$(EXEEXT): test_sort_all_parameters.cpp # takes long to compile
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_sort_all_parameters.cpp 
	$(LINKER)  test_sort_all_parameters.$(OBJEXT) $(OUT)test_sort_all_parameters.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_ksort: test_ksort.$(EXEEXT)
test_ksort.$(EXEEXT): test_ksort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ksort.cpp -DSTXXL_CHECK_ORDER_IN_SORTS
	$(LINKER)  test_ksort.$(OBJEXT) $(OUT)test_ksort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_ksort_all_parameters: test_ksort_all_parameters.$(EXEEXT)
test_ksort_all_parameters.$(EXEEXT): test_ksort_all_parameters.cpp $(DEPENDENCIES) # takes long to compile 
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_ksort_all_parameters.cpp
	$(LINKER)  test_ksort_all_parameters.$(OBJEXT) $(OUT)test_ksort_all_parameters.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_stable_ksort: test_stable_ksort.$(EXEEXT)
test_stable_ksort.$(EXEEXT): test_stable_ksort.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stable_ksort.cpp
	$(LINKER)  test_stable_ksort.$(OBJEXT) $(OUT)test_stable_ksort.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_stable_ksort_all_parameters: test_stable_ksort_all_parameters.$(EXEEXT)
test_stable_ksort_all_parameters.$(EXEEXT): test_stable_ksort_all_parameters.cpp $(DEPENDENCIES) # takes long to compile
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_stable_ksort_all_parameters.cpp
	$(LINKER)  test_stable_ksort_all_parameters.$(OBJEXT) $(OUT)test_stable_ksort_all_parameters.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

sort_file: sort_file.$(EXEEXT)
sort_file.$(EXEEXT): sort_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c sort_file.cpp
	$(LINKER)  sort_file.$(OBJEXT) $(OUT)sort_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

copy_and_sort_file: copy_and_sort_file.$(EXEEXT)
copy_and_sort_file.$(EXEEXT): copy_and_sort_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c copy_and_sort_file.cpp
	$(LINKER) copy_and_sort_file.$(OBJEXT) $(OUT)copy_and_sort_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_asch: test_asch.$(EXEEXT)
test_asch.$(EXEEXT): test_asch.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_asch.cpp
	$(LINKER)  test_asch.$(OBJEXT) $(OUT)test_asch.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_scan: test_scan.$(EXEEXT)
test_scan.$(EXEEXT): test_scan.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_scan.cpp
	$(LINKER)  test_scan.$(OBJEXT) $(OUT)test_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

gen_file: gen_file.$(EXEEXT)
gen_file.$(EXEEXT): gen_file.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c gen_file.cpp
	$(LINKER)  gen_file.$(OBJEXT) $(OUT)gen_file.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_random_shuffle: test_random_shuffle.$(EXEEXT)
test_random_shuffle.$(EXEEXT): test_random_shuffle.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_random_shuffle.cpp -DSTXXL_VERBOSE_LEVEL=0
	$(LINKER)  test_random_shuffle.$(OBJEXT) $(OUT)test_random_shuffle.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) $(CLEAN_TARGETS)
