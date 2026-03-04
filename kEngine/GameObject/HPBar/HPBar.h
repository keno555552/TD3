#pragma once
#include "kEngine.h"
#include "Object/Object.h"

class HPBar : private Object
{
public:
	HPBar(kEngine* system, float maxHP, float nowHP, Object* targetObject = nullptr, const Vector3 turningPos = {0.0f, 1.2f, 0.0f}, const Vector4 color = Vector4{ 1.0f,1.0f,1.0f,1.0f });

	void Update(Camera* camera) override;
	void Draw() override;

public:
	void SetMaxHP(float maxHP);
	void SetNowHP(float nowHP);
	void SetBarColor(const Vector4& color);
	void SetDamageColor(const Vector4& color);
	void SetBarWidth(float width);

	void LinkToObject(Object* targetObject);

private:

	/// =========== リソースハンドル ============///
	int HPBarModelHandle_ = 0;

	/// =============== HP関連 ===============///
	float maxHP_ = 100;
	float nowHP_ = 100;

	/// =============== Bar関連 ==============///
	bool barWidthChange_ = false;
	float barWidth_ = 1.0f;
	Vector3 barTurningPos_ = {0.0f,1.2f};

	/// =============== 色関連 ===============///
	Vector4 barColor_ = { 1.0f,0.0f,0.0f,1.0f };
	Vector4 DamageColor_ = { 1.0f,0.0f,0.0f,1.0f };

private:
	void UpdateBarPart();


};

