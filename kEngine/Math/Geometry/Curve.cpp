#include "Curve.h"

Vector2 Bezier(const Vector2 p0, const Vector2 p1, const Vector2 p2, float time, float clock) {
	Vector2 Np0 = {
		Lerp(p0.x,p1.x,time,clock),
		Lerp(p0.y,p1.y,time,clock)
	};
	Vector2 Np1 = {
		Lerp(p1.x,p2.x,time,clock),
		Lerp(p1.y,p2.y,time,clock)
	};
	return {
		Lerp(Np0.x,Np1.x,time,clock),
		Lerp(Np0.y,Np1.y,time,clock)
	};
}


Vector3 Bezier(const Vector3 p0, const Vector3 p1, const Vector3 p2, float time, float clock) {
	Vector3 Np0 = {
		Lerp(p0.x,p1.x,time,clock),
		Lerp(p0.y,p1.y,time,clock),
		Lerp(p0.z,p1.z,time,clock)
	};
	Vector3 Np1 = {
		Lerp(p1.x,p2.x,time,clock),
		Lerp(p1.y,p2.y,time,clock),
		Lerp(p1.z,p2.z,time,clock)
	};
	return {
		Lerp(Np0.x,Np1.x,time,clock),
		Lerp(Np0.y,Np1.y,time,clock),
		Lerp(Np0.z,Np1.z,time,clock)
	};
}


//void DrawBezier(const Vector3 p0, const Vector3 p1, const Vector3 p2, int function, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color) {
//	Vector3 pointL = p0;
//	Vector3 pointN = p0;
//	for (int i = 1; i < function + 1; i++) {
//		pointL = pointN;
//		pointN = Bezier(p2, p1, p0, i, function);
//		Draw3DLine({ pointL,{pointN - pointL} }, viewProjectionMatrix, viewportMatrix, color);
//	}
//}
