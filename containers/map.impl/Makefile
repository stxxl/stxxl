STXXL_ROOT = ../../
include $(STXXL_ROOT)/compiler.make
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
# test for speed
# *******************************************************************

speed_test_map_erase: speed_test_map_erase.o
	$(GCC) speed_test_map_erase.o -o speed_test_map_erase $(STXXL_LIB)

speed_test_map_erase.o: speed_test_map_erase.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(GCC) -c speed_test_map_erase.cpp -o speed_test_map_erase.o

speed_test_map_insert: speed_test_map_insert.o
	$(GCC) speed_test_map_insert.o -o speed_test_map_insert $(STXXL_LIB)

speed_test_map_insert.o: speed_test_map_insert.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(GCC) -c speed_test_map_insert.cpp -o speed_test_map_insert.o

speed_test_map_search: speed_test_map_search.o
	$(GCC) speed_test_map_search.o -o speed_test_map_search $(STXXL_LIB)

speed_test_map_search.o: speed_test_map_search.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(GCC) -c speed_test_map_search.cpp -o speed_test_map_search.o

speed_test_map_iterate: speed_test_map_iterate.o
	$(GCC) speed_test_map_iterate.o -o speed_test_map_iterate $(STXXL_LIB)

speed_test_map_iterate.o: speed_test_map_iterate.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(GCC) -c speed_test_map_iterate.cpp -o speed_test_map_iterate.o



# *******************************************************************
# test for corectnes
# *******************************************************************

test_map_random: test_map_random.o
	$(DGCC) test_map_random.o -o test_map_random $(STXXL_LIB)

test_map_erase: test_map_erase.o
	$(DGCC) test_map_erase.o -o test_map_erase $(STXXL_LIB)

test_map_search: test_map_search.o
	$(DGCC) test_map_search.o -o test_map_search $(STXXL_LIB)

test_map_iterator: test_map_iterator.o
	$(DGCC) test_map_iterator.o -o test_map_iterator $(STXXL_LIB)

test_map_insert: test_map_insert.o
	$(DGCC) test_map_insert.o -o test_map_insert $(STXXL_LIB)

#-lncurses

test.o: test.cpp $(MAP_FILES)
	$(DGCC) -c test.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_random.o: test_map_random.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(DGCC) -c test_map_random.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_erase.o: test_map_erase.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(DGCC) -c test_map_erase.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_iterator.o: test_map_iterator.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(DGCC) -c test_map_iterator.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_insert.o: test_map_insert.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(DGCC) -c test_map_insert.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6

test_map_search.o: test_map_search.cpp $(MAP_FILES) $(MAP_TEST_FILES)
	$(DGCC) -c test_map_search.cpp -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_DEBUG_ON -DSTXXL_BTREE_STATISTIC -DSTXXL_MAP_MAX_COUNT=6


clean:
	rm -f *~ *.o speed_test_map_erase speed_test_map_insert speed_test_map_iterate \
	speed_test_map_search test_map_erase test_map_insert test_map_iterator \
	test_map_random test_map_search \
	test_btree test_btnode
