#include "crashDecision.h"


////////////////////////////////////////////////////////////////////Order//////////////////////////////////////////////////////////////////

extern void DrawHitBox(float posX, float posY, float width, float height, unsigned int color) {
	//Novice::DrawLine(int(posX - width / 2), int(posY - height / 2),
	//	int(posX + width / 2), int(posY - height / 2), color);
	//Novice::DrawLine(int(posX + width / 2), int(posY - height / 2),
	//	int(posX + width / 2), int(posY + height / 2), color);
	//Novice::DrawLine(int(posX + width / 2), int(posY + height / 2),
	//	int(posX - width / 2), int(posY + height / 2), color);
	//Novice::DrawLine(int(posX - width / 2), int(posY + height / 2),
	//	int(posX - width / 2), int(posY - height / 2), color);
}

void DrawAABB(const AABB& aabb, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color) {
	Vector3 p[8];
	Vector3 p2[8];
	Vector3 p3[8];
	p[0] = { aabb.min.x,aabb.min.y,aabb.min.z };
	p[1] = { aabb.min.x,aabb.min.y,aabb.max.z };
	p[2] = { aabb.min.x,aabb.max.y,aabb.max.z };
	p[3] = { aabb.min.x,aabb.max.y,aabb.min.z };

	p[4] = { aabb.max.x,aabb.min.y,aabb.min.z };
	p[5] = { aabb.max.x,aabb.min.y,aabb.max.z };
	p[6] = { aabb.max.x,aabb.max.y,aabb.max.z };
	p[7] = { aabb.max.x,aabb.max.y,aabb.min.z };

	for (int i = 0; i < 8; i++) {
		p2[i] = viewFinalTransform(p[i], viewProjectionMatrix);
		p3[i] = viewFinalTransform(p2[i], viewportMatrix);
	}

	for (int i = 0; i < 4; i++) {
		if (i < 3) {
			//Novice::DrawLine(int(p3[i].x), int(p3[i].y), int(p3[i + 1].x), int(p3[i + 1].y), color);
			//Novice::DrawLine(int(p3[i + 4].x), int(p3[i + 4].y), int(p3[i + 4 + 1].x), int(p3[i + 4 + 1].y), color);
		} else {
			//Novice::DrawLine(int(p3[i].x), int(p3[i].y), int(p3[i - 3].x), int(p3[i - 3].y), color);
			//Novice::DrawLine(int(p3[i + 4].x), int(p3[i + 4].y), int(p3[i + 4 - 3].x), int(p3[i + 4 - 3].y), color);
		}
		//Novice::DrawLine(int(p3[i].x), int(p3[i].y), int(p3[i + 4].x), int(p3[i + 4].y), color);
	}
}




bool CrashDecisionBoxBool(float XA, float YA, float widthA, float heightA, float XB, float YB, float widthB, float heightB) {
	Vector2 A[2] = { { XA - widthA / 2, YA + heightA / 2},{ XA + widthA / 2, YA - heightA / 2 } };
	Vector2 B[2] = { { XB - widthB / 2, YB + heightB / 2},{ XB + widthB / 2, YB - heightB / 2 } };
	int DFlat[2] = {};

	if (B[0].x < A[1].x) {
		DFlat[0]++;
	}
	if (B[1].x > A[0].x) {
		DFlat[0]++;
	}

	if (B[0].y > A[1].y) {
		DFlat[1]++;
	}
	if (B[1].y < A[0].y) {
		DFlat[1]++;
	}

	if (DFlat[0] == 2 && DFlat[1] == 2) {
		return true;
	} else {
		return false;
	}
}

bool CrashDecisionHitBoxBool(HitBox* hitBoxA, HitBox* hitBoxB) {
	Vector2 A[2] = { { hitBoxA->midPos.x - hitBoxA->width / 2, hitBoxA->midPos.y + hitBoxA->height / 2},
					 { hitBoxA->midPos.x + hitBoxA->width / 2, hitBoxA->midPos.y - hitBoxA->height / 2 } };
	Vector2 B[2] = { { hitBoxB->midPos.x - hitBoxB->width / 2, hitBoxB->midPos.y + hitBoxB->height / 2},
					 { hitBoxB->midPos.x + hitBoxB->width / 2, hitBoxB->midPos.y - hitBoxB->height / 2 } };
	int DFlat[2] = {};

	if (B[0].x < A[1].x) {
		DFlat[0]++;
	}
	if (B[1].x > A[0].x) {
		DFlat[0]++;
	}

	if (B[0].y > A[1].y) {
		DFlat[1]++;
	}
	if (B[1].y < A[0].y) {
		DFlat[1]++;
	}

	if (DFlat[0] == 2 && DFlat[1] == 2) {
		return true;
	} else {
		return false;
	}
}

bool CrashDecisionHitBoxPointBool(HitBox* hitBox, const Vector2& point) {
	float left = hitBox->midPos.x - hitBox->width / 2.0f;
	float right = hitBox->midPos.x + hitBox->width / 2.0f;
	float top = hitBox->midPos.y + hitBox->height / 2.0f;
	float bottom = hitBox->midPos.y - hitBox->height / 2.0f;

	return (point.x >= left && point.x <= right &&
		point.y >= bottom && point.y <= top);
}

int crashDecisionCircleBool(float XA, float YA, float RA, float XB, float YB, float RB) {

	if (RA + RB >= sqrtf(powf(XA - XB, 2) + powf(YA - YB, 2))) {
		return 1;
	} else {
		return 0;
	}
}

void calculateCollision(Vector2& pos1, Vector2& velocity1, float mass1, Vector2& pos2, Vector2& velocity2, float mass2, float restitution) {
	Vector2 normal = { fabsf(pos2.x - pos1.x),fabsf(pos2.y - pos1.y) };
	Vector2 relativeVelocity = { fabsf(velocity2.x - velocity1.x), fabsf(velocity2.y - velocity1.y) };

	float velocityAlongNormal = relativeVelocity.x * normal.x + relativeVelocity.y * normal.y;
	if (velocityAlongNormal > 0) { return; };

	float impulse = -(1 + restitution) * velocityAlongNormal / (1 / mass1 + 1 / mass2);
	Vector2 impulseVector = { normal.x * impulse , normal.y * impulse };

	velocity1 = { velocity1.x + (impulseVector.x * (1 / mass1)),
				  velocity1.y + (impulseVector.y * (1 / mass1)) };
	velocity2 = { velocity2.x + (impulseVector.x * (1 / mass2)),
				  velocity2.y + (impulseVector.y * (1 / mass2)) };
}


void crashMap(float* x, float* y, float w, float h, int lx, int rx, int ty) {
	if (int(*x - w / 2) < lx) {
		*x = float(lx + w / 2);
	}
	if (int(*x + w / 2) > rx) {
		*x = float(rx - w / 2);
	}
	if (int(*y + h / 2) > ty) {
		*y = float(ty - h / 2);
	}
}


void crashBorder(float* x, float* y, float w, float h, int lx, int rx, int ty) {
	if (int(*x - w / 2) < lx) {
		*x = float(lx + w / 2);
	}
	if (int(*x + w / 2) > rx) {
		*x = float(rx - w / 2);
	}
	if (int(*y + h / 2) > ty) {
		*y = float(ty - h / 2);
	}
}

bool crashDecision(const Sphere& s1, const Sphere& s2) {
	Sphere ss1 = s1;
	Sphere ss2 = s2;
	float distance = Length(ss1.center - ss2.center);
	if (distance < ss1.radius + ss2.radius) {
		return true;
	}
	return false;
}

//bool crashDecision(const Sphere& s1, const Plane& s2) {
//	Sphere ss1 = s1;
//	Plane ss2 = s2;
//	float distance = Dot(ss1.center, ss2.normal) - ss2.distance;
//	if (distance < ss1.radius && distance >= 0) {
//		return true;
//	}
//	if (distance > -ss1.radius && distance < 0) {
//		return true;
//	}
//	return false;
//}
//
//bool crashDecision(const Segment& segment, const Plane& plane) {
//	/// まずは内積を求める
//	float dot = Dot(plane.normal, segment.diff);
//
//	/// 平行してるかをチェック
//	if (dot == 0.0f) {
//		return false;
//	}
//
//	/// tを求める
//	float t = (plane.distance - Dot(segment.origin, plane.normal)) / dot;
//
//	/// tの値と線の種類によって衝突してるかを判断する
//	if (t >= 0 && t <= 1) {
//		return true;
//	}
//	return false;
//}
//
//bool crashDecision(const Segment& segment, const Triangle& triangle) {
//	Vector3 tp1 = triangle.vertex[0];
//	Vector3 tp2 = triangle.vertex[1];
//	Vector3 tp3 = triangle.vertex[2];
//
//	Vector3 lOrigin = segment.origin;
//	Vector3 lDiff = segment.diff;
//
//	/// 三角の法線を求む
//	Vector3 v1 = tp2 - tp1;
//	Vector3 v2 = tp3 - tp2;
//	Vector3 n = Cross(v1, v2);
//
//	/// 平面との衝突点を探す(基本線と平面の当たり判定と同じ)
//	// まずは内積を求める
//	float dot = Dot(n, segment.diff);
//	if (dot == 0.0f) { return false; }
//	// tを求める
//	float t = (0 - Dot(segment.origin, n)) / dot;
//	if (t < 0 || t > 1) { return false; }
//	Vector3 p = lOrigin + (t * segment.diff);
//
//	/// 本判定
//	// 各辺を結んだベクトルと、頂点と突点pを結んだベクトルのクロス積を取る
//	Vector3 cross01 = Cross({ tp2 - tp1 }, { p - tp2 });
//	Vector3 cross12 = Cross({ tp3 - tp2 }, { p - tp3 });
//	Vector3 cross20 = Cross({ tp1 - tp3 }, { p - tp1 });
//
//	// すべての小三角形のクロス積と法線が同じ方向に向いていたら衝突
//	if (Dot(cross01, n) >= 0.0f &&
//		Dot(cross12, n) >= 0.0f &&
//		Dot(cross20, n) >= 0.0f) {
//		return true;
//	}
//	return false;
//}
//
//bool crashDecision(const AABB& a, const AABB& b) {
//	if ((a.min.x <= b.max.x && a.max.x >= b.min.x) &&
//		(a.min.y <= b.max.y && a.max.y >= b.min.y) &&
//		(a.min.z <= b.max.z && a.max.z >= b.min.z)) {
//		return true;
//	}
//	return false;
//}
//
//bool crashDecision(const AABB& a, const Sphere& sphere) {
//	Vector3 closestPoint{ std::clamp(sphere.center.x, a.min.x,a.max.x),
//						  std::clamp(sphere.center.y, a.min.y,a.max.y) ,
//						  std::clamp(sphere.center.z, a.min.z,a.max.z) };
//
//	float distance = Length(closestPoint - sphere.center);
//
//	if (distance <= sphere.radius) {
//		return true;
//	}
//	return false;
//}
//
//bool crashDecision(const AABB& a, const Segment& segment) {
//	float tNearX, tFarX, tNearY, tFarY, tNearZ, tFarZ;
//
//	///まず距離が0どうかを判断
//	if (segment.diff.x != 0) {
//		///線分のtを見つかる(主に使う判断材料)
//		tNearX = (a.min.x - segment.origin.x) / segment.diff.x;
//		tFarX = (a.max.x - segment.origin.x) / segment.diff.x;
//		if (segment.diff.x < 0) { std::swap(tNearX, tFarX); }
//	} else {
//		///もし距離が0になると、線は平面的に、上の計算が０除算になると
//		///それを避けるには、まず線がAABBの中にあるかどうかを判断
//		/// なければfalseを返す、あると-INFINITYで、他の判断に回る
//		if (segment.origin.x < a.min.x || segment.origin.x > a.max.x) return false;
//		tNearX = -INFINITY;
//		tFarX = INFINITY;
//	}
//
//	if (segment.diff.y != 0) {
//		tNearY = (a.min.y - segment.origin.y) / segment.diff.y;
//		tFarY = (a.max.y - segment.origin.y) / segment.diff.y;
//		if (segment.diff.y < 0) { std::swap(tNearY, tFarY); }
//	} else {
//		if (segment.origin.y < a.min.y || segment.origin.y > a.max.y) return false;
//		tNearY = -INFINITY;
//		tFarY = INFINITY;
//	}
//
//	if (segment.diff.z != 0) {
//		tNearZ = (a.min.z - segment.origin.z) / segment.diff.z;
//		tFarZ = (a.max.z - segment.origin.z) / segment.diff.z;
//		if (segment.diff.z < 0) { std::swap(tNearZ, tFarZ); }
//	} else {
//		if (segment.origin.z < a.min.z || segment.origin.z > a.max.z) return false;
//		tNearZ = -INFINITY;
//		tFarZ = INFINITY;
//	}
//
//	/// AABBに入る点を探す
//	float tmin = max(max(tNearX, tNearY), tNearZ);
//	/// AABBに出る点を探す
//	float tmax = min(min(tFarX, tFarY), tFarZ);
//
//	/// 点が線の長さ外になるとfalse
//	if (tmax < 0 || tmin > 1) return false;
//
//	/// 判定基準は2_7 p.5参考
//	return tmin <= tmax;
//}



