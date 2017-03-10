#pragma once

#include <chrono>

namespace PetTracer
{
	typedef std::chrono::duration<float>			 seconds;
	typedef std::chrono::duration<float, std::milli> milliseconds;
	typedef std::chrono::duration<float, std::micro> microseconds;
	typedef std::chrono::duration<float, std::nano>	 nanoseconds;

	typedef std::chrono::high_resolution_clock::time_point	TimePoint;
	typedef std::chrono::high_resolution_clock				Clock;

	template<typename T>
	class Timer
	{
	public:
		Timer() { mStartTime = Clock::now(); }

		void Start() { mStartTime = Clock::now(); }
		float ElapsedTime()
		{
			TimePoint now = Clock::now();
			T dT = std::chrono::duration_cast< T >( now - mStartTime );
			return dT.count();
		}


	private:
		TimePoint	mStartTime;
	};

}