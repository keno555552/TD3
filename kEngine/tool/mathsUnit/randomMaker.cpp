#include "randomMaker.h"

///int randomInt(int min, int max) {
///	srand((unsigned int)time(NULL));
///	randomMaker = rand() + randomMaker;
///	if (randomMaker < 0)randomMaker *= -1;
///	int range = max - min + 1;
///	int num = randomMaker % range;
///	return min + num;
///}
///
///float randomFloat(float min, float max, float numAfterPoint) {
///	srand((unsigned int)time(NULL));
///	randomMaker = rand() + randomMaker;
///	if (randomMaker < 0)randomMaker *= -1;
///	int range = int(max - min);
///	float num = float(randomMaker % range);
///	for (int i = 0; i < numAfterPoint; i++) {
///		randomMaker = rand() + randomMaker;
///		num += float(randomMaker % 10) / powf(10.0f, float(i + 1));
///	}
///
///	return min + num;
///}
///
///int randomNum(int numAfterPoint) {
///	int resuit = 0;
///	for (int i = 0; i < numAfterPoint; i++) {
///		randomMaker = rand() + randomMaker;
///		if (randomMaker < 0)randomMaker *= -1;
///		resuit += randomMaker % 10 * int(powf(10.0f, float(i)));
///	}
///	return resuit;
///}

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
