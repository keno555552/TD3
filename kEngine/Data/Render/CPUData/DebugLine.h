#pragma once
#include "kEngine.h"
#include "Vector3.h"
#include "Vector4.h"

class DebugLine
{
public:
	DebugLine(kEngine* kEngine);
	~DebugLine();

	/// 描画
	void Draw();

	/// Setter
	void SetStartPoint(const Vector3& start) { startPoint = start; }
	void SetEndPoint(const Vector3& end) { endPoint = end; }
	void SetColor(const Vector3& col) { color = col; }
	
	/// Getter
	Vector3 GetStartPoint() const { return startPoint; }
	Vector3 GetEndPoint() const { return endPoint; }
	Vector3 GetColor() const { return color; }

private:
	kEngine* kEngine_ = nullptr;      /// 借り

	Vector3 startPoint{};
	Vector3 endPoint{};
	Vector3 color;
};