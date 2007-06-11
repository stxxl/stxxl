all: test_const_scan test_btree

include ../../make.settings

# DEPENDENCIES = $(DEPENDENCIES)  btree.h leaf.h iterator.h iterator_map.h node_cache.h

STXXL_COMPILER_OPTIONS += -DSTXXL_VERBOSE_LEVEL=0  -O3 -DNDEBUG 
STXXL_LINKER_OPTIONS +=   

test_const_scan: $(DEPENDENCIES) test_const_scan.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_const_scan.cpp
	$(LINKER) test_const_scan.$(OBJEXT) $(OUT)test_const_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_erase: $(DEPENDENCIES) test_corr_insert_erase.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_erase.cpp
	$(LINKER) test_corr_insert_erase.$(OBJEXT) $(OUT)test_corr_insert_erase.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_find: $(DEPENDENCIES) test_corr_insert_find.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_find.cpp
	$(LINKER) test_corr_insert_find.$(OBJEXT) $(OUT)test_corr_insert_find.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_scan: $(DEPENDENCIES) test_corr_insert_scan.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_scan.cpp
	$(LINKER) test_corr_insert_scan.$(OBJEXT) $(OUT)test_corr_insert_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_btree: $(DEPENDENCIES) test_btree.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_btree.cpp
	$(LINKER) test_btree.$(OBJEXT) $(OUT)test_btree.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT)

	
