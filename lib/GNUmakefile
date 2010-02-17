TOPDIR	?= ..
include Makefile.common

all: lib

LIB_OBJECTS	+= $(LIB_OBJECTS_NON_MSVC)

# this is set to empty to prevent triggering recompilation while building in libs/
EXTRA_DEPS_COMPILE	?=

include $(TOPDIR)/Makefile.subdir.gnu

lib: lib$(LIBNAME).$(LIBEXT)

lib$(LIBNAME).$(LIBEXT): $(LIB_OBJECTS) $(DEPS_MAKEFILES)
	$(RM) $@
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean::
	$(RM) lib$(LIBNAME).$(LIBEXT)

