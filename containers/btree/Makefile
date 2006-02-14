all: test_btree

include ../../make.settings

DEPENDENCIES +=  btree.h leaf.h iterator.h iterator_map.h node_cache.h


test_btree: $(DEPENDENCIES) test_btree.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_btree.cpp
	$(LINKER) test_btree.$(OBJEXT) $(OUT)test_btree.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_leaf: $(DEPENDENCIES) test_leaf.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_leaf.cpp
	$(LINKER) test_leaf.$(OBJEXT) $(OUT)test_leaf.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) test_leaf.$(EXEEXT)

	
