# This -*- Makefile -*- is intended for processing with GNU make.

############################################################################
#  make.settings.gnu
#
#  Part of the STXXL. See http://stxxl.sourceforge.net
#
#  Copyright (C) 2002-2007 Roman Dementiev <dementiev@mpi-sb.mpg.de>
#  Copyright (C) 2006-2008 Johannes Singler <singler@ira.uka.de>
#  Copyright (C) 2007-2008 Andreas Beckmann <beckmann@cs.uni-frankfurt.de>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################


TOPDIR	?= $(error TOPDIR not defined) # DO NOT CHANGE! This is set elsewhere.

# Change this file according to your paths.

# Instead of modifying this file, you could also set your modified variables
# in make.settings.local (needs to be created first).
-include $(TOPDIR)/make.settings.local


USE_BOOST	?= no	# set 'yes' to use Boost libraries or 'no' to not use Boost libraries
USE_MACOSX	?= no	# set 'yes' if you run Mac OS X, 'no' otherwise
USE_PMODE	?= no	# will be overridden from main Makefile
USE_MCSTL	?= no	# will be overridden from main Makefile
USE_ICPC	?= no	# will be overridden from main Makefile

STXXL_ROOT	?= $(HOME)/work/stxxl

ifeq ($(strip $(USE_ICPC)),yes)
COMPILER_ICPC	?= icpc
COMPILER	?= $(COMPILER_ICPC)
#ICPC_GCC	?= gcc-x.y    # override the gcc/g++ used to find headers and libraries
WARNINGS	?= -Wall -w1 -openmp-report0 -vec-report0
endif

ifeq ($(strip $(USE_PMODE)),yes)
COMPILER_GCC	?= g++-4.3.x
LIBNAME		?= pmstxxl
endif

ifeq ($(strip $(USE_MCSTL)),yes)
COMPILER_GCC	?= g++-4.2.3
LIBNAME		?= mcstxxl
# the root directory of your MCSTL installation
MCSTL_ROOT	?= $(HOME)/work/mcstl
endif

#BOOST_ROOT	?= /usr/local/boost-1.34.1

COMPILER_GCC	?= g++
COMPILER	?= $(COMPILER_GCC)
LINKER		?= $(COMPILER)
OPT_LEVEL	?= 3
OPT		?= -O$(OPT_LEVEL) # compiler optimization level
WARNINGS	?= -W -Wall
DEBUG		?= # put here -g option to include the debug information into the binaries

LIBNAME		?= stxxl

# Hint: for g++-4.3 with c++0x support, enable the following:
#STXXL_SPECIFIC	+= -std=c++0x


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


#### MACOSX CONFIGURATION ########################################

ifeq ($(strip $(USE_MACOSX)),yes)

PTHREAD_FLAG	?=

GET_FILE_ID	?= stat -L -f '%d:%i' $1

endif

##################################################################


#### LINUX (DEFAULT) CONFIGURATION ###############################

PTHREAD_FLAG	?= -pthread

# get a unique identifier for a file or directory,
# e.g. device number + inode number
GET_FILE_ID	?= stat -L -c '%d:%i' $1

##################################################################


#### STXXL CONFIGURATION #########################################

# check, whether stxxl has been configured
ifeq (,$(strip $(wildcard $(STXXL_ROOT)/include/stxxl.h)))
$(warning *** WARNING: STXXL has not been configured correctly)
ifeq (,$(strip $(wildcard $(CURDIR)/make.settings.local)))
ifneq (,$(strip $(wildcard $(CURDIR)/include/stxxl.h)))
$(warning *** WARNING: trying autoconfiguration for STXXL_ROOT=$(CURDIR:$(HOME)%=$$(HOME)%))
$(warning *** WARNING: you did not have a make.settings.local file -- creating ...)
$(shell echo 'STXXL_ROOT	 = $(CURDIR:$(HOME)%=$$(HOME)%)' >> $(CURDIR)/make.settings.local)
MCSTL_ROOT	?= $(HOME)/work/mcstl
$(shell echo -e '\043MCSTL_ROOT	 = $(MCSTL_ROOT:$(HOME)%=$$(HOME)%)' >> $(CURDIR)/make.settings.local)
$(shell echo -e '\043COMPILER_GCC	 = g++-4.2.3' >> $(CURDIR)/make.settings.local)
$(shell echo -e '\043COMPILER_ICPC	 = icpc' >> $(CURDIR)/make.settings.local)
ifeq (Darwin,$(strip $(shell uname)))
$(shell echo -e 'USE_MACOSX	 = yes' >> $(CURDIR)/make.settings.local)
else
$(shell echo -e '\043USE_MACOSX	 = no' >> $(CURDIR)/make.settings.local)
endif
$(error ERROR: Please check make.settings.local and try again)
endif
else
$(warning *** WARNING: Please check make.settings.local)
endif
$(error ERROR: could not find a STXXL installation in STXXL_ROOT=$(STXXL_ROOT))
endif

# in the top dir, check whether STXXL_ROOT points to ourselves
ifneq (,$(wildcard make.settings))
stat1	 = $(shell $(call GET_FILE_ID, ./))
stat2	 = $(shell $(call GET_FILE_ID, $(STXXL_ROOT)/))

ifneq ($(stat1),$(stat2))
$(error ERROR: STXXL_ROOT=$(STXXL_ROOT) points to a different STXXL installation)
endif
endif

##################################################################


#### STXXL OPTIONS ###############################################

STXXL_SPECIFIC	+= \
	$(PTHREAD_FLAG) \
	$(CPPFLAGS_ARCH) \
	-DSORT_OPTIMAL_PREFETCHING \
	-DUSE_MALLOC_LOCK \
	-DCOUNT_WAIT_TIME \
	-I$(strip $(STXXL_ROOT))/include \
	-include stxxl/bits/defines.h \
	-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE \
	$(POSIX_MEMALIGN) $(XOPEN_SOURCE)

STXXL_LDFLAGS	+= $(PTHREAD_FLAG)
STXXL_LDLIBS	+= -L$(strip $(STXXL_ROOT))/lib -l$(LIBNAME)

STXXL_LIBDEPS	+= $(strip $(STXXL_ROOT))/lib/lib$(LIBNAME).$(LIBEXT)

UNAME_M		:= $(shell uname -m)
CPPFLAGS_ARCH	+= $(CPPFLAGS_$(UNAME_M))
CPPFLAGS_i686	?= -march=i686

##################################################################


#### ICPC OPTIONS ################################################

ifeq ($(strip $(USE_ICPC)),yes)

OPENMPFLAG	?= -openmp

ICPC_CPPFLAGS	+= $(if $(ICPC_GCC),-gcc-name=$(strip $(ICPC_GCC)))
ICPC_LDFLAGS	+= $(if $(ICPC_GCC),-gcc-name=$(strip $(ICPC_GCC)))

STXXL_SPECIFIC	+= -include bits/intel_compatibility.h

endif

##################################################################


#### PARALLEL_MODE OPTIONS ###############################################

ifeq ($(strip $(USE_PMODE)),yes)

OPENMPFLAG			?= -fopenmp

PARALLEL_MODE_CPPFLAGS		+= $(OPENMPFLAG) -D_GLIBCXX_PARALLEL
PARALLEL_MODE_LDFLAGS		+= $(OPENMPFLAG)

endif

##################################################################


#### MCSTL OPTIONS ###############################################

ifeq ($(strip $(USE_MCSTL)),yes)

OPENMPFLAG	?= -fopenmp

ifeq (,$(strip $(wildcard $(strip $(MCSTL_ROOT))/c++/mcstl.h)))
$(error ERROR: could not find a MCSTL installation in MCSTL_ROOT=$(MCSTL_ROOT))
endif

MCSTL_CPPFLAGS		+= $(OPENMPFLAG) -D__MCSTL__ -I$(MCSTL_ROOT)/c++
MCSTL_LDFLAGS		+= $(OPENMPFLAG)

endif

##################################################################


#### BOOST OPTIONS ###############################################

BOOST_COMPILER_OPTIONS		+= $(if $(strip $(BOOST_ROOT)),-I$(strip $(BOOST_ROOT)))
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_CONFIG
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_FILESYSTEM
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_RANDOM
BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_THREADS
#BOOST_COMPILER_OPTIONS		+= -DSTXXL_BOOST_TIMESTAMP   # probably less efficient than gettimeofday()

BOOST_LIB_COMPILER_SUFFIX	?= 
BOOST_LIB_MT_SUFFIX		?= -mt
BOOST_LINKER_OPTIONS		 = \
	-lboost_thread$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_date_time$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_iostreams$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX) \
	-lboost_filesystem$(BOOST_LIB_COMPILER_SUFFIX)$(BOOST_LIB_MT_SUFFIX)

##################################################################


#### CPPUNIT OPTIONS #############################################

CPPUNIT_COMPILER_OPTIONS	+=

CPPUNIT_LINKER_OPTIONS		+= -lcppunit -ldl

##################################################################


#### DEPENDENCIES ################################################

HEADER_FILES_BITS	+= namespace.h noncopyable.h version.h
HEADER_FILES_BITS	+= compat_hash_map.h compat_hash_set.h
HEADER_FILES_BITS	+= compat_auto_ptr.h parallel.h singleton.h defines.h

HEADER_FILES_COMMON	+= aligned_alloc.h mutex.h rand.h semaphore.h state.h
HEADER_FILES_COMMON	+= timer.h utils.h simple_vector.h
HEADER_FILES_COMMON	+= switch.h tmeta.h log.h exceptions.h debug.h tuple.h
HEADER_FILES_COMMON	+= types.h settings.h seed.h is_sorted.h

HEADER_FILES_IO		+= request.h
HEADER_FILES_IO		+= disk_queues.h
HEADER_FILES_IO		+= completion_handler.h io.h iobase.h iostats.h
HEADER_FILES_IO		+= basic_waiters_request.h basic_request_state.h
HEADER_FILES_IO		+= mmap_file.h simdisk_file.h syscall_file.h
HEADER_FILES_IO		+= ufs_file.h wincall_file.h wfs_file.h boostfd_file.h
HEADER_FILES_IO		+= mem_file.h

HEADER_FILES_MNG	+= adaptor.h block_prefetcher.h
HEADER_FILES_MNG	+= buf_istream.h buf_ostream.h buf_writer.h mng.h
HEADER_FILES_MNG	+= write_pool.h prefetch_pool.h
HEADER_FILES_MNG	+= block_alloc_interleaved.h

HEADER_FILES_CONTAINERS	+= pager.h stack.h vector.h priority_queue.h queue.h
HEADER_FILES_CONTAINERS	+= map.h deque.h

HEADER_FILES_CONTAINERS_BTREE	+= btree.h iterator_map.h leaf.h node_cache.h
HEADER_FILES_CONTAINERS_BTREE	+= root_node.h node.h btree_pager.h iterator.h

HEADER_FILES_ALGO	+= adaptor.h inmemsort.h intksort.h run_cursor.h sort.h
HEADER_FILES_ALGO	+= async_schedule.h ksort.h
HEADER_FILES_ALGO	+= losertree.h scan.h stable_ksort.h random_shuffle.h

HEADER_FILES_STREAM	+= stream.h sort_stream.h

HEADER_FILES_UTILS	+= malloc.h

###################################################################


#### MISC #########################################################

DEPEXT	 = $(LIBNAME).d # extension of dependency files
OBJEXT	 = $(LIBNAME).o	# extension of object files
IIEXT	 = $(LIBNAME).ii
LIBEXT	 = a		# static library file extension
EXEEXT	 = $(LIBNAME).bin # executable file extension
RM	 = rm -f	# remove file command
LIBGEN	 = ar cr	# library generation
OUT	 = -o		# output file option for the compiler and linker

d	?= $(strip $(DEPEXT))
o	?= $(strip $(OBJEXT))
ii	?= $(strip $(IIEXT))
bin	?= $(strip $(EXEEXT))

###################################################################


#### COMPILE/LINK RULES ###########################################

DEPS_MAKEFILES	:= $(wildcard ../Makefile.subdir.gnu ../make.settings ../make.settings.local GNUmakefile Makefile.local)
%.$o: %.cpp $(DEPS_MAKEFILES)
	@$(RM) $@ $*.$d
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -MD -MF $*.$dT -c $(OUTPUT_OPTION) $< && mv $*.$dT $*.$d

%.$(ii): %.cpp $(DEPS_MAKEFILES)
	$(COMPILER) $(STXXL_COMPILER_OPTIONS) -E $(OUTPUT_OPTION) $<

LINK_STXXL	 = $(LINKER) $1 $(STXXL_LINKER_OPTIONS) -o $@

%.$(bin): %.$o $(STXXL_LIBDEPS)
	$(call LINK_STXXL, $<)


# last resort rules to ignore header files missing due to renames etc.
%.h::
	@echo "MISSING HEADER: '$@' (ignored)"

/%::
	@echo "MISSING FILE:   '$@' (ignored)"

###################################################################


STXXL_SPECIFIC		+= $(STXXL_EXTRA)
WARNINGS		+= $(WARNINGS_EXTRA)

ifeq ($(strip $(USE_ICPC)),yes)
STXXL_CPPFLAGS_CXX	+= $(ICPC_CPPFLAGS)
STXXL_LDLIBS_CXX	+= $(ICPC_LDFLAGS)
endif

STXXL_COMPILER_OPTIONS	+= $(STXXL_CPPFLAGS_CXX)
STXXL_COMPILER_OPTIONS	+= $(STXXL_SPECIFIC)
STXXL_COMPILER_OPTIONS	+= $(OPT) $(DEBUG) $(WARNINGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS_CXX)
STXXL_LINKER_OPTIONS	+= $(DEBUG)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDFLAGS)
STXXL_LINKER_OPTIONS	+= $(STXXL_LDLIBS)

ifeq ($(strip $(USE_PMODE)),yes)
STXXL_COMPILER_OPTIONS	+= $(PARALLEL_MODE_CPPFLAGS)
STXXL_LINKER_OPTIONS	+= $(PARALLEL_MODE_LDFLAGS)
endif

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
