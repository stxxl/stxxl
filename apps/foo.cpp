#include <vector>
#include <functional>
#include <queue>

struct A
{
	typedef int type;
};

#define Macro(xxx) \
inline xxx operator + ( \
	const xxx & a, \
	xxx::type p) \
	{ \
		return a; \
	} \
inline xxx operator - ( \
    const xxx & a, \
	xxx::type p) \
	{ \ 
	    return a; \
	}

Macro(A)

int main()
{
	A a;
	a = a + 1;
	a = a - 1;
	
	// std::priority_queue< std::pair<short,int> , std::vector< std::pair<short,int> > , std::greater<  std::select2nd< std::pair<short,int> > > > b;
}
