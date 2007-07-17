include Makefile.common

TESTS		+= $(TESTS_EXTRA)

include ../Makefile.subdir.gnu

ifneq (,$(wildcard .svn))
version.$o: STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_SVN_DEFS
endif

ifneq (,$(wildcard version.defs))
version.$o: STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_DEFS
endif
