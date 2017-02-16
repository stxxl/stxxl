/***************************************************************************
 *  tools/extras/pq_param.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <cmath>
#include <iostream>
#include <stxxl/types>

int64_t D(int64_t m, int64_t k, int64_t MaxS, int64_t E, int64_t B)
{
    return (m * m / 4 - (MaxS * E / (k - m)) / B);
}

using std::cout;
using std::endl;

int main()
{
    int64_t IntM = 128 * 1024 * 1024;
    int64_t E = 4;
    int64_t B = 8 * 1024 * 1024;
    //int64_t Bstep = 128 * 1024;
    int64_t MaxS = (int64_t(128) * int64_t(1024 * 1024 * 1024)) / E;

    for ( ; B > 4096; B = B / 2)
    {
        int64_t m = 1;
        int64_t k = IntM / B;
        for ( ; m < k; ++m)
        {
            int64_t c = (k - m);
            //if( D(m,k,MaxS,E,B)>= 0 && c > 10)
            if ((k - m) * m * m * B / (E * 4) >= MaxS)
            {
                cout << (k - m) * (m) * (m * B / (E * 4)) << endl;
                cout << MaxS << endl;
                cout << "D: " << D(m, k, MaxS, E, B) << endl;
                cout << "B: " << B << endl;
                cout << "c: " << c << endl;
                cout << "k: " << k << endl;
                int64_t Ae = m / 2;
                int64_t Ae1 = Ae + int64_t(sqrt((double)D(m, k, MaxS, E, B)));
                int64_t Ae2 = Ae - int64_t(sqrt((double)D(m, k, MaxS, E, B)));
                int64_t x = c * B / E;
                int64_t N = x / 4096;
                cout << "Ae : " << Ae << endl;
                cout << "Ae1: " << Ae1 << endl;
                cout << "Ae2: " << Ae2 << endl;
                cout << "minAe :" << (MaxS / (x * Ae)) << endl;
                cout << "x  : " << x << endl;
                cout << "N  : " << N << endl;
                int64_t Cons = x * E + B * (m / 2) + MaxS * B / (x * (m / 2));
                cout << "COns : " << Cons << endl;
                return 0;
            }
        }
    }

    cout << "No valid parameter found" << endl;
}
