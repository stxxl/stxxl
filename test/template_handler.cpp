// gluk.cpp : Defines the entry point for the console application.
//

#include <iostream>

using namespace std;

struct nothing
{
	void operator () () {};
};

class request
{
public:
	virtual void complete() {};
	virtual void serve() {};
	virtual void wait() {};
	virtual void poll() {};
};

template <class _op_complete>
class completable: public request
{
protected:
	_op_complete on_complete;
public:
	void serve()
	{
		complete();
		on_complete();
	};
};

template <class _op_complete>
class ufs_request: public completable<_op_complete>
{
public:
	ufs_request(int param) {};
};


template <class _on_complete = nothing>
void read(int param,request * & req)
{
	req = new ufs_request<_on_complete>(param);
};


struct handler
{
	void operator() ()
	{
		cout << "handler"<< endl;
	};
};

int main(int argc, char* argv[])
{
	request * req;
	read(1,req);

	req->serve();
	req->wait();

	return 0;
}

