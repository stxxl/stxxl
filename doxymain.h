/*! \mainpage Documentation for \<stxxl\> library
 *
 *  \image html logo.png
 *
 * <BR><BR>
 * The core of \c \<stxxl\> is an implementation of the C++
 * standard template library STL for external memory (out-of-core)
 * computations, i.e., \c \<stxxl\> implements containers and algorithms
 * that can process huge volumes of data that only fit on 
 * disks. While the compatibility to the STL supports
 * ease of use and compatibility with existing applications,
 * another design priority is high performance. 
 * Here is a selection of \c \<stxxl\> performance features:
 * - transparent support of multiple disks
 * - variable block lengths
 * - overlapping of I/O and computation
 * - prevention of OS file buffering overhead
 *
 * \section platforms Platforms supported
 * Linux (kernel >= 2.4.18)
 *
 * \section current Current version
 * prerelease 0.2 contains a first implementation of the lower layers
 * (memory management, disk virtualization, prefetching,...).
 * From the higher layers, sorting and basic containers
 * (vectors, stacks) are supported.
 * Currently that sums to about 8000 lines of code.
 *
 * \section soon Coming soon
 * Priority queues and search trees (B+ trees) are the main nontrivial algorithms
 * missing from a complete implementation of STL. 
 * We are also working on a first expansion of STL that allows
 * pipelining several simple algorithms (sorting, filtering,...)
 * to minimize I/O volume. We will also soon have several nontrivial
 * example applications (minimum spanning trees, suffix array construction).
 *
 * Questions concerning use and development of the \c \<stxxl\> library mail to Roman
 *  Dementiev <A href='mailto:dementiev@mpi-sb.mpg.de'>dementiev@mpi-sb.mpg.de</A>.
 *
 * \link installation Installation \endlink
 *
 */


/*!
 * \page installation Installation
 *
 * \section download Download
 *
 * - Download gzipped tar ball from <A href="http://www.mpi-sb.mpg.de/~rdementi/files/stxxl.tgz">here</A>. 
 * - Unpack in some directory executing: \c tar \c zfxv \c stxxl.tgz ,
 * - Change to \c stxxl directory: \c cd \c stxxl ,
 * - Change file \c compiler.make according to your system configuration
 *   - name of your make program
 *   - name of your compiler
 *   - \c \<stxxl\> root directory ( \c directory_where_you_unpacked_the_tar_ball/stxxl )
 * - Run: \c make \c lib
 * - Run: \c make \c tests (optional, if you want to run some test programs)
 * 
 * In your makefiles of programs that will use \c \<stxxl\> you should include \c compiler.make
 * file (add 'include ../compiler.make' line) because it contains a useful variable (STXXL_VARS) 
 * that includes all compiler difinitions and library paths you need to compile an \c \<stxxl\> program.
 *
 * Before you try to run one of the \c \<stxxl\> examples (or your \c \<stxxl\> program) you must configure your disk's 
 * space that will be used as external memory for the library. See next section.
 *
 * \section space Disk space
 *
 * To achieve the best performance with \c \<stxxl\> library you should devote to it separate disk or disks. 
 * These disks should be used by the library only.
 * Since \c \<stxxl\> is developed to exploit disk parallelism, performance of your 
 * external memory application will increase if number of your disks more than one. But number of disks your 
 * application will benefit from depends on how I/O bound is it. With modern disk bandwidths
 * about 40-60 MB/s most of applications are I/O bound for one disk. This means that if you add another disk
 * the running time will be halved. Adding more disks might also increase performance significantly.
 *
 * \section filesystem Recommended file system
 * 
 * Our library take benefit of direct user memory - disk transfers (direct access) which involve
 * no superfluous copy. This method has some desadvantages when accessing files on \c ext2 partitions. 
 * Namely one requires one byte of internal memory per each accessed kilobyte of file space. For external 
 * memory applications with large inputs this could be not proper. Therefore we recommend you to use 
 * \c xfs file system <A href="http://oss.sgi.com/projects/xfs/">link</A> which does not have such overhead but
 * gives the same read and write performance. Note that file creation speed of \c xfs is slow, that is why disk
 * files must be precreated.
 * 
 * \section configuration Disk configuration file
 * 
 * You must define disk configuration for a \c \<stxxl\> program in a file named \c '.stxxl' that must reside 
 * in the same directory you execute the program. You can change default file name for the configuration 
 * file setting enviroment variable \c STXXLCFG . 
 * 
 *
 * Each line of the configuration file describes a disk. Format of the disk 
 * description is the following format:<BR>
 * \c disk=full_disk_filename,capacity,access_method
 *
 * Description of the parameters:
 * - \c full_disk_filename : full disk filename. In order to access disks \<stxxl\> uses file 
 * access methods. Each disk is respresented as a file. If you have disk that is mapped in unix
 * to the path /mnt/disk0/, then correct value for the \c full_disk_filename would be
 * \c /mnt/disk0/some_file_name ,
 * - \c capacity : maximum capacity of the disk in megabytes
 * - \c access_method : \c \<stxxl\> has number of different file access implementations, choose one of them:
 *   - \c syscall uses \c read and \c write system calls which directly perform disk transfers directly 
 *   on user memory pages without superfluous copy (currently the fastest method)
 *   - \c mmap : performs disks transfers using \c mmap and \c munmap system calls
 *   - \c simdisk : simulates timings of the IBM IC35L080AVVA07 disk, full_disk_filename must point 
 *     to a file on a RAM disk partition with sufficient space
 * 
 * \section excreation Formatting external memory files
 *
 * In order to get the maximum performance one should format disk files described in the configuration file,
 * before running \c \<stxxl\> applications.
 *
 * Format utility is included into set of \c \<stxxl\> utilities ( \c utils/createdisks ). Run this utility
 * for each disk described in your disk configuration file.
 *
 * 
 * */
