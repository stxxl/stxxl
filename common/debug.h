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
	};
	std::map<void *, tag > tags;
	
	static debugmon * instance;
	
	debugmon() {}
public:
	#ifdef NDEBUG
	void block_allocated(void * ptr)
	{
	}
	void block_deallocated(void * ptr)
	{
	}
	void io_started(void * ptr)
	{
	}
	void io_finished(void * ptr)
	{
	}
	#else
	void block_allocated(void * ptr)
	{
		// checks are here
		assert(tags.find(ptr) == tags.end()); // not allocated
		tag t;
		t.ongoing = false;
		tags[ptr] = t;
	}
	void block_deallocated(void * ptr)
	{
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		assert(t.ongoing == false); // not ongoing
		tags.erase(ptr);
	}
	void io_started(void * ptr)
	{
		assert(tags.find(ptr) != tags.end()); // allocated
		tag t = tags[ptr];
		assert(t.ongoing == false); // not ongoing
		t.ongoing = true;
		tags[ptr] = t;
	}
	void io_finished(void * ptr)
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
