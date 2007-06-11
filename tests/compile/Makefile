# $Id$

include ../../make.settings
include ../../stxxl.mk

CXX		 = $(STXXL_CXX)
OPT		 = -O3
WARNINGS	 = -W -Wall

CPPFLAGS	+= $(STXXL_CPPFLAGS)
CPPFLAGS	+= $(OPT) $(WARNINGS)
CPPFLAGS	+= -MD

#include ../mcstxxl.mk
#OPT		 = -O2

STXXL_HEADERS		+= stxxl
STXXL_HEADERS		+= vector stack queue priority_queue deque map
STXXL_HEADERS		+= algorithm scan sort ksort
STXXL_HEADERS		+= stxxl.h
STXXL_HEADERS		+= $(HEADER_FILES_COMMON:%=common/%)
STXXL_HEADERS		+= $(HEADER_FILES_IO:%=io/%)
STXXL_HEADERS		+= $(HEADER_FILES_MNG:%=mng/%)
STXXL_HEADERS		+= $(HEADER_FILES_CONTAINER:%=containers/%)
STXXL_HEADERS		+= $(HEADER_FILES_CONTAINERS_BTREE:%=containers/btree/%)
STXXL_HEADERS		+= $(HEADER_FILES_ALGO:%=algo/%)
STXXL_HEADERS		+= $(HEADER_FILES_STREAM:%=stream/%)
STXXL_HEADERS		+= $(HEADER_FILES_UTILS:%=utils/%)
HEADERS			 = $(STXXL_HEADERS:%=stxxl/%)
HEADERS_ESCAPED		 = $(subst /,~,$(HEADERS))


#all: preprocess-test
all: compile-test

preprocess-test: $(HEADERS_ESCAPED:%=test-header-%.ii)
compile-test: $(HEADERS_ESCAPED:%=test-header-%.o)

test-header-%.cpp::
	echo '#include "$(subst ~,/,$*)"' > $@

%.ii: %.cpp
	$(CXX) $(CPPFLAGS) -E $< -o $@

%.o: %.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

clean:
	$(RM) *.ii *.o *.d *.dT
	$(RM) test-header-*.cpp


.SECONDARY:

-include *.d
