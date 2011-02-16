/***************************************************************************
 *  common/test_argument_helper.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2010-2011 Raoul Steffen <R-Steffen@gmx.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

// Thanks Daniel Russel, Stanford University
#include <Argument_helper.h>

#include <iostream>
#include <limits>

int main(int argc, char **argv)
{
    int int_v = 0;
    long long long_v = 0;
    unsigned int uint_v = 0;
    unsigned long long ulong_v = 0;

    dsr::Argument_helper ah;
    ah.new_named_int               ('i', "int", "INT                 ", "", int_v);
    ah.new_named_unsigned_int      ('u', "unsigned", "UNSIGNED INT   ", "", uint_v);
    ah.new_named_long_long         ('l', "long", "LONG LONG          ", "", long_v);
    ah.new_named_unsigned_long_long('c', "ulong", "UNSIGNED LONG LONG", "", ulong_v);

    ah.set_description("argument helper test");
    ah.set_author("Raoul Steffen, R-Steffen@gmx.de");
    ah.process(argc, argv);

    std::cout << "int is                " << int_v << std::endl;
    std::cout << "unsigned int is       " << uint_v << std::endl;
    std::cout << "long long is          " << long_v << std::endl;
    std::cout << "unsigned long long is " << ulong_v << std::endl;

    return 0;
}
