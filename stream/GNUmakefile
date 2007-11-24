TOPDIR	?= ..
include Makefile.common

TESTS		+= $(TESTS_EXTRA)

include $(TOPDIR)/Makefile.subdir.gnu


test_stream1.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_sorted_runs.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_push_sort.$o:		CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_stream.$o:			CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
