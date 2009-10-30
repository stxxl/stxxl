TOPDIR	?= ..
include Makefile.common

TESTS-yes			+= $(TESTS_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu


tools: $(UTIL_BINARIES:=.$(bin))

clean::
	$(RM) $(UTIL_BINARIES:=.$(bin))

obj-test_manyunits	 = test_manyunits.$o test_manyunits2.$o

test_manyunits.$(bin): $(obj-test_manyunits) $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $(obj-test_manyunits))

ifneq (,$(wildcard .svn))
version.$o: STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_SVN_DEFS
endif

ifneq (,$(wildcard version.defs))
version.$o: STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_DEFS
endif
