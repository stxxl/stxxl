/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_multiseq_selection.h
 *  @brief Functions to find elements of a certain global rank in multiple sorted sequences.
 *  Also serves for splitting such sequence sets. */

#ifndef _MCSTL_MULTISEQ_SELECTION_H
#define _MCSTL_MULTISEQ_SELECTION_H 1

#include <vector>
#include <queue>

#include <mod_stl/stl_algo.h>

#include <bits/mcstl_sort.h>

namespace mcstl
{

/** @brief Compare a pair of types lexcigraphically, ascending. */
template<typename T1, typename T2, typename Comparator>
class lexicographic
{
	private:
	Comparator& comp;
		
	public:
	lexicographic(Comparator& _comp) : comp(_comp) { }

	inline bool operator()(const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2)
	{
		if(comp(p1.first, p2.first))
			return true;

		if(comp(p2.first, p1.first))
			return false;

		//firsts are equal

		return p1.second < p2.second;
	}
};

/** @brief Compare a pair of types lexcigraphically, descending. */
template<typename T1, typename T2, typename Comparator>
class lexicographic_rev
{
	private:
	Comparator& comp;
		
	public:
	lexicographic_rev(Comparator& _comp) : comp(_comp) { }

	inline bool operator()(const std::pair<T1, T2>& p1, const std::pair<T1, T2>& p2)
	{
		if(comp(p2.first, p1.first))
			return true;

		if(comp(p1.first, p2.first))
			return false;

		//firsts are equal

		return p2.second < p1.second;
	}
};

/** @brief Splits several sorted sequences at a certain global rank, resulting in a splitting point for each sequence. 
 *  The sequences are passed via a sequence of random-access iterator pairs, none of the sequences may be empty. 
 *  If there are several equal elements across the split, the ones on the left side will be chosen from sequences with smaller number.
 *  @param begin_seqs Begin of the sequence of iterator pairs.
 *  @param end_seqs End of the sequence of iterator pairs.
 *  @param rank The global rank to partition at.
 *  @param begin_offsets A random-access sequence begin where the result will be stored in. Each element of the sequence is an iterator that points to the first element on the greater part of the respective sequence.
 *  @param comp The ordering functor, defaults to std::less<T>. */
template<typename RanSeqs, typename RankType, typename RankIterator, typename Comparator>
void multiseq_partition(
	RanSeqs begin_seqs, RanSeqs end_seqs, 
	RankType rank,
	RankIterator begin_offsets, 
	Comparator comp = std::less<
		typename std::iterator_traits<typename std::iterator_traits<RanSeqs>::value_type::first_type>::value_type
		>()) //std::less<T>
{
	MCSTL_CALL(end_seqs - begin_seqs)

	typedef typename std::iterator_traits<RanSeqs>::value_type::first_type It;
	typedef typename std::iterator_traits<It>::difference_type DiffType;
	typedef typename std::iterator_traits<It>::value_type T;

	
	lexicographic<T, int, Comparator> lcomp(comp);
	lexicographic_rev<T, int, Comparator> lrcomp(comp);
		
	DiffType m = std::distance(begin_seqs, end_seqs), N = 0, nmax, n, r;	//number of sequences, number of elements in total (possibly including padding)
	
	for(int i = 0; i < m; i++)
		N += std::distance(begin_seqs[i].first, begin_seqs[i].second);
	
	if(rank == N)
	{
		for(int i = 0; i < m; i++)
			begin_offsets[i] = begin_seqs[i].second;	//very end
		//return m - 1;
	}
	
	assert(m != 0 && N != 0 && rank >= 0 && rank < N);

	DiffType* ns = new DiffType[m], * a = new DiffType[m], * b = new DiffType[m], l;
	
	ns[0] = std::distance(begin_seqs[0].first, begin_seqs[0].second);
	nmax = ns[0];
	for(int i = 0; i < m; i++)
	{
		ns[i] = std::distance(begin_seqs[i].first, begin_seqs[i].second);
		nmax = std::max(nmax, ns[i]);
	}
	
	r = log2(nmax) + 1;
	l = (1ULL << r) - 1;	//pad all lists to this length, at least as long as any ns[i], equliaty iff nmax = 2^k - 1
	
	N = l * m;	//from now on, including padding
	
	for(int i = 0; i < m; i++)
	{
		a[i] = 0;
		b[i] = l;
	}
	n = l / 2;

	// invariants:
	// 0 <= a[i] <= ns[i], 0 <= b[i] <= l
	
#define S(i) (begin_seqs[i].first)
	
	//initial partition
	
	std::vector<std::pair<T, int> > sample;
	
	for(int i = 0; i < m; i++)
		if(n < ns[i])	//sequence long enough
			sample.push_back(std::make_pair(S(i)[n], i));
	std::__mcstl_sequential_sort(sample.begin(), sample.end(), lcomp);
	for(int i = 0; i < m; i++)	//conceptual infinity
		if(n >= ns[i])	//sequence too short, conceptual infinity
			sample.push_back(std::make_pair(S(i)[0] /*dummy element*/, i));
		
	DiffType localrank = rank * m / N ;
	
	int j;
	for(j = 0; j < localrank && ((n + 1) <= ns[sample[j].second]); j++)
		a[sample[j].second] += n + 1;
	for(; j < m; j++)
		b[sample[j].second] -= n + 1;
	
	//further refinement
	
	while(n > 0)
	{
		n /= 2;
		
		int lmax_seq = -1;	//to avoid warning
		const T* lmax = NULL;	//impossible to avoid the warning?
		for(int i = 0; i < m; i++)
		{
			if(a[i] > 0)
			{
				if(!lmax)
				{
					lmax = &(S(i)[a[i] - 1]);
					lmax_seq = i;
				}
				else
				{
					if(!comp(S(i)[a[i] - 1], *lmax))	//max, favor rear sequences
					{
						lmax = &(S(i)[a[i] - 1]);
						lmax_seq = i;
					}
				}
			}
		}

		int i;
		for(i = 0; i < m; i++)
		{
			DiffType middle = (b[i] + a[i]) / 2;
			if(lmax && middle < ns[i] &&
				lcomp(std::make_pair(S(i)[middle], i), std::make_pair(*lmax, lmax_seq)))
				a[i] = std::min(a[i] + n + 1, ns[i]);
			else
				b[i] -= n + 1;
		}
		
		DiffType leftsize = 0, total = 0;
		for(int i = 0; i < m; i++)
		{
			leftsize += a[i] / (n + 1);
			total += l / (n + 1);
		}
		
		DiffType skew = static_cast<DiffType>(static_cast<uint64>(total) * rank / N - leftsize);
		
		if(skew > 0)
		{	//move to the left, find smallest
			std::priority_queue<std::pair<T, int>, std::vector<std::pair<T, int> >, lexicographic_rev<T, int, Comparator> > pq(lrcomp);
			
			for(int i = 0; i < m; i++)
				if(b[i] < ns[i])
					pq.push(std::make_pair(S(i)[b[i]], i));
			
			for(; skew != 0 && !pq.empty(); skew--)	
			{
				int source = pq.top().second;
				pq.pop();
				
				a[source] = std::min(a[source] + n + 1, ns[source]);
				b[source] += n + 1;
				
				if(b[source] < ns[source])
					pq.push(std::make_pair(S(source)[b[source]], source));
			}
		}
		else if(skew < 0)
		{	//move to the right, find greatest
			std::priority_queue<std::pair<T, int>, std::vector<std::pair<T, int> >, lexicographic<T, int, Comparator> > pq(lcomp);
			
			for(int i = 0; i < m; i++)
				if(a[i] > 0)
					pq.push(std::make_pair(S(i)[a[i] - 1], i));
			
			for(; skew != 0; skew++)
			{
				int source = pq.top().second;
				pq.pop();
				
				a[source] -= n + 1;
				b[source] -= n + 1;
				
				if(a[source] > 0)
					pq.push(std::make_pair(S(source)[a[source] - 1], source));
			}
		}
	}
	
	// postconditions 
	// a[i] == b[i] in most cases, except when a[i] has been clamped because of having reached the boundary
			
	//now return the result, calculate the offset
	
	//compare the keys on both edges of the border
	
	//maximum of left edge, minimum of right edge
	bool maxleftset = false, minrightset = false;
	T maxleft, minright;	//impossible to avoid the warning?
	for(int i = 0; i < m; i++)
	{
		if(a[i] > 0)
		{
			if(!maxleftset)
			{
				maxleft = S(i)[a[i] - 1];
				maxleftset = true;
			}
			else
			{
				if(!comp(S(i)[a[i] - 1], maxleft))	//max, favor rear sequences
					maxleft = S(i)[a[i] - 1];
			}
		}
		if(b[i] < ns[i])
		{
			if(!minrightset)
			{
				minright = S(i)[b[i]];
				minrightset = true;
			}
			else
			{
				if(comp(S(i)[b[i]], minright))	//min, favor fore sequences
					minright = S(i)[b[i]];
			}
		}
	}
	
	int seq = 0;
	for(int i = 0; i < m; i++)
		begin_offsets[i] = S(i) + a[i];
	
	delete[] ns;
	delete[] a;
	delete[] b;
}



/** @brief Selects the element at a certain global rank from several sorted sequences. 
 * 
 *  The sequences are passed via a sequence of random-access iterator pairs, none of the sequences may be empty.
 *  @param begin_seqs Begin of the sequence of iterator pairs.
 *  @param end_seqs End of the sequence of iterator pairs.
 *  @param rank The global rank to partition at.
 *  @param offset The rank of the selected element in the global subsequence of elements equal to the selected element. If the selected element is unique, this number is 0.
 *  @param comp The ordering functor, defaults to std::less. */
template<typename T, typename RanSeqs, typename RankType, typename Comparator>
T multiseq_selection(
	RanSeqs begin_seqs, RanSeqs end_seqs, 
	RankType rank,
	RankType& offset, 
	Comparator comp = std::less<T>())
{
	MCSTL_CALL(end_seqs - begin_seqs)

	typedef typename std::iterator_traits<RanSeqs>::value_type::first_type It;
	typedef typename std::iterator_traits<It>::difference_type DiffType;
	
	lexicographic<T, int, Comparator> lcomp(comp);
	lexicographic_rev<T, int, Comparator> lrcomp(comp);
		
	DiffType m = std::distance(begin_seqs, end_seqs), N = 0, nmax, n, r;	//number of sequences, number of elements in total (possibly including padding)
	
	for(int i = 0; i < m; i++)
		N += std::distance(begin_seqs[i].first, begin_seqs[i].second);
		
	if(m == 0 || N == 0 || rank < 0 || rank >= N)
		throw std::exception();	//result undefined when there is no data or rank is outside bounds
	
	DiffType* ns = new DiffType[m], * a = new DiffType[m], * b = new DiffType[m], l;
	
	ns[0] = std::distance(begin_seqs[0].first, begin_seqs[0].second);
	nmax = ns[0];
	for(int i = 0; i < m; i++)
	{
		ns[i] = std::distance(begin_seqs[i].first, begin_seqs[i].second);
		nmax = std::max(nmax, ns[i]);
	}
	
	r = log2(nmax) + 1;
	l = pow2(r) - 1;	//pad all lists to this length, at least as long as any ns[i], equliaty iff nmax = 2^k - 1
	
	N = l * m;	//from now on, including padding
	
	for(int i = 0; i < m; i++)
	{
		a[i] = 0;
		b[i] = l;
	}
	n = l / 2;

	// invariants:
	// 0 <= a[i] <= ns[i], 0 <= b[i] <= l
	
#define S(i) (begin_seqs[i].first)
	
	//initial partition
	
	std::vector<std::pair<T, int> > sample;
	
	for(int i = 0; i < m; i++)
		if(n < ns[i])
			sample.push_back(std::make_pair(S(i)[n], i));
	std::__mcstl_sequential_sort(sample.begin(), sample.end(), lcomp, sequential_tag());
	for(int i = 0; i < m; i++)	//conceptual infinity
		if(n >= ns[i])
			sample.push_back(std::make_pair(S(i)[0] /*dummy element*/, i));
		
	DiffType localrank = rank * m / N ;
	
	int j;
	for(j = 0; j < localrank && ((n + 1) <= ns[sample[j].second]); j++)
		a[sample[j].second] += n + 1;
	for(; j < m; j++)
		b[sample[j].second] -= n + 1;
	
	//further refinement
	
	while(n > 0)
	{
		n /= 2;
		
		const T* lmax = NULL;
		for(int i = 0; i < m; i++)
		{
			if(a[i] > 0)
			{
				if(!lmax)
				{
					lmax = &(S(i)[a[i] - 1]);
				}
				else
				{
					if(comp(*lmax, S(i)[a[i] - 1]))	//max
						lmax = &(S(i)[a[i] - 1]);
				}
			}
		}
		
		int i;
		for(i = 0; i < m; i++)
		{
			DiffType middle = (b[i] + a[i]) / 2;
			if(lmax && middle < ns[i] && comp(S(i)[middle], *lmax))
				a[i] = std::min(a[i] + n + 1, ns[i]);
			else
				b[i] -= n + 1;
		}
		
		DiffType leftsize = 0, total = 0;
		for(int i = 0; i < m; i++)
		{
			leftsize += a[i] / (n + 1);
			total += l / (n + 1);
		}
		
		DiffType skew = (unsigned long long)total * rank / N - leftsize;
		
		if(skew > 0)
		{	//move to the left, find smallest
			std::priority_queue<std::pair<T, int>, std::vector<std::pair<T, int> >, lexicographic_rev<T, int, Comparator> > pq(lrcomp);
			
			for(int i = 0; i < m; i++)
				if(b[i] < ns[i])
					pq.push(std::make_pair(S(i)[b[i]], i));
			
			for(; skew != 0 && !pq.empty(); skew--)	
			{
				int source = pq.top().second;
				pq.pop();
				
				a[source] = std::min(a[source] + n + 1, ns[source]);
				b[source] += n + 1;
				
				if(b[source] < ns[source])
					pq.push(std::make_pair(S(source)[b[source]], source));
			}
		}
		else if(skew < 0)
		{	//move to the right, find greatest
			std::priority_queue<std::pair<T, int>, std::vector<std::pair<T, int> >, lexicographic<T, int, Comparator> > pq(lcomp);
			
			for(int i = 0; i < m; i++)
				if(a[i] > 0)
					pq.push(std::make_pair(S(i)[a[i] - 1], i));
			
			for(; skew != 0; skew++)
			{
				int source = pq.top().second;
				pq.pop();
				
				a[source] -= n + 1;
				b[source] -= n + 1;
				
				if(a[source] > 0)
					pq.push(std::make_pair(S(source)[a[source] - 1], source));
			}
		}
	}
	
	// postconditions 
	// a[i] == b[i] in most cases, except when a[i] has been clamped because of having reached the boundary
			
	//now return the result, calculate the offset
	
	//compare the keys on both edges of the border
	
	//maximum of left edge, minimum of right edge
	bool maxleftset = false, minrightset = false;
	T maxleft, minright;	//impossible to avoid the warning?
	for(int i = 0; i < m; i++)
	{
		if(a[i] > 0)
		{
			if(!maxleftset)
			{
				maxleft = S(i)[a[i] - 1];
				maxleftset = true;
			}
			else
			{
				if(comp(maxleft, S(i)[a[i] - 1]))	//max
					maxleft = S(i)[a[i] - 1];
			}
		}
		if(b[i] < ns[i])
		{
			if(!minrightset)
			{
				minright = S(i)[b[i]];
				minrightset = true;
			}
			else
			{
				if(comp(S(i)[b[i]], minright))	//min
					minright = S(i)[b[i]];
			}
		}
	}
	
	//minright is the splitter, in any case
	
	if(!maxleftset || comp(minright, maxleft))
	{	//good luck, everything is split unambigiously
		offset = 0;
	}
	else
	{	//we have to calculate an offset
		offset = 0;
		
		for(int i = 0; i < m; i++)
		{
			DiffType lb = std::lower_bound(S(i), S(i) + ns[i], minright, comp) - S(i);
			offset += a[i] - lb;
		}
	}
	
	delete[] ns;
	delete[] a;
	delete[] b;
	
	return minright;
}

}

#undef S

#endif
