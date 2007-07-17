include Makefile.common
include ../make.settings

all: lib

tests:

lib: $(LIB_OBJECTS)
	$(LIBGEN)lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)

clean:
	$(RM) lib$(LIBNAME).$(LIBEXT)

