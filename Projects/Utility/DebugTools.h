#pragma once

#include <chrono>
#include <string>

#include "Logging.h"
#define LOG_TAG "Timer"

// namespace for debugging
namespace SebDbg
{
	// a simple timer class that prints its lifetime when it goes out of scope
	// this is useful for measuring the time taken by a function.
	// usage: DbgScopedTimer timer("function_name") at the beginning of the function 
	// it will automatically print the time taken when it goes out of scope.
	template <typename Time = std::chrono::milliseconds>
	struct ScopedTimer
	{
		const char * name;
		std::chrono::high_resolution_clock::time_point start;
		constexpr ScopedTimer(const char* name)
			: name(name), start(std::chrono::high_resolution_clock::now()) {
		}
		~ScopedTimer()
		{
			auto dur = std::chrono::duration_cast<Time>(
				std::chrono::high_resolution_clock::now() - start).count();
            std::string suffix = " ms";
			if (dur < 0.00001)
			{
				// cast it to a lower duration
				dur = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::high_resolution_clock::now() - start).count();
				suffix = " us";
			}
			LOG_DEBUG_NOFUNC(name << " took " << std::to_string(dur) << suffix);
		}
	};

	
}// namespace SebDbg
namespace sdbg = SebDbg; // for convenience

#undef LOG_TAG