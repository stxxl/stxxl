TOPDIR	?= ..
include Makefile.common

TESTS-yes			+= $(TESTS_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu


test_stream1.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_sorted_runs.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_push_sort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_stream.$o:			CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_materialize.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_VERBOSE_MATERIALIZE=STXXL_VERBOSE0


ifeq ($(strip $(USE_MCSTL)),yes)
# ICPC 10.0: internal error: backend signals (issue 466173)
$(call reduce_optimization,3,1,test_stream,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_stream1,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_sorted_runs,Version_10.0,32-bit,any)
$(call reduce_optimization,3,0,test_push_sort,Version_10.0,32-bit,any)
endif
