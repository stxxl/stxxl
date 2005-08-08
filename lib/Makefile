all: lib

include ../make.settings

LIB_OBJECTS = \
../io/diskqueue.$(OBJEXT) ../io/wfs_file.$(OBJEXT) \
../io/ufs_file.$(OBJEXT) ../io/syscall_file.$(OBJEXT) \
../io/wincall_file.$(OBJEXT) ../io/iostats.$(OBJEXT) \
../mng/mng.$(OBJEXT) \
../algo/async_schedule.$(OBJEXT) \
../common/log.$(OBJEXT) ../common/debug.$(OBJEXT) \
../common/rand.$(OBJEXT)


lib: $(LIB_OBJECTS)
	$(LIBGEN)libstxxl.$(LIBEXT) $(LIB_OBJECTS)

clean:
	$(RM) libstxxl.$(LIBEXT)

