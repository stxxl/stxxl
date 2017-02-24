/***************************************************************************
 *  include/stxxl/bits/common/cmdline.h
 *
 *  Part of the STXXL. See http://stxxl.org
 *
 *  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#ifndef STXXL_COMMON_CMDLINE_HEADER
#define STXXL_COMMON_CMDLINE_HEADER

#include <tlx/cmdline_parser.hpp>

namespace stxxl {

//! \addtogroup support
//! \{

using cmdline_parser = tlx::CmdlineParser;

//! \}

} // namespace stxxl

#endif // !STXXL_COMMON_CMDLINE_HEADER
