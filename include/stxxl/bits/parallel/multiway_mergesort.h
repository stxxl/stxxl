/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_multiway_mergesort.h
 *  @brief Parallel multiway mergesort. */

#ifndef _MCSTL_MERGESORT_H
#define _MCSTL_MERGESORT_H 1

#include <vector>

#include <bits/mcstl_basic_iterator.h>
#include <mod_stl/stl_algo.h>
#include <mcstl.h>
#include <bits/mcstl_multiway_merge.h>
#include <meta/mcstl_timing.h>

namespace mcstl
{

/** @brief Subsequence description. */
template<typename DiffType>
struct Piece
{
	/** @brief Begin of subsequence. */
	DiffType begin;
	/** @brief End of subsequence. */
	DiffType end;
};

/** @brief Data accessed by all threads. 
 * 
 *  PMWMS = parallel multiway mergesort */
template<typename RandomAccessIterator>
struct PMWMSSortingData
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	/** @brief Input begin. */
	RandomAccessIterator source;
	/** @brief Start indices, per thread. */
	DiffType* starts;
	/** @brief Temporary arrays for each thread. 
	 *
	 *  Indirection Allows using the temporary storage in different ways, without code duplication. 
	 *  @see MCSTL_MULTIWAY_MERGESORT_COPY_LAST */
	ValueType** temporaries;
#if MCSTL_MULTIWAY_MERGESORT_COPY_LAST
	/** @brief Storage in which to sort. */
	RandomAccessIterator* sorting_places;
	/** @brief Storage into which to merge. */
	ValueType** merging_places;
#else
	/** @brief Storage in which to sort. */
	ValueType** sorting_places;
	/** @brief Storage into which to merge. */
	RandomAccessIterator* merging_places;
#endif
	/** @brief Samples. */
	ValueType* samples;
	/** @brief Offsets to add to the found positions. */
	DiffType* offsets;
	/** @brief Pieces of data to merge @c [thread][sequence] */
	std::vector<Piece<DiffType> >* pieces;
};

/** @brief Thread local data for PMWMS. */
template<typename RandomAccessIterator>
struct PMWMSSorterPU
{
	/** @brief Total number of thread involved. */
	thread_index_t num_threads;
	/** @brief Number of owning thread. */
	thread_index_t iam;
	/** @brief Stable sorting desired. */
	bool stable;
	/** @brief Pointer to global data. */
	PMWMSSortingData<RandomAccessIterator>* sd;
};

/** @brief Select samples from a sequence.
 *  @param d Pointer to thread-local data. Result will be placed in @c d->ds->samples.
 *  @param num_samples Number of samples to select. */
template<typename RandomAccessIterator, typename DiffType>
inline void determine_samples(PMWMSSorterPU<RandomAccessIterator>* d, DiffType& num_samples)
{
	PMWMSSortingData<RandomAccessIterator>* sd = d->sd;
	
	num_samples = SETTINGS::sort_mwms_oversampling * d->num_threads - 1;

	DiffType es[num_samples + 2];
	equally_split(sd->starts[d->iam + 1] - sd->starts[d->iam], num_samples + 1, es);

	for(DiffType i = 0; i < num_samples; i++)
		sd->samples[d->iam * num_samples + i] = sd->source[sd->starts[d->iam] + es[i + 1]];
}

/** @brief PMWMS code executed by each thread.
 *  @param d Pointer to thread-local data.
 *  @param comp Comparator. */
template<typename RandomAccessIterator, typename Comparator>
inline void parallel_sort_mwms_pu(PMWMSSorterPU<RandomAccessIterator>* d, Comparator& comp)
{
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;

	Timing<inactive_tag> t;
	
	t.tic();

	PMWMSSortingData<RandomAccessIterator>* sd = d->sd;
	thread_index_t iam = d->iam;

	DiffType length_local = sd->starts[iam + 1] - sd->starts[iam];	//length of this thread's chunk, before merging

#if MCSTL_MULTIWAY_MERGESORT_COPY_LAST
	typedef RandomAccessIterator SortingPlacesIterator;
	sd->sorting_places[iam] = sd->source + sd->starts[iam];	//sort in input storage
#else
	typedef ValueType* SortingPlacesIterator;
	sd->sorting_places[iam] = sd->temporaries[iam] = static_cast<ValueType*>(::operator new(sizeof(ValueType) *(length_local + 1)));	//sort in temporary storage, leave space for sentinel
	std::uninitialized_copy(sd->source + sd->starts[iam], sd->source + sd->starts[iam] + length_local, sd->sorting_places[iam]);	//copy there
#endif

	//sort locally
	if(d->stable)
		std::__mcstl_sequential_stable_sort(sd->sorting_places[iam], sd->sorting_places[iam] + length_local, comp);
	else
		std::__mcstl_sequential_sort(sd->sorting_places[iam], sd->sorting_places[iam] + length_local, comp);

#if MCSTL_ASSERTIONS
	assert(is_sorted(sd->sorting_places[iam], sd->sorting_places[iam] + length_local, comp));
#endif

	//invariant: locally sorted subsequence in sd->sorting_places[iam], sd->sorting_places[iam] + length_local

	t.tic("local sort");

	if(SETTINGS::sort_splitting == SETTINGS::SAMPLING)
	{
		DiffType num_samples;
		determine_samples(d, num_samples);

		#pragma omp barrier

		t.tic("sample/wait");

		#pragma omp single
		std::__mcstl_sequential_sort(sd->samples, sd->samples + (num_samples * d->num_threads), comp);

		#pragma omp barrier

		for(int s = 0; s < d->num_threads; s++)
		{	//for each sequence
			if(num_samples * iam > 0)
				sd->pieces[iam][s].begin = 
					std::lower_bound(	sd->sorting_places[s],
								sd->sorting_places[s] + sd->starts[s + 1] - sd->starts[s],
								sd->samples[num_samples * iam],
								comp)
					- sd->sorting_places[s];
			else
				sd->pieces[iam][s].begin = 0;	//absolute beginning

			if((num_samples * (iam + 1)) < (num_samples * d->num_threads))
				sd->pieces[iam][s].end = 
					std::lower_bound(	sd->sorting_places[s], 
								sd->sorting_places[s] + sd->starts[s + 1] - sd->starts[s],
								sd->samples[num_samples * (iam + 1)],
								comp)
					- sd->sorting_places[s];
			else
				sd->pieces[iam][s].end = sd->starts[s + 1] - sd->starts[s];	//absolute end
		}

	}
	else if(SETTINGS::sort_splitting == SETTINGS::EXACT)
	{
		#pragma omp barrier

		t.tic("wait");

		std::vector<std::pair<SortingPlacesIterator, SortingPlacesIterator> > seqs(d->num_threads);
		for(int s = 0; s < d->num_threads; s++)
			seqs[s] = std::make_pair(sd->sorting_places[s], sd->sorting_places[s] + sd->starts[s + 1] - sd->starts[s]);

		std::vector<SortingPlacesIterator> offsets(d->num_threads);
		
		if(iam < d->num_threads - 1)	//if not last thread
			multiseq_partition(seqs.begin(), seqs.end(), sd->starts[iam + 1], offsets.begin(), comp);

		for(int seq = 0; seq < d->num_threads; seq++)
		{	//for each sequence
			if(iam < (d->num_threads - 1))
				sd->pieces[iam][seq].end = offsets[seq] - seqs[seq].first;
			else
				sd->pieces[iam][seq].end = sd->starts[seq + 1] - sd->starts[seq];	//absolute end of this sequence
		}
		
		#pragma omp barrier

		for(int seq = 0; seq < d->num_threads; seq++)
		{	//for each sequence
			if(iam > 0)
				sd->pieces[iam][seq].begin = sd->pieces[iam - 1][seq].end;
			else
				sd->pieces[iam][seq].begin = 0;		//absolute beginning
		}
	}
	
	t.tic("split");

	DiffType offset = 0, length_am = 0;	//offset from target begin, length after merging
	for(int s = 0; s < d->num_threads; s++)
	{
		length_am += sd->pieces[iam][s].end - sd->pieces[iam][s].begin;
		offset += sd->pieces[iam][s].begin;
	}
	
#if MCSTL_MULTIWAY_MERGESORT_COPY_LAST
	sd->merging_places[iam] = sd->temporaries[iam] = new ValueType[length_am];	//merge to temporary storage, uninitialized creation not possible since there is no multiway_merge calling the placement new instead of the assignment operator
#else
	sd->merging_places[iam] = sd->source + offset;	//merge directly to target
#endif
	std::vector<std::pair<SortingPlacesIterator, SortingPlacesIterator> > seqs(d->num_threads);
	
	for(int s = 0; s < d->num_threads; s++)
	{
		seqs[s] = std::make_pair(sd->sorting_places[s] + sd->pieces[iam][s].begin, sd->sorting_places[s] + sd->pieces[iam][s].end);

#if MCSTL_ASSERTIONS
		assert(is_sorted(seqs[s].first, seqs[s].second, comp));
#endif
	}
	
	multiway_merge(seqs.begin(), seqs.end(), sd->merging_places[iam], comp, length_am, d->stable, false, sequential_tag());
		
	t.tic("merge");
		
#if MCSTL_ASSERTIONS
	assert(is_sorted(sd->merging_places[iam], sd->merging_places[iam] + length_am, comp));
#endif

#	pragma omp barrier

#if MCSTL_MULTIWAY_MERGESORT_COPY_LAST
	//write back
	std::copy(sd->merging_places[iam], sd->merging_places[iam] + length_am, sd->source + offset);
#endif

	delete[] sd->temporaries[iam];

	t.tic("copy back");

	t.print();
}

/** @brief PMWMS main call.
 *  @param begin Begin iterator of sequence.
 *  @param end End iterator of sequence.
 *  @param comp Comparator.
 *  @param n Length of sequence.
 *  @param num_threads Number of threads to use.
 *  @param stable Stable sorting.
 */
template<typename RandomAccessIterator, typename Comparator>
inline void
parallel_sort_mwms(	RandomAccessIterator begin, 
			RandomAccessIterator end,
			Comparator comp,
			typename std::iterator_traits<RandomAccessIterator>::difference_type n,
			int num_threads,
			bool stable)
{
	MCSTL_CALL(n)
	
	typedef typename std::iterator_traits<RandomAccessIterator>::value_type
	ValueType;
	typedef typename std::iterator_traits<RandomAccessIterator>::difference_type
	DiffType;
	
	if(n <= 1)
		return;

	if(num_threads > n)
		num_threads = static_cast<thread_index_t>(n);	//at least one element per thread
	
	PMWMSSortingData<RandomAccessIterator> sd;

	sd.source = begin;
	sd.temporaries = new ValueType*[num_threads];
#if MCSTL_MULTIWAY_MERGESORT_COPY_LAST
	sd.sorting_places = new RandomAccessIterator[num_threads];
	sd.merging_places = new ValueType*[num_threads];
#else
	sd.sorting_places = new ValueType*[num_threads];
	sd.merging_places = new RandomAccessIterator[num_threads];
#endif
	if(SETTINGS::sort_splitting == SETTINGS::SAMPLING)
		sd.samples = new ValueType[num_threads * (SETTINGS::sort_mwms_oversampling * num_threads - 1)];
	else
		sd.samples = NULL;
	sd.offsets = new DiffType[num_threads - 1];
	sd.pieces = new std::vector<Piece<DiffType> >[num_threads];
	for(int s = 0; s < num_threads; s++)
		sd.pieces[s].resize(num_threads);
	PMWMSSorterPU<RandomAccessIterator>* pus = new PMWMSSorterPU<RandomAccessIterator>[num_threads];
	DiffType* starts = sd.starts = new DiffType[num_threads + 1];

	DiffType chunk_length = n / num_threads, split = n % num_threads, start = 0;
	for(int i = 0; i < num_threads; i++)
	{
		starts[i] = start;
		start += (i < split) ? (chunk_length + 1) : chunk_length;
		pus[i].num_threads = num_threads;
		pus[i].iam = i;
		pus[i].sd = &sd;
		pus[i].stable = stable;
	}
	starts[num_threads] = start;

	//now sort in parallel
	#pragma omp parallel num_threads(num_threads)
	parallel_sort_mwms_pu(&(pus[omp_get_thread_num()]), comp);

	delete[] starts;
	delete[] sd.temporaries;
	delete[] sd.sorting_places;
	delete[] sd.merging_places;
	
	if(SETTINGS::sort_splitting == SETTINGS::SAMPLING)
		delete[] sd.samples;

	delete[] sd.offsets;
	delete[] sd.pieces;

	delete[] pus;
}

}	//namespace mcstl

#endif
