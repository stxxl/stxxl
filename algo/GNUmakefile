TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_LONG_COMPILE		?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-$(ENABLE_LONG_COMPILE)	+= $(TESTS_LONG_COMPILE)

include $(TOPDIR)/Makefile.subdir.gnu


test_ksort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1 -DSTXXL_CHECK_ORDER_IN_SORTS
test_sort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_bad_cmp.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_random_shuffle.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_stable_ksort_all_parameters.$o:	CPPFLAGS += -DSTXXL_VERBOSE_STABLE_KSORT=STXXL_VERBOSE0 -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_CHECK_ORDER_IN_SORTS


ifeq ($(strip $(USE_PMODE)),yes)
# ICPC 12.0: internal error: backend signals (issue 613286)
$(call reduce_optimization,3,0,test_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_ksort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stable_ksort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_bad_cmp,Version_12.0,any,any)
$(call reduce_optimization,3,0,sort_file,Version_12.0,any,any)
$(call reduce_optimization,3,0,copy_and_sort_file,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_parallel_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stable_ksort_all_parameters,Version_12.0,any,any)
endif

ifeq ($(strip $(USE_MCSTL)),yes)
# ICPC 12.0: internal error: backend signals (issue 613286)
$(call reduce_optimization,3,0,test_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_ksort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stable_ksort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_bad_cmp,Version_12.0,any,any)
$(call reduce_optimization,3,0,sort_file,Version_12.0,any,any)
$(call reduce_optimization,3,0,copy_and_sort_file,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_parallel_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stable_ksort_all_parameters,Version_12.0,any,any)

# ICPC 11.0: internal error: 0_1374 (issue 539748)
$(call reduce_optimization,3,0,test_sort,Version_11.0,any,any)
$(call reduce_optimization,3,0,test_ksort,Version_11.0,any,any)
$(call reduce_optimization,3,0,sort_file,Version_11.0,any,any)
$(call reduce_optimization,3,0,test_parallel_sort,Version_11.0,any,any)

# ICPC 10.0: internal error: backend signals (issue 466173)
$(call reduce_optimization,3,1,async_schedule,Version_10.0,32-bit,any,o)
$(call reduce_optimization,3,1,test_scan,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_sort,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_ksort,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_stable_ksort,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,sort_file,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_random_shuffle,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_parallel_sort,Version_10.0,32-bit,any)
endif
