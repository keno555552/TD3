#pragma once
#include <vector>
#include <string>
#include <optional>
#include "MathsIncluder.h"
#include "Transform.h"

struct Joint {

    int32_t ID;                                 // 自身のIndex
	Vector3 translation;                        // ローカル座標での平行移動
	Quaternion rotate; 					        // ローカル座標での回転
	Vector3 scale;                              // ローカル座標での拡縮
    Matrix4x4 localMatrix;                      // localMatrix
    Matrix4x4 skeletonSpaceMatrix;              // skeletonSpaceでの変換行列
    Matrix4x4 skeletonSpaceMatrixInvers;        // skeletonSpaceでの逆変換行列
	Matrix4x4 finalMatrix;                      // finalMatrix,仮skeletonの最終変換の行列の保存用
    std::string name;                           // 名前
    std::vector<int32_t> childrenIDList;        // 子JointのIndexのリスト。  いなければ空
    std::optional<int32_t> parentID;            // 親JointのIndex。         いなければnull
};
