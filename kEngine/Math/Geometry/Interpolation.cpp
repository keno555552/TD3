#include "Interpolation.h"

float Lerp(float a, float b, float c, float t) {
	float time = c / t;
	return (1.0f - time) * a + (time) * b;
}

Vector3 CubicSpline(const Vector3& v0, const Vector3& outTan0, const Vector3& v1, const Vector3& inTan1, float t, float dt) {
    float t2 = t * t;
    float t3 = t2 * t;

    Vector3 IoutTan0 = outTan0;
    Vector3 IinTan0 = inTan1;

    Vector3 T0 = IoutTan0 * dt;
    Vector3 T1 = IinTan0 * dt;

    return
        (2.0f * t3 - 3.0f * t2 + 1.0f) * v0 +
        (t3 - 2.0f * t2 + t) * T0 +
        (-2.0f * t3 + 3.0f * t2) * v1 +
        (t3 - t2) * T1;
}

Quaternion CubicSplineQuat(const Quaternion& v0, const Quaternion& outTan0, const Quaternion& v1, const Quaternion& inTan1, float t, float dt) {
    Quaternion Iv0 = v0;
	Quaternion Iv1 = v1;
    Quaternion IoutTan0 = outTan0;
    Quaternion IinTan0 = inTan1;

    // glTF tangent for quaternion is stored as derivative, not normalized
    Quaternion T0 = IoutTan0 * dt;
    Quaternion T1 = IinTan0 * dt;

    float t2 = t * t;
    float t3 = t2 * t;

    Quaternion result =
        Iv0 * (2 * t3 - 3 * t2 + 1) +
        T0 * (t3 - 2 * t2 + t) +
        Iv1 * (-2 * t3 + 3 * t2) +
        T1 * (t3 - t2);

    return Normalize(result);
}

//void Draw3DLine(const Line& line, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color) {
//	// 線分は両端をそれぞれスクリーン座標系まで変換し、Novice::DrawLineを利用して描画する
//	Vector3 start = viewFinilTransform(viewFinilTransform(line.origin, viewProjectionMatrix), viewportMatrix);
//	Vector3 end = viewFinilTransform(viewFinilTransform(Add(line.origin, line.diff), viewProjectionMatrix), viewportMatrix);
//	Novice::DrawLine(int(start.x), int(start.y), int(end.x), int(end.y), color);
//}


//void DrawGrid(const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix) {
//	const float kGridHalfWidth = 2.0f;  // Gridの半分の幅
//	const int kSubdivision = 10;   // 分割数
//	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);  // 1つ分の長さ
//	// 奥から手前への線を順々に引いていく
//	for (int xIndex = 0; xIndex <= kSubdivision; ++xIndex) {
//		// 上の情報を使ってワールド座標系上の始点と終点を求める
//		Vector3 start = { -kGridHalfWidth + (kGridEvery * xIndex), 0.0f,  -kGridHalfWidth };
//		Vector3 end = { -kGridHalfWidth + (kGridEvery * xIndex), 0.0f,  kGridHalfWidth };
//		// スクリーン座標系まで変換をかける
//		Vector3 startV2 = viewFinilTransform(start, viewProjectionMatrix);
//		Vector3 endV2 = viewFinilTransform(end, viewProjectionMatrix);
//		Vector3 startV3 = viewFinilTransform(startV2, viewportMatrix);
//		Vector3 endV3 = viewFinilTransform(endV2, viewportMatrix);
//
//		// 変換した座標を使って表示。色は薄い灰色(0xAAAAAAFF)、原点は黒ぐらいが良いが、何でも良い
//		int color1 = 0xAAAAAAFF;
//		int color2 = 0x000000FF;
//		if (xIndex == 5) {
//			Novice::DrawLine((int)startV3.x, (int)startV3.y,
//				(int)endV3.x, (int)endV3.y,
//				color2);
//		} else {
//			Novice::DrawLine((int)startV3.x, (int)startV3.y,
//				(int)endV3.x, (int)endV3.y,
//				color1);
//		}
//	}
//	// 左から右も同じように順々に引いていく
//	for (int zIndex = 0; zIndex <= kSubdivision; ++zIndex) {
//		// 奥から手前が左右に変わるだけ
//
//		// 上の情報を使ってワールド座標系上の始点と終点を求める
//		Vector3 start = { -kGridHalfWidth , 0.0f, -kGridHalfWidth + kGridEvery * zIndex };
//		Vector3 end = { kGridHalfWidth , 0.0f, -kGridHalfWidth + kGridEvery * zIndex };
//
//		// スクリーン座標系まで変換をかける
//		Vector3 startV2 = viewFinilTransform(start, viewProjectionMatrix);
//		Vector3 endV2 = viewFinilTransform(end, viewProjectionMatrix);
//		Vector3 startV3 = viewFinilTransform(startV2, viewportMatrix);
//		Vector3 endV3 = viewFinilTransform(endV2, viewportMatrix);
//
//		// 変換した座標を使って表示。色は薄い灰色(0xAAAAAAFF)、原点は黒ぐらいが良いが、何でも良い
//		int color1 = 0xAAAAAAFF;
//		int color2 = 0x000000FF;
//		if (zIndex == 5) {
//			Novice::DrawLine((int)startV3.x, (int)startV3.y,
//				(int)endV3.x, (int)endV3.y,
//				color2);
//		} else {
//			Novice::DrawLine((int)startV3.x, (int)startV3.y,
//				(int)endV3.x, (int)endV3.y,
//				color1);
//		}
//	}
//}