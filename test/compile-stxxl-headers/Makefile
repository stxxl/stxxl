############################################################################
#  test/compile-stxxl-headers/Makefile
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

INSTANCE	?= stxxl
INSTANCE	?= pmstxxl
INSTANCE	?= mcstxxl

TOPDIR		?= ../..
include $(TOPDIR)/make.settings
-include $(TOPDIR)/$(INSTANCE).mk

CXX		 = $(STXXL_CXX)
OPT		 = -O3
WARNINGS	 = $(STXXL_WARNFLAGS)
DEBUG		 = $(STXXL_DEBUGFLAGS)

CPPFLAGS	+= $(STXXL_CPPFLAGS)
CPPFLAGS	+= $(OPT) $(DEBUG) $(WARNINGS)
CPPFLAGS	+= -MD

STXXL_MAIN_HEADER	+= stxxl.h
STXXL_HEADERS		+= all
STXXL_HEADERS		+= vector stack queue priority_queue deque map
STXXL_HEADERS		+= algorithm scan sort ksort stable_ksort random_shuffle
STXXL_HEADERS		+= io mng stream
STXXL_HEADERS		+= random timer mallocstats aligned_alloc types
STXXL_BITS		+= $(HEADER_FILES_BITS)
STXXL_BITS		+= $(HEADER_FILES_COMMON:%=common/%)
STXXL_BITS		+= $(HEADER_FILES_IO:%=io/%)
STXXL_BITS		+= $(HEADER_FILES_MNG:%=mng/%)
STXXL_BITS		+= $(HEADER_FILES_CONTAINERS:%=containers/%)
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

test-header-%.th.cpp::
	echo '#include "$(subst ~,/,$*)"' > $@

%.$(INSTANCE).ii: %.th.cpp
	$(CXX) $(CPPFLAGS) -E $< -o $@

%.$(INSTANCE).o: %.th.cpp $(wildcard ../../$(INSTANCE).mk)
	$(COMPILE.cc) $(OUTPUT_OPTION) $<

clean:
	$(RM) *.ii *.o *.d *.dT
	$(RM) test-header-*.cpp


.SECONDARY:

-include *.d
