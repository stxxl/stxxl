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

TEST_BINARIES		 = $(TESTS) $(TESTS-yes) $(TESTS-yesyes)
SKIPPED_TEST_BINARIES	 = $(TESTS-) $(TESTS-no) $(TESTS-yesno) $(TESTS-noyes) $(TESTS-nono)

tests: $(TEST_BINARIES:%=%.$(bin))

lib: $(LIB_SRC:.cpp=.$(lo))

clean::
	$(RM) *.$(lo) *.$o
	$(RM) *.$(lo:.o=).d *.$(o:.o=).d *.dT
	$(RM) $(TEST_BINARIES:=.$(bin))
	$(RM) $(SKIPPED_TEST_BINARIES:=.$(bin))

-include *.d


# Work around compiler bugs:
compiler_version	:= $(shell $(COMPILER) -v 2>&1 | tr ' ' '_')
bitness			:= $(shell file ../common/version.$(lo) 2>/dev/null)
#debug_override_opt	?= $(warning "DBGOO: $1")
debug_override_opt	?=
# usage: e.g. $(call needs_override,gcc_version_4.2,32-bit,3,[-g|any|none])
needs_override		?= $(call debug_override_opt,$1%$(compiler_version);$2%$(bitness);$3%$(OPT_LEVEL);$4%$(DEBUG))\
			   $(and $(findstring $1,$(compiler_version)),\
				$(or $(filter any,$2),$(filter $2,$(bitness))),\
				$(filter $3,$(OPT_LEVEL)),\
				$(or $(filter any,$4),$(if $(filter none,$4),$(if $(DEBUG),,empty)),$(filter $4,$(DEBUG))))
# usage: $(call apply_override yes|$(EMPTY),to,target)
apply_override		?= $(call debug_override_opt,enable=$1;to=$2;target=$3)\
			   $(if $(strip $1),$3: OPT_LEVEL=$2,)
# usage: $(call reduce_optimization,from,to,target,compiler,bits,debug[,type])
reduce_optimization	?= $(call apply_override,$(call needs_override,$4,$5,$1,$6),$2,$3.$($(or $(strip $7),bin)))


.SECONDARY:

.PHONY: tests lib clean
