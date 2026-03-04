#define NOMINMAX ///<<<必ずすべでのファイルの先頭に書くこと
#include "Player.h"
#include <algorithm>
#include <numbers>

Player::Player(kEngine* system, const Vector3& position) {
	Object::IntObject(system);
	Object::CreateDefaultData();
	mainPosition.transform = CreateDefaultTransform();
	mainPosition.transform.translate = position;
	objectParts_[0].materialConfig->lightModelType = LightModelType::HalfLambert;

	mainPosition.transform.rotate.y = 0.0f;
}

void Player::Update(Camera* camera) {
	deltaTime_ = system_->GetDeltaTime();
	BehaviorRootUpdate();
}

void Player::BehaviorRootUpdate() {
	/// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	/// 移動量に速度の値をコピー
	collisionMapInfo.moveVector = velocity_;

	/// マップ衝突チェック
	Move();
	MapCollisionDecideDown(collisionMapInfo);
	MovePlayerByResult(collisionMapInfo);
	OnGroundChanger(collisionMapInfo);
}

Vector3 Player::CornerPosition(const Vector3& center, Corner4 corner) {
	Vector3 offsetTable[ (int)Corner4::kNumCorner] = {
		{+kPlayerWidth / 2.0f, -kPlayerHeight / 2.0f, 0}, // kRightBottom
		{-kPlayerWidth / 2.0f, -kPlayerHeight / 2.0f, 0}, // kLeftBottom
		{+kPlayerWidth / 2.0f, +kPlayerHeight / 2.0f, 0}, // kRightTop
		{-kPlayerWidth / 2.0f, +kPlayerHeight / 2.0f, 0}, // kLeftTop
	};

	Vector3 result;
	result.x = center.x + offsetTable[static_cast<uint32_t>(corner)].x;
	result.y = center.y + offsetTable[static_cast<uint32_t>(corner)].y;
	result.z = center.z + offsetTable[static_cast<uint32_t>(corner)].z;

	return result;
}

void Player::Move() {
	bool landing = false;
	/// 移動
	// 左右移動捜索
	if (system_->GetIsPush(DIK_D) || system_->GetIsPush(DIK_A)) {
		Vector3 acceleration = {};
		if (system_->GetIsPush(DIK_D)) {
			if (velocity_.x < 0.0f) {
				// 速度と逆方向に入力中に急ブレーキ
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x += kAcceleration;

			if (lrDirection_ != LRDirection::kRight) {
				lrDirection_ = LRDirection::kRight;
				turnFirstRotationY_ = mainPosition.transform.rotate.y;
				turnTimer_ = kTimeTurn;
			}
		}
		if (system_->GetIsPush(DIK_A)) {
			if (velocity_.x > 0.0f) {
				// 速度と逆方向に入力中に急ブレーキ
				velocity_.x *= (1.0f - kAttenuation);
			}
			acceleration.x -= kAcceleration;
			if (lrDirection_ != LRDirection::kLeft) {
				lrDirection_ = LRDirection::kLeft;
				turnFirstRotationY_ = mainPosition.transform.rotate.y;
				turnTimer_ = kTimeTurn;
			}
		}

		if (system_->GetIsPush(DIK_A) && system_->GetIsPush(DIK_D)) {
			if (lrDirection_ != LRDirection::None) {
				lrDirection_ = LRDirection::None;
				turnFirstRotationY_ = mainPosition.transform.rotate.y;
				turnTimer_ = kTimeTurn;
			}
		}

		// 加速/減速
		velocity_.x += acceleration.x * deltaTime_;

		velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
	} else {
		// 非入力時は移動減衰をかける
		velocity_.x *= std::exp(-kAttenuation * deltaTime_);
		if (velocity_.x < 0.01f && velocity_.x > -0.01f) { 
			velocity_.x = 0.0f;
		}
		//if (behavior_ != Behavior::kAttack) {
		if (lrDirection_ != LRDirection::None) { 
			lrDirection_ = LRDirection::None;
			turnFirstRotationY_ = mainPosition.transform.rotate.y;
			turnTimer_ = kTimeTurn;
		}
		//}
	}

	// 回転制御
	if (turnTimer_ >= 0.0f) {
		turnTimer_ -= deltaTime_;
		turnTimer_ = std::min(turnTimer_, 0.0f);

		float destinationRotationYTable[] = {
			-std::numbers::pi_v<float> / 2.0f,                            // 右
			std::numbers::pi_v<float> /2.0f,       // 左
			0.0f // なし
		};
		// 状態に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];
		// 自キャラの角度を設定する
		mainPosition.transform.rotate.y = turnFirstRotationY_ * (turnTimer_ / kTimeTurn) + destinationRotationY * (1 - turnTimer_ / kTimeTurn);
	}


	if (onGround_) {

		// ジャンプ
		if (system_->GetIsPush(DIK_W)) {
			// ジャンプ初速を加える
			velocity_.y += kJumpAcceleration;
		}

	} else {
		// 落下速度
		velocity_.y -= kGravityAcceleration * deltaTime_;
		// 落下速度制限
		velocity_.y = std::max(velocity_.y, -kLimitFallSpeed);
		// 空中
		if (velocity_.y < 0) {
			if ((mainPosition.transform.translate.y + velocity_.y) <= 2.0f) {
				landing = true;
			}
		}
	}
}

void Player::OnGroundChanger(const CollisionMapInfo& info) {
	if (onGround_) {
		if (velocity_.y > 0.0f) {
			onGround_ = false;
		} else {
			//MapChipType mapChipType;
			//MapChipField::IndexSet indexSet;
			std::vector<Vector3> positionsNew((int)Corner4::kNumCorner);
			for (uint32_t i = 0; i < positionsNew.size(); i++) {
				Vector3 translation_ = {};
				translation_.x = mainPosition.transform.translate.x + info.moveVector.x* deltaTime_;
				translation_.y = mainPosition.transform.translate.y + info.moveVector.y* deltaTime_;
				translation_.z = mainPosition.transform.translate.z + info.moveVector.z* deltaTime_;
				positionsNew[i] = CornerPosition(translation_, static_cast<Corner4>(i));
			}
			bool hit = false;
			// 左下点の判定
			//indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
			//mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			//if (mapChipType == MapChipType::kBlock) {
			//	hit = true;
			//}
			if (positionsNew[(int)Corner4::kLeftBottom].y <= 0) {
				hit = true;
			}
			// 右下点の判定
			//indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
			//mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
			//if (mapChipType == MapChipType::kBlock) {
			//	hit = true;
			//}
			if (positionsNew[(int)Corner4::kRightBottom].y <= 0) {
				hit = true;
			}

			if (!hit) {
				onGround_ = false;
			}
		}
	} else {
		if (info.floorHit) {
			onGround_ = true;
			velocity_.x *= (1.0f - kAttenuationLanding);
			velocity_.y = 0.0f;
		}
	}
}

void Player::MapCollisionDecideDown(CollisionMapInfo& info) {
	/// 移動後の4つの角の座標
	std::vector<Vector3> positionsNew((int)Corner4::kNumCorner);

	for (uint32_t i = 0; i < positionsNew.size(); i++) {
		Vector3 translation_ = {};
		translation_.x = mainPosition.transform.translate.x + info.moveVector.x* deltaTime_;
		translation_.y = mainPosition.transform.translate.y + info.moveVector.y* deltaTime_;
		translation_.z = mainPosition.transform.translate.z + info.moveVector.z* deltaTime_;
		positionsNew[i] = CornerPosition(translation_, static_cast<Corner4>(i));
	}
	if (info.moveVector.y > 0)
		return;

	// 当たり判定を行う
	bool hit = false;

	//MapChipType mapChipType;
	//MapChipType mapChipTypeNext;
	//MapChipField::IndexSet indexSet;
	//// 左下点の判定
	//indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
	//mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	//mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	//if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
	//	hit = true;
	//}
	if (positionsNew[(int)Corner4::kLeftBottom].y <= 0) {
		hit = true;
	}
	//// 右下点の判定
	//indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kRightBottom]);
	//mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex);
	//mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex - 1);
	//if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) {
	//	hit = true;
	//}
	if (positionsNew[(int)Corner4::kRightBottom].y <= 0) {
		hit = true;
	}
	// ブロックにヒット?
	if (hit) {
		//// めり込みを排除する方向に移動量を設定する
		//indexSet = mapChipField_->GetMapChipIndexByPosition(positionsNew[kLeftBottom]);
		//MapChipField::IndexSet indexSetNow = mapChipField_->GetMapChipIndexByPosition(worldTransform_.translation_);
		//if (indexSetNow.yIndex != indexSet.yIndex) {
		//	// めり込み先ブロックの矩形取得
		//	MapChipField::Rect rect = mapChipField_->GetRectByIndex(indexSet.xIndex, indexSet.yIndex);
		//	info.moveVector.y = std::max(0.0f, rect.top - (kPlayerHeight / 2.0f) - worldTransform_.translation_.y);
		//	// 床判定であることを記録する
		//	info.floorHit = true;
		//}
		info.moveVector.y = std::max(0.0f, 0.0f - (kPlayerHeight / 2.0f) - mainPosition.transform.translate.y);
		info.floorHit = true;
	}
}



void Player::MovePlayerByResult(const CollisionMapInfo& info) {
	mainPosition.transform.translate.x += info.moveVector.x * deltaTime_;
	mainPosition.transform.translate.y += info.moveVector.y * deltaTime_;
	mainPosition.transform.translate.z += info.moveVector.z * deltaTime_;
}