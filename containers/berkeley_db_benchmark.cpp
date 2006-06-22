/***************************************************************************
 *            berkeley_db_benchmark.cpp
 *
 *  Fri Mar  3 16:33:04 2006
 *  Copyright  2006  Roman Dementiev
 *  Email
 ****************************************************************************/

#include <stxxl>

#include <db_cxx.h>

#define KEY_SIZE 		8
#define DATA_SIZE 	32

#define NODE_BLOCK_SIZE 	(16*1024)
#define LEAF_BLOCK_SIZE 	(32*1024)


#define TOTAL_CACHE_SIZE    (110*1024*1024)
#define NODE_CACHE_SIZE 	(1*(TOTAL_CACHE_SIZE/5))
#define LEAF_CACHE_SIZE 	(4*(TOTAL_CACHE_SIZE/5))

#define BDB_FILE "/data3/bdb_file"
//#define BDB_FILE "/var/tmp/bdb_file"

// BDB settings
u_int32_t    pagesize = LEAF_BLOCK_SIZE;
u_int        bulkbufsize = 4 * 1024 * 1024;
u_int        logbufsize = 8 * 1024 * 1024;
u_int        cachesize = 3*TOTAL_CACHE_SIZE/5;
//u_int        cachesize = 1*TOTAL_CACHE_SIZE/5;
u_int        datasize = DATA_SIZE;
u_int        keysize = KEY_SIZE;
u_int        numitems = 0;


struct my_key
{
	char keybuf[KEY_SIZE];
};

std::ostream & operator << (std::ostream & o, my_key & obj)
{
	for(int i = 0;i<KEY_SIZE;++i)
		o << obj.keybuf[i];
	return o;
}

bool operator == (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) == 0;
}

bool operator != (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) != 0;
}

bool operator < (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) < 0;
}

bool operator > (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) > 0;
}

bool operator <= (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) <= 0;
}

bool operator >= (const my_key & a, const my_key & b)
{
	return strncmp(a.keybuf,b.keybuf,KEY_SIZE) >= 0;
}


struct my_data
{
	char databuf[DATA_SIZE];
};

struct comp_type
{
	bool operator () (const my_key & a, const my_key & b) const
	{
		return strncmp(a.keybuf,b.keybuf,KEY_SIZE) < 0;
	}
	static my_key max_value()
	{ 
		my_key obj;
		memset(obj.keybuf, (std::numeric_limits<char>::max)() , KEY_SIZE);
		return obj;
	}
};
typedef stxxl::map<my_key,my_data,comp_type,NODE_BLOCK_SIZE,LEAF_BLOCK_SIZE> map_type;

#define KEYPOS 	(i % KEY_SIZE)
#define VALUE 		(myrand() % 26)

void run_bdb_btree(stxxl::int64 ops)
{
	char *filename = BDB_FILE;
	char *letters = "abcdefghijklmnopqrstuvwxuz";
	
	my_key key1_storage;
	my_data data1_storage;
    
	unlink(filename);
	
	memset(key1_storage.keybuf, 'a', KEY_SIZE);
	memset(data1_storage.databuf, 'b', DATA_SIZE);
	
	
	Db db(NULL, 0);               // Instantiate the Db object
	
	try {
	
		
		db.set_errfile(stderr);
		db.set_pagesize(pagesize);
		db.set_cachesize(0,cachesize,1);
		
    	// Open the database
    	db.open(NULL,                // Transaction pointer 
            filename,          // Database file name 
            NULL,                // Optional logical database name
            DB_BTREE,            // Database access method
            DB_CREATE,              // Open flags
            0);                  // File mode (using defaults)

		
		// here we start with the tests
		Dbt key1(key1_storage.keybuf,  KEY_SIZE);
		Dbt data1(data1_storage.databuf, DATA_SIZE);
		
		stxxl::timer Timer;
		stxxl::int64 n_inserts = ops, n_locates=ops, n_range_queries=ops, n_deletes = ops;
		stxxl::int64 i;
		comp_type cmp_;
		
		stxxl::ran32State = 0xdeadbeef;
		stxxl::random_number32 myrand;
		
		DB_BTREE_STAT * dbstat;
		
		db.stat(NULL,&dbstat,0);
		STXXL_MSG("Records in map: "<<dbstat->bt_ndata)
		
		Timer.start();
					
		for (i = 0; i < n_inserts; ++i)
		{
			key1_storage.keybuf[KEYPOS] = letters[VALUE];
			db.put(NULL, &key1, &data1, DB_NOOVERWRITE);
		}
	
		Timer.stop();
		db.stat(NULL,&dbstat,0);
		STXXL_MSG("Records in map: "<<dbstat->bt_ndata)
		STXXL_MSG("Insertions elapsed time: "<<(Timer.mseconds()/1000.)<<
					" seconds : "<< (double(n_inserts)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")

		/////////////////////////////////////////
		Timer.reset();
		Timer.start();
		
	
		Dbc *cursorp;
		db.cursor(NULL, &cursorp, 0);
		
		for (i = 0; i < n_locates; ++i)
		{
			key1_storage.keybuf[KEYPOS] = letters[VALUE];
			Dbt keyx(key1_storage.keybuf,  KEY_SIZE);
			Dbt datax(data1_storage.databuf, DATA_SIZE);
			
			cursorp->get(&keyx, &datax, DB_SET_RANGE);
		}
	
		Timer.stop();
		STXXL_MSG("Locates elapsed time: "<<(Timer.mseconds()/1000.)<<
			" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")

		
		////////////////////////////////////
		Timer.reset();
		
		Timer.start();		
		
		stxxl::int64 n_scanned = 0;
	
		for (i = 0; i < n_range_queries; ++i)
		{
			key1_storage.keybuf[KEYPOS] = letters[VALUE];
			my_key last_key = key1_storage;
			key1_storage.keybuf[KEYPOS] = letters[VALUE];
			if(last_key<key1_storage)
				std::swap(last_key,key1_storage);
			
			Dbt keyx(key1_storage.keybuf,  KEY_SIZE);
			Dbt datax(data1_storage.databuf, DATA_SIZE);
			
			if( cursorp->get(&keyx, &datax, DB_SET_RANGE) == DB_NOTFOUND)
				continue;
			
			while(*((my_key *)keyx.get_data()) <= last_key)
			{
				++n_scanned;
				if(cursorp->get(&keyx, &datax, DB_NEXT)==DB_NOTFOUND)
					break;
			}
			
			if(n_scanned >= 10*n_range_queries)	break;
		}
	
		n_range_queries = i;
		
		Timer.stop();
		if (cursorp != NULL) cursorp->close();
			
		STXXL_MSG("Range query elapsed time: "<<(Timer.mseconds()/1000.)<<
			" seconds : "<< (double(n_scanned)/(Timer.mseconds()/1000.))<<
			" key/data pairs per sec, #queries "<< n_range_queries<<" #scanned elements: "<<n_scanned)
		
		//////////////////////////////////////
		
		stxxl::ran32State = 0xdeadbeef;
		memset(key1_storage.keybuf, 'a', KEY_SIZE);
		
		Timer.reset();
		Timer.start();
	
		for (i = 0; i < n_deletes; ++i)
		{
			key1_storage.keybuf[KEYPOS] = letters[VALUE];
			Dbt keyx(key1_storage.keybuf,  KEY_SIZE);
			db.del(NULL, &keyx, 0);
		}
	
		Timer.stop();
		db.stat(NULL,&dbstat,0);
		STXXL_MSG("Records in map: "<<dbstat->bt_ndata)
		STXXL_MSG("Erase elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
		
		db.close(0);
	}
	catch(DbException &e)
	{
    	STXXL_ERRMSG("DbException happened")
	} 
	catch(std::exception &e)
	{
    	STXXL_ERRMSG("std::exception happened")
	} 

	unlink(filename);
}

void run_stxxl_map(stxxl::int64 ops)
{
	map_type Map(NODE_CACHE_SIZE,LEAF_CACHE_SIZE);
	Map.enable_prefetching();
	
	char *letters = "abcdefghijklmnopqrstuvwxuz";
	std::pair<my_key,my_data> element;
    
	memset(element.first.keybuf, 'a', KEY_SIZE);
	memset(element.second.databuf, 'b', DATA_SIZE);

	
	stxxl::timer Timer;
	stxxl::int64 n_inserts = ops, n_locates=ops, n_range_queries=ops, n_deletes = ops;
	stxxl::int64 i;
	comp_type cmp_;
	
	stxxl::ran32State = 0xdeadbeef;
	
	stxxl::random_number32 myrand;
	
	STXXL_MSG("Records in map: "<<Map.size())
	
	Timer.start();
	            
	for (i = 0; i < n_inserts; ++i)
	{
		element.first.keybuf[KEYPOS] = letters[VALUE];
		Map.insert(element);
	}

	Timer.stop();
	STXXL_MSG("Records in map: "<<Map.size())
	STXXL_MSG("Insertions elapsed time: "<<(Timer.mseconds()/1000.)<<
				" seconds : "<< (double(n_inserts)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")

	////////////////////////////////////////////////
	Timer.reset();

	const map_type & CMap(Map); // const map reference
	
	Timer.start();

	for (i = 0; i < n_locates; ++i)
	{
		element.first.keybuf[KEYPOS] = letters[VALUE];
		CMap.lower_bound(element.first);
	}

	Timer.stop();
	STXXL_MSG("Locates elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
	
	////////////////////////////////////
	Timer.reset();
	
	Timer.start();
	
	stxxl::int64 n_scanned = 0, skipped_qieries=0;

	for (i = 0; i < n_range_queries; ++i)
	{
		element.first.keybuf[KEYPOS] = letters[VALUE];
		my_key begin_key = element.first;
		map_type::const_iterator begin=CMap.lower_bound(element.first);
		element.first.keybuf[KEYPOS] = letters[VALUE];
		map_type::const_iterator beyond=CMap.lower_bound(element.first);
		if(element.first<begin_key)
			std::swap(begin,beyond);
		while(begin!=beyond)
		{
			my_data tmp =  begin->second;
			++n_scanned;
			++begin;
		}
		if(n_scanned >= 10*n_range_queries)	break;
	}
	
	n_range_queries = i;

	Timer.stop();
	STXXL_MSG("Range query elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(n_scanned)/(Timer.mseconds()/1000.))<<
		" key/data pairs per sec, #queries "<< n_range_queries<<" #scanned elements: "<<n_scanned)
	
	//////////////////////////////////////
	stxxl::ran32State = 0xdeadbeef;
	memset(element.first.keybuf, 'a', KEY_SIZE);
	memset(element.second.databuf, 'b', DATA_SIZE);
	
	Timer.reset();
	Timer.start();

	for (i = 0; i < n_deletes; ++i)
	{
		element.first.keybuf[KEYPOS] = letters[VALUE];
		Map.erase(element.first);
	}

	Timer.stop();
	STXXL_MSG("Records in map: "<<Map.size())
	STXXL_MSG("Erase elapsed time: "<<(Timer.mseconds()/1000.)<<
		" seconds : "<< (double(ops)/(Timer.mseconds()/1000.))<<" key/data pairs per sec")
}


int main(int argc, char * argv[])
{
	if(argc < 3)
	{
		STXXL_MSG("Usage: "<<argv[0]<<" version #ops")
		STXXL_MSG("\t version = 1: test stxxl map")
		STXXL_MSG("\t version = 2: test Berkeley DB btree")
		return 0;
	}

	int version = atoi(argv[1]);
	stxxl::int64 ops = atoll(argv[2]);

	STXXL_MSG("Running version      : "<<version)
	STXXL_MSG("Operations to perform: "<<ops);
	STXXL_MSG("Btree cache size     : "<<TOTAL_CACHE_SIZE<<" bytes")
	STXXL_MSG("Leaf block size      : "<<LEAF_BLOCK_SIZE<<" bytes")
	
	switch(version)
	{
		case 1:
			run_stxxl_map(ops);
			break;
		case 2:
			run_bdb_btree(ops);
			break;
		default:
			STXXL_MSG("Unsupported version "<<version)
	}
	
}
