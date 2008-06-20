############################################################################
#  Makefile.subdir.gnu
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2007 Andreas Beckmann <beckmann@mpi-inf.mpg.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################


include $(TOPDIR)/make.settings

TEST_BINARIES	 = $(foreach t, $(TESTS), $t.$(bin))

tests: $(TEST_BINARIES)

lib: $(LIB_SRC:.cpp=.$o)

clean::
	$(RM) *.$o
	$(RM) *.$d *.dT
	$(RM) $(TEST_BINARIES)

-include *.d

.SECONDARY:

.PHONY: tests lib clean
