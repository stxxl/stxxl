/***************************************************************************
 *   Copyright (C) 2007 by Johannes Singler                                *
 *   singler@ira.uka.de                                                    *
 *   Distributed under the Boost Software License, Version 1.0.            *
 *   (See accompanying file LICENSE_1_0.txt or copy at                     *
 *   http://www.boost.org/LICENSE_1_0.txt)                                 *
 *   Part of the MCSTL   http://algo2.iti.uni-karlsruhe.de/singler/mcstl/  *
 ***************************************************************************/

/** @file mcstl_losertree.h
 *  @brief Many generic loser tree variants. */

#ifndef _MCSTL_LOSERTREE_H
#define _MCSTL_LOSERTREE_H

#include <functional>

#include <mod_stl/stl_algobase.h>
#include <bits/mcstl_features.h>
#include <bits/mcstl_base.h>

namespace mcstl
{

#if MCSTL_LOSER_TREE_EXPLICIT

/** @brief Guarded loser tree, copying the whole element into the tree structure.
 *
 *  Guarding is done explicitly through two flags per element, inf and sup
 *  This is a quite slow variant. */
template<typename T, typename Comparator = std::less<T> >
class LoserTreeExplicit
{
private:
	struct Loser
	{
		T key;	//the relevant element
		bool inf, sup;	//is this an infimum or supremum element?
		int source;	//number of the sequence the element comes from
	};
	
	unsigned int size, offset;
	Loser* losers;
	Comparator comp;

public:
	inline LoserTreeExplicit(unsigned int _size, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		size = _size;
		offset = size;
		losers = new Loser[size];
		for(unsigned int l = 0; l < size; l++)
		{
			//losers[l].key = ... 	stays unset
			losers[l].inf = true;
			losers[l].sup = false;
			//losers[l].source = -1;	//sentinel
		}
	}

	inline ~LoserTreeExplicit()
	{
		delete[] losers;
	}
	
	inline void print()
	{
	}

	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(T key, int source, bool sup)
	{
		bool inf = false;
		for(unsigned int pos = (offset + source) / 2; pos > 0; pos /= 2)
		{
			if(	(!inf && !losers[pos].inf && !sup && !losers[pos].sup && comp(losers[pos].key, key)) || 
				losers[pos].inf || 
				sup)
			{	//the other one is smaller
				std::swap(losers[pos].key, key);
				std::swap(losers[pos].inf, inf);
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
			}
		}
		
		losers[0].key = key;
		losers[0].inf = inf;
		losers[0].sup = sup;
		losers[0].source = source;
	}
	
	inline void init()
	{
	}

	inline void delete_min_insert(T key, bool sup)
	{
		bool inf = false;
		int source = losers[0].source;
		for(unsigned int pos = (offset + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	(!inf && !losers[pos].inf && !sup && !losers[pos].sup && comp(losers[pos].key, key)) || 
				losers[pos].inf || 
				sup)
			{	//the other one is smaller
				std::swap(losers[pos].key, key);
				std::swap(losers[pos].inf, inf);
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
			}
		}
		
		losers[0].key = key;
		losers[0].inf = inf;
		losers[0].sup = sup;
		losers[0].source = source;
	}

	inline void insert_start_stable(T key, int source, bool sup)
	{
		bool inf = false;
		for(unsigned int pos = (offset + source) / 2; pos > 0; pos /= 2)
		{
			if(	(!inf && !losers[pos].inf && !sup && !losers[pos].sup && 
			
			((comp(losers[pos].key, key)) ||
			    (!comp(key, losers[pos].key) && losers[pos].source < source))
			
			) || 
				losers[pos].inf || 
				sup)
			{	//take next key
				std::swap(losers[pos].key, key);
				std::swap(losers[pos].inf, inf);
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
			}
		}
		
		losers[0].key = key;
		losers[0].inf = inf;
		losers[0].sup = sup;
		losers[0].source = source;
	}

	inline void init_stable()
	{
	}

	inline void delete_min_insert_stable(T key, bool sup)
	{
		bool inf = false;
		int source = losers[0].source;
		for(unsigned int pos = (offset + source) / 2; pos > 0; pos /= 2)
		{
			if(	(!inf && !losers[pos].inf && !sup && !losers[pos].sup && 
			
			((comp(losers[pos].key, key)) ||
			    (!comp(key, losers[pos].key) && losers[pos].source < source))
			
			) || 
				losers[pos].inf || 
				sup)
			{
				std::swap(losers[pos].key, key);
				std::swap(losers[pos].inf, inf);
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
			}
		}
		
		losers[0].key = key;
		losers[0].inf = inf;
		losers[0].sup = sup;
		losers[0].source = source;
	}
};

#endif

#if MCSTL_LOSER_TREE

/** @brief Guarded loser tree, either copying the whole element into the tree structure, or looking up the element via the index.
 *
 *  Guarding is done explicitly through one flag sup per element, inf is not needed due to a better initialization routine.
 *  This is a well-performing variant. */
template<typename T, typename Comparator = std::less<T> >
class LoserTree
{
private:
	struct Loser
	{
		bool sup;
		int source;
		T key;
	};
	
	unsigned int ik, k, offset;
	Loser* losers;
	Comparator comp;

public:
	inline LoserTree(unsigned int _k, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		ik = _k;
		k = 1 << (log2(ik - 1) + 1);	//next greater power of 2
		offset = k;
		losers = new Loser[k * 2];
		for(unsigned int i = ik - 1; i < k; i++)
			losers[i + k].sup = true;
	}

	inline ~LoserTree()
	{
		delete[] losers;
	}

	void print()
	{
		for(unsigned int i = 0; i < (k * 2); i++)
			printf("%d    %d from %d,  %d\n", i, losers[i].key, losers[i].source, losers[i].sup);
	}
	
	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(const T& key, int source, bool sup)
	{
		unsigned int pos = k + source;
		
		losers[pos].sup = sup;
		losers[pos].source = source;
		losers[pos].key = key;
	}
	
	unsigned int init_winner (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(losers[right].key, losers[left].key)))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init()
	{
		losers[0] = losers[init_winner(1)];
	}

	inline void delete_min_insert(T key, bool sup)	//do not pass const reference since key will be used as local variable
	{
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	sup ||
				(!losers[pos].sup && comp(losers[pos].key, key)))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].key, key);
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		losers[0].key = key;
	}
	
	inline void insert_start_stable(const T& key, int source, bool sup)
	{
		return insert_start(key, source, sup);
	}
	
	unsigned int init_winner_stable (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(losers[right].key, losers[left].key)))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init_stable()
	{
		losers[0] = losers[init_winner_stable(1)];
	}

	inline void delete_min_insert_stable(T key, bool sup)	//do not pass const reference since key will be used as local variable
	{
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted, ties are broken by source
			if(	(sup && (!losers[pos].sup || losers[pos].source < source)) ||
				(!sup && !losers[pos].sup && 
				((comp(losers[pos].key, key)) ||
					(!comp(key, losers[pos].key) && losers[pos].source < source))))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].key, key);
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		losers[0].key = key;
	}
};

#endif

#if MCSTL_LOSER_TREE_REFERENCE

/** @brief Guarded loser tree, either copying the whole element into the tree structure, or looking up the element via the index.
 *
 *  Guarding is done explicitly through one flag sup per element, inf is not needed due to a better initialization routine.
 *  This is a well-performing variant.
 */
template<typename T, typename Comparator = std::less<T> >
class LoserTreeReference
{
#undef COPY
#ifdef COPY
	#define KEY(i) losers[i].key
	#define KEY_SOURCE(i) key
#else
	#define KEY(i) keys[losers[i].source]
	#define KEY_SOURCE(i) keys[i]
#endif
private:
	struct Loser
	{
		bool sup;
		int source;
		#ifdef COPY
		T key;
		#endif
	};
	
	unsigned int ik, k, offset;
	Loser* losers;
	#ifndef COPY
	T* keys;
	#endif
	Comparator comp;

public:
	inline LoserTreeReference(unsigned int _k, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		ik = _k;
		k = 1 << (log2(ik - 1) + 1);	//next greater power of 2
		offset = k;
		losers = new Loser[k * 2];
		#ifndef COPY
		keys = new T[ik];
		#endif
		for(unsigned int i = ik - 1; i < k; i++)
			losers[i + k].sup = true;
	}

	inline ~LoserTreeReference()
	{
		delete[] losers;
		#ifndef COPY
		delete[] keys;
		#endif
	}

	void print()
	{
		for(unsigned int i = 0; i < (k * 2); i++)
			printf("%d    %d from %d,  %d\n", i, KEY(i), losers[i].source, losers[i].sup);
	}
	
	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(T key, int source, bool sup)
	{
		unsigned int pos = k + source;
		
		losers[pos].sup = sup;
		losers[pos].source = source;
		KEY(pos) = key;
	}
	
	unsigned int init_winner (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(KEY(right), KEY(left))))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init()
	{
		losers[0] = losers[init_winner(1)];
	}

	inline void delete_min_insert(T key, bool sup)
	{
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	sup ||
				(!losers[pos].sup && comp(KEY(pos), KEY_SOURCE(source))))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				#ifdef COPY
				std::swap(KEY(pos), KEY_SOURCE(source));
				#endif
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		#ifdef COPY
		KEY(0) = KEY_SOURCE(source);
		#endif
	}
	
	inline void insert_start_stable(T key, int source, bool sup)
	{
		return insert_start(key, source, sup);
	}
	
	unsigned int init_winner_stable (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(KEY(right), KEY(left))))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init_stable()
	{
		losers[0] = losers[init_winner_stable(1)];
	}

	inline void delete_min_insert_stable(T key, bool sup)
	{
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted, ties are broken by source
			if(	(sup && (!losers[pos].sup || losers[pos].source < source)) ||
				(!sup && !losers[pos].sup && 
				((comp(KEY(pos), KEY_SOURCE(source))) ||
					(!comp(KEY_SOURCE(source), KEY(pos)) && losers[pos].source < source))))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				#ifdef COPY
				std::swap(KEY(pos), KEY_SOURCE(source));
				#endif
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		#ifdef COPY
		KEY(0) = KEY_SOURCE(source);
		#endif
	}
};
#undef KEY
#undef KEY_SOURCE

#endif

#if MCSTL_LOSER_TREE_POINTER

/** @brief Guarded loser tree, either copying the whole element into the tree structure, or looking up the element via the index.
 *  Guarding is done explicitly through one flag sup per element, inf is not needed due to a better initialization routine.
 *  This is a well-performing variant.
*/
template<typename T, typename Comparator = std::less<T> >
class LoserTreePointer
{
private:
	struct Loser
	{
		bool sup;
		int source;
		const T* keyp;
	};
	
	unsigned int ik, k, offset;
	Loser* losers;
	Comparator comp;

public:
	inline LoserTreePointer(unsigned int _k, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		ik = _k;
		k = 1 << (log2(ik - 1) + 1);	//next greater power of 2
		offset = k;
		losers = new Loser[k * 2];
		for(unsigned int i = ik - 1; i < k; i++)
			losers[i + k].sup = true;
	}

	inline ~LoserTreePointer()
	{
		delete[] losers;
	}

	void print()
	{
		for(unsigned int i = 0; i < (k * 2); i++)
			printf("%d    %d from %d,  %d\n", i, losers[i].keyp, losers[i].source, losers[i].sup);
	}
	
	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(const T& key, int source, bool sup)
	{
		unsigned int pos = k + source;
		
		losers[pos].sup = sup;
		losers[pos].source = source;
		losers[pos].keyp = &key;
	}
	
	unsigned int init_winner (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(*losers[right].keyp, *losers[left].keyp)))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init()
	{
		losers[0] = losers[init_winner(1)];
	}

	inline void delete_min_insert(const T& key, bool sup)
	{
		const T* keyp = &key;
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	sup ||
				(!losers[pos].sup && comp(*losers[pos].keyp, *keyp)))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].keyp, keyp);
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		losers[0].keyp = keyp;
	}
	
	inline void insert_start_stable(const T& key, int source, bool sup)
	{
		return insert_start(key, source, sup);
	}
	
	unsigned int init_winner_stable (unsigned int root)
	{
		if (root >= k)
		{
			return root;
		}
		else
		{
			unsigned int left = init_winner (2 * root);
			unsigned int right = init_winner (2 * root + 1);
			if(	losers[right].sup || 
				(!losers[left].sup && !comp(*losers[right].keyp, *losers[left].keyp)))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init_stable()
	{
		losers[0] = losers[init_winner_stable(1)];
	}

	inline void delete_min_insert_stable(const T& key, bool sup)
	{
		const T* keyp = &key;
		int source = losers[0].source;
		for(unsigned int pos = (k + source) / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted, ties are broken by source
			if(	(sup && (!losers[pos].sup || losers[pos].source < source)) ||
				(!sup && !losers[pos].sup && 
				((comp(*losers[pos].keyp, *keyp)) ||
					(!comp(*keyp, *losers[pos].keyp) && losers[pos].source < source))))
			{	//the other one is smaller
				std::swap(losers[pos].sup, sup);
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].keyp, keyp);
			}
		}
		
		losers[0].sup = sup;
		losers[0].source = source;
		losers[0].keyp = keyp;
	}
};

#endif

#if MCSTL_LOSER_TREE_UNGUARDED

/** @brief Unguarded loser tree, copying the whole element into the tree structure.
 *
 *  No guarding is done, therefore not a single input sequence must run empty.
 *  This is a very fast variant. */
template<typename T, typename Comparator = std::less<T> >
class LoserTreeUnguarded
{
private:
	struct Loser
	{
		int source;
		T key;
	};
	
	unsigned int ik, k, offset;
	unsigned int* mapping;
	Loser* losers;
	Comparator comp;

	void map(unsigned int root, unsigned int begin, unsigned int end)
	{
		if(begin + 1 == end)
			mapping[begin] = root;
		else
		{
			unsigned int left = 1 << (log2(end - begin - 1));	//next greater or equal power of 2
			map(root * 2, begin, begin + left);
			map(root * 2 + 1, begin + left, end);
		}
	}

public:
	inline LoserTreeUnguarded(unsigned int _k, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		ik = _k;
		k = 1 << (log2(ik - 1) + 1);	//next greater or equal power of 2
		offset = k;
		losers = new Loser[k + ik];
		mapping = new unsigned int[ik];
		map(1, 0, ik);
	}

	inline ~LoserTreeUnguarded()
	{
		delete[] losers;
		delete[] mapping;
	}

	void print()
	{
		for(unsigned int i = 0; i < k + ik; i++)
			printf("%d    %d from %d\n", i, losers[i].key, losers[i].source);
	}
	
	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(const T& key, int source, bool)
	{
		unsigned int pos = mapping[source];
		
		losers[pos].source = source;
		losers[pos].key = key;
	}
	
	unsigned int init_winner(unsigned int root, unsigned int begin, unsigned int end)
	{
		if(begin + 1 == end)
			return mapping[begin];
		else
		{
			unsigned int division = 1 << (log2(end - begin - 1));	//next greater or equal power of 2
			unsigned int left = init_winner(2 * root, begin, begin + division);
			unsigned int right = init_winner(2 * root + 1, begin + division, end);
			if(	!comp(losers[right].key, losers[left].key))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init()
	{
		losers[0] = losers[init_winner(1, 0, ik)];
	}

	inline void delete_min_insert(const T& key, bool)	//do not pass const reference since key will be used as local variable
	{
		losers[0].key = key;
		T& keyr = losers[0].key;
		int& source = losers[0].source;
		for(int pos = mapping[source] / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	comp(losers[pos].key, keyr))
			{	//the other one is smaller
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].key, keyr);
			}
		}
	}
	
	inline void insert_start_stable(const T& key, int source, bool)
	{
		return insert_start(key, source, false);
	}
	
	inline void init_stable()
	{
		init();
	}

	inline void delete_min_insert_stable(const T& key, bool)
	{
		losers[0].key = key;
		T& keyr = losers[0].key;
		int& source = losers[0].source;
		for(int pos = mapping[source] / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted, ties are broken by source
			if(	comp(losers[pos].key, keyr) || (!comp(keyr, losers[pos].key) && losers[pos].source < source))
			{	//the other one is smaller
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].key, keyr);
			}
		}
	}
};

#endif

#if MCSTL_LOSER_TREE_POINTER_UNGUARDED

/** @brief Unguarded loser tree, keeping only pointers to the elements in the tree structure.
 *
 *  No guarding is done, therefore not a single input sequence must run empty.
 *  This is a very fast variant. */
template<typename T, typename Comparator = std::less<T> >
class LoserTreePointerUnguarded
{
private:
	struct Loser
	{
		int source;
		const T* keyp;
	};
	
	unsigned int ik, k, offset;
	unsigned int* mapping;
	Loser* losers;
	Comparator comp;

	void map(unsigned int root, unsigned int begin, unsigned int end)
	{
		if(begin + 1 == end)
			mapping[begin] = root;
		else
		{
			unsigned int left = 1 << (log2(end - begin - 1));	//next greater or equal power of 2
			map(root * 2, begin, begin + left);
			map(root * 2 + 1, begin + left, end);
		}
	}

public:
	inline LoserTreePointerUnguarded(unsigned int _k, Comparator _comp = std::less<T>()) : comp(_comp)
	{
		ik = _k;
		k = 1 << (log2(ik - 1) + 1);	//next greater power of 2
		offset = k;
		losers = new Loser[k + ik];
		mapping = new unsigned int[ik];
		map(1, 0, ik);
	}

	inline ~LoserTreePointerUnguarded()
	{
		delete[] losers;
		delete[] mapping;
	}

	void print()
	{
		for(unsigned int i = 0; i < k + ik; i++)
			printf("%d    %d from %d\n", i, *losers[i].keyp, losers[i].source);
	}
	
	inline int get_min_source()
	{
		return losers[0].source;
	}

	inline void insert_start(const T& key, int source, bool)
	{
		unsigned int pos = mapping[source];
		
		losers[pos].source = source;
		losers[pos].keyp = &key;
	}
	
	unsigned int init_winner(unsigned int root, unsigned int begin, unsigned int end)
	{
		if(begin + 1 == end)
			return mapping[begin];
		else
		{
			unsigned int division = 1 << (log2(end - begin - 1));	//next greater or equal power of 2
			unsigned int left = init_winner(2 * root, begin, begin + division);
			unsigned int right = init_winner(2 * root + 1, begin + division, end);
			if(	!comp(*losers[right].keyp, *losers[left].keyp))
			{	//left one is less or equal
				losers[root] = losers[right];
				return left;
			}
			else
			{	//right one is less
				losers[root] = losers[left];
				return right;
			}
		}
	}
	
	inline void init()
	{
		losers[0] = losers[init_winner(1, 0, ik)];
	}

	inline void delete_min_insert(const T& key, bool)
	{
		const T* keyp = &key;
		int& source = losers[0].source;
		for(int pos = mapping[source] / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted
			if(	comp(*losers[pos].keyp, *keyp))
			{	//the other one is smaller
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].keyp, keyp);
			}
		}
		
		losers[0].keyp = keyp;
	}
	
	inline void insert_start_stable(const T& key, int source, bool)
	{
		return insert_start(key, source, false);
	}
	
	inline void init_stable()
	{
		init();
	}

	inline void delete_min_insert_stable(const T& key, bool)
	{
		int& source = losers[0].source;
		const T* keyp = &key;
		for(int pos = mapping[source] / 2; pos > 0; pos /= 2)
		{
			//the smaller one gets promoted, ties are broken by source
			if(	comp(*losers[pos].keyp, *keyp) || (!comp(*keyp, *losers[pos].keyp) && losers[pos].source < source))
			{	//the other one is smaller
				std::swap(losers[pos].source, source);
				std::swap(losers[pos].keyp, keyp);
			}
		}
		
		losers[0].keyp = keyp;
	}
};

#endif

}	//namespace mcstl

#endif
