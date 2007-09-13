# This -*- Makefile -*- is intended for processing with GNU make.

# Change this file according to your paths.

# Instead of modifying this file, you could also set your modified variables
# in make.settings.local (needs to be created first).
-include $(dir $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))make.settings.local


USE_BOOST	?= no	# set 'yes' to use Boost libraries or 'no' to not use Boost libraries
USE_LINUX	?= yes	# set 'yes' if you run Linux, 'no' otherwise
USE_MCSTL	?= no	# will be overriden from main Makefile
USE_ICPC	?= no	# will be overriden from main Makefile

STXXL_ROOT	?= $(HOME)/work/stxxl

ifeq ($(strip $(USE_ICPC)),yes)
COMPILER	?= icpc
OPENMPFLAG	?= -openmp
ICPC_MCSTL_CPPFLAGS	?= -gcc-version=420 -cxxlib=$(FAKEGCC)
endif

ifeq ($(strip $(USE_MCSTL)),yes)
COMPILER	?= g++-4.2
OPENMPFLAG	?= -fopenmp
LIBNAME		?= mcstxxl
# the base directory of your MCSTL installation
MCSTL_BASE	?= $(HOME)/work/mcstl
# mcstl branch, leave empty for default branch (e.g. if installed from release tarball)
MCSTL_BRANCH	?= #branches/standalone
# only set the following variables if autodetection does not work:
#MCSTL_ROOT		?= $(MCSTL_BASE)/$(MCSTL_BRANCH)
#MCSTL_ORIGINAL_INC_CXX	?= /path/to/your/$(COMPILER)'s/include/c++
#MCSTL_ORIGINALS	?= /where/you/put/the/original/symlink
endif

BOOST_INCLUDE	?= /usr/include/boost

COMPILER	?= g++
LINKER		?= $(COMPILER)
OPT		?= -O3 # compiler optimization level
WARNINGS	?= -Wall
DEBUG		?= # put here -g option to include the debug information into the binaries

LIBNAME		?= stxxl


#### TROUBLESHOOTING
#
# For automatical checking of order of the output elements in
# the sorters: stxxl::stream::sort, stxxl::stream::merge_runs,
# stxxl::sort, and stxxl::ksort use
#
#STXXL_SPECIFIC	+= -DSTXXL_CHECK_ORDER_IN_SORTS
#
# If your program aborts with message "read/write: wrong parameter"
# or "Invalid argument"
# this could be that your kernel does not support direct I/O
# then try to set it off recompiling the libs and your code with option
#
#STXXL_SPECIFIC	+= -DSTXXL_DIRECT_IO_OFF
#
# But for the best performance it is strongly recommended
# to reconfigure the kernel for the support of the direct I/O.
#
# FIXME: documentation needed
#
#STXXL_SPECIFIC += -DSTXXL_DEBUGMON


#### You usually shouldn't need to change the sections below #####


#### STXXL OPTIONS ###############################################

STXXL_SPECIFIC	+= \
	$(CPPFLAGS_ARCH) \
	-DSORT_OPTIMAL_PREFETCHING \
	-DUSE_MALLOC_LOCK \
	-DCOUNT_WAIT_TIME \
	-I$(strip $(STXXL_ROOT))/include \
	-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
	$(POSIX_MEMALIGN) $(XOPEN_SOURCE)

STXXL_LDLIBS	+= -L$(strip $(STXXL_ROOT))/lib -l$(LIBNAME) -lpthread

STXXL_LIBDEPS	+= $(strip $(STXXL_ROOT))/lib/lib$(LIBNAME).$(LIBEXT)

UNAME_M		:= $(shell uname -m)
CPPFLAGS_ARCH	+= $(CPPFLAGS_$(UNAME_M))
CPPFLAGS_i686	?= -march=i686

##################################################################


#### ICPC OPTIONS ################################################

ifeq ($(strip $(USE_ICPC)),yes)

endif

##################################################################


#### MCSTL OPTIONS ###############################################

ifeq ($(strip $(USE_MCSTL)),yes)

MCSTL_ROOT		?= $(strip $(MCSTL_BASE))$(if $(strip $(MCSTL_BRANCH)),/$(strip $(MCSTL_BRANCH)))

ifeq (,$(strip $(wildcard $(MCSTL_ROOT)/c++/mcstl.h)))
$(error ERROR: could not find a MCSTL installation in MCSTL_ROOT=$(MCSTL_ROOT))
endif

ifeq ($(strip $(USE_ICPC)),yes)
MCSTL_CPPFLAGS		+= $(ICPC_MCSTL_CPPFLAGS)
endif

MCSTL_CPPFLAGS		+= $(OPENMPFLAG) -D__MCSTL__ $(MCSTL_INCLUDES_PREPEND) -I$(MCSTL_ROOT)/c++
MCSTL_LDFLAGS		+= $(OPENMPFLAG)

ifeq (,$(strip $(wildcard $(MCSTL_ROOT)/c++/bits/stl_algo.h)))
# not from libstdc++ branch, need to find the correct original symlink

ifneq (,$(strip $(wildcard $(MCSTL_ROOT)/originals)))
MCSTL_ORIG_BASE		?= $(MCSTL_ROOT)
endif
MCSTL_ORIG_BASE		?= $(MCSTL_BASE)

# find a KEY=VALUE element in WORDS and return VALUE
# usage: $(call get_value,KEY,WORDS)
get_value		 = $(subst $(1)=,,$(filter $(1)=%,$(2)))
empty			 =#
space			 = $(empty) $(empty)
gcc_version_result	:= $(shell $(COMPILER) -v 2>&1)
gcc_version		 = $(call get_value,THE_GCC_VERSION,$(subst gcc$(space)version$(space),THE_GCC_VERSION=,$(gcc_version_result)))
gcc_prefix		 = $(call get_value,--prefix,$(gcc_version_result))
gcc_gxx_incdir		 = $(call get_value,--with-gxx-include-dir,$(gcc_version_result))
MCSTL_ORIGINAL_INC_CXX	?= $(firstword $(wildcard $(gcc_gxx_incdir) $(gcc_prefix)/include/c++/$(gcc_version)) $(cxx_incdir_from_compile))
ifeq (,$(strip $(MCSTL_ORIGINAL_INC_CXX)))
# do a test compilation, generate dependencies and parse the header paths
compile_test_vector_deps:= $(shell echo -e '\043include <vector>' > cxx-header-path-test.cpp; $(COMPILER) -M cxx-header-path-test.cpp 2>/dev/null; $(RM) cxx-header-path-test.cpp)
cxx_incdir_from_compile	 = $(patsubst %/vector,%,$(firstword $(filter %/vector, $(compile_test_vector_deps))))
export cxx_incdir_from_compile
endif
MCSTL_ORIGINALS		?= $(strip $(MCSTL_ORIG_BASE))/originals/$(subst /,_,$(MCSTL_ORIGINAL_INC_CXX))

ifeq (,$(strip $(MCSTL_ORIGINAL_INC_CXX)))
$(error ERROR: could not determine MCSTL_ORIGINAL_INC_CXX, please set this variable manually, it's your compilers ($(COMPILER)) include/c++ path)
endif
ifeq (,$(strip $(wildcard $(MCSTL_ORIGINALS)/original)))
$(error ERROR: your mcstl in $(MCSTL_ORIG_BASE) is not configured properly: $(MCSTL_ORIGINALS)/original does not exist)
endif

MCSTL_CPPFLAGS		+= -I$(MCSTL_ORIGINALS)

else # from libstdc++ branch

MCSTL_INCLUDES_PREPEND	+= $(if $(findstring 4.3,$(COMPILER)),-I$(MCSTL_ROOT)/c++/mod_stl/gcc-4.3)

endif # (not) from libstdc++ branch

endif

##################################################################


#### BOOST OPTIONS ###############################################

BOOST_COMPILER_OPTIONS	 = \
	-DSTXXL_BOOST_TIMESTAMP \
	-DSTXXL_BOOST_CONFIG \
	-DSTXXL_BOOST_FILESYSTEM \
	-DSTXXL_BOOST_THREADS \
	-DSTXXL_BOOST_RANDOM \
	-I$(strip $(BOOST_INCLUDE)) \
	-pthread

BOOST_LIB_COMPILER_SUFFIX	?= 
BOOST_LIB_MT_SUFFIX		?= -mt
BOOST_LINKER_OPTIONS		 = \
	-lboost_thread$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_date_time$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_iostreams$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_filesystem$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX)

##################################################################


#### CPPUNIT OPTIONS ############################################

CPPUNIT_COMPILER_OPTIONS	+=

CPPUNIT_LINKER_OPTIONS		+= -lcppunit -ldl

##################################################################


#### DEPENDENCIES ################################################

HEADER_FILES_BITS	+= namespace.h version.h

HEADER_FILES_COMMON	+= aligned_alloc.h mutex.h rand.h semaphore.h state.h
HEADER_FILES_COMMON	+= timer.h utils.h gprof.h rwlock.h simple_vector.h
HEADER_FILES_COMMON	+= switch.h tmeta.h log.h exceptions.h debug.h tuple.h
HEADER_FILES_COMMON	+= types.h utils_ledasm.h settings.h

HEADER_FILES_IO		+= completion_handler.h io.h iobase.h iostats.h
HEADER_FILES_IO		+= mmap_file.h simdisk_file.h syscall_file.h
HEADER_FILES_IO		+= ufs_file.h wincall_file.h wfs_file.h boostfd_file.h

HEADER_FILES_MNG	+= adaptor.h async_schedule.h block_prefetcher.h
HEADER_FILES_MNG	+= buf_istream.h buf_ostream.h buf_writer.h mng.h
HEADER_FILES_MNG	+= write_pool.h prefetch_pool.h

HEADER_FILES_CONTAINERS	+= pager.h stack.h vector.h priority_queue.h queue.h
HEADER_FILES_CONTAINERS	+= map.h deque.h

HEADER_FILES_CONTAINERS_BTREE	+= btree.h iterator_map.h leaf.h node_cache.h
HEADER_FILES_CONTAINERS_BTREE	+= root_node.h node.h btree_pager.h iterator.h

HEADER_FILES_ALGO	+= adaptor.h inmemsort.h intksort.h run_cursor.h sort.h
HEADER_FILES_ALGO	+= async_schedule.h interleaved_alloc.h ksort.h
HEADER_FILES_ALGO	+= losertree.h scan.h stable_ksort.h random_shuffle.h

HEADER_FILES_STREAM	+= stream.h sort_stream.h

HEADER_FILES_UTILS	+= malloc.h

###################################################################


#### MISC #########################################################

DEPEXT	 = $(LIBNAME).d # extension of dependency files
OBJEXT	 = $(LIBNAME).o	# extension of object files
LIBEXT	 = a		# static library file extension
EXEEXT	 = $(LIBNAME).bin # executable file extension
RM	 = rm -f	# remove file command
LIBGEN	 = ar cr	# library generation
OUT	 = -o		# output file option for the compiler and linker

d	?= $(strip $(DEPEXT))
o	?= $(strip $(OBJEXT))
bin	?= $(strip $(EXEEXT))

###################################################################


#### COMPILE/LINK RULES ###########################################

%.$o: %.cpp ../make.settings
	@$(RM) $*.$d
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -MD -MF $*.$dT -c $(OUTPUT_OPTION) $< && mv $*.$dT $*.$d

LINK_STXXL	 = $(LINKER) $1 $(STXXL_LINKER_OPTIONS) -o $@

%.$(bin): %.$o $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $<)

###################################################################


STXXL_COMPILER_OPTIONS	+= $(STXXL_SPECIFIC)
STXXL_COMPILER_OPTIONS	+= $(OPT) $(DEBUG) $(WARNINGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS)

ifeq ($(strip $(USE_MCSTL)),yes)
STXXL_COMPILER_OPTIONS	+= $(MCSTL_CPPFLAGS)
STXXL_LINKER_OPTIONS	+= $(MCSTL_LDFLAGS)
endif

ifeq ($(strip $(USE_BOOST)),yes)
STXXL_COMPILER_OPTIONS	+= $(BOOST_COMPILER_OPTIONS)
STXXL_LINKER_OPTIONS	+= $(BOOST_LINKER_OPTIONS)
endif

STXXL_COMPILER_OPTIONS	+= $(CPPFLAGS)

# vim: syn=make
