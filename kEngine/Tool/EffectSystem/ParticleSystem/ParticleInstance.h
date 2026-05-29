#pragma once
#include <string>
#include "Vector3.h"
#include "Vector4.h"
#include "Transform.h"
#include "TimeManager.h"

struct ParticleInstance
{
	/// パーティクル状態

	int particleId = -1;		/// 粒子ID（Emitter内で一意）

	bool isAlive = false;		/// 生存フラグ
	Timer life{};				/// 寿命タイマー

	Vector3 velocity{};			/// 速度

	int objectIndex = -1;		/// 表示用物体のインデックス（objectList或いはspriteList）

	Vector3 startTranslate{};	/// 位置
	Vector3 startRotate{};		/// 旋轉
	Vector3 startScale{};		/// 縮放

	Vector3 endTranslate{};		/// 位置
	Vector3 endRotate{};		/// 旋轉
	Vector3 endScale{};			/// 縮放

	Vector4 startColor{};		/// 顏色
	Vector4 endColor{};			/// 顏色

	Vector3 nowTranslate{};		/// 位置
	Vector3 nowRotate{};			/// 旋轉
	Vector3 nowScale{};			/// 縮放

	Vector4 nowColor{};			/// 顏色

};