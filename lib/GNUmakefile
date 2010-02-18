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
ifeq ($(strip $(ENABLE_SHARED)),yes)
	$(RM) $@.*
	# build shared library
	$(LINKER) -shared -Wl,-soname=lib$(LIBNAME).so.$(SOVERSION) -lpthread -o lib$(LIBNAME).so.$(LIBVERSION) $(LIB_OBJECTS)
	ln -sf lib$(LIBNAME).so.$(LIBVERSION) lib$(LIBNAME).so.$(SOVERSION)
	ln -sf lib$(LIBNAME).so.$(SOVERSION) lib$(LIBNAME).so
else
	# build static library
	$(LIBGEN) lib$(LIBNAME).$(LIBEXT) $(LIB_OBJECTS)
endif

clean::
	$(RM) lib$(LIBNAME).$(LIBEXT)
	$(RM) lib$(LIBNAME).$(strip $(LIBEXT)).*

