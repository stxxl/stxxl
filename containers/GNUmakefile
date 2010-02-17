TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_BTREE			?= yes
ENABLE_BDB			?= no
ENABLE_LEDASM			?= no
ENABLE_TPIE			?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-$(ENABLE_BTREE)		+= $(TESTS_MAP)
TESTS-$(ENABLE_BTREE)		+= $(TESTS_BTREE:%=btree/%)
TESTS-$(ENABLE_BDB)$(ENABLE_TPIE)	+= $(TESTS_BDB)
ifneq ($(strip $(USE_BOOST)),yes)
TESTS-$(ENABLE_LEDASM)		+= $(TESTS_LEDASM)
endif
TESTS-$(ENABLE_TPIE)		+= $(TESTS_TPIE)

include $(TOPDIR)/Makefile.subdir.gnu


CPPFLAGS_BDB			?=
LIBS_BDB			?= -ldb_cxx

TPIE_ROOT			?= /usr/local/tpie
TPIE_ROOT			?= $(HOME)/projects/tpie_091905
TPIE_ROOT			?= $(HOME)/work/lib/tpie
TPIE_ROOT			?= $(HOME)/work/lib/$(UNAME_M)/tpie
CPPFLAGS_TPIE			?= -I$(TPIE_ROOT)/include -DHAVE_CONFIG_H
LIBS_TPIE			?= -L$(TPIE_ROOT)/lib -ltpie

LEDA_COMPILER			?= g++-2.95
LEDA_FLAGS			?= -D_LINUX $(filter-out -openmp -fopenmp -D__MCSTL__ -include stxxl/bits/defines.h,$(STXXL_COMPILER_OPTIONS)) -Wno-error
LEDA_ROOT			?= /usr/local/LEDA
LEDA_SM_ROOT			?= /usr/local/LEDA-SM

LEDA_COMPILER			?= /usr/local/bin/g++-2.95.3
LEDA_FLAGS			?= -fpic -D_LINUX -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -O3
LEDA_ROOT			?= /software/LEDA/4.2.1
LEDA_SM_ROOT			?= /home/dementiev/projects/LEDA-SM-1.3

LEDA_SM_CPP_FLAGS		?= -I$(LEDA_ROOT)/incl -I$(LEDA_SM_ROOT)/incl $(LEDA_FLAGS)
LEDA_SM_LDFLAGS			?= -L$(LEDA_ROOT) -Wl,-R$(LEDA_ROOT) -L$(LEDA_SM_ROOT) #-L/usr/X11R6/lib
LEDA_SM_LDFLAGS			?= -L$(LEDA_ROOT) -L$(LEDA_SM_ROOT) -L/usr/X11R6/lib -Xlinker -rpath -Xlinker $(LEDA_ROOT) #-Xlinker -rpath -Xlinker $(LEDA_SM_ROOT)
LEDA_SM_LDLIBS			?= -lLEDA-SM -lL
LEDA_SM_LDLIBS			?= -lW -lP -lG -lL -lX11 -lm -lLEDA-SM


#stack_benchmark.$o:	CPPFLAGS += -DNDEBUG
#pq_benchmark.$o:	CPPFLAGS += -DNDEBUG
test_many_stacks.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1
test_ext_merger.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=2
test_ext_merger2.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=3
bench_pqueue.$o:	CPPFLAGS += -DSTXXL_VERBOSE_LEVEL=1


$(TESTS_BDB:=.$(bin)):	CPPFLAGS += $(CPPFLAGS_TPIE) $(CPPFLAGS_BDB)
$(TESTS_BDB:=.$(bin)):	STXXL_LINKER_OPTIONS += $(LIBS_TPIE) $(LIBS_BDB)
$(TESTS_TPIE:=.$(bin)):	CPPFLAGS += $(CPPFLAGS_TPIE)
$(TESTS_TPIE:=.$(bin)):	STXXL_LINKER_OPTIONS += $(LIBS_TPIE)

leda_sm_pq_benchmark.$(bin): leda_sm_pq_benchmark.cpp GNUmakefile
	$(LEDA_COMPILER) $(LEDA_SM_CPP_FLAGS) -o leda_sm_pq_benchmark.$(OBJEXT) -c leda_sm_pq_benchmark.cpp
	$(LEDA_COMPILER) -o leda_sm_pq_benchmark.$(EXEEXT) leda_sm_pq_benchmark.$(OBJEXT) $(LEDA_SM_LDFLAGS) $(LEDA_SM_LDLIBS)

leda_sm_stack_benchmark.$(bin): leda_sm_stack_benchmark.cpp GNUmakefile
	$(LEDA_COMPILER) $(LEDA_SM_CPP_FLAGS) -o leda_sm_stack_benchmark.$(OBJEXT) -c leda_sm_stack_benchmark.cpp
	$(LEDA_COMPILER) -o leda_sm_stack_benchmark.$(EXEEXT) leda_sm_stack_benchmark.$(OBJEXT) $(LEDA_SM_LDFLAGS) $(LEDA_SM_LDLIBS)


ifeq ($(strip $(USE_MCSTL)),yes)
# PR33361: internal compiler error: in find_outermost_region_in_block, at tree-cfg.c:4803
# when using g++-4.2 -fopenmp -O3. Fixed in g++-4.3, wontfix in g++-4.2.
ifneq ($(strip $(USE_BOOST)),yes)
$(call reduce_optimization,3,0,test_iterators,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,1,test_iterators,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,1,test_map,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,0,test_map,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,0,test_map_random,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,0,test_map_random,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,0,btree/test_btree,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,0,btree/test_btree,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,0,btree/test_const_scan,gcc_version_4.2,any,any)
else
$(call reduce_optimization,3,0,berkeley_db_benchmark,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,0,berkeley_db_benchmark,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,2,test_map,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,test_map,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,0,test_map_random,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,test_map_random,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,2,btree/test_btree,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,btree/test_btree,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,2,btree/test_corr_insert_scan,gcc_version_4.2,32-bit,any)
$(call reduce_optimization,3,2,btree/test_corr_insert_scan,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,btree/test_corr_insert_erase,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,1,btree/test_const_scan,gcc_version_4.2,64-bit,any)
$(call reduce_optimization,3,2,btree/test_const_scan,gcc_version_4.2,32-bit,any)
endif

# ICPC 11.0: internal error: 0_1374 (issue 539748)
$(call reduce_optimization,3,1,btree/test_btree,Version_11.0,any,any)
$(call reduce_optimization,3,1,btree/test_const_scan,Version_11.0,any,any)
$(call reduce_optimization,3,0,btree/test_corr_insert_erase,Version_11.0,any,any)
$(call reduce_optimization,3,0,btree/test_corr_insert_scan,Version_11.0,any,any)

# ICPC 10.0: internal error: backend signals (issue 466173)
$(call reduce_optimization,3,1,btree/test_corr_insert_erase,Version_10.0,32-bit,any)
$(call reduce_optimization,3,1,btree/test_corr_insert_scan,Version_10.0,32-bit,any)
endif

clean::
	$(RM) btree/*.$o btree/*.$d btree/*.dT

-include btree/*.d
