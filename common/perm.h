// Stoled from Peter Sanders
// pieces of code for computing random permutations.
#ifndef PERM_HEADER
#define PERM_HEADER


/***************************************************************************
 *            perm.h
 *
 *  Sat Aug 24 23:53:29 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#error Header perm.h is depricated

#define register

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sys/types.h>
#include <time.h>


typedef int Element;

////////////////// uniform deviates ///////////////////////////////////////
// Quick and dirty generator from Numerical recipes
//typedef unsigned int Card32;

//#define rand32() (ran32State = 1664525 * ran32State + 1013904223)
//#define randUniform() ((double)rand32() * (0.5 / 0x80000000))

// return random int between 0 and k-1
//#define rand0K(k) ((int)(randUniform() * k))


//register Card32 ran32State = time (NULL);

///////////// Knuths algorithm ////////////////////////////////////////////
/*
   void
   knuthPerm (register Element * input, int n)
   {
        register double nDouble = (double) n;
        register Element temp;
        register Element *beyond = input + n - 1;
        register Element *other;

        for (; input < beyond; input++, nDouble--)
        {
                other = input + rand0K (nDouble);
                temp = *input;
 *input = *other;
 *other = temp;
        }
   }
 */
#endif
