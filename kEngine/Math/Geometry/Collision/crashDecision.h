#pragma once
#include <Math.h>
#include <algorithm>
#include "MathsIncluder.h"
#define M_PI 3.14159265358979323846f

#include "Camera/camera.h"
#include "CrashData/crashIncluder.h"


/// <summary>
/// Draw HitBox
/// </summary>
/// <param name="posX"></param>
/// <param name="posY"></param>
/// <param name="Pos Move X"></param>
/// <param name="Pos Move Y"></param>
/// <param name="HitBox Width"></param>
/// <param name="HitBox Height"></param>
/// <param name="color"></param>
extern void DrawHitBox(float posX, float posY, float width, float height, unsigned int color);

/// <summary>
/// Draw HitBox
/// </summary>
/// <param name="posX"></param>
/// <param name="posY"></param>
/// <param name="Pos Move X"></param>
/// <param name="Pos Move Y"></param>
/// <param name="HitBox Width"></param>
/// <param name="HitBox Height"></param>
/// <param name="color"></param>
extern void DrawAABB(const AABB& aabb, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, int color);



#pragma region 当たり判定

/// <summary>
/// Hit Box Crash Decision
/// </summary>
/// <param name="hit box 1 center pos.x"></param>
/// <param name="hit box 1 center pos.y"></param>
/// <param name="hit box 1 width"></param>
/// <param name="hit box 1 height"></param>
/// <param name="hit box 2 center pos.x"></param>
/// <param name="hit box 2 center pos.y"></param>
/// <param name="hit box 2 width"></param>
/// <param name="hit box 2 height"></param>
bool CrashDecisionBoxBool(float XA, float YA, float widthA, float heightA, float XB, float YB, float widthB, float heightB);

/// <summary>
/// Hit Box Crash Decision
/// </summary>
/// <param name="hit box 1 center pos.x"></param>
/// <param name="hit box 1 center pos.y"></param>
/// <param name="hit box 1 width"></param>
/// <param name="hit box 1 height"></param>
/// <param name="hit box 2 center pos.x"></param>
/// <param name="hit box 2 center pos.y"></param>
/// <param name="hit box 2 width"></param>
/// <param name="hit box 2 height"></param>
bool CrashDecisionHitBoxBool(HitBox* hitBoxA,HitBox* hitBoxB);

bool CrashDecisionHitBoxPointBool(HitBox* hitBox, const Vector2& point);


/// <summary>
/// Circle Hit Box Crash Decision
/// </summary>
/// <param name="XA"></param>
/// <param name="YA"></param>
/// <param name="RA"></param>
/// <param name="XB"></param>
/// <param name="YB"></param>
/// <param name="RB"></param>
/// <returns></returns>
int crashDecisionCircleBool(float XA, float YA, float RA, float XB, float YB, float RB);


/// <summary>
/// 2 Circle Hit Decision
/// </summary>
/// <param name="pos1">Circle 1 pos</param>
/// <param name="velocity1">Circle 1 velocity</param>
/// <param name="mass1">Circle 1 mass</param>
/// <param name="pos2">Circle 2 pos</param>
/// <param name="velocity2">Circle 2 velocity</param>
/// <param name="mass2">Circle 2 mass</param>
/// <param name="restitution"> restitution </param>
void calculateCollision(Vector2& pos1, Vector2& velocity1, float mass1, Vector2& pos2, Vector2& velocity2, float mass2, float restitution);


/// <summary>
/// Out Screen limiter
/// </summary>
/// <param name="center pos x"></param>
/// <param name="center pos y"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="Left limit"></param>
/// <param name="Right limit"></param>
/// <param name="Top limit"></param>
void crashMap(float* x, float* y, float w, float h, int lx, int rx, int ty);

/// <summary>
/// Out Screen limiter
/// </summary>
/// <param name="center pos x"></param>
/// <param name="center pos y"></param>
/// <param name="width"></param>
/// <param name="height"></param>
/// <param name="Left limit"></param>
/// <param name="Right limit"></param>
/// <param name="Top limit"></param>
void crashBorder(float* x, float* y, float w, float h, int lx, int rx, int ty);

/// <summary>
/// Balls(Sphere) Hit Decision
/// </summary>
/// <param name="s1">Sphere1</param>
/// <param name="s2">Sphere2</param>
/// <returns>isHit</returns>
bool crashDecision(const Sphere& s1, const Sphere& s2);

///// <summary>
///// Ball(Sphere) & Plane Hit Decision
///// </summary>
///// <param name="s1">Sphere</param>
///// <param name="s2">Plane</param>
///// <returns>isHit</returns>
//bool crashDecision(const Sphere& s1, const Plane& s2);
//
///// <summary>
///// Segment & Plane Hit Decision
///// </summary>
///// <param name="s1">Segment</param>
///// <param name="s2">Plane</param>
///// <returns>isHit</returns>
//bool crashDecision(const Segment& segment, const Plane& plane);
//
///// <summary>
///// Segment & Triangle Hit Decision
///// </summary>
///// <param name="s1">Segment</param>
///// <param name="s2">Triangle</param>
///// <returns>isHit</returns>
//bool crashDecision(const Segment& segment, const Triangle& triangle);
//
///// <summary>
///// AABB & AABB Hit Decision
///// </summary>
///// <param name="s1">AABB1</param>
///// <param name="s2">AABB2</param>
///// <returns>isHit</returns>
//bool crashDecision(const AABB& aabb1, const AABB& aabb2);
//
///// <summary>
///// AABB & Sphere Hit Decision
///// </summary>
///// <param name="s1">AABB</param>
///// <param name="s2">Sphere</param>
///// <returns>isHit</returns>
//bool crashDecision(const AABB& aabb1, const Sphere& sphere);
//
/// <summary>
/// AABB & Segment Hit Decision
/// </summary>
/// <param name="s1">AABB</param>
/// <param name="s2">Sphere</param>
/// <returns>isHit</returns>
bool crashDecision(const AABB& aabb1, const Segment& lineBase);


/// <summary>
/// Ray & AABB Hit Decision
/// </summary>
/// <param name="ray">Ray</param>
/// <param name="aabb">AABB</param>
/// <returns>isHit</returns>
bool crashDecision(const AABB& aabb, const Ray& ray);


/// <summary>
/// Ray & AABB Hit Decision
/// </summary>
/// <param name="ray">Ray</param>
/// <param name="aabb">AABB</param>
/// <returns>isHit</returns>
bool crashDecision(const Sphere& sphere, const Ray& ray, float* tOut = nullptr);

#pragma endregion


