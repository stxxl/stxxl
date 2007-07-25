#ifndef SIMDISK_HEADER
#define SIMDISK_HEADER

/***************************************************************************
 *            simdisk_file.h
 *
 *  Sat Aug 24 23:55:03 2002
 *  Copyright  2002  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/

#include "stxxl/bits/io/ufs_file.h"

#include <cmath>

#ifdef STXXL_BOOST_CONFIG
 #include <boost/config.hpp>
#endif

#ifdef BOOST_MSVC
// mmap call does not exist in Windows
#else

 #include <sys/mman.h>

__STXXL_BEGIN_NAMESPACE

//! \addtogroup fileimpl
//! \{

 #define AVERAGE_SPEED (15 * 1024 * 1024)

class DiskGeometry
{
    struct Zone
    {
        // manufactuted data
        //    int last_cyl;
        //    int sect_per_track;
        // derived data
        int first_sector;
        int sectors;
        double sustained_data_rate;                     // in Mb/s
        inline Zone (int
                     _first_sector) : first_sector (_first_sector)
        { };                    // constructor for zone search

        inline Zone (                   //int _last_cyl,
                                        //int _sect_per_track,
            int _first_sector,
            int _sectors, double _rate) :
            //last_cyl(_last_cyl),
            //       sect_per_track(_sect_per_track) ,

            first_sector (_first_sector),
            sectors (_sectors),
            sustained_data_rate (_rate)
        { };
    };
    struct ZoneCmp
    {
        inline bool operator  ()  (const Zone & a, const Zone & b) const
        {
            return a.first_sector < b.first_sector;
        }
    };

protected:
    int nsurfaces;
    int bytes_per_sector;
    double cmd_ovh;             // in s
    double seek_time;                   // in s
    double rot_latency;                 // in s
    double head_switch_time;                    // in s
    double cyl_switch_time;             // in s
    double revolution_time;             // in s
    double interface_speed;             // in byte/s
    std::set < Zone, ZoneCmp > zones;

    void add_zone (int &first_cyl, int last_cyl,
                   int sec_per_track, int &first_sect);
public:
    inline DiskGeometry ()
    { }
    double get_delay (stxxl::int64 offset, size_t size);                // returns delay in s

    inline ~DiskGeometry ()
    { };
};


class IC35L080AVVA07 : public DiskGeometry              // IBM series 120GXP
{
public:
    IC35L080AVVA07 ();
};

class sim_disk_request;

//! \brief Implementation of disk emulation
//! \remark It is emulation of IBM IC35L080AVVA07 disk's timings
class sim_disk_file : public ufs_file_base, public IC35L080AVVA07
{
public:
    //! \brief constructs file object
    //! \param filename path of file
    //! \attention filename must be resided at memory disk partition
    //! \param mode open mode, see \c stxxl::file::open_modes
    //! \param disk disk(file) identifier
    inline sim_disk_file (const std::string & filename, int mode, int disk) : ufs_file_base (filename, mode, disk)
    {
        std::cout << "Please, make sure that '" << filename <<
        "' is resided on swap memory partition!" <<
        std::endl;
    };
    request_ptr aread(void * buffer, stxxl::int64 pos, size_t bytes,
                      completion_handler on_cmpl);
    request_ptr awrite(void * buffer, stxxl::int64 pos, size_t bytes,
                       completion_handler on_cmpl);
    void set_size (stxxl::int64 newsize);
};


//! \brief Implementation of disk emulation
class sim_disk_request : public ufs_request_base
{
    friend class sim_disk_file;
protected:
    inline sim_disk_request (sim_disk_file * f, void * buf, stxxl::int64 off,
                             size_t b, request_type t,
                             completion_handler on_cmpl) :
        ufs_request_base (f,
                          buf,
                          off,
                          b,
                          t,
                          on_cmpl)
    { };
    void serve ();
public:
    inline const char * io_type ()
    {
        return "simdisk";
    }
private:
    // Following methods are declared but not implemented
    // intentionnaly to forbid their usage
    sim_disk_request(const sim_disk_request &);
    sim_disk_request & operator=(const sim_disk_request &);
    sim_disk_request();
};

//! \}

__STXXL_END_NAMESPACE

#endif

#endif
