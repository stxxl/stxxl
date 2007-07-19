all: phonebills phonebills_stl genlog

include ../../../stxxl.mk

CXX		 = $(STXXL_CXX)
CPPFLAGS	+= -O3
CPPFLAGS	+= -W -Wall
CPPFLAGS	+= $(STXXL_CPPFLAGS)


phonebills: phonebills.cpp
	$(CXX) $(CPPFLAGS) phonebills.cpp -o phonebills $(STXXL_LDLIBS) -DUSE_STXXL

phonebills_stl: phonebills.cpp
	$(CXX) $(CPPFLAGS) phonebills.cpp -o phonebills_stl $(STXXL_LDLIBS) -DNDEBUG


genlog: genlog.cpp
	$(CXX) $(CPPFLAGS) genlog.cpp -o genlog $(STXXL_LDLIBS) -DNDEBUG 


clean:
	rm -f genlog phonebills_stl phonebills *~

