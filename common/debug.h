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
#include <map>

__STXXL_BEGIN_NAMESPACE 

class debugmon
{
	struct tag
	{
		bool ongoing;
		char * end;
		size_t size;
	};
	std::map<char *, tag > tags;
	
	static debugmon * instance;
	
	debugmon() {}
public:
	#ifdef NDEBUG
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
		// checks are here
		assert(tags.find(ptr) == tags.end()); // not allocated
		tag t;
		t.ongoing = false;
		t.end = end;
		t.size = size;
		tags[ptr] = t;
	}
	void block_deallocated(char * ptr)
	{
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
	}
	void io_started(char * ptr)
	{
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		assert(t.ongoing == false); // not ongoing
		t.ongoing = true;
		tags[ptr] = t;
	}
	void io_finished(char * ptr)
	{
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		assert(t.ongoing == true); // ongoing
		t.ongoing = false;
		tags[ptr] = t;
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
