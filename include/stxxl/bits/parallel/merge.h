/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_merge.h
 *  @brief Parallel implementation of std::merge(). */

#ifndef _MCSTL_MERGE_H
#define _MCSTL_MERGE_H 1

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_algo.h>

namespace mcstl
{

/** @brief Merge routine being able to merge only the @c max_length smallest elements.
 *
 * The @c begin iterators are advanced accordingly, they might not reach @c end, in contrast to the usual variant.
 * @param begin1 Begin iterator of first sequence.
 * @param end1 End iterator of first sequence.
 * @param begin2 Begin iterator of second sequence.
 * @param end2 End iterator of second sequence.
 * @param target Target begin iterator.
 * @param max_length Maximum number of elements to merge.
 * @param comp Comparator.
 * @return Output end iterator. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename OutputIterator, typename DiffType, typename Comparator>
OutputIterator merge_advance_usual(RandomAccessIterator1& begin1, RandomAccessIterator1 end1, RandomAccessIterator2& begin2, RandomAccessIterator2 end2, OutputIterator target, DiffType max_length, Comparator comp)
{
	while(begin1 != end1 && begin2 != end2 && max_length > 0)
	{
		if(comp(*begin2, *begin1))//array1[i1] < array0[i0]
			*target++ = *begin2++;
		else
			*target++ = *begin1++;
		max_length--;
	}
	
	if(begin1 != end1)
	{
		target = std::copy(begin1, begin1 + max_length, target);
		begin1 += max_length;
	}
	else
	{
		target = std::copy(begin2, begin2 + max_length, target);
		begin2 += max_length;
	}
	return target;
}

/** @brief Merge routine being able to merge only the @c max_length smallest elements.
 *
 * The @c begin iterators are advanced accordingly, they might not reach @c end, in contrast to the usual variant.
 * Specially designed code should allow the compiler to generate conditional moves instead of branches.
 * @param begin1 Begin iterator of first sequence.
 * @param end1 End iterator of first sequence.
 * @param begin2 Begin iterator of second sequence.
 * @param end2 End iterator of second sequence.
 * @param target Target begin iterator.
 * @param max_length Maximum number of elements to merge.
 * @param comp Comparator.
 * @return Output end iterator. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename OutputIterator, typename DiffType, typename Comparator>
OutputIterator merge_advance_movc(RandomAccessIterator1& begin1, RandomAccessIterator1 end1, RandomAccessIterator2& begin2, RandomAccessIterator2 end2, OutputIterator target, DiffType max_length, Comparator comp)
{
	typedef typename std::iterator_traits<RandomAccessIterator1>::value_type ValueType1;
	typedef typename std::iterator_traits<RandomAccessIterator2>::value_type ValueType2;

#if MCSTL_ASSERTIONS
	assert(max_length >= 0);
#endif

	while(begin1 != end1 && begin2 != end2 && max_length > 0)
	{
		ValueType1 element1;
		ValueType2 element2;
		RandomAccessIterator1 next1;
		RandomAccessIterator2 next2;

		next1 = begin1 + 1;
		next2 = begin2 + 1;
		element1 = *begin1;
		element2 = *begin2;

		if(comp(element2, element1))
		{
			element1 = element2;
			begin2 = next2;
		}
		else
		{
			begin1 = next1;
		}

		*target = element1;
		
		target++;
		max_length--;
	}
	if(begin1 != end1)
	{
		target = std::copy(begin1, begin1 + max_length, target);
		begin1 += max_length;
	}
	else
	{
		target = std::copy(begin2, begin2 + max_length, target);
		begin2 += max_length;
	}
	return target;
}

/** @brief Merge routine being able to merge only the @c max_length smallest elements.
 *
 *  The @c begin iterators are advanced accordingly, they might not reach @c end, in contrast to the usual variant.
 *  Static switch on whether to use the conditional-move variant.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence.
 *  @param end2 End iterator of second sequence.
 *  @param target Target begin iterator.
 *  @param max_length Maximum number of elements to merge.
 *  @param comp Comparator.
 *  @return Output end iterator. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename OutputIterator, typename DiffType, typename Comparator>
inline OutputIterator merge_advance(RandomAccessIterator1& begin1, RandomAccessIterator1 end1, RandomAccessIterator2& begin2, RandomAccessIterator2 end2, OutputIterator target, DiffType max_length, Comparator comp)
{
	MCSTL_CALL(max_length)

	return merge_advance_movc(begin1, end1, begin2, end2, target, max_length, comp);
}

/** @brief Merge routine fallback to sequential in case the iterators of the two input sequences are of different type.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence.
 *  @param end2 End iterator of second sequence.
 *  @param target Target begin iterator.
 *  @param max_length Maximum number of elements to merge.
 *  @param comp Comparator.
 *  @return Output end iterator. */
template<typename RandomAccessIterator1, typename RandomAccessIterator2, typename RandomAccessIterator3, typename Comparator>
inline RandomAccessIterator3
parallel_merge_advance(RandomAccessIterator1& begin1, RandomAccessIterator1 end1,
		RandomAccessIterator2& begin2, RandomAccessIterator2 end2,	//different iterators, parallel implementation not available
		RandomAccessIterator3 target, 
		typename std::iterator_traits<RandomAccessIterator1>::difference_type max_length,
		Comparator comp)
{
	return merge_advance(begin1, end1, begin2, end2, target, max_length, comp);
}

/** @brief Parallel merge routine being able to merge only the @c max_length smallest elements.
 *
 *  The @c begin iterators are advanced accordingly, they might not reach @c end, in contrast to the usual variant.
 *  The functionality is projected onto parallel_multiway_merge.
 *  @param begin1 Begin iterator of first sequence.
 *  @param end1 End iterator of first sequence.
 *  @param begin2 Begin iterator of second sequence.
 *  @param end2 End iterator of second sequence.
 *  @param target Target begin iterator.
 *  @param max_length Maximum number of elements to merge.
 *  @param comp Comparator.
 *  @return Output end iterator. */
template<typename RandomAccessIterator1, typename RandomAccessIterator3, typename Comparator>
inline RandomAccessIterator3
parallel_merge_advance(	RandomAccessIterator1& begin1, RandomAccessIterator1 end1,
		RandomAccessIterator1& begin2, RandomAccessIterator1 end2,
		RandomAccessIterator3 target, 
		typename std::iterator_traits<RandomAccessIterator1>::difference_type max_length,
		Comparator comp)
{
	typedef typename std::iterator_traits<RandomAccessIterator1>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator1>::difference_type
	DiffType1 /* == DiffType2 */;
	typedef typename std::iterator_traits<RandomAccessIterator3>::difference_type
	DiffType3;
	
	std::pair<RandomAccessIterator1, RandomAccessIterator1> seqs[2] = { std::make_pair(begin1, end1), std::make_pair(begin2, end2) };
	RandomAccessIterator3 target_end = parallel_multiway_merge(seqs, seqs + 2, target, comp, max_length, true, false);
	
	return target_end;
}

}	//namespace mcstl

#endif
