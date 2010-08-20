TOPDIR	?= ..
CPPS	:= $(wildcard *.cpp)
TESTS	 = $(CPPS:.cpp=)
-include $(TOPDIR)/make.settings.local
include $(TOPDIR)/Makefile.subdir.gnu
-include GNUmakefile.local

clean::
	$(RM) $(TESTS:=.*.d) $(TESTS:=.*.o) $(TESTS:=.*.bin)
