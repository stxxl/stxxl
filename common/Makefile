include Makefile.common
include ../make.settings

tests: $(TESTS)
#tests: $(TESTS_NON_MSVC)

lib: $(LIB_SRC)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c log.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c debug.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c rand.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c version.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c seed.cpp


clean:
	$(RM) *.$(OBJEXT) *.$(EXEEXT)
