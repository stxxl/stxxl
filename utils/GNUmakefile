TOPDIR	?= ..
include Makefile.common

all: create

TESTS-yes			+= $(TESTS_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu


create: $(UTIL_BINARIES:=.$(bin))

createdisks.$o:		CPPFLAGS += -DNDEBUG

clean::
	$(RM) $(UTIL_BINARIES:=.$(bin))
