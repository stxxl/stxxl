# $Id$

INSTANCE	?= stxxl
INSTANCE	?= mcstxxl

include ../../make.settings
-include ../../$(INSTANCE).mk

CXX		 = $(STXXL_CXX)
OPT		 = -O3
WARNINGS	 = -W -Wall

CPPFLAGS	+= $(STXXL_CPPFLAGS)
CPPFLAGS	+= $(OPT) $(WARNINGS)
CPPFLAGS	+= -MD

STXXL_MAIN_HEADER	+= stxxl.h
STXXL_HEADERS		+= all
STXXL_HEADERS		+= vector stack queue priority_queue deque map
STXXL_HEADERS		+= algorithm scan sort ksort stable_ksort
STXXL_HEADERS		+= io mng stream
STXXL_HEADERS		+= random timer mallocstats
STXXL_BITS		+= $(HEADER_FILES_BITS)
STXXL_BITS		+= $(HEADER_FILES_COMMON:%=common/%)
STXXL_BITS		+= $(HEADER_FILES_IO:%=io/%)
STXXL_BITS		+= $(HEADER_FILES_MNG:%=mng/%)
STXXL_BITS		+= $(HEADER_FILES_CONTAINER:%=containers/%)
STXXL_BITS		+= $(HEADER_FILES_CONTAINERS_BTREE:%=containers/btree/%)
STXXL_BITS		+= $(HEADER_FILES_ALGO:%=algo/%)
STXXL_BITS		+= $(HEADER_FILES_STREAM:%=stream/%)
STXXL_BITS		+= $(HEADER_FILES_UTILS:%=utils/%)
HEADERS			 = $(STXXL_MAIN_HEADER) $(STXXL_HEADERS:%=stxxl/%) $(STXXL_BITS:%=stxxl/bits/%)
HEADERS_ESCAPED		 = $(subst /,~,$(HEADERS))


#all: preprocess-test
all: compile-test

preprocess-test: $(HEADERS_ESCAPED:%=test-header-%.$(INSTANCE).ii)
compile-test: $(HEADERS_ESCAPED:%=test-header-%.$(INSTANCE).o)

test-header-%.cpp::
	echo '#include "$(subst ~,/,$*)"' > $@

%.$(INSTANCE).ii: %.cpp
	$(CXX) $(CPPFLAGS) -E $< -o $@

%.$(INSTANCE).o: %.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

clean:
	$(RM) *.ii *.o *.d *.dT
	$(RM) test-header-*.cpp


.SECONDARY:

-include *.d
