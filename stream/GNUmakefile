TOPDIR	?= ..
include Makefile.common

TESTS-yes			+= $(TESTS_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu


test_stream1.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_push_sort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0
test_stream.$o:			CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_materialize.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=0 -DSTXXL_VERBOSE_MATERIALIZE=STXXL_VERBOSE0


ifeq ($(strip $(USE_PMODE)),yes)
# ICPC 12.0: internal error: backend signals (issue 613286)
$(call reduce_optimization,3,0,test_stream,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stream1,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_sorted_runs,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_push_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_transpose,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_loop,Version_12.0,any,any)
endif

ifeq ($(strip $(USE_MCSTL)),yes)
# ICPC 12.0: internal error: backend signals (issue 613286)
$(call reduce_optimization,3,0,test_stream,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_stream1,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_sorted_runs,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_push_sort,Version_12.0,any,any)
$(call reduce_optimization,3,0,test_transpose,Version_12.0,any,any)

# ICPC 10.0: internal error: backend signals (issue 466173)
$(call reduce_optimization,3,1,test_stream,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_stream1,Version_10.0,32-bit,any)
$(call reduce_optimization,3,0,test_sorted_runs,Version_10.0,32-bit,any)
$(call reduce_optimization,3,0,test_push_sort,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,test_transpose,Version_10.0,32-bit,any)
endif
