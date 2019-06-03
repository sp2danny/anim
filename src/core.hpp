
#pragma once

#include <vector>
#include <cassert>
#include <chrono>

using UC = unsigned char;
using UL = unsigned int;

using BVec = std::vector<UC>;

using hrc = std::chrono::high_resolution_clock;
using TP = hrc::time_point;

struct PerformanceTester
{
	long long decompress;
};

inline PerformanceTester* pt = nullptr;

template<typename T>
long long Dur(const T& t)
{
	return std::chrono::duration_cast<long long, std::micro>>(t).count();
}

