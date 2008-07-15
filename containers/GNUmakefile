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

ifeq ($(strip $(USE_MCSTL)),yes)
# Work around compiler bugs:
compiler_version	:= $(shell $(COMPILER) -v 2>&1 | tr ' ' '_')
bitness			:= $(shell file ../common/stxxl_info.$(bin) 2>/dev/null)
# usage: e.g. $(call needs_override,gcc_version_4.2,32-bit,3,[-g|any|none])
needs_override		?= $(and $(findstring $1,$(compiler_version)),\
				$(filter $2,$(bitness)),\
				$(filter $3,$(OPT_LEVEL)),\
				$(or $(filter any,$4),$(if $(filter none,$4),$(if $(DEBUG),,empty)),$(filter $4,$(DEBUG))))
# usage: $(call reduce_optimization,from,to,target,compiler,bits,debug)
reduce_optimization	?= $(if $(call needs_override,$4,$5,$1,$6),$3.$(bin): OPT_LEVEL=$2)

# PR33361: internal compiler error: in find_outermost_region_in_block, at tree-cfg.c:4803
# when using g++-4.2 -fopenmp -O3. Fixed in g++-4.3, wontfix in g++-4.2.
$(call reduce_optimization,3,1,test_map,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,0,test_map,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,btree/test_btree,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,0,btree/test_corr_insert_scan,gcc_version_4.2,64-bit,-g)
$(call reduce_optimization,3,2,btree/test_corr_insert_scan,gcc_version_4.2,64-bit,none)
endif

clean::
	$(RM) btree/*.$o btree/*.$d btree/*.dT

-include btree/*.d
