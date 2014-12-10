/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_equally_split.h
 *  @brief Function to split a sequence into parts of almost equal size. */

#ifndef _MCSTL_EQUALLY_SPLIT_H
#define _MCSTL_EQUALLY_SPLIT_H 1

namespace mcstl
{

/** @brief Split a sequence into parts of almost equal size.
 * 
 *  The resulting sequence s of length p+1 contains the splitting positions when 
 *  splitting the range [0,n) into parts of almost equal size (plus minus 1). 
 *  The first entry is 0, the last one n. There may result empty parts.
 *  @param n Number of elements
 *  @param p Number of parts
 *  @param s Splitters 
 *  @returns End of splitter sequence, i. e. @c s+p+1 */
template<typename DiffType, typename DiffTypeOutputIterator>
DiffTypeOutputIterator equally_split(DiffType n, thread_index_t p, DiffTypeOutputIterator s)
{
	DiffType chunk_length = n / p, split = n % p, start = 0;
	for(int i = 0; i < p; i++)
	{
		*s++ = start;
		start += ((DiffType)i < split) ? (chunk_length + 1) : chunk_length;
	}
	*s++ = n;
	
	return s;
}

}

#endif
