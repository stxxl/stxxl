TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_BDB			?= no
ENABLE_LEDASM			?= no
ENABLE_TPIE			?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-yes			+= $(TESTS_BTREE)
TESTS-$(ENABLE_BDB)$(ENABLE_TPIE)	+= $(TESTS_BDB)
TESTS-$(ENABLE_LEDASM)		+= $(TESTS_LEDASM)
TESTS-$(ENABLE_TPIE)		+= $(TESTS_TPIE)

include $(TOPDIR)/Makefile.subdir.gnu


#stack_benchmark.$o:	CPPFLAGS += -DNDEBUG
#pq_benchmark.$o:	CPPFLAGS += -DNDEBUG
test_many_stacks.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_ext_merger.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=2
test_ext_merger2.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=3
bench_pqueue.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1


$(TESTS_BDB:=.$(bin)) $(TESTS_LEDASM:=.$(bin)) $(TESTS_TPIE:=.$(bin)):
	$(MAKE) -f Makefile $(@:.$(bin)=) TOPDIR=$(TOPDIR)


clean::
	$(RM) btree/*.$o btree/*.$d btree/*.dT

-include btree/*.d
