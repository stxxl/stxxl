include Makefile.common

all: lib

include ../Makefile.subdir.gnu

lib: lib$(LIBNAME).$(LIBEXT)

lib$(LIBNAME).$(LIBEXT): $(LIB_OBJECTS)
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean::
	$(RM) lib$(LIBNAME).$(LIBEXT)

