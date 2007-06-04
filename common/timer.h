#ifndef TIMER_HEADER
#define TIMER_HEADER

/* Simple timer class by Roman Dementiev 
 */
#include "../common/namespace.h"

#include <time.h>

#ifndef BOOST_MSVC
#include <sys/time.h>
#endif

#ifdef STXXL_BOOST_TIMESTAMP
#include <boost/date_time/posix_time/posix_time.hpp>
#include <math.h>
#endif

__STXXL_BEGIN_NAMESPACE


class timer
{
	bool running;
	double accumulated;
	double last_clock;
	inline  double timestamp();
public:
	inline timer();
	inline void start();
	inline void stop();
	inline void reset();
	inline double seconds();
	inline double mseconds();
	inline double useconds();
};

timer::timer():running(false),accumulated(0.)
{
}

double timer::timestamp()
{
	#ifdef STXXL_BOOST_TIMESTAMP
	boost::posix_time::ptime MyTime = boost::posix_time::microsec_clock::local_time();
	boost::posix_time::time_duration Duration = MyTime.time_of_day();
	double sec= double(Duration.hours())*3600. +
						  double(Duration.minutes())*60. +
						  double(Duration.seconds()) +
						  double(Duration.fractional_seconds())/(pow(10.,Duration.num_fractional_digits()));
	return sec;
	#else
	struct timeval tp;
  	gettimeofday(&tp, NULL);
  	return double(tp.tv_sec) + tp.tv_usec / 1000000.;
	#endif
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
