#pragma once
#include <cstdint>
#include <array>
#include <vector>

const uint32_t kNumMaxInflunce = 15;
struct VertexInfluence
{
	std::array<float, kNumMaxInflunce> weights;
	std::array<int32_t, kNumMaxInflunce> jointIndices;
};

