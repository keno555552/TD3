#include "randomMaker.h"

RandomMaker::RandomMaker(bool useFixedSeed, unsigned int fixedSeed) {
	if (useFixedSeed) {
		randomEngine.seed(fixedSeed); // 固定種子
		usingSeed_ = fixedSeed;
	} else {
		randomEngine.seed(seedGenerator()); // 系統隨機種子
	}
}

int RandomMaker::randomInt(int min, int max) {
	std::uniform_int_distribution<int> distribution(min, max);
	return distribution(randomEngine);
}

float RandomMaker::randomFloat(float min, float max) {
	std::uniform_real_distribution<float> distribution(min, max);
	return distribution(randomEngine);
}

void RandomMaker::SetSeed(unsigned int seed) {
	randomEngine.seed(seed);
	usingSeed_ = seed;
}
