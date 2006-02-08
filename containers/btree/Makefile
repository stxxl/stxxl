all: test_leaf 

include ../../make.settings



test_leaf: $(DEPENDENCIES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_leaf.cpp
	$(LINKER) test_leaf.$(OBJEXT) $(OUT)test_leaf.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) test_leaf.$(EXEEXT)

	
