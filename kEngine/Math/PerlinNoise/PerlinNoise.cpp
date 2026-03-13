#include "PerlinNoise.h"
#include "tool/TimeManager/TimeManager.h"
#include "Vector3.h"

PerlinNoise::PerlinNoise() {
	for (int x = 0; x < (sizeof(p) / sizeof(p[0])); x++) {
		p[x] = permutation[(x) % (sizeof(permutation) / sizeof(permutation[0]))];
	}
}

void PerlinNoise::SetSeed(int seed) {
	seed_ = seed;
	for (int x = 0; x < (sizeof(p) / sizeof(p[0])); x++) {
		p[x] = permutation[(x * seed_) % (sizeof(permutation) / sizeof(permutation[0]))];

	}
}

int PerlinNoise::inc(int num) {
	num++;
	if (repeat_ > 0) num %= repeat_;
	return num;
}

float PerlinNoise::grad(int hash, Vector3 target) {
	{
		/////Perlin原文、複雑過ぎて使えたくない
		//int h = hash & 15;
		//float u = h < 8 ? target.x : target.y;
		//float v;
		//if (h < 4/*0b0100*/) {
		//	v = target.y;
		//} else if (h == 12/*0b1100*/ || h == 14/*0b1110*/) {
		//	v = target.x;
		//} else {
		//	v = target.z;
		//}
		//return ((h & 1) == 0 ? u : -u)
		//	+ ((h & 2) == 0 ? v : -v);

		//switch (hash & 0xF) {
		//case 0x0: return  target.x + target.y;
		//case 0x1: return -target.x + target.y;
		//case 0x2: return  target.x - target.y;
		//case 0x3: return -target.x - target.y;
		//case 0x4: return  target.x + target.z;
		//case 0x5: return -target.x + target.z;
		//case 0x6: return  target.x - target.z;
		//case 0x7: return -target.x - target.z;
		//case 0x8: return  target.y + target.z;
		//case 0x9: return -target.y + target.z;
		//case 0xA: return  target.y - target.z;
		//case 0xB: return -target.y - target.z;
		//case 0xC: return  target.y + target.x;
		//case 0xD: return -target.y + target.z;
		//case 0xE: return  target.y - target.x;
		//case 0xF: return -target.y - target.z;
		//default: return 0; // never happens
		//}
	}

	Vector3 grads[12] = {
		{1,1,0},
		{-1,1,0},
		{1,-1,0},
		{-1,-1,0},

		{1,0,1},
		{-1,0,1},
		{1,0,-1},
		{-1,0,-1},

		{0,1,1},
		{0,-1,1},
		{0,1,-1},
		{0,-1,-1}
	};

	Vector3 v3 = target;
	hash = hash & 0xb;
	return Dot(grads[hash], v3);
}

float PerlinNoise::lerp(float a, float b, float x) {
	return a + smootherstep(x) * (b - a);
}

int PerlinNoise::scrambledIndex(int index) {
	return ((index * 2654435761) ^ (index >> 5)) & 255;
}


float PerlinNoise::perlin(Vector3 target) {
	Vector3 intTarget{};
	if (repeat_ > 0) {
		intTarget.x = float((int)target.x % repeat_);
		intTarget.y = float((int)target.y % repeat_);
		intTarget.z = float((int)target.z % repeat_);
	} else {
		intTarget = target;
	}

	int xi = (int)(intTarget.x) & 255;
	int yi = (int)(intTarget.y) & 255;
	int zi = (int)(intTarget.z) & 255;

	float xf = target.x - (int)target.x;
	float yf = target.y - (int)target.y;
	float zf = target.z - (int)target.z;

	//float u = smootherstep(xf);
	//float v = smootherstep(yf);
	//float w = smootherstep(zf);

	float u = xf;
	float v = yf;
	float w = zf;

	int aaa = p[p[p[xi] + yi] + zi];
	int aba = p[p[p[xi] + inc(yi)] + zi];
	int aab = p[p[p[xi] + yi] + inc(zi)];
	int abb = p[p[p[xi] + inc(yi)] + inc(zi)];
	int baa = p[p[p[inc(xi)] + yi] + zi];
	int bba = p[p[p[inc(xi)] + inc(yi)] + zi];
	int bab = p[p[p[inc(xi)] + yi] + inc(zi)];
	int bbb = p[p[p[inc(xi)] + inc(yi)] + inc(zi)];

	//int aaa = permutation[permutation[permutation[xi] + yi] + zi];
	//int baa = permutation[permutation[permutation[inc(xi)] + yi] + zi];
	//int aba = permutation[permutation[permutation[xi] + inc(yi)] + zi];
	//int aab = permutation[permutation[permutation[xi] + yi] + inc(zi)];
	//int bba = permutation[permutation[permutation[inc(xi)] + inc(yi)] + zi];
	//int abb = permutation[permutation[permutation[xi] + inc(yi)] + inc(zi)];
	//int bab = permutation[permutation[permutation[inc(xi)] + yi] + inc(zi)];
	//int bbb = permutation[permutation[permutation[inc(xi)] + inc(yi)] + inc(zi)];

	float g000 = grad(aaa, { xf	   , yf	   , zf });
	float g100 = grad(baa, { xf - 1, yf	   , zf });
	float g010 = grad(aba, { xf	   , yf - 1, zf });
	float g001 = grad(aab, { xf	   , yf	   , zf - 1 });
	float g110 = grad(bba, { xf - 1, yf - 1, zf });
	float g011 = grad(abb, { xf    , yf - 1, zf - 1 });
	float g101 = grad(bab, { xf - 1, yf	   , zf - 1 });
	float g111 = grad(bbb, { xf - 1, yf - 1, zf - 1 });

	float x1, x2, y1, y2;
	x1 = lerp(g000, g100, u);
	x2 = lerp(g010, g110, u);
	y1 = lerp(x1, x2, v);
	x1 = lerp(g001, g101, u);
	x2 = lerp(g011, g111, u);
	y2 = lerp(x1, x2, v);

	return (lerp(y1, y2, w) + 1.0f) / 2.0f;
}

float PerlinNoise::fBm(Vector3 pos, int octaves, float persistence, float amplitude, float frequency) {
	float total = 0.0f;
	float maxValue = 0;
	float amplitude_ = amplitude;
	float frequency_ = frequency;

	for (int i = 0; i < octaves; i++) { // 疊加 4 層噪聲
		total += perlin({ pos *  frequency_ }) * amplitude_;

		maxValue += amplitude_;
		amplitude_ *= persistence;
		frequency_ = frequency_ * 2.0f;
	}
	return total - (int)total;

}

bool PerlinNoise::SetRepeat(int repeat) {
	repeat_ = repeat;
	if (repeat == 0)return true;
	return false;
}
