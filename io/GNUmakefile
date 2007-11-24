TOPDIR	?= ..
include Makefile.common

TESTS		+= $(TESTS_EXTRA)
#TESTS		+= $(TESTS_UNITTEST)

include $(TOPDIR)/Makefile.subdir.gnu


# requires libcppunit-dev to be installed
unittest.$o:		CPPFLAGS += $(CPPUNIT_COMPILER_OPTIONS)
unittest.$(bin):	STXXL_LINKER_OPTIONS += $(CPPUNIT_LINKER_OPTIONS)
