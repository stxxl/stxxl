TOPDIR	?= ..
include Makefile.common

#TESTS		+= $(TESTS_BDB)
#TESTS		+= $(TESTS_LEDASM)
#TESTS		+= $(TESTS_TPIE)
TESTS		+= $(TESTS_EXTRA)
TESTS		+= $(TESTS_BTREE)

include $(TOPDIR)/Makefile.subdir.gnu


stack_benchmark.$o:	CPPFLAGS += -DNDEBUG
pq_benchmark.$o:	CPPFLAGS += -DNDEBUG
test_many_stacks.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_ext_merger.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=2
bench_pqueue.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1


$(TESTS_BDB:=.$(bin)) $(TESTS_LEDASM:=.$(bin)) $(TESTS_TPIE:=.$(bin)):
	$(MAKE) -f Makefile $(@:.$(bin)=)


clean::
	$(RM) btree/*.$o btree/*.$d btree/*.dT

-include btree/*.d
