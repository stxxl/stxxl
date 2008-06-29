TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_LONG_COMPILE		?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-$(ENABLE_LONG_COMPILE)	+= $(TESTS_LONG_COMPILE)

include $(TOPDIR)/Makefile.subdir.gnu


obj-test_manyunits	 = test_manyunits1.$o test_manyunits2.$o

test_manyunits.$(bin): $(obj-test_manyunits) $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $(obj-test_manyunits))

test_ksort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1 -DSTXXL_CHECK_ORDER_IN_SORTS
test_sort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_random_shuffle.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_stable_ksort_all_parameters.$o:	CPPFLAGS += -DSTXXL_VERBOSE_STABLE_KSORT=STXXL_VERBOSE0 -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_CHECK_ORDER_IN_SORTS
