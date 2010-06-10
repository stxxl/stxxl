TOPDIR	?= ..
include Makefile.common

all: tools

TESTS-yes			+= $(TESTS_NON_MSVC)
UTIL_BINARIES			+= $(UTIL_BINARIES_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu


tools: $(UTIL_BINARIES:=.$(bin))

#createdisks.$o:		CPPFLAGS += -DNDEBUG

clean::
	$(RM) $(UTIL_BINARIES:=.$(bin))
