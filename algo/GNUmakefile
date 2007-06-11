include Makefile.common

#TESTS		+= $(TESTS_LONG_COMPILE)
TESTS		+= $(TESTS_EXTRA)

include ../Makefile.subdir.gnu


obj-test_manyunits	 = test_manyunits1.$o test_manyunits2.$o

test_manyunits.$(bin): $(obj-test_manyunits) $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $(obj-test_manyunits))

test_ksort1.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1 -DSTXXL_CHECK_ORDER_IN_SORTS
test_sort1.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_random_shuffle.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
