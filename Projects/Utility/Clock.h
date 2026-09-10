#pragma once
#include <chrono>

class Clock
{
public:
    Clock() { Reset(); }

    void Reset()
    {
        m_startTime = m_lastTime = std::chrono::high_resolution_clock::now();
        m_elapsedTime = 0.0;
        m_deltaTime = 0.0;
        m_paused = false;
    }

    void Tick()
    {
        if (m_paused) 
            return;

        auto now = std::chrono::high_resolution_clock::now();
        m_deltaTime = std::chrono::duration<float>(now - m_lastTime).count();
        m_elapsedTime = std::chrono::duration<float>(now - m_startTime).count();
        m_lastTime = now;
    }

    void Pause() { m_paused = true; }
    void Resume() { 
        m_paused = false; 
        m_lastTime = std::chrono::high_resolution_clock::now(); 
    }

    float GetDeltaTime()   const { return m_deltaTime; }
    float GetElapsedTime() const { return m_elapsedTime; }
    bool   IsPaused()       const { return m_paused; }

private:
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_lastTime;

    float m_deltaTime = 0.0;
    float m_elapsedTime = 0.0;
    bool m_paused = false;
};

// Timer for calculating delta time. It starts counting at the beginning of its life span.
class DeltaTimer
{
	using milliseconds = std::chrono::milliseconds;
	using clock = std::chrono::high_resolution_clock;
	using time_point = std::chrono::steady_clock::time_point;
public:
	DeltaTimer()noexcept
		:start(clock::now()), last(start), delta(std::chrono::duration<float>::zero()), lastdelta(delta)
	{
		this->Restart();
	}
	~DeltaTimer()
	{
	}
	// Returns the current time delta
	float Delta()
	{
		auto now = clock::now();
		delta = std::chrono::duration_cast<std::chrono::duration<float>>(now - last);
		last = now;
		lastdelta = delta;
		return delta.count();
	}
	// Returns the previous time delta
	const float LastDelta()const
	{
		return static_cast<float>(lastdelta.count());
	}
	// Restarts the clock
	float Restart()
	{
		start = clock::now();
		last = start;
		return static_cast<float>(start.time_since_epoch().count());
	}
	// Returns elapsed time in seconds
	const float GetElapsed()const
	{
		auto now = clock::now();
		std::chrono::duration<float> elapsed = now - start;
		return elapsed.count();
	}
	// Returns elapsed time in milliseconds
	const float GetElapsedAsMilliseconds()const
	{
		return GetElapsed() * 1000;
	}
private:
	time_point start; //start of life
	time_point  last;	 //last call for delta time
	std::chrono::duration<float> delta;
	std::chrono::duration<float> lastdelta;
};

