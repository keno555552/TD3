#pragma once
/// いる資料形のヘッダー
#include "Vector3.h"

/// メッシュの種類を管理する列挙型
#include "Mesh/Sprite2DMesh.h"
#include "Mesh/SimpleSpriteMesh.h"
#include "Mesh/TriangleMesh.h"
#include "Mesh/SphereMesh.h"
#include "Mesh/CubeMesh.h"
#include "Mesh/Model.h"
#include "Mesh/ModelGroup.h"
#include "Mesh/SkyCube.h"
#include "Mesh/PlaneMesh.h"
#include "Mesh/RingMesh.h"
#include "Mesh/CylinderMesh.h"

enum class VertexIndex {
	TRIANGLE,
	PLANE,
	CUBE,
	SPHERE,
	SKYCUBE,
	RING,
	CYLINDER,
	//SIMPLE_SPRITE,
	//SPRITE2D
	NumberOfVertexTypes
};

struct TriangleBuildMaterial {

	Vector3 vertices[3]
	{
		{0,0,0},
		{0,0,0},
		{0,0,0}
	};
	
};

struct PlaneBuildMaterial {

	Vector3 vertices[4]
	{
		{0,0,0},
		{0,0,0},
		{0,0,0},
		{0,0,0}
	};
		
};

struct CubeBuildMaterial {

	Vector3 vertices[8]
	{
		{0,0,0},{0,0,0},
		{0,0,0},{0,0,0},
		{0,0,0},{0,0,0},
		{0,0,0},{0,0,0}
	};
		
};

struct SphereBuildMaterial {
	
	int LatitudeSegments = 0;
	int LongitudeSegments = 0;

};

struct RingBuildMaterial {
	int Subdivision = 0;
	float OuterRadius = 0.0f;
	float InnerRadius = 0.0f;
};

struct CylinderBuildMaterial {
	int Division = 0;
	float TopRadius = 0.0f;
	float BottomRadius = 0.0f;
	float Height = 0.0f;
	bool isReverseY = false;
};