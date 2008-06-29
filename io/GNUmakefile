TOPDIR	?= ..
include Makefile.common
-include $(TOPDIR)/make.settings.local

ENABLE_UNITTEST			?= no

TESTS-yes			+= $(TESTS_NON_MSVC)
TESTS-$(ENABLE_UNITTEST)	+= $(TESTS_UNITTEST)

include $(TOPDIR)/Makefile.subdir.gnu


# requires libcppunit-dev to be installed
unittest.$o:		CPPFLAGS += $(CPPUNIT_COMPILER_OPTIONS)
unittest.$(bin):	STXXL_LINKER_OPTIONS += $(CPPUNIT_LINKER_OPTIONS)
