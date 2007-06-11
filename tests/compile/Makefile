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

SKIP_HEADERS		 = ../make.settings
ALL_HEADERS		+= ../stxxl.h
ALL_HEADERS		+= $(COMMON_FILES)
ALL_HEADERS		+= ../common/utils_ledasm.h
ALL_HEADERS		+= $(IO_LAYER_FILES)
ALL_HEADERS		+= $(MNG_LAYER_FILES)
ALL_HEADERS		+= $(CONTAINER_FILES)
ALL_HEADERS		+= $(HEADER_FILES_CONTAINERS_BTREE:%=../containers/btree/%)
ALL_HEADERS		+= $(ALGO_FILES)
ALL_HEADERS		+= $(STREAM_FILES)
ALL_HEADERS		+= $(HEADER_FILES_UTILS:%=stxxl/utils/%)
USER_HEADERS		+= stxxl
USER_HEADERS		+= vector stack queue priority_queue deque map
USER_HEADERS		+= algorithm scan sort ksort
HEADERS			 = $(filter-out $(SKIP_HEADERS), $(USER_HEADERS:%=stxxl/%) $(ALL_HEADERS))
HEADERS_ESCAPED		 = $(subst ..,stxxl,$(subst /,~,$(HEADERS)))


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
