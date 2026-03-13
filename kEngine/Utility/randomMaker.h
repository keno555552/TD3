#pragma once
//#include <stdlib.h>
//#include <stdio.h>
//#include <math.h>
//#include <time.h>
#include <random>

//inline int randomMaker = 0;
//
///// <summary>
///// int形上下限によるの乱数生成
///// </summary>
///// <param name="min">int形上限</param>
///// <param name="max">int形下限</param>
///// <returns>int数値</returns>
//int randomInt(int min, int max);
//
///// <summary>
///// float形上下限によるの乱数生成
///// </summary>
///// <param name="min">float形上限</param>
///// <param name="max">float形下限</param>
///// <param name="numAfterPoint">小数点後の位数</param>
///// <returns>float乱数</returns>
//float randomFloat(float min, float max, float numAfterPoint);
//
///// <summary>
///// 数位によるの乱数生成
///// </summary>
///// <param name="numAfterPoint">Int形数位</param>
///// <returns>int乱数</returns>
//int randomNum(int numAfterPoint = 9);

class RandomMaker {
public:
	RandomMaker(bool useFixedSeed = false, unsigned int fixedSeed = 12345);



	/// ============= 関数 ==============///
	/// セッティング
	void	SetSeed(unsigned int seed);
	int		GetSeedGenerator() { return seedGenerator(); };

	/// 乱数生成
	int		randomInt(int min, int max);
	float	randomFloat(float min, float max);

private:

	/// ============== 核心 ==============///
	std::random_device seedGenerator;
	std::mt19937 randomEngine{ seedGenerator()};

	/// ============ 固定シード ============///
	unsigned int	usingSeed_ = 12345;
};