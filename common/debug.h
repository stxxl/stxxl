#ifndef HEADER_STXXL_DEBUG
#define HEADER_STXXL_DEBUG

/***************************************************************************
 *            debug.h
 *
 *  Thu Dec 30 14:52:00 2004
 *  Copyright  2004  Roman Dementiev
 *  Email dementiev@ira.uka.de
 ****************************************************************************/

#include "../common/utils.h"
#include "../common/mutex.h"
#include <map>
#include <ext/hash_map>

__STXXL_BEGIN_NAMESPACE 

class debugmon
{
	struct tag
	{
		bool ongoing;
		char * end;
		size_t size;
	};
	struct hash_fct
	{
		size_t operator () (char * arg) const
		{
			return long(arg);
		}
	};
	struct eqt
	{
		bool operator () (char * arg1, char * arg2) const
		{
			return arg1 == arg2;
		}
	};
	__gnu_cxx::hash_map<char *, tag, hash_fct, eqt > tags;
	mutex mutex1;
	
	static debugmon * instance;
	
	debugmon() {}
public:
	#ifndef STXXL_DEBUGMON
	void block_allocated(char * ptr, char * end, size_t size)
	{
	}
	void block_deallocated(char * ptr)
	{
	}
	void io_started(char * ptr)
	{
	}
	void io_finished(char * ptr)
	{
	}
	#else
	void block_allocated(char * ptr, char * end, size_t size)
	{
		mutex1.lock();
		// checks are here
		STXXL_VERBOSE1("debugmon: block "<<long(ptr)<<" allocated")
		assert(tags.find(ptr) == tags.end()); // not allocated
		tag t;
		t.ongoing = false;
		t.end = end;
		t.size = size;
		tags[ptr] = t;
		mutex1.unlock();
	}
	void block_deallocated(char * ptr)
	{
		mutex1.lock();
		STXXL_VERBOSE1("debugmon: block_deallocated from "<<long(ptr))
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		assert(t.ongoing == false); // not ongoing
		tags.erase(ptr);
		size_t size = t.size;
		STXXL_VERBOSE1("debugmon: block_deallocated to "<<long(t.end))
		char * endptr =(char *) t.end; 
		char * ptr1 = (char *) ptr;
		ptr1 += size;
		while(ptr1 < endptr)
		{
			STXXL_VERBOSE1("debugmon: block_deallocated next "<<long(ptr1))
			assert(tags.find(ptr1) != tags.end()); // allocated
			tag t = tags[ptr1];
			assert(t.ongoing == false); // not ongoing
			assert(t.size == size); // chunk size
			assert(t.end == endptr); // array end address
			tags.erase(ptr1);
			ptr1 += size;
		}
		mutex1.unlock();
	}
	void io_started(char * ptr)
	{
		mutex1.lock();
		STXXL_VERBOSE1("debugmon: I/O on block "<<long(ptr)<<" started")
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		//assert(t.ongoing == false); // not ongoing
		if(t.ongoing == true)
			STXXL_ERRMSG("debugmon: I/O on block "<<long(ptr)<<" started, but block is already busy")
		t.ongoing = true;
		tags[ptr] = t;
		mutex1.unlock();
	}
	void io_finished(char * ptr)
	{
		mutex1.lock();
		STXXL_VERBOSE1("debugmon: I/O on block "<<long(ptr)<<" finished")
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		//assert(t.ongoing == true); // ongoing
		if(t.ongoing == false)
			STXXL_ERRMSG("debugmon: I/O on block "<<long(ptr)<<" finished, but block was not busy")
		t.ongoing = false;
		tags[ptr] = t;
		mutex1.unlock();
	}
	#endif
	static debugmon *get_instance ()
	{
			if (!instance)
				instance = new debugmon ();
			return instance;
	}
};

__STXXL_END_NAMESPACE

#endif
