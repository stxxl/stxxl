
include ../../make.settings
DEPENDENCIES = $(COMMON_FILES) $(IO_LAYER_FILES) $(MNG_LAYER_FILES) $(CONTAINER_FILES)

# test_handlers.h \

MAP_FILES = \
Makefile \
bt_counter.h \
btnode.h \
btnode_iterator.h \
btnode_manager.h \
btree.h \
btree_completion_handler.h \
btree_iterator.h \
btree_queue.h \
btree_request.h \
map.h \
map_async.h \
map_impl.h \
map_iterator.h \
map_queue.h \
map_request.h \
smart_ptr.h \


MAP_TEST_FILES = \
map_test.h \
my_type.h \


map: test_map_erase test_map_insert test_map_search test_map_iterator test_map_random \
 speed_test_map_insert speed_test_map_search speed_test_map_erase speed_test_map_iterate


# *******************************************************************
# tests for speed
# *******************************************************************

speed_test_map_erase: speed_test_map_erase.$(OBJEXT)
	$(LINKER)  speed_test_map_erase.$(OBJEXT) $(OUT)speed_test_map_erase.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

speed_test_map_erase.$(OBJEXT): speed_test_map_erase.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c speed_test_map_erase.cpp

speed_test_map_insert: speed_test_map_insert.$(OBJEXT)
	$(LINKER)  speed_test_map_insert.$(OBJEXT) $(OUT)speed_test_map_insert.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

speed_test_map_insert.$(OBJEXT): speed_test_map_insert.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c speed_test_map_insert.cpp

speed_test_map_search: speed_test_map_search.$(OBJEXT)
	$(LINKER)  speed_test_map_search.$(OBJEXT) $(OUT)speed_test_map_search.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

speed_test_map_search.$(OBJEXT): speed_test_map_search.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c speed_test_map_search.cpp

speed_test_map_iterate: speed_test_map_iterate.$(OBJEXT)
	$(LINKER)  speed_test_map_iterate.$(OBJEXT) $(OUT)speed_test_map_iterate.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

speed_test_map_iterate.$(OBJEXT): speed_test_map_iterate.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c speed_test_map_iterate.cpp



# *******************************************************************
# tests for correctness
# *******************************************************************

test_map_random: test_map_random.$(OBJEXT)
	$(LINKER)  test_map_random.$(OBJEXT) $(OUT)test_map_random.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_erase: test_map_erase.$(OBJEXT)
	$(LINKER)  test_map_erase.$(OBJEXT) $(OUT)test_map_erase.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_search: test_map_search.$(OBJEXT)
	$(LINKER)  test_map_search.$(OBJEXT) $(OUT)test_map_search.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_iterator: test_map_iterator.$(OBJEXT)
	$(LINKER)  test_map_iterator.$(OBJEXT) $(OUT)test_map_iterator.$(EXEEXT) $(STXXL_LINKER_OPTIONS)

test_map_insert: test_map_insert.$(OBJEXT)
	$(LINKER)  test_map_insert.$(OBJEXT) $(OUT)test_map_insert.$(EXEEXT) $(STXXL_LINKER_OPTIONS)



test.$(OBJEXT): test.cpp $(MAP_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c test.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_random.$(OBJEXT): test_map_random.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c test_map_random.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_erase.$(OBJEXT): test_map_erase.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c test_map_erase.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_iterator.$(OBJEXT): test_map_iterator.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c test_map_iterator.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_insert.$(OBJEXT): test_map_insert.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS)  -c test_map_insert.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_search.$(OBJEXT): test_map_search.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -c test_map_search.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6


clean:
	$(RM) *.$(OBJEXT) speed_test_map_erase.$(EXEEXT)
	$(RM) speed_test_map_insert.$(EXEEXT) speed_test_map_iterate.$(EXEEXT)
	$(RM) speed_test_map_search.$(EXEEXT) test_map_erase.$(EXEEXT)
	$(RM) test_map_insert.$(EXEEXT) test_map_iterator.$(EXEEXT)
	$(RM) test_map_random.$(EXEEXT) test_map_search.$(EXEEXT)
	$(RM) test_btree.$(EXEEXT) test_btnode.$(EXEEXT)
