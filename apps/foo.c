#include <iostream>

using namespace std;

struct A
{
	int value;
	A():value(0xdeadbeef) {};
};


template <class T>
void foo(T * p)
{
	new  (p)  T();
}

int main()
{
	int  p[2];
	p[1] = 0x888;
    new (p+1) int;	
	new (p) A;

	cout << hex <<p[0] << " " << p[1] << endl;
			
	return 0;
}
