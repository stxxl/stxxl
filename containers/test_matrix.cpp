/***************************************************************************
 *  include/stxxl/bits/containers/test_matrix.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/containers/matrix.h>

int main()
{
	const int rank = 10000;
	stxxl::matrix<double> m(rank, rank);

	return 0;
}
