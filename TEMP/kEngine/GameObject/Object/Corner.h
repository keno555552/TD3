#pragma once

enum class Corner4 {
	kRightBottom, // 右下
	kLeftBottom,  // 左下
	kRightTop,    // 右上
	kLeftTop,     // 左上

	kNumCorner // 要素数
};

struct Rect {
	float left;   // 右下
	float right;  // 左下
	float bottom; // 右上
	float top;    // 左上
};