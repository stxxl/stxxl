/***************************************************************************
 *  mng/mng.h
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2004 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *  Copyright (C) 2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <sstream>
#include <stxxl/mng>
#include <stxxl/bits/io/io.h>
#include <stxxl/bits/version.h>
#include <stxxl/bits/common/debug.h>


__STXXL_BEGIN_NAMESPACE

void DiskAllocator::dump()
{
    int64 total = 0;
    sortseq::const_iterator cur = free_space.begin();
    STXXL_ERRMSG("Free regions dump:");
    for ( ; cur != free_space.end(); ++cur)
    {
        STXXL_ERRMSG("Free chunk: begin: " << (cur->first) << " size: " << (cur->second));
        total += cur->second;
    }
    STXXL_ERRMSG("Total bytes: " << total);
}

void config::init(const char * config_path)
{
    logger::get_instance();
    STXXL_MSG(get_version_string());
    std::vector<DiskEntry> flash_props;
    std::ifstream cfg_file(config_path);
    if (!cfg_file)
    {
        STXXL_ERRMSG("Warning: no config file found.");
        STXXL_ERRMSG("Using default disk configuration.");
#ifndef BOOST_MSVC
        DiskEntry entry1 = { "/var/tmp/stxxl", "syscall", 1000 * 1024 * 1024, true };
#else
        DiskEntry entry1 = { "", "wincall", 1000 * 1024 * 1024, true };
        char * tmpstr = new char[255];
        stxxl_check_ne_0(GetTempPath(255, tmpstr), resource_error);
        entry1.path = tmpstr;
        entry1.path += "stxxl";
        delete[] tmpstr;
#endif
#if 0
        DiskEntry entry2 =
        { "/tmp/stxxl1", "mmap", 100 * 1024 * 1024, true };
        DiskEntry entry3 =
        { "/tmp/stxxl2", "simdisk", 1000 * 1024 * 1024, false };
#endif
        disks_props.push_back(entry1);
        //disks_props.push_back(entry2);
        //disks_props.push_back(entry3);
    }
    else
    {
        std::string line;

        while (cfg_file >> line)
        {
            std::vector<std::string> tmp = split(line, "=");
            bool is_disk;

            if (tmp[0][0] == '#')
            { }
            else if ((is_disk = (tmp[0] == "disk")) || tmp[0] == "flash")
            {
                tmp = split(tmp[1], ",");
                DiskEntry entry = {
                    tmp[0], tmp[2],
                    int64(atoi(tmp[1].c_str())) * int64(1024 * 1024),
                    false
                };
                if (is_disk)
                    disks_props.push_back(entry);
                else
                    flash_props.push_back(entry);
            }
            else
            {
                std::cerr << "Unknown token " <<
                tmp[0] << std::endl;
            }
        }
        cfg_file.close();
    }

    // put flash devices after regular disks
    first_flash = disks_props.size();
    disks_props.insert(disks_props.end(), flash_props.begin(), flash_props.end());

    if (disks_props.empty())
    {
        STXXL_THROW(std::runtime_error, "config::config", "No disks found in '" << config_path << "' .");
    }
    else
    {
#ifdef STXXL_VERBOSE_DISKS
        for (std::vector<DiskEntry>::const_iterator it =
                 disks_props.begin(); it != disks_props.end();
             it++)
        {
            STXXL_MSG("Disk '" << (*it).path << "' is allocated, space: " <<
                      ((*it).size) / (1024 * 1024) <<
                      " MiB, I/O implementation: " << (*it).io_impl);
        }
#else
        int64 total_size = 0;
        for (std::vector<DiskEntry>::const_iterator it =
                 disks_props.begin(); it != disks_props.end();
             it++)
            total_size += (*it).size;

        STXXL_MSG("" << disks_props.size() << " disks are allocated, total space: " <<
                  (total_size / (1024 * 1024)) <<
                  " MiB");
#endif
    }
}

file * FileCreator::create(const std::string & io_impl,
                                  const std::string & filename,
                                  int options, int disk)
{
    if (io_impl == "syscall")
    {
        ufs_file_base * result = new syscall_file(filename, options, disk);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_syscall")
    {
        fileperblock_file<syscall_file> * result = new fileperblock_file<syscall_file>(filename, options, disk);
        result->lock();
        return result;
    }
#ifndef BOOST_MSVC
    else if (io_impl == "mmap")
    {
        ufs_file_base * result = new mmap_file(filename, options, disk);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_mmap")
    {
        fileperblock_file<mmap_file> * result = new fileperblock_file<mmap_file>(filename, options, disk);
        result->lock();
        return result;
    }
    else if (io_impl == "simdisk")
    {
        ufs_file_base * result = new sim_disk_file(filename, options, disk);
        result->lock();
        return result;
    }
#else
    else if (io_impl == "wincall")
    {
        wfs_file_base * result = new wincall_file(filename, options, disk);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_wincall")
    {
        fileperblock_file<wincall_file> * result = new fileperblock_file<wincall_file>(filename, options, disk);
        result->lock();
        return result;
    }
#endif
#ifdef STXXL_BOOST_CONFIG
    else if (io_impl == "boostfd")
    {
        boostfd_file* result = new boostfd_file(filename, options, disk);
        result->lock();
        return result;
    }
    else if (io_impl == "fileperblock_boostfd")
    {
        fileperblock_file<boostfd_file> * result = new fileperblock_file<boostfd_file>(filename, options, disk);
        result->lock();
        return result;
    }
#endif
    else if (io_impl == "memory")
    {
        mem_file * result = new mem_file(disk);
        result->lock();
        return result;
    }
    else if (io_impl == "wbtl")
    {
        ufs_file_base * backend = new syscall_file(filename, options, disk + 1000); // FIXME: ID
        wbtl_file * result = new stxxl::wbtl_file(backend, 16 * 1024 * 1024, 2, disk);
        result->lock();
        return result;
    }

    STXXL_THROW(std::runtime_error, "FileCreator::create", "Unsupported disk I/O implementation " <<
                io_impl << " .");

    return NULL;
}

block_manager::block_manager()
{
    FileCreator fc;
    debugmon::get_instance();
    config * cfg = config::get_instance();

    ndisks = cfg->disks_number();
    disk_allocators = new DiskAllocator *[ndisks];
    disk_files = new file *[ndisks];

    for (unsigned i = 0; i < ndisks; i++)
    {
        disk_files[i] = fc.create(cfg->disk_io_impl(i),
                                  cfg->disk_path(i),
                                  file::CREAT | file::RDWR | file::DIRECT, i);
        disk_files[i]->set_size(cfg->disk_size(i));
        disk_allocators[i] = new DiskAllocator(cfg->disk_size(i));
    }
}

block_manager::~block_manager()
{
    STXXL_VERBOSE1("Block manager destructor");
    for (unsigned i = 0; i < ndisks; i++)
    {
        delete disk_allocators[i];
        delete disk_files[i];
    }
    delete[] disk_allocators;
    delete[] disk_files;
}

__STXXL_END_NAMESPACE
// vim: et:ts=4:sw=4
