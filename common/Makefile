include Makefile.common
include ../make.settings

tests: $(TESTS)

lib: $(LIB_SRC)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c log.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c debug.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c rand.cpp


clean:
	$(RM) *.$(OBJEXT)
