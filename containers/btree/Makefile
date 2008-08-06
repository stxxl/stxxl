include ../Makefile.common
include ../../make.settings

tests: $(TESTS_BTREE)

# DEPENDENCIES = $(DEPENDENCIES)  btree.h leaf.h iterator.h iterator_map.h node_cache.h

STXXL_COMPILER_OPTIONS = $(STXXL_COMPILER_OPTIONS) -DSTXXL_VERBOSE_LEVEL=0

test_const_scan: test_const_scan.$(EXEEXT)
test_const_scan.$(EXEEXT): $(DEPENDENCIES) test_const_scan.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_const_scan.cpp
	$(LINKER) test_const_scan.$(OBJEXT) $(OUT)test_const_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_erase: test_corr_insert_erase.$(EXEEXT)
test_corr_insert_erase.$(EXEEXT): $(DEPENDENCIES) test_corr_insert_erase.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_erase.cpp
	$(LINKER) test_corr_insert_erase.$(OBJEXT) $(OUT)test_corr_insert_erase.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_find: test_corr_insert_find.$(EXEEXT)
test_corr_insert_find.$(EXEEXT): $(DEPENDENCIES) test_corr_insert_find.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_find.cpp
	$(LINKER) test_corr_insert_find.$(OBJEXT) $(OUT)test_corr_insert_find.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_corr_insert_scan: test_corr_insert_scan.$(EXEEXT)
test_corr_insert_scan.$(EXEEXT): $(DEPENDENCIES) test_corr_insert_scan.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_corr_insert_scan.cpp
	$(LINKER) test_corr_insert_scan.$(OBJEXT) $(OUT)test_corr_insert_scan.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_btree: test_btree.$(EXEEXT)
test_btree.$(EXEEXT): $(DEPENDENCIES) test_btree.cpp
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_btree.cpp
	$(LINKER) test_btree.$(OBJEXT) $(OUT)test_btree.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

clean:
	$(RM) *.$(OBJEXT)

	
