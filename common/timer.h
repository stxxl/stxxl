#ifndef TIMER_HEADER
#define TIMER_HEADER

/* Simple timer class by Roman Dementiev 
 */
#include "../common/utils.h"

#include <time.h>
#include <sys/time.h>


__STXXL_BEGIN_NAMESPACE


class timer
{
	bool running;
	double accumulated;
	double last_clock;
	double timestamp();
public:
	timer();
	void start();
	void stop();
	void reset();
	double seconds();
	double mseconds();
	double useconds();
};

timer::timer():running(false),accumulated(0.)
{
}

double timer::timestamp()
{
	struct timeval tp;
  gettimeofday(&tp, NULL);
  return double(tp.tv_sec) + tp.tv_usec / 1000000.;
}

void timer::start()
{
	running = true;
	last_clock = timestamp();	
}
void timer::stop()
{
	running = false;
	accumulated += timestamp() - last_clock;
}
void timer::reset()
{
	accumulated = 0.;
	last_clock = timestamp();
}

double timer::mseconds()
{
	if(running)
		return (accumulated + timestamp() - last_clock)*1000.;

	return (accumulated*1000.);
}

double timer::useconds()
{
	if(running)
		return (accumulated + timestamp() - last_clock)*1000000.;

	return (accumulated*1000000.);
}

double timer::seconds()
{
	if(running)
		return (accumulated + timestamp() - last_clock);
	
	return (accumulated);
}

__STXXL_END_NAMESPACE

#endif
