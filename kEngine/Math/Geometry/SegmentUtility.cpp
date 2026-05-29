#include "SegmentUtility.h"

Vector3 Project(const Vector3& v1, const Vector3& v2) {
	Vector3 b = Normalize(v2);
	return Dot(v1, b) * b;
}

Vector3 ClosestPoint(const Vector3& point, const Segment& segment) {
	Vector3 resuit = {};
	float divisions = 100.0f;
	Segment targetLine = segment;

	for (float t = 0; t < divisions; t++) {
		Vector3 now = targetLine.origin + (targetLine.diff / divisions) * t;
		if (Length(Project(point, resuit)) < Length(Project(point, now))) {
			resuit = now;
		}
	}
	return resuit;
}