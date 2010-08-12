include Makefile.common
include ../make.settings

all: lib

tests:

lib: lib$(LIBNAME).$(LIBEXT)
lib$(LIBNAME).$(LIBEXT): $(LIB_OBJECTS)
	del lib$(LIBNAME).$(LIBEXT)
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean:
	$(RM) $(CLEAN_TARGETS)

