TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_BDB			?= no
ENABLE_LEDASM			?= no
ENABLE_TPIE			?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-yes			+= $(TESTS_BTREE:%=btree/%)
TESTS-$(ENABLE_BDB)$(ENABLE_TPIE)	+= $(TESTS_BDB)
TESTS-$(ENABLE_LEDASM)		+= $(TESTS_LEDASM)
TESTS-$(ENABLE_TPIE)		+= $(TESTS_TPIE)

include $(TOPDIR)/Makefile.subdir.gnu


CPPFLAGS_BDB			?=
LIBS_BDB			?= -ldb_cxx

TPIE_ROOT			?= /usr/local/tpie
CPPFLAGS_TPIE			?= -I$(TPIE_ROOT)/include -DHAVE_CONFIG_H -Wno-error
LIBS_TPIE			?= -L$(TPIE_ROOT)/lib -ltpie


#stack_benchmark.$o:	CPPFLAGS += -DNDEBUG
#pq_benchmark.$o:	CPPFLAGS += -DNDEBUG
test_many_stacks.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_ext_merger.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=2
test_ext_merger2.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=3
bench_pqueue.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1


$(TESTS_LEDASM:=.$(bin)): Makefile
	$(MAKE) -f Makefile $(@:.$(bin)=) TOPDIR=$(TOPDIR)

$(TESTS_BDB:=.$(bin)):	CPPFLAGS += $(CPPFLAGS_TPIE) $(CPPFLAGS_BDB)
$(TESTS_BDB:=.$(bin)):	STXXL_LINKER_OPTIONS += $(LIBS_TPIE) $(LIBS_BDB)
$(TESTS_TPIE:=.$(bin)):	CPPFLAGS += $(CPPFLAGS_TPIE)
$(TESTS_TPIE:=.$(bin)):	STXXL_LINKER_OPTIONS += $(LIBS_TPIE)

# Work around PR33361: internal compiler error: in find_outermost_region_in_block, at tree-cfg.c:4803
# when using g++-4.2 -fopenmp -O3. Fixed in g++-4.3, wontfix in g++-4.2.
$(if $(findstring -O3,$(OPT)),$(if $(findstring 4.2,$(COMPILER)),berkeley_db_benchmark.mcstxxl.bin: OPT=-O0))
$(if $(findstring -O3,$(OPT)),$(if $(findstring 4.2,$(COMPILER)),btree/test_corr_insert_erase.mcstxxl.bin: OPT=-O2))

clean::
	$(RM) btree/*.$o btree/*.$d btree/*.dT

-include btree/*.d
