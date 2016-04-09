#ifndef TIMER_H_
#define TIMER_H_

// c includes
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

class Timer {
public:
  typedef double Time;
  static const Time NOTIME = -1;
  Timer() { reset(); }
  virtual inline void reset() {
    gettimeofday(&start, NULL);
  }
  //! get the time since ctor or last reset (milliseconds)
  virtual inline Time getTimeSeconds() const {
    struct timeval end;
    gettimeofday(&end, NULL);
    return (Time) (// seconds
                   (end.tv_sec - start.tv_sec)
                   +
                   // useconds
                   (end.tv_usec - start.tv_usec)
                   / 1E6);
  }
private:
  struct timeval start;
}; // end class Timer

////////////////////////////////////////////////////////////////////////////////

class Rate {
public:
  Rate(double rate_hz) : _rate_hz(rate_hz) {
    _period_sec = 1. / _rate_hz;
  }

  double sleep() {
    double time_left = _period_sec - _timer.getTimeSeconds();
    usleep(1E6 * time_left);
    _timer.reset();
  }

private:
  Timer _timer;
  double _rate_hz, _period_sec;
};


#endif /*TIMER_H_*/

