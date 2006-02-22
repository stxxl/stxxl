all: test_btree

include ../../make.settings

DEPENDENCIES +=  btree.h leaf.h iterator.h iterator_map.h node_cache.h

STXXL_COMPILER_OPTIONS+= -DSTXXL_VERBOSE_LEVEL=0 -O3

test_corr_insert_erase: $(DEPENDENCIES) test_corr_insert_erase.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_erase.cpp
	$(LINKER) test_corr_insert_erase.$(OBJEXT) $(OUT)test_corr_insert_erase.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_find: $(DEPENDENCIES) test_corr_insert_find.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_find.cpp
	$(LINKER) test_corr_insert_find.$(OBJEXT) $(OUT)test_corr_insert_find.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_scan: $(DEPENDENCIES) test_corr_insert_scan.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_scan.cpp
	$(LINKER) test_corr_insert_scan.$(OBJEXT) $(OUT)test_corr_insert_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_node_cache: $(DEPENDENCIES) test_node_cache.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_node_cache.cpp
	$(LINKER) test_node_cache.$(OBJEXT) $(OUT)test_node_cache.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_btree: $(DEPENDENCIES) test_btree.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_btree.cpp
	$(LINKER) test_btree.$(OBJEXT) $(OUT)test_btree.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_leaf: $(DEPENDENCIES) test_leaf.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_leaf.cpp
	$(LINKER) test_leaf.$(OBJEXT) $(OUT)test_leaf.$(EXEEXT) $(STXXL_LINKER_OPTIONS)


clean:
	$(RM) *.$(OBJEXT) test_leaf.$(EXEEXT)

	
