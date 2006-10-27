#include "mng.h"

__STXXL_BEGIN_NAMESPACE

block_manager *block_manager::instance = NULL;
config *config::instance = NULL;

void DiskAllocator::dump()
{
			stxxl::int64 total = 0;
			sortseq::const_iterator cur =	free_space.begin ();
			STXXL_ERRMSG("Free regions dump:")
			for(;cur!=free_space.end();++cur)	
			{
				STXXL_ERRMSG("Free chunk: begin: "<<(cur->first)<<" size: "<<(cur->second))
				total += cur->second;
			}
			STXXL_ERRMSG("Total bytes: "<<total)
}

	config *config::get_instance ()
	{
		if (!instance)
		{
			char *cfg_path = getenv ("STXXLCFG");
			if (cfg_path)
				instance = new config (cfg_path);
			else
				instance = new config ();
		}

		return instance;
	}


	config::config (const char *config_path)
	{
		std::ifstream cfg_file (config_path);
		if (!cfg_file)
		{
			STXXL_ERRMSG("Warning: no config file found." )
			STXXL_ERRMSG("Using default disk configuration." )
			#ifndef BOOST_MSVC
			DiskEntry entry1 = { "/var/tmp/stxxl", "syscall",
				1000 * 1024 * 1024
			};
			#else
			DiskEntry entry1 = { "", "wincall",
				1000 * 1024 * 1024
			};
			char * tmpstr = new char[255]; 
			stxxl_ifcheck_win(GetTempPath(255,tmpstr),resource_error);
			entry1.path = tmpstr;
			entry1.path += "stxxl";
			delete [] tmpstr;
			#endif
			/*
			DiskEntry entry2 =
				{ "/tmp/stxxl1", "mmap", 100 * 1024 * 1024 };
			DiskEntry entry3 = { "/tmp/stxxl2", "simdisk",
				1000 * 1024 * 1024
			}; */
			disks_props.push_back (entry1);
			//disks_props.push_back (entry2);
			//disks_props.push_back (entry3);
		}
		else
		{
			std::string line;

			while (cfg_file >> line)
			{
				std::vector < std::string > tmp = split (line, "=");
				
				if(tmp[0][0] == '#')
				{
				}
				else
				if (tmp[0] == "disk")
				{
					tmp = split (tmp[1], ",");
					DiskEntry entry = { tmp[0], tmp[2],
							stxxl::int64 (str2int (tmp[1])) *
							stxxl::int64 (1024 * 1024)
					};
					disks_props.push_back (entry);
				}
				else
				{
					std::cerr << "Unknown token " <<
						tmp[0] << std::endl;
				}
			}
			cfg_file.close ();
		}

		if (disks_props.empty ())
		{
			STXXL_FORMAT_ERROR_MSG(msg,"config::config No disks found in '" << config_path << "' .")
      throw std::runtime_error(msg.str());
		}
		else
		{
			for (std::vector < DiskEntry >::const_iterator it =
			     disks_props.begin (); it != disks_props.end ();
			     it++)
			{
				STXXL_MSG("Disk '" << (*it).path << "' is allocated, space: " <<
					((*it).size) / (1024 * 1024) <<
					" Mb, I/O implementation: " << (*it).io_impl )
			}
		}
	};

	block_manager *block_manager::get_instance ()
	{
		if (!instance)
			instance = new block_manager ();

		return instance;
	}

	block_manager::block_manager ()
	{
		FileCreator fc;
		config *cfg = config::get_instance ();

		ndisks = cfg->disks_number ();
		disk_allocators = new DiskAllocator *[ndisks];
		disk_files = new stxxl::file *[ndisks];

		for (unsigned i = 0; i < ndisks; i++)
		{
			disk_files[i] = fc.create (cfg->disk_io_impl (i),
						   cfg->disk_path (i),
						   stxxl::file::CREAT | stxxl::file::RDWR  | stxxl::file::DIRECT
						   ,i);
			disk_files[i]->set_size (cfg->disk_size (i));
			disk_allocators[i] = new DiskAllocator (cfg->disk_size (i));
		}
	};

	block_manager::~block_manager()
	{
		STXXL_VERBOSE1("Block manager deconstructor")
		for (unsigned i = 0; i < ndisks; i++)
		{
			delete disk_allocators[i];
			delete disk_files[i];
		}
		delete[]disk_allocators;
		delete[]disk_files;
	}



__STXXL_END_NAMESPACE
