#pragma once
#include <vector>
#include <memory>
#include "Transform.h"
#include "TransformationMatrix.h"
#include "MaterialConfig.h"
#include "MathsIncluder.h"
#include "kEngine.h"
#include "Camera.h"
#include "DrawData/SpriteData.h"

class SimpleSprite : public SimpleSpriteData {
public:
	void IntObject(kEngine* system);

	virtual void Update(Camera* camera);

	virtual void Draw();

	/// パーツの数を取得
	int GetPartIndex()const { return (int)objectParts_.size(); }

	/// デフォルトデータ
	void CreateDefaultData();


	/// 転換用関数
	
	/// <summary>
	/// theatを(0,0,theat)に変換
	/// </summary>
	/// <param name="theat">radian</param>
	/// <returns>Vector3 rotate</returns>
	Vector3 TransRotation(float theat);
	/// <summary>
	/// Vector2 x,yをVector3 x,y,0に変換
	/// </summary>
	/// <param name="Pos">Vector2 x,y</param>
	/// <returns>Vector3 translate</returns>
	Vector3 TransTransform(Vector2 Pos);
	/// <summary>
	/// int layerを深度に変換
	/// </summary>
	/// <param name="layer">Layer</param>
	/// <returns>float (*0.001 x layer)</returns>
	float TransLayerDepth(int layer);

protected:
	kEngine* system_ = nullptr; /// 借り
};