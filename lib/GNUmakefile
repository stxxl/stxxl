TOPDIR	?= ..
include Makefile.common

all: lib

LIB_OBJECTS	+= $(LIB_OBJECTS_NON_MSVC)

# this is set to empty to prevent triggering recompilation while building in libs/
EXTRA_DEPS_COMPILE	?=

include $(TOPDIR)/Makefile.subdir.gnu

# do not recompile missing object files here
COMPILE_STXXL	= false "can't build $@ from lib/"


lib: lib$(LIBNAME).$(LIBEXT)

lib$(LIBNAME).$(LIBEXT): $(LIB_OBJECTS) $(DEPS_MAKEFILES)
	$(RM) $@
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean::
	$(RM) lib$(LIBNAME).$(LIBEXT)

