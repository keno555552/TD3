#include "DebugDraw.h"
//#define M_PI 3.1415926f

std::vector<DebugLine> DebugDraw::lines = {};

void DebugDraw::AddLine(const Vector3& start, const Vector3& end, const Vector4& color) {

#ifdef _DEBUG
    DebugLine newLine;

	newLine.startPoint = start;
	newLine.endPoint = end;
	newLine.color = color;

	lines.push_back(newLine);
#endif
}

void DebugDraw::AddLine(const DebugLine line) {

#ifdef _DEBUG
    DebugLine newLine;

	newLine.startPoint = line.startPoint;
	newLine.endPoint = line.endPoint;
	newLine.color = line.color;

    lines.push_back(newLine);
#endif
}

void DebugDraw::AddAABB(const AABB& aabb, const Vector4& color) {
	
	// 1. 八つの頂点を計算
    Vector3 p0 = { aabb.min.x, aabb.min.y, aabb.min.z };
    Vector3 p1 = { aabb.max.x, aabb.min.y, aabb.min.z };
    Vector3 p2 = { aabb.max.x, aabb.max.y, aabb.min.z };
    Vector3 p3 = { aabb.min.x, aabb.max.y, aabb.min.z };

    Vector3 p4 = { aabb.min.x, aabb.min.y, aabb.max.z };
    Vector3 p5 = { aabb.max.x, aabb.min.y, aabb.max.z };
    Vector3 p6 = { aabb.max.x, aabb.max.y, aabb.max.z };
    Vector3 p7 = { aabb.min.x, aabb.max.y, aabb.max.z };

	// 2. 底を描く
    AddLine(p0, p1, color);
    AddLine(p1, p2, color);
    AddLine(p2, p3, color);
    AddLine(p3, p0, color);
    
	// 3. 上を描く
    AddLine(p4, p5, color);
    AddLine(p5, p6, color);
    AddLine(p6, p7, color);
    AddLine(p7, p4, color);
    
	// 4. 側面を描く
    AddLine(p0, p4, color);
    AddLine(p1, p5, color);
    AddLine(p2, p6, color);
    AddLine(p3, p7, color);
}

void DebugDraw::AddSphere(const Sphere& sphere, const Vector4& color, Camera* camera) {

	Vector4 darkColor; // 半透明の暗い色
	float scale = 0.2f; // 半透明のアルファ値
	darkColor.x = color.x * scale;
	darkColor.y = color.y * scale;
	darkColor.z = color.z * scale;
	darkColor.w = 1.0f;

	Sphere tempSphere = sphere;

	/// 頂点データの配列
	Vector3 point[30][4]{}; 

	/// 分割数
	int sudivision = 30;

	// 取得攝影機方向
	Matrix4x4 view = camera->GetViewMatrix();

	Vector3 camRight = Normalize(Vector3(view.m[0][0], view.m[1][0], view.m[2][0]));
	Vector3 camUp = Normalize(Vector3(view.m[0][1], view.m[1][1], view.m[2][1]));
	Vector3 camForward = Normalize(Vector3(view.m[0][2], view.m[1][2], view.m[2][2]));

	/// 分割ことの角度
	const float step = 2.0f * M_PI / sudivision;

	/// 点を計算して配列に格納
	for (int i = 0; i < sudivision; i++) {
		float angle = i * step;

		// X 平面の大圓
		point[i][0].x = sphere.center.x + sphere.radius * cosf(angle);
		point[i][0].y = sphere.center.y;
		point[i][0].z = sphere.center.z + sphere.radius * sinf(angle);

		// Y 平面の大圓
		point[i][1].x = sphere.center.x;
		point[i][1].y = sphere.center.y + sphere.radius * cosf(angle);
		point[i][1].z = sphere.center.z + sphere.radius * sinf(angle);

		// Y 平面の大圓
		point[i][2].x = sphere.center.x + sphere.radius * cosf(angle);
		point[i][2].y = sphere.center.y + sphere.radius * sinf(angle);
		point[i][2].z = sphere.center.z;

		// Billboard 平面（永遠面向鏡頭）
		point[i][3] = tempSphere.center
					  + (camRight * (sphere.radius * cosf(angle)))
					  + (camUp * (sphere.radius * sinf(angle)));

	}

	for (int i = 0; i < sudivision; i++) {
		int next = (i + 1) % sudivision; // 次の点のインデックス（ループするためにモジュロ演算）

		// 点を結ぶ
		AddLine(point[i][0], point[next][0], darkColor);
		AddLine(point[i][1], point[next][1], darkColor);
		AddLine(point[i][2], point[next][2], darkColor);
		AddLine(point[i][3], point[next][3], color);
	}
}

void DebugDraw::DrawAll(kEngine* kEngine) {

    for (auto& line : lines) {
        // ここでlineを描画するコードを追加
		kEngine->DrawDebugLine(&line);
	}

}

void DebugDraw::Clear() {
	lines.clear();
}
