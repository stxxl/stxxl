/***************************************************************************
 *  utils/off_t_size.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <sys/types.h>
#include <iostream>

int main()
{
    std::cout << "Size of 'off_t' type is " << sizeof(off_t) * 8 << " bits." << std::endl;
}
