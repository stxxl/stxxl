include Makefile.common

TESTS		+= $(TESTS_EXTRA)
#TESTS		+= $(TESTS_UNITTEST)

include ../Makefile.subdir.gnu


unittest.$o:		CPPFLAGS += $(CPPUNIT_COMPILER_OPTIONS)
unittest.$(bin):	STXXL_LINKER_OPTIONS += $(CPPUNIT_LINKER_OPTIONS)
