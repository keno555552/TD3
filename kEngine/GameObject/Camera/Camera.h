#pragma once
#include "Transform.h"
#include "TransformationMatrix.h"
#include "MathsIncluder.h"
#include "config.h"


Transform CameraDefaultTransform();

class CameraManager;
class DebugCamera;
class Camera {
public:

	/// CameraManagerからのみ生成・破棄可能
	friend class CameraManager;
	friend class DebugCamera;

	/// 更新
	virtual void Update();

	/// スクリーン座標系転換
	TransformationMatrix transformationMatrixTransform(const Transform objTransform);
	TransformationMatrix transformationMatrixTransform(const Matrix4x4 worldMatrix);

	/// 直接座標指定
	void SetCamera(Transform cameraTransform);


	/// 平行移動
	void Move(Vector3 speed);
	void Rotate(Vector3 Theta);


	/// セットカメラ
	void SetRotation(Vector3 rotate);
	void SetTranslate(Vector3 translate);
	void SetFovY(float fovY);
	void SetAspect(float aspect);
	void SetNearClip(float nearClip);
	void SetFarClip(float farClip);

	/// ゲットカメラ
	Matrix4x4 GetWorldMatrix();
	Matrix4x4 GetViewMatrix();
	Matrix4x4 GetProjectionMatrix();
	Transform GetTransform();
	Vector2 GetObjectScreenPos(Vector3 objPos);

	/// デフォルト変換設定・取得
	void SetDefaultTransform(Transform defaultTransform) { defaultTransform_ = defaultTransform; }
	Transform GetDefaultTransform() const { return defaultTransform_; }

	/// カメラツール
	bool isObjectFaceCamera(const Vector3& objectForward,const Vector3& objectPos, const float& thresholdDegree = 90.0f);


	void ResetCamera(); 

private:
	/// =============　将来カメラは必ずエンジン側で管理されるべき ============= ///
	Camera();
	virtual ~Camera() = default;


protected:

	/// 水平方向視野角
	float fovY_;
	/// アスペクト比
	float aspect_ ;
	/// ニアクリップ距離
	float nearClip_;
	/// ファークリップ距離
	float farClip_;

	/// 累積回転行列
	Matrix4x4 matRot_;

	/// カメラ変数を作る
	Transform cameraTransform_;

	/// リセット用カメラ変数
	Transform defaultTransform_;

	/// WorldMatrix用
	Matrix4x4 worldMatrix_{};

	/// ProjectionMatrix
	Matrix4x4 projectionMatrix_{};

	/// ビュー行列
	Matrix4x4 viewMatrix_{};

	/// 更新フラグ
	bool dirty_{ true };

};

