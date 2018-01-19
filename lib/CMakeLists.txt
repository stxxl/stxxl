############################################################################
#  lib/CMakeLists.txt
#
#  CMake file for libstxxl.a/so/lib
#
#  Part of the STXXL. See http://stxxl.org
#
#  Copyright (C) 2013 Timo Bingmann <tb@panthema.net>
#
#  Distributed under the Boost Software License, Version 1.0.
#  (See accompanying file LICENSE_1_0.txt or copy at
#  http://www.boost.org/LICENSE_1_0.txt)
############################################################################

set(LIBSTXXL_SOURCES

  common/rand.cpp
  common/seed.cpp
  common/version.cpp

  )

if(USE_MALLOC_COUNT)
  # enable light-weight heap profiling tool malloc_count
  set(LIBSTXXL_SOURCES ${LIBSTXXL_SOURCES}
    utils/malloc_count.cpp
    )

  set_source_files_properties(utils/malloc_count.cpp
    PROPERTIES COMPILE_FLAGS "-Wno-pedantic")

endif(USE_MALLOC_COUNT)

# tell top-level cmakelists which library we build
set(STXXL_EXPORTED_LIBS stxxl)

# we name debug library builds "stxxl_debug" and release builds "stxxl"
string(TOLOWER "stxxl_${CMAKE_BUILD_TYPE}" STXXL_LIBNAME)
if(STXXL_LIBNAME STREQUAL "stxxl_release" OR STXXL_LIBNAME STREQUAL "stxxl_")
  set(STXXL_LIBNAME "stxxl")
endif()

if(BUILD_SHARED_LIBS)

  # build stxxl SHARED library and link all programs again it
  add_library(stxxl SHARED ${LIBSTXXL_SOURCES})
  target_link_libraries(stxxl foxxll)
  set_target_properties(stxxl PROPERTIES OUTPUT_NAME "${STXXL_LIBNAME}")
  set_target_properties(stxxl PROPERTIES VERSION "${STXXL_VERSION_STRING}")
  install(TARGETS stxxl
    EXPORT stxxl-targets
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR})

  if(BUILD_STATIC_LIBS)
    # but also build STATIC library
    add_library(stxxl_static STATIC ${LIBSTXXL_SOURCES})
    target_link_libraries(stxxl_static foxxll)
    set_target_properties(stxxl_static PROPERTIES OUTPUT_NAME "${STXXL_LIBNAME}")
    set_target_properties(stxxl_static PROPERTIES VERSION "${STXXL_VERSION_STRING}")
    install(TARGETS stxxl_static
      EXPORT stxxl-targets
      ARCHIVE DESTINATION ${INSTALL_LIB_DIR})

    # we build both shared and static, export both
    set(STXXL_EXPORTED_LIBS stxxl stxxl_static)
  endif()

else()

  if(NOT BUILD_STATIC_LIBS)
    message(FATAL_ERROR "You must build either a static or shared STXXL library")
  endif()

  # build stxxl STATIC library and link all programs again it
  add_library(stxxl STATIC ${LIBSTXXL_SOURCES})
  target_link_libraries(stxxl foxxll)
  set_target_properties(stxxl PROPERTIES OUTPUT_NAME "${STXXL_LIBNAME}")
  set_target_properties(stxxl PROPERTIES VERSION "${STXXL_VERSION_STRING}")
  install(TARGETS stxxl
    EXPORT stxxl-targets
    ARCHIVE DESTINATION ${INSTALL_LIB_DIR}
    LIBRARY DESTINATION ${INSTALL_LIB_DIR})

endif()

# export variables to top-level cmakelists
set(STXXL_LIBNAME ${STXXL_LIBNAME} PARENT_SCOPE)
set(STXXL_EXPORTED_LIBS ${STXXL_EXPORTED_LIBS} PARENT_SCOPE)
