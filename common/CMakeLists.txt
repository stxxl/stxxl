###############################################################################
# CMakeLists.txt
#
# Cmake file for stxxl/common
#
# Part of the STXXL. See http://stxxl.sourceforge.net
#
# Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)
###############################################################################

stxxl_build_test(test_argument_helper)
stxxl_build_test(test_globals)
# TODO: test_manyunits and test_manyunits2 checks compilation with more than
# one object file.
stxxl_build_test(test_manyunits2)
stxxl_build_test(test_random)

stxxl_build_tool(stxxl_info)

# Left-overs from GNUmakefile
#
# ifneq (,$(wildcard .svn ../.git))
# version.$(lo): STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_SVN_DEFS
# endif
#
# ifneq (,$(wildcard version.defs))
# version.$(lo): STXXL_COMPILER_OPTIONS += -DHAVE_VERSION_DEFS
# endif
