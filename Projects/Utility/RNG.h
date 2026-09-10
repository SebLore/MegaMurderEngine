#pragma once

#include <random>
#include <DirectXMath.h>

namespace RNG 
{
	static std::random_device rd;
	static std::mt19937 gen(rd());

    // Get thread-local random number generator
	inline std::mt19937& Generator()
	{
		thread_local static std::mt19937 gen{ std::random_device{}() };
		return gen;
	}
	/*
	* Generates a random float in the range min-max
	*/
	inline float SingleFloatRange(float min, float max)
	{
		std::uniform_real_distribution<float> dist(min, max);
		return dist(gen);
	}

	inline DirectX::XMFLOAT3 RandomFloat3(float minVal, float maxVal)
	{	
		std::uniform_real_distribution<float> dist(-minVal, maxVal);

		return DirectX::XMFLOAT3(dist(gen), dist(gen), dist(gen));
	}
}