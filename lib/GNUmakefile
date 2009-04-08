TOPDIR	?= ..
include Makefile.common

all: lib

LIB_OBJECTS	+= $(LIB_OBJECTS_NON_MSVC)

include $(TOPDIR)/Makefile.subdir.gnu

lib: lib$(LIBNAME).$(LIBEXT)

lib$(LIBNAME).$(LIBEXT): $(LIB_OBJECTS) $(DEPS_MAKEFILES)
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean::
	$(RM) lib$(LIBNAME).$(LIBEXT)

