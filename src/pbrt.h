// \brief
//		basic types and macros
//

#pragma once


#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cmath>
#include <climits>
#include <cassert>
#include <iosfwd>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>


namespace pbrt
{


#define PBRT_CONSTEXPR	constexpr
#define Float	float


#define PBRT_PRINT(fmt, ...)		log_print(fmt, __VA_ARGS__)
#define PBRT_ERROR(fmt, ...)		log_print(fmt, __VA_ARGS__)
#define PBRT_DOCHECK(x)			{ assert(x); if(!(x)) { PBRT_PRINT("PBRT_DOCHECK(%s) failed at %s:%d\n", #x, __FILE__, __LINE__); } }

void log_print(const char* fmt, ...);


PBRT_CONSTEXPR Float kEpsilon = std::numeric_limits<Float>::epsilon();
PBRT_CONSTEXPR Float kInfinity = std::numeric_limits<Float>::infinity();
PBRT_CONSTEXPR Float kPi = (Float)3.14159265358979323846;
PBRT_CONSTEXPR Float k2Pi = (Float)2.0 * kPi;
PBRT_CONSTEXPR Float k4Pi = (Float)4.0 * kPi;
PBRT_CONSTEXPR Float kPiOver2 = kPi / (Float)2.0;
PBRT_CONSTEXPR Float kPiOver4 = kPi / (Float)4.0;
PBRT_CONSTEXPR Float kInvPi = (Float)1.0 / kPi;
PBRT_CONSTEXPR Float kInv2Pi = (Float)1.0 / k2Pi;
PBRT_CONSTEXPR Float kInv4Pi = (Float)1.0 / k4Pi;


template <typename T>
inline bool isNaN(const T x)
{
	return std::isnan(x);
}

template<>
inline bool isNaN(const int x) {
	return false;
}

template <typename T>
inline bool isInfinity(const T x)
{
	return std::isinf(x);
}

template <typename T>
inline bool isInvalid(const T x) { return isNaN(x) || isInfinity(x); }

template <typename T>
inline bool isValid(const T x) { return !isInvalid(x); }


inline Float Lerp(Float t, Float v1, Float v2) { return (1 - t) * v1 + t * v2; }

template <typename T, typename U, typename V>
inline T Clamp(T val, U low, V high) {
	if (val < low)
		return low;
	else if (val > high)
		return high;
	else
		return val;
}

inline Float Degree2Rad(Float x) { return (x * kPi) / (Float)180; }
inline Float Rad2Degree(Float x) { return (x * (Float)180) / kPi; }

// https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float_t-and-double-comparison
// http://realtimecollisiondetection.net/blog/?p=89
template <typename T>
struct EqualEpsilon
{
	static PBRT_CONSTEXPR T absolute_epsilon = std::numeric_limits<T>::epsilon();
	static PBRT_CONSTEXPR T relative_epsilon = std::numeric_limits<T>::epsilon();
};

template <typename T>
PBRT_CONSTEXPR bool isEqual(T x, T y, T epsilon = EqualEpsilon<T>::absolute_epsilon)
{
	if (std::is_floating_point_v<T>)
		return std::abs(x - y) <= epsilon * std::max(T(1), std::max(std::abs(x), std::abs(y)));
	else
		return x == y;
}

inline Float random_double() {
	return rand() * (1.0f / (RAND_MAX + 1.0f));
}

inline Float random_double(Float min, Float max)
{
	// return a random real in [min, max).
	return min + (max - min) * random_double();
}

inline int random_int(int min, int max)
{
	// return a random integer in [min, max]
	return static_cast<int>(random_double((Float)min, (Float)(max + 1)));
}


double appInitTiming();
double appSeconds();
double appMicroSeconds();
int64_t appCycles();

class FPerformanceCounter
{
public:
	FPerformanceCounter() : _timestamp(0.0)
	{}

	inline void StartPerf()
	{
		_timestamp = appMicroSeconds();
	}

	// return micro-seconds elapsed
	inline double EndPerf()
	{
		double _endstamp = appMicroSeconds();
		return _endstamp - _timestamp;
	}

private:
	double  _timestamp;
};

}
