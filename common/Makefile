include Makefile.common
include ../make.settings

tools: $(UTIL_BINARIES)

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: debug.$(OBJEXT) rand.$(OBJEXT) version.$(OBJEXT) seed.$(OBJEXT)


debug.$(OBJEXT): debug.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c debug.cpp
	
rand.$(OBJEXT): rand.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c rand.cpp
	
version.$(OBJEXT): version.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c version.cpp
	
seed.$(OBJEXT): seed.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c seed.cpp


DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES) $(ALGO_FILES)


stxxl_info: stxxl_info.$(EXEEXT)
stxxl_info.$(EXEEXT): stxxl_info.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c stxxl_info.cpp
	$(LINKER) stxxl_info.$(OBJEXT) $(OUT)stxxl_info.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_random: test_random.$(EXEEXT)
test_random.$(EXEEXT): test_random.cpp $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_random.cpp
	$(LINKER) test_random.$(OBJEXT) $(OUT)test_random.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
