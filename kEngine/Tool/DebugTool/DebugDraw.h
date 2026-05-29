#pragma once
#include <vector>
#include <memory>
#include "kEngine.h"
#include "MathsIncluder.h"
#include "Data/Geometry/Shape/crashIncluder.h"
#include "Data/Render/CPUData/DebugLine.h"

class DebugDraw {
public:
	static void AddLine(const Vector3& start, const Vector3& end, const Vector4& color);
    static void AddLine(const DebugLine line);
    static void AddAABB(const AABB& aabb, const Vector4& color);
    //static void AddOBB(const OBB& obb, const Vector3& color);
    static void AddSphere(const Sphere& sphere, const Vector4& color,Camera* camera);
    //static void AddRay(const Ray& ray, float length, const Vector3& color);
    static void DrawAll(kEngine* kEngine);
    static void Clear();

	static bool IsEmpty() { return lines.empty(); }

private:
    static std::vector<DebugLine> lines;

};