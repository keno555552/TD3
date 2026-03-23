#include "Camera.h"
#include "Logger.h"

Transform CameraDefaultTransform() {
	return { { 1.0f, 1.0f, 1.0f },   // scale
			{ 0.0f, 0.0f, 0.0f },   // rotate
			{ 0.0f, 0.0f, -5.0f } // translate
	};
}

Camera::Camera()
	:fovY_(0.45f)
	, aspect_(float(config::GetClientWidth()) / float(config::GetClientHeight()))
	, nearClip_(0.1f)
	, farClip_(100.0f)
	, projectionMatrix_(MakePerspectiveFovMatrix(0.45f, float(config::GetClientWidth()) / float(config::GetClientHeight()), 0.1f, 100.0f))
	, cameraTransform_(CameraDefaultTransform())
	, defaultTransform_(CameraDefaultTransform())
	, matRot_(MakeRotateMatrix4x4(cameraTransform_.rotate)) {
}

void Camera::Update() {
	/// ProjectionMatrix更新
	projectionMatrix_ = MakePerspectiveFovMatrix(fovY_, aspect_, nearClip_, farClip_);
	/// カメラMatrix更新
	if (cameraTransform_.scale != Vector3{ 1,1,1 })cameraTransform_.scale = { 1,1,1 }; /// カメラのスケールは1固定
	worldMatrix_ = MakeAffineMatrix(cameraTransform_.scale, cameraTransform_.rotate, cameraTransform_.translate);
	/// ビュー行列更新
	viewMatrix_ = Inverse(worldMatrix_);
	/// 更新フラグオフ
	dirty_ = false;
}

TransformationMatrix Camera::transformationMatrixTransform(Transform objTransform) {

	Matrix4x4 objWorldMatrix = MakeAffineMatrix(objTransform.scale, objTransform.rotate, objTransform.translate);
	Matrix4x4 worldViewProjectionMatrix = objWorldMatrix * (viewMatrix_ * projectionMatrix_);

	return { worldViewProjectionMatrix, objWorldMatrix };

}

TransformationMatrix Camera::transformationMatrixTransform(Matrix4x4 worldMatrix) {
	//return { worldMatrix * (viewMatrix_ * projectionMatrix_), worldMatrix,Inverse(Transpose(worldMatrix)) };
	return{ worldMatrix * (viewMatrix_ * projectionMatrix_),worldMatrix,worldMatrix.Inverse().Transpose() };
}

void Camera::SetCamera(Transform cameraTransform) {
	cameraTransform_.rotate = cameraTransform.rotate;
	cameraTransform_.translate = cameraTransform.translate;
	dirty_ = true;
}


void Camera::SetRotation(Vector3 theta) {
	cameraTransform_.rotate = theta;
	dirty_ = true;
}

void Camera::SetTranslate(Vector3 translate) {
	cameraTransform_.translate = translate;
	dirty_ = true;
}

void Camera::SetFovY(float fovY) {
	if (fovY <= 0.0f || fovY >= 3.13f) return; // 異常値回避
	fovY_ = fovY;
	dirty_ = true;
}

void Camera::SetAspect(float aspect) {
	if (aspect <= 0.0f) return; // 0以下回避
	aspect_ = aspect;
	dirty_ = true;
}

void Camera::SetNearClip(float nearClip) {
	if (nearClip <= 0.0f) return; // 0以下回避
	nearClip_ = nearClip;
	dirty_ = true;
}

void Camera::SetFarClip(float farClip) {
	if (farClip <= nearClip_) return; // far <= near 回避
	farClip_ = farClip;
	dirty_ = true;
}


void Camera::Move(Vector3 speed) {
	Matrix4x4 handle = MakeTranslateMatrix4x4(speed) * MakeRotateMatrix4x4(cameraTransform_.rotate);
	cameraTransform_.translate = cameraTransform_.translate + MakeTranslateVector3(handle);
	dirty_ = true;
}

void Camera::Rotate(Vector3 Theta) {
	cameraTransform_.rotate = cameraTransform_.rotate + Theta;
	dirty_ = true;
}

Matrix4x4 Camera::GetWorldMatrix() {
	if (dirty_)Update();
	return worldMatrix_;
}

Matrix4x4 Camera::GetViewMatrix() {
	if (dirty_)Update();
	return viewMatrix_;
}

Matrix4x4 Camera::GetProjectionMatrix() {
	if (dirty_)Update();
	return projectionMatrix_;
}

Transform Camera::GetTransform() {
	if (dirty_)Update();
	return cameraTransform_;
}

Vector2 Camera::GetObjectScreenPos(Vector3 objPos) {

	Vector4 clip = Vector4(objPos.x, objPos.y, objPos.z, 1.0f) * viewMatrix_ * projectionMatrix_;

	if (clip.w <= 0.0f)
		return Vector2(-9999, -9999);

	float ndcX = clip.x / clip.w;
	float ndcY = clip.y / clip.w;

	float screenX = (ndcX * 0.5f + 0.5f) * config::GetClientWidth();
	float screenY = (1.0f - (ndcY * 0.5f + 0.5f)) * config::GetClientHeight();

	return Vector2(screenX, screenY);

}

bool Camera::isObjectFaceCamera(const Vector3& objectForward, const Vector3& objectPos, const float& thresholdDegree) {
	Vector3 toCamera = Normalize(cameraTransform_.translate - objectPos);

	float dot = Dot(objectForward, toCamera);

	float thresholdDot = cosf(thresholdDegree * (3.14159265f / 180.0f));

	return dot < thresholdDot;
}

Ray Camera::ScreenPointToRay(const Vector2& screenPos) {
	float w = (float)config::GetClientWidth();
	float h = (float)config::GetClientHeight();

	/// カメラスクリーン座標系をNDCに変換
	float ndcX = (screenPos.x / w) * 2.0f - 1.0f;
	float ndcY = 1.0f - (screenPos.y / h) * 2.0f;

	/// NDC座標をクリップ空間に変換
	Vector4 clip = { ndcX, ndcY, 1.0f, 1.0f };

	/// クリップ空間をビュー空間に変換
	Matrix4x4 invProj = Inverse(GetProjectionMatrix());
	Vector4 view =  clip * invProj;
	view /= view.w; // perspective divide

	/// ビュー空間をワールド空間に変換
	Matrix4x4 invView = Inverse(GetViewMatrix());
	Vector4 worldDir4 = Vector4{ view.x, view.y, view.z, 0.0f } * invView;

	Vector3 worldDir = Normalize(Vector3{ worldDir4.x, worldDir4.y, worldDir4.z });

	/// カメラの位置を取得
	Vector3 origin = cameraTransform_.translate;

	return Ray{ origin, worldDir };
}

void Camera::ResetCamera() {
	cameraTransform_ = defaultTransform_;
	dirty_ = true;
}




