#include "Random.h"

#include <algorithm>

int Random::Int(int min, int max)
{
	if (min > max)
		std::swap(min, max);

	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(Engine());
}

float Random::Float(float min, float max)
{
	if (min > max)
		std::swap(min, max);

	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(Engine());
}

double Random::Double(double min, double max)
{
	if (min > max)
		std::swap(min, max);

	std::uniform_real_distribution<double> distribution(min, max);
	return distribution(Engine());
}

std::mt19937_64& Random::Engine()
{
	static std::mt19937_64 generator{ std::random_device{}() };
	return generator;
}
