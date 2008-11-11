/***************************************************************************
 *  io/simdisk_file.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2002-2003 Roman Dementiev <dementiev@mpi-sb.mpg.de>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

#include <stxxl/bits/io/simdisk_file.h>

#ifndef BOOST_MSVC
// mmap call does not exist in Windows


__STXXL_BEGIN_NAMESPACE


void DiskGeometry::add_zone(int & first_cyl, int last_cyl,
                            int sec_per_track, int & first_sect)
{
    double rate =
        nsurfaces * sec_per_track * bytes_per_sector /
        ((nsurfaces - 1) * head_switch_time +
         cyl_switch_time +
         nsurfaces * revolution_time);
    int sectors =
        (last_cyl - first_cyl +
         1) * nsurfaces * sec_per_track;
    zones.insert(Zone(first_sect, sectors, rate));
    first_sect += sectors;
    first_cyl = last_cyl + 1;
}

double DiskGeometry::get_delay(stxxl::int64 /*offset*/, size_t size)                   // returns delay in s
{
    /*

       int first_sect = offset / bytes_per_sector;
       int last_sect = (offset + size) / bytes_per_sector;
       int sectors = size / bytes_per_sector;
       double delay =
            cmd_ovh + seek_time + rot_latency +
            double (bytes_per_sector) /
            double (interface_speed);

       std::set < Zone, ZoneCmp >::iterator zone =
            zones.lower_bound (first_sect);
       // cout << __PRETTY_FUNCTION__ << " " << (*zone).first_sector << endl;
       while (1)
       {
            int from_this_zone =
                    last_sect - ((*zone).first_sector +
                                 (*zone).sectors);
            if (from_this_zone <= 0)
            {
                    delay += sectors * bytes_per_sector /
                            ((*zone).sustained_data_rate);
                    break;
            }
            else
            {
                    delay += from_this_zone *
                            bytes_per_sector /
                            ((*zone).sustained_data_rate);
                    zone++;
                    stxxl_nassert (zone == zones.end ());
                    sectors -= from_this_zone;
            }
       }

       return delay;

     */
    return double(size) / double(AVERAGE_SPEED);
}


IC35L080AVVA07::IC35L080AVVA07()
{
    std::cout << "Creating IBM 120GXP IC35L080AVVA07" <<
    std::endl;

    nsurfaces = 4;
    bytes_per_sector = 512;
    cmd_ovh = 0.0002;                           // in s
    seek_time = 0.0082;                         // in s
    rot_latency = 0.00417;                      // in s
    head_switch_time = 0.0015;                  // in s
    cyl_switch_time = 0.002;                    // in s
    revolution_time = 0.0083;                   // in s
    interface_speed = 100000000;                // in byte/s

    int first_sect = 0;
    int last_cyl = 0;
    add_zone(last_cyl, 1938, 928, first_sect);
    add_zone(last_cyl, 3756, 921, first_sect);
    add_zone(last_cyl, 5564, 896, first_sect);
    add_zone(last_cyl, 7687, 896, first_sect);
    add_zone(last_cyl, 9526, 888, first_sect);
    add_zone(last_cyl, 11334, 883, first_sect);
    add_zone(last_cyl, 13331, 864, first_sect);
    add_zone(last_cyl, 15128, 850, first_sect);
    add_zone(last_cyl, 16925, 840, first_sect);
    add_zone(last_cyl, 18922, 822, first_sect);
    add_zone(last_cyl, 20709, 806, first_sect);
    add_zone(last_cyl, 22601, 792, first_sect);
    add_zone(last_cyl, 24138, 787, first_sect);
    add_zone(last_cyl, 26024, 768, first_sect);
    add_zone(last_cyl, 27652, 752, first_sect);
    add_zone(last_cyl, 29501, 740, first_sect);
    add_zone(last_cyl, 31234, 725, first_sect);
    add_zone(last_cyl, 33009, 698, first_sect);
    add_zone(last_cyl, 34784, 691, first_sect);
    add_zone(last_cyl, 36609, 672, first_sect);
    add_zone(last_cyl, 38374, 648, first_sect);
    add_zone(last_cyl, 40139, 630, first_sect);
    add_zone(last_cyl, 41904, 614, first_sect);
    add_zone(last_cyl, 43519, 595, first_sect);
    add_zone(last_cyl, 45250, 576, first_sect);
    add_zone(last_cyl, 47004, 552, first_sect);
    add_zone(last_cyl, 48758, 533, first_sect);
    add_zone(last_cyl, 50491, 512, first_sect);
    add_zone(last_cyl, 52256, 493, first_sect);
    add_zone(last_cyl, 54010, 471, first_sect);
    add_zone(last_cyl, 55571, 448, first_sect);

    /*
     * set<Zone,ZoneCmp>::iterator it=zones.begin();
     * int i=0;
     * for(;it!=zones.end();it++,i++)
     * {
     * //const int block_size = 128*3*1024* 4; // one cylinder
     *
     * cout << "Zone " << i << " first sector: " << (*it).first_sector;
     * cout << " sectors: " << (*it).sectors << " sustained rate: " ;
     * cout << (*it).sustained_data_rate/1000000 << " Mb/s"  << endl;
     *
     * }
     *
     *
     * cout << "Last sector     : " << first_sect <<endl;
     * cout << "Approx. capacity: " << (first_sect/1000000)*bytes_per_sector << " Mb" << endl;
     */

    std::cout << "Transfer 16 Mb from zone 0 : " <<
    get_delay(0,
              16 * 1024 *
              1024) << " s" << std::endl;
    std::cout << "Transfer 16 Mb from zone 30: " <<
    get_delay(stxxl::int64(158204036) *
              stxxl::int64(bytes_per_sector),
              16 * 1024 *
              1024) << " s" << std::endl;
}

////////////////////////////////////////////////////////////////////////////

void sim_disk_request::serve()
{
    //      static_cast<syscall_file*>(file_)->set_size(offset+bytes);
    double op_start = timestamp();
    stats * iostats = stats::get_instance();
    if (type == READ)
    {
        iostats->read_started(bytes);
    }
    else
    {
        iostats->write_started(bytes);
    }

    try {
        void * mem =
            mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, static_cast<sim_disk_file *>(file_)->get_file_des(), offset);
        if (mem == MAP_FAILED)
        {
            STXXL_FORMAT_ERROR_MSG(msg, "Mapping failed. " <<
                                   "Page size: " << sysconf(_SC_PAGESIZE) << " offset modulo page size " <<
                                   (offset % sysconf(_SC_PAGESIZE)));

            error_occured(msg.str());
        }
        else if (mem == 0)
        {
            stxxl_function_error(io_error);
        }
        else
        {
            if (type == READ)
            {
                memcpy(buffer, mem, bytes);
                stxxl_check_ge_0(munmap((char *)mem, bytes), io_error);
            } else {
                memcpy(mem, buffer, bytes);
                stxxl_check_ge_0(munmap((char *)mem, bytes), io_error);
            }
        }

        double delay =
            (static_cast<sim_disk_file *>(file_))->get_delay(offset, bytes);


        delay = delay - timestamp() + op_start;

        assert(delay > 0.0);

        int seconds_to_wait = static_cast<int>(floor(delay));
        if (seconds_to_wait)
            sleep(seconds_to_wait);


        usleep((unsigned long)((delay - seconds_to_wait) * 1000000.));
    }
    catch (const io_error & ex)
    {
        error_occured(ex.what());
    }

    if (type == READ)
    {
        iostats->read_finished();
    }
    else
    {
        iostats->write_finished();
    }

    _state.set_to(DONE);

    {
        scoped_mutex_lock Lock(waiters_mutex);

        // << notification >>
        for (std::set<onoff_switch *>::iterator i =
                 waiters.begin(); i != waiters.end(); i++)
            (*i)->on();
    }

    completed();
    _state.set_to(READY2DIE);
}

////////////////////////////////////////////////////////////////////////////

void sim_disk_file::set_size(stxxl::int64 newsize)
{
    if (newsize > size())
    {
        stxxl_check_ge_0(::lseek(file_des, newsize - 1, SEEK_SET), io_error);
        stxxl_check_ge_0(::write(file_des, "", 1), io_error);
    }
}

request_ptr sim_disk_file::aread(void * buffer, stxxl::int64 pos, size_t bytes,
                                 completion_handler on_cmpl)
{
    request_ptr req = new sim_disk_request(this, buffer, pos, bytes,
                                           request::READ, on_cmpl);
    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_readreq(req, get_id());

    return req;
}

request_ptr sim_disk_file::awrite(
    void * buffer, stxxl::int64 pos, size_t bytes,
    completion_handler on_cmpl)
{
    request_ptr req = new sim_disk_request(this, buffer, pos, bytes,
                                           request::WRITE, on_cmpl);

    if (!req.get())
        stxxl_function_error(io_error);

    disk_queues::get_instance()->add_writereq(req, get_id());

    return req;
}

__STXXL_END_NAMESPACE

#endif // #ifndef BOOST_MSVC
