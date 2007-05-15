tests:

include ../make.settings

lib: log.cpp debug.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c log.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c debug.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c rand.cpp

clean:
	$(RM) *.$(OBJEXT)
