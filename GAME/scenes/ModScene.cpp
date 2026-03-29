#include "ModScene.h"
#include "GAME/actor/ModObjectUtil.h"
#include "GAME/actor/prompt/PromptData.h"
#include "Math/Geometry/Collision/crashDecision.h"
#include <Windows.h>
#include <unordered_set>

namespace {

size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

Vector3 Mul(const Vector3 &a, const Vector3 &b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

Vector3 NormalizeSafeV(const Vector3 &v, const Vector3 &fallback) {
  const float len = Length(v);
  if (len < 0.0001f) {
    return fallback;
  }

  const float inv = 1.0f / len;
  return {v.x * inv, v.y * inv, v.z * inv};
}

float Max3(float a, float b, float c) {
  return (std::max)(a, (std::max)(b, c));
}

Vector4 MakeColor(float r, float g, float b, float a = 1.0f) {
  return {r, g, b, a};
}

Vector3 ZeroV() { return {0.0f, 0.0f, 0.0f}; }

int FindRoleIndexInModPoints(const std::vector<ModControlPoint> &points,
                             ModControlPointRole role) {
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

const char *PartName(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
    return "ChestBody";
  case ModBodyPart::StomachBody:
    return "StomachBody";
  case ModBodyPart::Neck:
    return "Neck";
  case ModBodyPart::Head:
    return "Head";
  case ModBodyPart::LeftUpperArm:
    return "LeftUpperArm";
  case ModBodyPart::LeftForeArm:
    return "LeftForeArm";
  case ModBodyPart::RightUpperArm:
    return "RightUpperArm";
  case ModBodyPart::RightForeArm:
    return "RightForeArm";
  case ModBodyPart::LeftThigh:
    return "LeftThigh";
  case ModBodyPart::LeftShin:
    return "LeftShin";
  case ModBodyPart::RightThigh:
    return "RightThigh";
  case ModBodyPart::RightShin:
    return "RightShin";
  default:
    return "Unknown";
  }
}

std::string ModelPath(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::ChestBody:
    return "GAME/resources/modBody/chest/chest.obj";

  case ModBodyPart::StomachBody:
    return "GAME/resources/modBody/stomach/stomach.obj";

  case ModBodyPart::Neck:
    return "GAME/resources/modBody/neck/neck.obj";

  case ModBodyPart::Head:
    return "GAME/resources/modBody/head/head.obj";

  case ModBodyPart::LeftUpperArm:
    return "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj";
  case ModBodyPart::LeftForeArm:
    return "GAME/resources/modBody/leftForeArm/leftForeArm.obj";
  case ModBodyPart::RightUpperArm:
    return "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj";
  case ModBodyPart::RightForeArm:
    return "GAME/resources/modBody/rightForeArm/rightForeArm.obj";
  case ModBodyPart::LeftThigh:
    return "GAME/resources/modBody/leftThighs/leftThighs.obj";
  case ModBodyPart::LeftShin:
    return "GAME/resources/modBody/leftShin/leftShin.obj";
  case ModBodyPart::RightThigh:
    return "GAME/resources/modBody/rightThighs/rightThighs.obj";
  case ModBodyPart::RightShin:
    return "GAME/resources/modBody/rightShin/rightShin.obj";
  default:
    return "GAME/resources/modBody/chest/chest.obj";
  }
}

int FindFirstChildPartId(const ModAssemblyGraph &assembly, int parentId,
                         ModBodyPart wantedPart) {
  const std::vector<int> children = assembly.GetChildren(parentId);
  for (size_t i = 0; i < children.size(); ++i) {
    const PartNode *child = assembly.FindNode(children[i]);
    if (child != nullptr && child->part == wantedPart) {
      return child->id;
    }
  }
  return -1;
}

int ResolveControlOwnerPartId(const ModAssemblyGraph &assembly, int partId) {
  const PartNode *node = assembly.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  switch (node->part) {
  case ModBodyPart::Neck: {
    const int headId =
        FindFirstChildPartId(assembly, node->id, ModBodyPart::Head);
    if (headId >= 0) {
      return headId;
    }
    break;
  }

  case ModBodyPart::LeftForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftThigh) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightThigh) {
        return parent->id;
      }
    }
    break;

  default:
    break;
  }

  return partId;
}

bool IsSelectablePart(ModBodyPart part) { return part != ModBodyPart::Count; }

bool IsMouseLeftPressed() {
  return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
}

bool IsMouseLeftTriggered() {
  static bool wasPressed = false;
  const bool nowPressed = IsMouseLeftPressed();
  const bool triggered = (!wasPressed && nowPressed);
  wasPressed = nowPressed;
  return triggered;
}

bool IsMouseLeftReleased() {
  static bool wasPressed = false;
  const bool nowPressed = IsMouseLeftPressed();
  const bool released = (wasPressed && !nowPressed);
  wasPressed = nowPressed;
  return released;
}

bool RayPlaneIntersectionZ(const Ray &ray, float planeZ, Vector3 *hitPoint) {
  const float epsilon = 0.0001f;

  if (fabsf(ray.direction.z) < epsilon) {
    return false;
  }

  const float t = (planeZ - ray.origin.z) / ray.direction.z;
  if (t < 0.0f) {
    return false;
  }

  if (hitPoint != nullptr) {
    hitPoint->x = ray.origin.x + ray.direction.x * t;
    hitPoint->y = ray.origin.y + ray.direction.y * t;
    hitPoint->z = ray.origin.z + ray.direction.z * t;
  }

  return true;
}

Vector3 ClampDistance(const Vector3 &origin, const Vector3 &target,
                      float minLength, float maxLength,
                      const Vector3 &fallbackDir) {
  Vector3 diff = Subtract(target, origin);
  float length = Length(diff);

  Vector3 dir = fallbackDir;
  if (length > 0.0001f) {
    dir = NormalizeSafeV(diff, fallbackDir);
  }

  if (length < minLength) {
    return Add(origin, Multiply(minLength, dir));
  }
  if (length > maxLength) {
    return Add(origin, Multiply(maxLength, dir));
  }
  return target;
}

float ClampFloatLocal(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

float GetControlPointGizmoDrawRadius(float influenceRadius) {
  return ClampFloatLocal(influenceRadius * 0.45f, 0.035f, 0.12f);
}

float GetWheelScaleFactorFromDelta(int wheelDelta) {
  if (wheelDelta > 0) {
    return 1.08f;
  }
  if (wheelDelta < 0) {
    return 1.0f / 1.08f;
  }
  return 1.0f;
}

bool GetPickSegmentRoles(ModBodyPart part, ModControlPointRole& startRole,
    ModControlPointRole& endRole) {
    switch (part) {
    case ModBodyPart::LeftUpperArm:
    case ModBodyPart::RightUpperArm:
    case ModBodyPart::LeftThigh:
    case ModBodyPart::RightThigh:
        startRole = ModControlPointRole::Root;
        endRole = ModControlPointRole::Bend;
        return true;

    case ModBodyPart::LeftForeArm:
    case ModBodyPart::RightForeArm:
    case ModBodyPart::LeftShin:
    case ModBodyPart::RightShin:
        startRole = ModControlPointRole::Bend;
        endRole = ModControlPointRole::End;
        return true;

    case ModBodyPart::ChestBody:
        startRole = ModControlPointRole::Chest;
        endRole = ModControlPointRole::Belly;
        return true;

    case ModBodyPart::StomachBody:
        startRole = ModControlPointRole::Belly;
        endRole = ModControlPointRole::Waist;
        return true;

    case ModBodyPart::Neck:
        startRole = ModControlPointRole::LowerNeck;
        endRole = ModControlPointRole::UpperNeck;
        return true;

    case ModBodyPart::Head:
        startRole = ModControlPointRole::UpperNeck;
        endRole = ModControlPointRole::HeadCenter;
        return true;

    default:
        return false;
    }
}

float DistancePointToSegmentSq(const Vector3& point, const Vector3& a,
    const Vector3& b) {
    const Vector3 ab = Subtract(b, a);
    const Vector3 ap = Subtract(point, a);

    const float abLenSq = Dot(ab, ab);
    if (abLenSq <= 0.000001f) {
        const Vector3 diff = Subtract(point, a);
        return Dot(diff, diff);
    }

    float t = Dot(ap, ab) / abLenSq;
    t = ClampFloatLocal(t, 0.0f, 1.0f);

    const Vector3 closest = Add(a, Multiply(t, ab));
    const Vector3 diff = Subtract(point, closest);
    return Dot(diff, diff);
}

bool IntersectRaySphereLocal(const Ray& ray, const Vector3& center, float radius,
    float* outT) {
    const Vector3 m = Subtract(ray.origin, center);
    const float a = Dot(ray.direction, ray.direction);
    const float b = Dot(m, ray.direction);
    const float c = Dot(m, m) - radius * radius;

    if (c <= 0.0f) {
        if (outT != nullptr) {
            *outT = 0.0f;
        }
        return true;
    }

    if (a <= 0.000001f) {
        return false;
    }

    const float discriminant = b * b - a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    float t = (-b - sqrtf(discriminant)) / a;
    if (t < 0.0f) {
        t = (-b + sqrtf(discriminant)) / a;
        if (t < 0.0f) {
            return false;
        }
    }

    if (outT != nullptr) {
        *outT = t;
    }
    return true;
}

bool IntersectRayCapsule(const Ray& ray, const Vector3& capsuleStart,
    const Vector3& capsuleEnd, float capsuleRadius,
    float* outT) {
    const float epsilon = 0.0001f;
    const float rayDirLenSq = Dot(ray.direction, ray.direction);
    if (rayDirLenSq <= epsilon) {
        return false;
    }

    const Vector3 d = Subtract(capsuleEnd, capsuleStart);
    const float segLenSq = Dot(d, d);

    if (segLenSq <= epsilon) {
        return IntersectRaySphereLocal(ray, capsuleStart, capsuleRadius, outT);
    }

    const Vector3 m = Subtract(ray.origin, capsuleStart);
    const Vector3 n = ray.direction;

    const float md = Dot(m, d);
    const float nd = Dot(n, d);
    const float dd = segLenSq;
    const float mn = Dot(m, n);
    const float nn = Dot(n, n);

    const float a = dd * nn - nd * nd;
    const float k = Dot(m, m) - capsuleRadius * capsuleRadius;
    const float c = dd * k - md * md;

    float bestT = FLT_MAX;
    bool hit = false;

    if (fabsf(a) > epsilon) {
        const float b = dd * mn - nd * md;
        const float discriminant = b * b - a * c;

        if (discriminant >= 0.0f) {
            const float sqrtDiscriminant = sqrtf(discriminant);

            float t0 = (-b - sqrtDiscriminant) / a;
            float t1 = (-b + sqrtDiscriminant) / a;

            if (t0 > t1) {
                const float temp = t0;
                t0 = t1;
                t1 = temp;
            }

            if (t1 >= 0.0f) {
                if (t0 < 0.0f) {
                    t0 = 0.0f;
                }

                const float candidates[2] = { t0, t1 };
                for (int i = 0; i < 2; ++i) {
                    const float t = candidates[i];
                    const float s = md + t * nd;
                    if (s >= 0.0f && s <= dd) {
                        if (t < bestT) {
                            bestT = t;
                            hit = true;
                        }
                    }
                }
            }
        }
    }

    float sphereT = 0.0f;
    if (IntersectRaySphereLocal(ray, capsuleStart, capsuleRadius, &sphereT)) {
        if (sphereT < bestT) {
            bestT = sphereT;
            hit = true;
        }
    }

    if (IntersectRaySphereLocal(ray, capsuleEnd, capsuleRadius, &sphereT)) {
        if (sphereT < bestT) {
            bestT = sphereT;
            hit = true;
        }
    }

    if (!hit) {
        return false;
    }

    if (outT != nullptr) {
        *outT = bestT;
    }
    return true;
}

} // namespace

ModScene::ModScene(kEngine *system) {
  // エンジン本体を保持する
  system_ = system;

  // シーン用ライトを作成して登録する
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  // 通常カメラとデバッグカメラを作成する
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // どちらのカメラも初期位置をそろえておく
  debugCamera_->SetTranslate({0.0f, 0.0f, -8.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  camera_->SetTranslate({0.0f, 0.0f, -8.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  // 初期状態ではデバッグカメラを使用する
  usingCamera_ = debugCamera_;
  system_->SetCamera(usingCamera_);

  // PromptScene で決まったお題文を取得する
  selectedPrompt_ = PromptData::GetSelectedPrompt();

  // 前シーンから共有されている改造データを取得する
  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  // 初期部位構成を作成し、保存データがあれば見た目へ読み込む
  SetupModObjects();
  LoadCustomizeData();
  EnsureValidSelection();

  // フェードインを開始する
  fade_.Initialize(system_);
  fade_.StartFadeIn();

  // 操作点表示用の白テクスチャを読み込む
  controlPointGizmoTextureHandle_ =
      system_->LoadTexture("GAME/resources/texture/white100x100.png");

  // 共有データから制限時間を復元する
  timeLimit_ = customizeData_->timeLimit_;
  isTimeUp_ = customizeData_->isTimeUp_;
}

ModScene::~ModScene() {
  // 作成したカメラを破棄する
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  // 登録したライトを解除して解放する
  system_->RemoveLight(light1_);
  delete light1_;
}

void ModScene::Update() {

  // 共通制限時間を更新する
  if (!isTimeUp_) {
    timeLimit_ -= system_->GetDeltaTime();

    if (timeLimit_ <= 0.0f) {
      timeLimit_ = 0.0f;
      isTimeUp_ = true;
    }
  } else {
    SetupModObjects();            // 時間切れで構造をリセット
    timeLimit_ = totalTimeLimit_; // 制限時間をリセット
    isTimeUp_ = false;            // 時間切れ状態をリセット
    outcome_ = SceneOutcome::RETRY;
  }

  // 使用中カメラを更新する
  CameraPart();

  // このフレームで構造変更が発生したかを記録する
  bool assemblyChanged = false;

  // キー入力で腕セットを追加する
  if (system_->GetTriggerOn(DIK_1)) {
    assemblyChanged |= assembly_.AddArmAssembly(PartSide::Left);
  }
  if (system_->GetTriggerOn(DIK_2)) {
    assemblyChanged |= assembly_.AddArmAssembly(PartSide::Right);
  }

  // キー入力で脚セットを追加する
  if (system_->GetTriggerOn(DIK_3)) {
    assemblyChanged |= assembly_.AddLegAssembly(PartSide::Left);
  }
  if (system_->GetTriggerOn(DIK_4)) {
    assemblyChanged |= assembly_.AddLegAssembly(PartSide::Right);
  }

  // キー入力で首と胴体と頭を追加する
  if (system_->GetTriggerOn(DIK_5)) {
    assemblyChanged |= assembly_.AddNeckPart();
  }
  if (system_->GetTriggerOn(DIK_6)) {
    assemblyChanged |= assembly_.AddBodyPart();
  }
  if (system_->GetTriggerOn(DIK_7)) {
    assemblyChanged |= assembly_.AddHeadPart();
  }

  // Delete キーで選択部位を削除する
  if (system_->GetTriggerOn(DIK_DELETE)) {
    DeleteSelectedPart();
    assemblyChanged = true;
  }

  // 矢印キーで選択部位のローカル位置を微調整する
  if (system_->GetTriggerOn(DIK_UP)) {
    NudgeSelectedPart({0.0f, 0.1f, 0.0f});
  }
  if (system_->GetTriggerOn(DIK_DOWN)) {
    NudgeSelectedPart({0.0f, -0.1f, 0.0f});
  }
  if (system_->GetTriggerOn(DIK_LEFT)) {
    NudgeSelectedPart({-0.1f, 0.0f, 0.0f});
  }
  if (system_->GetTriggerOn(DIK_RIGHT)) {
    NudgeSelectedPart({0.1f, 0.0f, 0.0f});
  }

  // 構造変更があった場合は Object 一覧と選択状態を同期し直す
  if (assemblyChanged) {
    SyncObjectsWithAssembly();
    LoadCustomizeData();
    EnsureValidSelection();
    ClearControlPointSelection();
  }

  // 操作点の選択とドラッグ移動を処理する
  UpdateControlPointEditing();

  // 現在の構造とパラメータを見た目へ反映する
  UpdateModObjects();

  if (isStartTransition_) {
      // 胴体(id=1)と頭(id=4)の操作点を確認する
      for (const auto& pair : modBodies_) {
          const auto& points = pair.second.GetControlPoints();
          if (!points.empty()) {
              Logger::Log("[PreSync] partId=%d partType=%d points=%d",
                  pair.first,
                  static_cast<int>(pair.second.GetPart()),
                  static_cast<int>(points.size()));
              for (size_t pi = 0; pi < points.size(); ++pi) {
                  Logger::Log("  point[%d] role=%d pos=(%.3f,%.3f,%.3f) radius=%.3f",
                      static_cast<int>(pi),
                      static_cast<int>(points[pi].role),
                      points[pi].localPosition.x,
                      points[pi].localPosition.y,
                      points[pi].localPosition.z,
                      points[pi].radius);
              }
          }
      }
  }

  // 現在のシーン状態を共有データへ書き戻す
  SyncCustomizeDataFromScene();

  // カメラ切り替えを行う
  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  // Space 入力で次シーンへのフェードアウトを開始する
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  // フェード演出を更新する
  fade_.Update(usingCamera_);

  // フェードアウト完了後に共有データを保存して次シーンへ進む
  if (isStartTransition_ && fade_.IsFinished()) {
      if (customizeData_ != nullptr) {
          Logger::Log("[ModScene Exit] controlPointSnapshots=%d",
              static_cast<int>(customizeData_->controlPointSnapshots.size()));
          for (size_t i = 0; i < customizeData_->controlPointSnapshots.size(); ++i) {
              const auto& s = customizeData_->controlPointSnapshots[i];
              Logger::Log("  [%d] ownerType=%d role=%d pos=(%.3f,%.3f,%.3f) radius=%.3f",
                  static_cast<int>(i),
                  static_cast<int>(s.ownerPartType),
                  static_cast<int>(s.role),
                  s.localPosition.x, s.localPosition.y, s.localPosition.z,
                  s.radius);
          }
          ModBody::SetSharedCustomizeData(*customizeData_);
      }
      outcome_ = SceneOutcome::NEXT;
  }
}

void ModScene::Draw() {
  // 改造部位 Object を描画する
  DrawModObjects();

  // 操作点表示球を描画する
  DrawControlPointGizmos();

#ifdef USE_IMGUI
  // シーン共通の簡易操作説明を表示する
  ImGui::Begin("Scene");
  ImGui::Text("ModScene");
  ImGui::Text("Selected Prompt:");
  ImGui::Text("%s", selectedPrompt_.c_str());
  ImGui::Separator();
  ImGui::Text("DIK_1/2 : Add Arm Assembly");
  ImGui::Text("DIK_3/4 : Add Leg Assembly");
  ImGui::Text("DIK_5   : Add Neck");
  ImGui::Text("DIK_6   : Add Body");
  ImGui::Text("DIK_7   : Add Head");
  ImGui::Text("Delete  : Remove Selected Part");
  ImGui::Text("Arrow   : Move Selected Part");
  ImGui::Text("Remaining Time : %.2f", timeLimit_);
  ImGui::Text("Time Up        : %s", isTimeUp_ ? "YES" : "NO");
  ImGui::End();

  // 改造用UI本体を表示する
  DrawModGui();
#endif

  // フェードを描画する
  fade_.Draw();
}

void ModScene::CameraPart() {
  // デバッグカメラ使用時は操作も含めて更新する

  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
#ifdef USE_IMGUI
    if (!ImGui::GetIO().WantCaptureMouse) {
      debugCamera_->MouseControlUpdate();
    }
#else
    debugCamera_->MouseControlUpdate();
#endif
  }
  // 通常カメラ使用時はこちらへ切り替える
  else {
    usingCamera_ = camera_;
  }

  // エンジン側へ現在の使用カメラを設定する
  system_->SetCamera(usingCamera_);
}

void ModScene::SetupModObjects() {
  // 初期人型構造を作成する
  assembly_.InitializeDefaultHumanoid();

  // 胴体共有操作点を初期化する
  ResetTorsoControlPoints();

  // Graph に合わせて Object 一覧を生成する
  SyncObjectsWithAssembly();

  // 初期レイアウトを整える
  SetupInitialLayout();

  // 改造パラメータを初期化する
  ResetModBodies();

  // SetupModObjects() の末尾の defaultControlPointSnapshots 保存部分を差し替え
  if (customizeData_ != nullptr) {
      customizeData_->defaultControlPointSnapshots.clear();

      // まず torsoControlPoints_ から胴体のデフォルト操作点を保存する
      {
          int chestPartId = -1;
          for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
              const PartNode* node = assembly_.FindNode(orderedPartIds_[i]);
              if (node != nullptr && node->part == ModBodyPart::ChestBody) {
                  chestPartId = orderedPartIds_[i];
                  break;
              }
          }

          if (chestPartId >= 0) {
              for (size_t pi = 0; pi < torsoControlPoints_.size(); ++pi) {
                  const auto& point = torsoControlPoints_[pi];
                  ModControlPointSnapshot snap;
                  snap.ownerPartId = chestPartId;
                  snap.ownerPartType = ModBodyPart::ChestBody;
                  snap.role = point.role;
                  snap.localPosition = point.localPosition;
                  snap.radius = point.radius;
                  snap.movable = point.movable;
                  snap.isConnectionPoint = point.isConnectionPoint;
                  snap.acceptsParent = point.acceptsParent;
                  snap.acceptsChild = point.acceptsChild;
                  customizeData_->defaultControlPointSnapshots.push_back(snap);
              }
          }
      }

      // 残りの部位は modBodies_ から（胴体はスキップ）
      for (const auto& id : orderedPartIds_) {
          if (modBodies_.count(id) == 0) continue;
          const PartNode* node = assembly_.FindNode(id);
          if (node == nullptr) continue;

          // ChestBody と StomachBody は torsoControlPoints_ で処理済み
          if (node->part == ModBodyPart::ChestBody ||
              node->part == ModBodyPart::StomachBody) {
              continue;
          }

          const auto& points = modBodies_[id].GetControlPoints();
          for (const auto& point : points) {
              ModControlPointSnapshot snap;
              snap.ownerPartId = id;
              snap.ownerPartType = node->part;
              snap.role = point.role;
              snap.localPosition = point.localPosition;
              snap.radius = point.radius;
              snap.movable = point.movable;
              snap.isConnectionPoint = point.isConnectionPoint;
              snap.acceptsParent = point.acceptsParent;
              snap.acceptsChild = point.acceptsChild;
              customizeData_->defaultControlPointSnapshots.push_back(snap);
          }
      }
  }
}

void ModScene::SetupInitialLayout() {
  // 現在の部位ID一覧を取得する
  orderedPartIds_ = assembly_.GetNodeIdsSorted();

  // すべての部位スケールを等倍で初期化する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    assembly_.SetPartScale(orderedPartIds_[i], {1.0f, 1.0f, 1.0f});
  }
}

void ModScene::SyncObjectsWithAssembly() {
  // 現在の構造から部位ID一覧を更新する
  orderedPartIds_ = assembly_.GetNodeIdsSorted();

  // 現在生存している部位ID集合を作る
  std::unordered_set<int> alive;
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    alive.insert(orderedPartIds_[i]);
  }

  // 既に削除された部位に対応する Object やデータを消す
  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end();) {
    if (alive.count(it->first) == 0) {
      modBodies_.erase(it->first);
      modModelHandles_.erase(it->first);
      it = modObjects_.erase(it);
    } else {
      ++it;
    }
  }

  // 新しく追加された部位の Object を生成する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) > 0) {
      continue;
    }

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }
    CreateObjectForNode(id, *node);
  }

  // 選択状態を有効なものへ補正する
  EnsureValidSelection();
}

void ModScene::CreateObjectForNode(int partId, const PartNode &node) {
  // 部位種類に対応するモデルパスを取得する
  const std::string path = ModelPath(node.part);

  // モデルを読み込み、ハンドルを保持する
  modModelHandles_[partId] = system_->SetModelObj(path);

  // Object を生成してモデルデータを設定する
  std::unique_ptr<Object> object = std::make_unique<Object>();
  object->IntObject(system_);
  object->CreateModelData(modModelHandles_[partId]);
  object->mainPosition.transform = CreateDefaultTransform();

  // 管理コンテナへ登録し、対応する ModBody も初期化する
  modObjects_[partId] = std::move(object);
  modBodies_[partId].Initialize(modObjects_[partId].get(), node.part);
}

void ModScene::ApplyAssemblyToSceneHierarchy() {
  // まず全 Object の親参照をクリアする
  for (std::unordered_map<int, std::unique_ptr<Object>>::iterator it =
           modObjects_.begin();
       it != modObjects_.end(); ++it) {
    Object *object = it->second.get();
    if (object == nullptr) {
      continue;
    }

    object->followObject_ = nullptr;
    object->mainPosition.parentPart = nullptr;
  }

  // AssemblyGraph の親子関係を Scene の Object 階層へ反映する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];

    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr || modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr) {
      continue;
    }

    Vector3 localTranslate = node->localTransform.translate;
    const Vector3 localRotate = {0.0f, 0.0f, 0.0f};

    if (node->parentId >= 0 && modObjects_.count(node->parentId) > 0) {
      Object *parentObject = modObjects_[node->parentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      bool handledByOwnerPoint = false;

      // 前腕・脛は owner の Bend に追従する
      switch (node->part) {
      case ModBodyPart::LeftForeArm:
      case ModBodyPart::RightForeArm:
      case ModBodyPart::LeftShin:
      case ModBodyPart::RightShin: {
        const int ownerId = ResolveControlOwnerPartId(assembly_, id);
        if (ownerId >= 0 && modBodies_.count(ownerId) > 0) {
          const std::vector<ModControlPoint> &ownerPoints =
              modBodies_[ownerId].GetControlPoints();

          const int bendIndex = modBodies_[ownerId].FindControlPointIndex(
              ModControlPointRole::Bend);

          if (bendIndex >= 0) {
            localTranslate =
                ownerPoints[static_cast<size_t>(bendIndex)].localPosition;
            handledByOwnerPoint = true;
          }
        }
        break;
      }

      case ModBodyPart::Head: {
        const PartNode *parentNode = assembly_.FindNode(node->parentId);

        // Neck の子である Head は Neck root と一致させる
        if (parentNode != nullptr && parentNode->part == ModBodyPart::Neck) {
          localTranslate = {0.0f, 0.0f, 0.0f};
          handledByOwnerPoint = true;
        }
        break;
      }

      default:
        break;
      }

      // それ以外は、親の現在形状を使って接続位置を解決する
      if (!handledByOwnerPoint) {
        localTranslate = ResolveAttachedLocalTranslate(*node);
      }
    }

    object->mainPosition.transform.translate = localTranslate;
    object->mainPosition.transform.rotate = localRotate;
    object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
  }
}

void ModScene::LoadCustomizeData() {
  // 共有データが無い場合は何もしない
  if (customizeData_ == nullptr) {
    return;
  }

  // 新方式の partId 単位データがある場合はそちらを優先して読み込む
  if (!customizeData_->partInstances.empty()) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int id = orderedPartIds_[i];
      if (modBodies_.count(id) == 0) {
        continue;
      }

      bool found = false;
      for (size_t j = 0; j < customizeData_->partInstances.size(); ++j) {
        const ModPartInstanceData &instance = customizeData_->partInstances[j];
        if (instance.partId != id) {
          continue;
        }

        // 一致する partId の見た目パラメータを適用する
        modBodies_[id].SetParam(instance.param);

        // 保存されているローカル位置を Graph へ戻す
        if (assembly_.FindNode(id) != nullptr) {
          assembly_.SetPartLocalTranslate(id,
                                          instance.localTransform.translate);
        }

        found = true;
        break;
      }

      // 新方式データが無い部位は旧方式データからフォールバックする
      if (!found) {
        const PartNode *node = assembly_.FindNode(id);
        if (node == nullptr) {
          continue;
        }

        const size_t index = ToIndex(node->part);
        if (index >= static_cast<size_t>(ModBodyPart::Count)) {
          continue;
        }

        ModBodyPartParam param = customizeData_->partParams[index];
        param.count = 1;
        modBodies_[id].SetParam(param);
      }
    }

    return;
  }

  // 新方式データが無い場合は旧方式の部位種別配列から読み込む
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    if (modBodies_.count(id) == 0) {
      continue;
    }

    const size_t index = ToIndex(node->part);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    ModBodyPartParam param = customizeData_->partParams[index];
    param.count = 1;
    modBodies_[id].SetParam(param);
  }
}

void ModScene::SyncCustomizeDataFromScene() {
  // 共有データが無い場合は何もしない
  if (customizeData_ == nullptr) {
    return;
  }

  // 現在のシーン状態から新方式の partInstances を作り直す
  customizeData_->partInstances.clear();
  customizeData_->partInstances.reserve(orderedPartIds_.size());

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr || modBodies_.count(id) == 0) {
      continue;
    }

    // 部位ごとの構造情報と改造パラメータを保存用データへ詰める
    ModPartInstanceData instance;
    instance.partId = id;
    instance.partType = node->part;
    instance.parentId = node->parentId;
    instance.parentConnectorId = node->parentConnectorId;
    instance.selfConnectorId = node->selfConnectorId;
    instance.localTransform = node->localTransform;
    instance.param = modBodies_[id].GetParam();

    // 新方式ではインスタンス単位で count は常に 1
    instance.param.count = 1;

    customizeData_->partInstances.push_back(instance);
  }

  // 新方式の可変長操作点配列を再構築する
  RebuildControlPointSnapshotsFromScene();

  // 旧固定操作点配列も互換用に保存する
  SaveControlPointsToCustomizeData();

  // 旧方式配列も互換用に再構築する
  RebuildLegacyCustomizeDataFromInstances();

  // 共通制限時間を共有データへ保存する
  customizeData_->timeLimit_ = timeLimit_;
  customizeData_->isTimeUp_ = isTimeUp_;

  // shared に積む前に整合性を揃えておく
  ModBody::NormalizeCustomizeData(*customizeData_);
}

void ModScene::RebuildControlPointSnapshotsFromScene() {
    if (customizeData_ == nullptr) {
        return;
    }

    customizeData_->controlPointSnapshots.clear();

    // まず torsoControlPoints_ から胴体の操作点を保存する
    {
        // ChestBody の partId を探す
        int chestPartId = -1;
        for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
            const PartNode* node = assembly_.FindNode(orderedPartIds_[i]);
            if (node != nullptr && node->part == ModBodyPart::ChestBody) {
                chestPartId = orderedPartIds_[i];
                break;
            }
        }

        if (chestPartId >= 0) {
            for (size_t pi = 0; pi < torsoControlPoints_.size(); ++pi) {
                const auto& point = torsoControlPoints_[pi];

                ModControlPointSnapshot snapshot;
                snapshot.ownerPartId = chestPartId;
                snapshot.ownerPartType = ModBodyPart::ChestBody;
                snapshot.role = point.role;
                snapshot.localPosition = point.localPosition;
                snapshot.radius = point.radius;
                snapshot.movable = point.movable;
                snapshot.isConnectionPoint = point.isConnectionPoint;
                snapshot.acceptsParent = point.acceptsParent;
                snapshot.acceptsChild = point.acceptsChild;

                customizeData_->controlPointSnapshots.push_back(snapshot);
            }
        }
    }

    // 残りの部位は modBodies_ から読み取る（胴体はスキップ）
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
        const int id = orderedPartIds_[i];

        const PartNode* node = assembly_.FindNode(id);
        if (node == nullptr) {
            continue;
        }

        // ChestBody と StomachBody は torsoControlPoints_ で処理済みなのでスキップ
        if (node->part == ModBodyPart::ChestBody ||
            node->part == ModBodyPart::StomachBody) {
            continue;
        }

        if (modBodies_.count(id) == 0) {
            continue;
        }

        const std::vector<ModControlPoint>& points =
            modBodies_.at(id).GetControlPoints();

        for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
            const ModControlPoint& point = points[pointIndex];

            ModControlPointSnapshot snapshot;
            snapshot.ownerPartId = id;
            snapshot.ownerPartType = node->part;
            snapshot.role = point.role;
            snapshot.localPosition = point.localPosition;
            snapshot.radius = point.radius;
            snapshot.movable = point.movable;
            snapshot.isConnectionPoint = point.isConnectionPoint;
            snapshot.acceptsParent = point.acceptsParent;
            snapshot.acceptsChild = point.acceptsChild;

            customizeData_->controlPointSnapshots.push_back(snapshot);
        }
    }
}

void ModScene::RebuildLegacyCustomizeDataFromInstances() {
  // 共有データが無い場合は何もしない
  if (customizeData_ == nullptr) {
    return;
  }

  // 旧方式配列を一旦初期化する
  for (size_t i = 0; i < static_cast<size_t>(ModBodyPart::Count); ++i) {
    customizeData_->partParams[i].scale = {1.0f, 1.0f, 1.0f};
    customizeData_->partParams[i].length = 1.0f;
    customizeData_->partParams[i].count = 0;
    customizeData_->partParams[i].enabled = false;
  }

  // 各部位種別について代表値を1つ保存するためのフラグを用意する
  std::array<bool, static_cast<size_t>(ModBodyPart::Count)> hasRepresentative{};
  for (size_t i = 0; i < hasRepresentative.size(); ++i) {
    hasRepresentative[i] = false;
  }

  // partInstances から代表値を拾いながら、同時に部位数を増やす
  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const ModPartInstanceData &instance = customizeData_->partInstances[i];
    const size_t index = ToIndex(instance.partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    customizeData_->partParams[index].count += 1;

    if (!hasRepresentative[index]) {
      customizeData_->partParams[index] = instance.param;
      customizeData_->partParams[index].count = 1;
      hasRepresentative[index] = true;
    }
  }

  // 元コードの流れを保つため、部位種別ごとの count を再計算する
  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const ModPartInstanceData &instance = customizeData_->partInstances[i];
    const size_t index = ToIndex(instance.partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }

    customizeData_->partParams[index].count += 0;
  }

  // 正確な部位数を数え直す
  std::array<int, static_cast<size_t>(ModBodyPart::Count)> counts{};
  for (size_t i = 0; i < counts.size(); ++i) {
    counts[i] = 0;
  }

  for (size_t i = 0; i < customizeData_->partInstances.size(); ++i) {
    const size_t index = ToIndex(customizeData_->partInstances[i].partType);
    if (index >= static_cast<size_t>(ModBodyPart::Count)) {
      continue;
    }
    counts[index] += 1;
  }

  // 数え直した count を最終的な値として入れる
  for (size_t i = 0; i < counts.size(); ++i) {
    customizeData_->partParams[i].count = counts[i];
  }
}

void ModScene::ResetModBodies() {
  // 全部位の改造パラメータを初期化する
  for (std::unordered_map<int, ModBody>::iterator it = modBodies_.begin();
       it != modBodies_.end(); ++it) {
    it->second.Reset();
  }
}

void ModScene::ResetSelectedPartParams() {
  // 選択中部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }
  if (modBodies_.count(selectedPartId_) == 0) {
    return;
  }

  // 選択中部位だけ見た目パラメータを初期化する
  modBodies_[selectedPartId_].Reset();
}

void ModScene::ResetToDefaultHumanoid() {

  // 制限時間も初期化する
  timeLimit_ = 30.0f;
  isTimeUp_ = false;

  // 構造を初期人型へ戻す
  assembly_.InitializeDefaultHumanoid();

  // 胴体共有操作点も初期化する
  ResetTorsoControlPoints();

  // Object 一覧を構造に合わせて再同期する
  SyncObjectsWithAssembly();

  // 初期レイアウトと改造パラメータを作り直す
  SetupInitialLayout();
  ResetModBodies();

  // 選択状態と共有データも初期状態へ寄せる
  EnsureValidSelection();
  SyncCustomizeDataFromScene();

  if (customizeData_ != nullptr) {
    customizeData_->totalTimeLimit_ = 30.0f;
    customizeData_->timeLimit_ = 30.0f;
    customizeData_->isTimeUp_ = false;
  }
}

void ModScene::SelectPart(int partId) {
  // 選択部位を更新する
  selectedPartId_ = partId;

  // 付け替え候補は選択し直しになるのでリセットする
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  // 部位を選び直したら操作点選択もいったん解除する
  ClearControlPointSelection();
}

void ModScene::EnsureValidSelection() {
  // 現在の選択がまだ有効ならそのまま維持する
  if (selectedPartId_ >= 0 && assembly_.FindNode(selectedPartId_) != nullptr &&
      modObjects_.count(selectedPartId_) > 0) {
    return;
  }

  // 無効ならいったん未選択に戻す
  selectedPartId_ = -1;

  // 選択可能な先頭部位を新たな選択対象にする
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }
    if (!IsSelectablePart(node->part)) {
      continue;
    }

    selectedPartId_ = id;
    break;
  }

  // 付け替え候補はリセットする
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  // 部位選択が無効化された場合は操作点選択も解除する
  ClearControlPointSelection();
}

void ModScene::DeleteSelectedPart() {
  // 選択中部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }

  // セット単位削除が必要な場合を考慮して対象部位IDを補正する
  const int targetId = ResolveAssemblyOperationPartId(selectedPartId_);
  if (targetId < 0) {
    return;
  }

  // Graph から削除できなければ終了する
  if (!assembly_.RemovePart(targetId)) {
    return;
  }

  // 削除後の Object 一覧と選択状態を再同期する
  selectedPartId_ = targetId;
  SyncObjectsWithAssembly();
  EnsureValidSelection();
  ClearControlPointSelection();
}

void ModScene::ReattachSelectedPart() {
  if (selectedPartId_ < 0 || reattachParentId_ < 0) {
    return;
  }

  const int targetId = ResolveAssemblyOperationPartId(selectedPartId_);
  if (targetId < 0) {
    return;
  }

  if (!assembly_.MovePart(targetId, reattachParentId_, -1)) {
    return;
  }

  SyncObjectsWithAssembly();
  ClearControlPointSelection();
}

void ModScene::NudgeSelectedPart(const Vector3 &delta) {
  // 選択部位が無い場合は何もしない
  if (selectedPartId_ < 0) {
    return;
  }

  // 現在のローカル位置を取得する
  const PartNode *node = assembly_.FindNode(selectedPartId_);
  if (node == nullptr) {
    return;
  }

  // delta を加算してローカル位置を更新する
  assembly_.SetPartLocalTranslate(selectedPartId_,
                                  Add(node->localTransform.translate, delta));
}

int ModScene::ResolveAssemblyOperationPartId(int partId) const {
  // 対象ノードを取得する
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  // 今の段階では腕・脚は既存の 2 パーツ構造のまま扱っている
  // そのため、前腕を選んだ場合は上腕、脛を選んだ場合は腿を
  // 編集対象へ寄せて、1 セットとして扱う
  switch (node->part) {
  case ModBodyPart::LeftForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightForeArm:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightUpperArm) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::LeftShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::LeftThigh) {
        return parent->id;
      }
    }
    break;

  case ModBodyPart::RightShin:
    if (node->parentId >= 0) {
      const PartNode *parent = assembly_.FindNode(node->parentId);
      if (parent != nullptr && parent->part == ModBodyPart::RightThigh) {
        return parent->id;
      }
    }
    break;

  default:
    break;
  }

  // 補正が不要なら元の部位IDを返す
  return partId;
}

void ModScene::UpdateControlPointEditing() {
#ifdef USE_IMGUI
  if (ImGui::GetIO().WantCaptureMouse) {
    if (!IsMouseLeftPressed()) {
      isDraggingControlPoint_ = false;
    }
    hoveredPartId_ = -1;
    return;
  }
#endif

  if (usingCamera_ == nullptr) {
    return;
  }

  Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

  // まずマウスが当たっている部位を更新する
  UpdateHoveredPartFromMouseRay(mouseRay);

  // すでに操作点を選択している場合、
  // マウスがその部位メッシュ範囲から外れたら選択解除する
  if (selectedControlPointIndex_ >= 0 && !isDraggingControlPoint_) {
    if (!IsMouseRayInsideSelectedControlMesh(mouseRay)) {
      ClearControlPointSelection();
      return;
    }
  }

  // ホイールで操作点の半径を拡縮する
  UpdateControlPointWheelScaling();

  // 左クリック開始時に操作点を拾う
  if (IsMouseLeftTriggered()) {
    if (PickControlPointFromMouseRay(mouseRay)) {
      isDraggingControlPoint_ = true;
    }
  }

  // ドラッグ中は現在の Ray から操作点位置を更新する
  if (isDraggingControlPoint_ && IsMouseLeftPressed()) {
    MoveSelectedControlPointFromMouseRay(mouseRay);
  }

  // 左ボタンを離したらドラッグ終了
  if (IsMouseLeftReleased()) {
    isDraggingControlPoint_ = false;
  }
}

bool ModScene::PickControlPointFromMouseRay(const Ray &mouseRay) {
  const int visiblePartId =
      (hoveredPartId_ >= 0) ? hoveredPartId_ : selectedPartId_;

  if (visiblePartId < 0) {
    return false;
  }

  // 胴体共有点を優先して判定する
  if (IsTorsoVisiblePartId(visiblePartId)) {
    float nearestT = FLT_MAX;
    int nearestPointIndex = -1;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Sphere sphere{};
      sphere.center =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      sphere.radius = torsoControlPoints_[i].radius;

      float t = 0.0f;
      if (!crashDecision(sphere, mouseRay, &t)) {
        continue;
      }

      if (t < nearestT) {
        nearestT = t;
        nearestPointIndex = static_cast<int>(i);
      }
    }

    if (nearestPointIndex < 0) {
      return false;
    }

    selectedControlPartId_ = -2;
    selectedControlPointIndex_ = nearestPointIndex;
    selectedPartId_ = visiblePartId;
    reattachParentId_ = -1;
    reattachConnectorId_ = -1;

    const Vector3 worldPos = GetTorsoControlPointWorldPosition(
        torsoControlPoints_[static_cast<size_t>(nearestPointIndex)].role);

    dragControlPlaneZ_ = worldPos.z;

    Vector3 hitPoint{};
    if (RayPlaneIntersectionZ(mouseRay, dragControlPlaneZ_, &hitPoint)) {
      dragControlPointOffset_ = Subtract(worldPos, hitPoint);
    } else {
      dragControlPointOffset_ = ZeroV();
    }

    return true;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return false;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return false;
  }

  float nearestT = FLT_MAX;
  int nearestPointIndex = -1;

  Object *object = modObjects_[controlOwnerId].get();
  if (object == nullptr) {
    return false;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  for (size_t pointIndex = 0; pointIndex < points.size(); ++pointIndex) {
    if (!(points[pointIndex].movable || points[pointIndex].isConnectionPoint)) {
      continue;
    }

    Sphere sphere{};
    sphere.center = body.GetControlPointWorldPosition(object, pointIndex);
    sphere.radius = points[pointIndex].radius;

    float t = 0.0f;
    if (!crashDecision(sphere, mouseRay, &t)) {
      continue;
    }

    if (t < nearestT) {
      nearestT = t;
      nearestPointIndex = static_cast<int>(pointIndex);
    }
  }

  if (nearestPointIndex < 0) {
    return false;
  }

  selectedControlPartId_ = controlOwnerId;
  selectedControlPointIndex_ = nearestPointIndex;

  selectedPartId_ = visiblePartId;
  reattachParentId_ = -1;
  reattachConnectorId_ = -1;

  const Vector3 worldPos =
      modBodies_[controlOwnerId].GetControlPointWorldPosition(
          object, static_cast<size_t>(nearestPointIndex));

  dragControlPlaneZ_ = worldPos.z;

  Vector3 hitPoint{};
  if (RayPlaneIntersectionZ(mouseRay, dragControlPlaneZ_, &hitPoint)) {
    dragControlPointOffset_ = Subtract(worldPos, hitPoint);
  } else {
    dragControlPointOffset_ = ZeroV();
  }

  return true;
}

void ModScene::MoveSelectedControlPointFromMouseRay(const Ray &mouseRay) {
  if (selectedControlPointIndex_ < 0) {
    return;
  }

  Vector3 hitPoint{};
  if (!RayPlaneIntersectionZ(mouseRay, dragControlPlaneZ_, &hitPoint)) {
    return;
  }

  const Vector3 targetWorld = Add(hitPoint, dragControlPointOffset_);

  // torso 共有点
  if (selectedControlPartId_ == -2) {
    const int chestBodyId = assembly_.GetBodyId();
    if (chestBodyId < 0 || modObjects_.count(chestBodyId) == 0) {
      return;
    }

    Object *bodyObject = modObjects_[chestBodyId].get();
    if (bodyObject == nullptr) {
      return;
    }

    const Vector3 rootWorld =
        ModObjectUtil::ComputeObjectRootWorldTranslate(bodyObject);
    const Vector3 targetLocal = Subtract(targetWorld, rootWorld);

    MoveTorsoControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                          targetLocal);
    return;
  }

  if (selectedControlPartId_ < 0) {
    return;
  }

  if (modBodies_.count(selectedControlPartId_) == 0 ||
      modObjects_.count(selectedControlPartId_) == 0) {
    return;
  }

  ModBody &body = modBodies_[selectedControlPartId_];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  if (selectedControlPointIndex_ >= static_cast<int>(points.size())) {
    return;
  }

  if (!points[static_cast<size_t>(selectedControlPointIndex_)].movable) {
    return;
  }

  Object *object = modObjects_[selectedControlPartId_].get();
  if (object == nullptr) {
    return;
  }

  const Vector3 rootWorld =
      ModObjectUtil::ComputeObjectRootWorldTranslate(object);
  const Vector3 targetLocal = Subtract(targetWorld, rootWorld);

  body.MoveControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                        targetLocal);
}

void ModScene::ClearControlPointSelection() {
  selectedControlPartId_ = -1;
  selectedControlPointIndex_ = -1;
  isDraggingControlPoint_ = false;
  dragControlPlaneZ_ = 0.0f;
  dragControlPointOffset_ = {0.0f, 0.0f, 0.0f};
  hoveredPartId_ = -1;
}

void ModScene::UpdateHoveredPartFromMouseRay(const Ray& mouseRay) {
    // ドラッグ中は選択部位を優先して表示する
    if (isDraggingControlPoint_ && selectedControlPartId_ >= 0) {
        hoveredPartId_ = selectedPartId_;
        return;
    }

    float nearestT = FLT_MAX;
    int nearestPartId = -1;

    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
        const int partId = orderedPartIds_[i];

        Vector3 capsuleStart{};
        Vector3 capsuleEnd{};
        float capsuleRadius = 0.0f;

        if (!BuildPartPickCapsule(partId, capsuleStart, capsuleEnd, capsuleRadius)) {
            continue;
        }

        float t = 0.0f;
        if (!IntersectRayCapsule(mouseRay, capsuleStart, capsuleEnd, capsuleRadius,
            &t)) {
            continue;
        }

        if (t < nearestT) {
            nearestT = t;
            nearestPartId = partId;
        }
    }

    hoveredPartId_ = nearestPartId;
}

void ModScene::EnsureControlPointGizmoCount(size_t requiredCount) {
  while (controlPointGizmos_.size() < requiredCount) {
    std::unique_ptr<Object> gizmo = std::make_unique<Object>();
    gizmo->IntObject(system_);
    gizmo->CreateDefaultData();
    gizmo->modelHandle_ = config::default_Sphere_MeshBufferHandle_;

    // カメラ正面向きで見やすくする
    gizmo->isBillboard_ = true;

    if (!gizmo->objectParts_.empty()) {
      gizmo->objectParts_[0].materialConfig->textureHandle =
          controlPointGizmoTextureHandle_;
      gizmo->objectParts_[0].materialConfig->useModelTexture = false;
      gizmo->objectParts_[0].materialConfig->enableLighting = false;
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 1.0f, 1.0f, 1.0f);
    }

    controlPointGizmos_.push_back(std::move(gizmo));
  }
}

void ModScene::UpdateControlPointGizmos() {
  activeControlPointGizmoCount_ = 0;

  int visiblePartId = -1;
  if (isDraggingControlPoint_ && selectedPartId_ >= 0) {
    visiblePartId = selectedPartId_;
  } else if (hoveredPartId_ >= 0) {
    visiblePartId = hoveredPartId_;
  } else if (selectedPartId_ >= 0) {
    visiblePartId = selectedPartId_;
  }

  if (IsTorsoVisiblePartId(visiblePartId)) {
    const int chestId = assembly_.GetBodyId();
    if (chestId >= 0) {
      visiblePartId = chestId;
    }
  }

  if (visiblePartId < 0) {
    return;
  }

  // torso 共有点表示
  if (IsTorsoVisiblePartId(visiblePartId)) {
    size_t visibleCount = 0;
    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (torsoControlPoints_[i].movable ||
          torsoControlPoints_[i].isConnectionPoint) {
        ++visibleCount;
      }
    }

    EnsureControlPointGizmoCount(visibleCount);

    const Vector3 cameraPos = usingCamera_->GetTransform().translate;
    size_t gizmoIndex = 0;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Object *gizmo = controlPointGizmos_[gizmoIndex].get();
      if (gizmo == nullptr || gizmo->objectParts_.empty()) {
        continue;
      }

      const bool isSelected =
          (selectedControlPartId_ == -2 &&
           selectedControlPointIndex_ == static_cast<int>(i));

      const Vector3 worldPos =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      const float influenceRadius = torsoControlPoints_[i].radius;
      const float drawRadius = GetControlPointGizmoDrawRadius(influenceRadius);

      const Vector3 toCamera =
          NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
      const Vector3 drawPos =
          Add(worldPos, Multiply(drawRadius * 0.75f, toCamera));

      gizmo->mainPosition.transform.translate = drawPos;
      gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
      gizmo->mainPosition.transform.scale = {
          drawRadius * 2.0f, drawRadius * 2.0f, drawRadius * 2.0f};

      gizmo->objectParts_[0].materialConfig->useModelTexture = false;
      gizmo->objectParts_[0].materialConfig->enableLighting = false;

      if (isSelected) {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(1.0f, 0.45f, 0.2f, 1.0f);
      } else if (torsoControlPoints_[i].isConnectionPoint) {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(1.0f, 0.9f, 0.2f, 1.0f);
      } else {
        gizmo->objectParts_[0].materialConfig->textureColor =
            MakeColor(0.35f, 0.9f, 1.0f, 1.0f);
      }

      gizmo->Update(usingCamera_);
      ++gizmoIndex;
    }

    activeControlPointGizmoCount_ = gizmoIndex;
    return;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return;
  }

  Object *partObject = modObjects_[controlOwnerId].get();
  if (partObject == nullptr) {
    return;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  size_t visibleCount = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (points[i].movable || points[i].isConnectionPoint) {
      ++visibleCount;
    }
  }

  EnsureControlPointGizmoCount(visibleCount);

  const Vector3 cameraPos = usingCamera_->GetTransform().translate;

  size_t gizmoIndex = 0;
  for (size_t i = 0; i < points.size(); ++i) {
    if (!(points[i].movable || points[i].isConnectionPoint)) {
      continue;
    }

    Object *gizmo = controlPointGizmos_[gizmoIndex].get();
    if (gizmo == nullptr || gizmo->objectParts_.empty()) {
      continue;
    }

    const bool isSelected = (selectedControlPartId_ == controlOwnerId &&
                             selectedControlPointIndex_ == static_cast<int>(i));

    const Vector3 worldPos = body.GetControlPointWorldPosition(partObject, i);
    const float influenceRadius = points[i].radius;
    const float drawRadius = GetControlPointGizmoDrawRadius(influenceRadius);

    const Vector3 toCamera =
        NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
    const Vector3 drawPos =
        Add(worldPos, Multiply(drawRadius * 0.75f, toCamera));

    gizmo->mainPosition.transform.translate = drawPos;
    gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
    gizmo->mainPosition.transform.scale = {drawRadius * 2.0f, drawRadius * 2.0f,
                                           drawRadius * 2.0f};

    gizmo->objectParts_[0].materialConfig->useModelTexture = false;
    gizmo->objectParts_[0].materialConfig->enableLighting = false;

    if (isSelected) {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 0.45f, 0.2f, 1.0f);
    } else if (points[i].isConnectionPoint) {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(1.0f, 0.9f, 0.2f, 1.0f);
    } else {
      gizmo->objectParts_[0].materialConfig->textureColor =
          MakeColor(0.35f, 0.9f, 1.0f, 1.0f);
    }

    gizmo->Update(usingCamera_);
    ++gizmoIndex;
  }

  activeControlPointGizmoCount_ = gizmoIndex;
}

void ModScene::DrawControlPointGizmos() {
  for (size_t i = 0; i < activeControlPointGizmoCount_; ++i) {
    Object *gizmo = controlPointGizmos_[i].get();
    if (gizmo != nullptr) {
      gizmo->Draw();
    }
  }
}

void ModScene::ResetTorsoControlPoints() {
  torsoControlPoints_.clear();

  torsoChestToBellyLength_ = 0.45f;
  torsoBellyToWaistLength_ = 0.45f;

  TorsoControlPoint chest{};
  chest.role = ModControlPointRole::Chest;
  chest.localPosition = {0.0f, 0.45f, 0.0f};
  chest.radius = 0.12f;
  chest.movable = true;
  chest.isConnectionPoint = true;
  chest.acceptsParent = false;
  chest.acceptsChild = true;
  torsoControlPoints_.push_back(chest);

  TorsoControlPoint belly{};
  belly.role = ModControlPointRole::Belly;
  belly.localPosition = {0.0f, 0.0f, 0.0f};
  belly.radius = 0.10f;
  belly.movable = true;
  belly.isConnectionPoint = true;
  belly.acceptsParent = true;
  belly.acceptsChild = true;
  torsoControlPoints_.push_back(belly);

  TorsoControlPoint waist{};
  waist.role = ModControlPointRole::Waist;
  waist.localPosition = {0.0f, -0.45f, 0.0f};
  waist.radius = 0.12f;
  waist.movable = true;
  waist.isConnectionPoint = true;
  waist.acceptsParent = true;
  waist.acceptsChild = true;
  torsoControlPoints_.push_back(waist);
}

int ModScene::FindTorsoControlPointIndex(ModControlPointRole role) const {
  for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
    if (torsoControlPoints_[i].role == role) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

Vector3 ModScene::ResolveDynamicAttachBase(const PartNode& parentNode,
    const PartNode& childNode) const {
    const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
        parentNode.part, childNode.part, childNode.side);

    // ------------------------------------------------------------
    // torso 親:
    // 胸系の子は Chest、脚系の子は Waist を基準にする。
    // さらに基準操作点の radius 比率で接続オフセットも拡縮する。
    // ------------------------------------------------------------
    if (parentNode.part == ModBodyPart::ChestBody ||
        parentNode.part == ModBodyPart::StomachBody) {
        const int chestIndex =
            FindTorsoControlPointIndex(ModControlPointRole::Chest);
        const int waistIndex =
            FindTorsoControlPointIndex(ModControlPointRole::Waist);

        const Vector3 defaultChest = { 0.0f, 0.45f, 0.0f };
        const Vector3 defaultWaist = { 0.0f, -0.45f, 0.0f };

        const Vector3 currentChest =
            (chestIndex >= 0)
            ? torsoControlPoints_[static_cast<size_t>(chestIndex)].localPosition
            : defaultChest;

        const Vector3 currentWaist =
            (waistIndex >= 0)
            ? torsoControlPoints_[static_cast<size_t>(waistIndex)].localPosition
            : defaultWaist;

        const float defaultChestRadius = 0.12f;
        const float defaultWaistRadius = 0.12f;

        const float currentChestRadius =
            GetTorsoControlPointRadius(ModControlPointRole::Chest,
                defaultChestRadius);

        const float currentWaistRadius =
            GetTorsoControlPointRadius(ModControlPointRole::Waist,
                defaultWaistRadius);

        const float chestRadiusRatio =
            currentChestRadius / (std::max)(defaultChestRadius, 0.0001f);

        const float waistRadiusRatio =
            currentWaistRadius / (std::max)(defaultWaistRadius, 0.0001f);

        switch (childNode.part) {
        case ModBodyPart::Neck:
        case ModBodyPart::Head: {
            const Vector3 relative = Subtract(defaultAttach, defaultChest);

            // 上方向と横方向を胸の拡縮に追従させる
            const Vector3 scaledRelative = ScaleVectorComponents(
                relative, { chestRadiusRatio, chestRadiusRatio, chestRadiusRatio });

            return Add(currentChest, scaledRelative);
        }

        case ModBodyPart::LeftUpperArm:
        case ModBodyPart::RightUpperArm: {
            const Vector3 relative = Subtract(defaultAttach, defaultChest);

            // 肩位置も胸の太さ変化に追従
            const Vector3 scaledRelative = ScaleVectorComponents(
                relative, { chestRadiusRatio, chestRadiusRatio, chestRadiusRatio });

            return Add(currentChest, scaledRelative);
        }

        case ModBodyPart::LeftThigh:
        case ModBodyPart::RightThigh: {
            const Vector3 relative = Subtract(defaultAttach, defaultWaist);

            // 腰位置も waist 半径の変化に追従
            const Vector3 scaledRelative = ScaleVectorComponents(
                relative, { waistRadiusRatio, waistRadiusRatio, waistRadiusRatio });

            return Add(currentWaist, scaledRelative);
        }

        default:
            return defaultAttach;
        }
    }

    // ------------------------------------------------------------
    // Head 親:
    // 将来的に頭に腕や脚が付くときのため、
    // HeadCenter 半径を使って接続位置を拡縮する。
    // ------------------------------------------------------------
    if (parentNode.part == ModBodyPart::Head) {
        const float defaultHeadRadius = 0.11f;
        const float currentHeadRadius =
            GetPartControlPointRadius(parentNode.id, ModControlPointRole::HeadCenter,
                defaultHeadRadius);

        const float headRadiusRatio =
            currentHeadRadius / (std::max)(defaultHeadRadius, 0.0001f);

        return ScaleVectorComponents(defaultAttach,
            { headRadiusRatio, headRadiusRatio,
             headRadiusRatio });
    }

    // ------------------------------------------------------------
    // Neck 親にぶら下がる Head は root 一致
    // ------------------------------------------------------------
    if (parentNode.part == ModBodyPart::Neck &&
        childNode.part == ModBodyPart::Head) {
        return { 0.0f, 0.0f, 0.0f };
    }

    // ------------------------------------------------------------
    // それ以外:
    // 既存の見た目スケールで追従させる
    // param.scale / length を使う部位向け
    // ------------------------------------------------------------
    if (modBodies_.count(parentNode.id) > 0) {
        const Vector3 parentScale = modBodies_.at(parentNode.id).GetVisualScaleRatio();
        return ScaleVectorComponents(defaultAttach, parentScale);
    }

    return defaultAttach;
}

float ModScene::GetTorsoControlPointRadius(ModControlPointRole role,
    float defaultRadius) const {
    const int index = FindTorsoControlPointIndex(role);
    if (index < 0) {
        return defaultRadius;
    }

    return torsoControlPoints_[static_cast<size_t>(index)].radius;
}

float ModScene::GetPartControlPointRadius(int partId, ModControlPointRole role,
    float defaultRadius) const {
    auto it = modBodies_.find(partId);
    if (it == modBodies_.end()) {
        return defaultRadius;
    }

    const int pointIndex = it->second.FindControlPointIndex(role);
    if (pointIndex < 0) {
        return defaultRadius;
    }

    const std::vector<ModControlPoint>& points = it->second.GetControlPoints();
    if (static_cast<size_t>(pointIndex) >= points.size()) {
        return defaultRadius;
    }

    return points[static_cast<size_t>(pointIndex)].radius;
}

Vector3 ModScene::ScaleVectorComponents(const Vector3& value,
    const Vector3& scale) const {
    return {
        value.x * scale.x,
        value.y * scale.y,
        value.z * scale.z,
    };
}

Vector3
ModScene::ResolveAttachedLocalTranslate(const PartNode &childNode) const {
  if (childNode.parentId < 0) {
    return childNode.localTransform.translate;
  }

  const PartNode *parentNode = assembly_.FindNode(childNode.parentId);
  if (parentNode == nullptr) {
    return childNode.localTransform.translate;
  }

  const Vector3 defaultAttach = assembly_.GetDefaultAttachLocal(
      parentNode->part, childNode.part, childNode.side);

  const Vector3 dynamicBase = ResolveDynamicAttachBase(*parentNode, childNode);

  // 保存してある localTransform.translate は「デフォルトからの編集込み位置」
  // なので差分だけ取り出して現在の親形状へ乗せ直す
  const Vector3 offsetFromDefault =
      Subtract(childNode.localTransform.translate, defaultAttach);

  return Add(dynamicBase, offsetFromDefault);
}

void ModScene::UpdateControlPointWheelScaling() {
  if (usingCamera_ == nullptr) {
    return;
  }

  // 中ボタン押し中はデバッグカメラ操作を優先する
  if (system_->GetMouseIsPush(2)) {
    return;
  }

  const int wheelDelta = system_->GetMouseScrollOrigin();
  if (wheelDelta == 0) {
    return;
  }

  const float scaleFactor = GetWheelScaleFactorFromDelta(wheelDelta);
  if (scaleFactor == 1.0f) {
    return;
  }

  // すでに選択中の操作点があるなら最優先でそれを拡縮する
  if (selectedControlPointIndex_ >= 0) {
    if (selectedControlPartId_ == -2) {
      ScaleTorsoControlPoint(static_cast<size_t>(selectedControlPointIndex_),
                             scaleFactor);
      return;
    }

    if (selectedControlPartId_ >= 0 &&
        modBodies_.count(selectedControlPartId_) > 0) {
      modBodies_[selectedControlPartId_].ScaleControlPoint(
          static_cast<size_t>(selectedControlPointIndex_), scaleFactor);
      return;
    }
  }

  // 未選択なら、hover中または選択中の部位を基準に
  // マウス Ray に最も近い操作点を見つけてその場で拡縮する
  const int visiblePartId =
      (hoveredPartId_ >= 0) ? hoveredPartId_ : selectedPartId_;

  if (visiblePartId < 0) {
    return;
  }

  Ray mouseRay = usingCamera_->ScreenPointToRay(system_->GetMousePosVector2());

  // 胴体共有点
  if (IsTorsoVisiblePartId(visiblePartId)) {
    float nearestT = FLT_MAX;
    int nearestPointIndex = -1;

    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      if (!(torsoControlPoints_[i].movable ||
            torsoControlPoints_[i].isConnectionPoint)) {
        continue;
      }

      Sphere sphere{};
      sphere.center =
          GetTorsoControlPointWorldPosition(torsoControlPoints_[i].role);
      sphere.radius = torsoControlPoints_[i].radius;

      float t = 0.0f;
      if (!crashDecision(sphere, mouseRay, &t)) {
        continue;
      }

      if (t < nearestT) {
        nearestT = t;
        nearestPointIndex = static_cast<int>(i);
      }
    }

    if (nearestPointIndex >= 0) {
      selectedControlPartId_ = -2;
      selectedControlPointIndex_ = nearestPointIndex;
      selectedPartId_ = visiblePartId;
      ScaleTorsoControlPoint(static_cast<size_t>(nearestPointIndex),
                             scaleFactor);
    }

    return;
  }

  const int controlOwnerId =
      ResolveControlOwnerPartId(assembly_, visiblePartId);

  if (controlOwnerId < 0) {
    return;
  }

  if (modBodies_.count(controlOwnerId) == 0 ||
      modObjects_.count(controlOwnerId) == 0) {
    return;
  }

  Object *object = modObjects_[controlOwnerId].get();
  if (object == nullptr) {
    return;
  }

  ModBody &body = modBodies_[controlOwnerId];
  const std::vector<ModControlPoint> &points = body.GetControlPoints();

  float nearestT = FLT_MAX;
  int nearestPointIndex = -1;

  for (size_t i = 0; i < points.size(); ++i) {
    if (!(points[i].movable || points[i].isConnectionPoint)) {
      continue;
    }

    Sphere sphere{};
    sphere.center = body.GetControlPointWorldPosition(object, i);
    sphere.radius = points[i].radius;

    float t = 0.0f;
    if (!crashDecision(sphere, mouseRay, &t)) {
      continue;
    }

    if (t < nearestT) {
      nearestT = t;
      nearestPointIndex = static_cast<int>(i);
    }
  }

  if (nearestPointIndex >= 0) {
    selectedControlPartId_ = controlOwnerId;
    selectedControlPointIndex_ = nearestPointIndex;
    selectedPartId_ = visiblePartId;

    body.ScaleControlPoint(static_cast<size_t>(nearestPointIndex), scaleFactor);
  }
}

bool ModScene::MoveTorsoControlPoint(size_t index,
                                     const Vector3 &newLocalPosition) {
  if (index >= torsoControlPoints_.size()) {
    return false;
  }

  if (!torsoControlPoints_[index].movable) {
    return false;
  }

  const int chestIndex = FindTorsoControlPointIndex(ModControlPointRole::Chest);
  const int bellyIndex = FindTorsoControlPointIndex(ModControlPointRole::Belly);
  const int waistIndex = FindTorsoControlPointIndex(ModControlPointRole::Waist);

  if (chestIndex < 0 || bellyIndex < 0 || waistIndex < 0) {
    return false;
  }

  Vector3 chestPos =
      torsoControlPoints_[static_cast<size_t>(chestIndex)].localPosition;
  Vector3 bellyPos =
      torsoControlPoints_[static_cast<size_t>(bellyIndex)].localPosition;
  Vector3 waistPos =
      torsoControlPoints_[static_cast<size_t>(waistIndex)].localPosition;

  const float chestBellyMin = 0.20f;
  const float chestBellyMax = 1.50f;
  const float bellyWaistMin = 0.20f;
  const float bellyWaistMax = 1.50f;

  if (index == static_cast<size_t>(chestIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(bellyPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, 1.0f, 0.0f});

    if (candidate.y <= bellyPos.y + 0.05f) {
      candidate.y = bellyPos.y + 0.05f;
      candidate = ClampDistance(bellyPos, candidate, chestBellyMin,
                                chestBellyMax, {0.0f, 1.0f, 0.0f});
    }

    torsoControlPoints_[index].localPosition = candidate;
    torsoChestToBellyLength_ = Length(Subtract(candidate, bellyPos));
    return true;
  }

  if (index == static_cast<size_t>(bellyIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(chestPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, -1.0f, 0.0f});

    candidate = ClampDistance(waistPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, 1.0f, 0.0f});

    if (candidate.y >= chestPos.y - 0.05f) {
      candidate.y = chestPos.y - 0.05f;
    }
    if (candidate.y <= waistPos.y + 0.05f) {
      candidate.y = waistPos.y + 0.05f;
    }

    candidate = ClampDistance(chestPos, candidate, chestBellyMin, chestBellyMax,
                              {0.0f, -1.0f, 0.0f});
    candidate = ClampDistance(waistPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, 1.0f, 0.0f});

    torsoControlPoints_[index].localPosition = candidate;
    torsoChestToBellyLength_ = Length(Subtract(chestPos, candidate));
    torsoBellyToWaistLength_ = Length(Subtract(candidate, waistPos));
    return true;
  }

  if (index == static_cast<size_t>(waistIndex)) {
    Vector3 candidate = newLocalPosition;

    candidate = ClampDistance(bellyPos, candidate, bellyWaistMin, bellyWaistMax,
                              {0.0f, -1.0f, 0.0f});

    if (candidate.y >= bellyPos.y - 0.05f) {
      candidate.y = bellyPos.y - 0.05f;
      candidate = ClampDistance(bellyPos, candidate, bellyWaistMin,
                                bellyWaistMax, {0.0f, -1.0f, 0.0f});
    }

    torsoControlPoints_[index].localPosition = candidate;
    torsoBellyToWaistLength_ = Length(Subtract(bellyPos, candidate));
    return true;
  }

  return false;
}

bool ModScene::ScaleTorsoControlPoint(size_t index, float scaleFactor) {
  if (index >= torsoControlPoints_.size()) {
    return false;
  }

  if (scaleFactor <= 0.0f) {
    return false;
  }

  const ModControlPointRole role = torsoControlPoints_[index].role;

  float defaultRadius = 0.10f;
  switch (role) {
  case ModControlPointRole::Chest:
    defaultRadius = 0.12f;
    break;
  case ModControlPointRole::Belly:
    defaultRadius = 0.10f;
    break;
  case ModControlPointRole::Waist:
    defaultRadius = 0.12f;
    break;
  default:
    defaultRadius = 0.10f;
    break;
  }

  const float minRadius = defaultRadius * 0.60f;
  const float maxRadius = defaultRadius * 2.50f;

  float newRadius = torsoControlPoints_[index].radius * scaleFactor;
  newRadius = ClampFloatLocal(newRadius, minRadius, maxRadius);

  torsoControlPoints_[index].radius = newRadius;
  return true;
}

bool ModScene::BuildPartPickCapsule(int partId, Vector3& outStart,
    Vector3& outEnd, float& outRadius) const {
    outStart = { 0.0f, 0.0f, 0.0f };
    outEnd = { 0.0f, 0.0f, 0.0f };
    outRadius = 0.0f;

    const PartNode* node = assembly_.FindNode(partId);
    if (node == nullptr) {
        return false;
    }

    ModControlPointRole startRole = ModControlPointRole::None;
    ModControlPointRole endRole = ModControlPointRole::None;
    if (!GetPickSegmentRoles(node->part, startRole, endRole)) {
        return false;
    }

    // torso は共有操作点から取る
    if (node->part == ModBodyPart::ChestBody ||
        node->part == ModBodyPart::StomachBody) {
        const int startIndex = FindTorsoControlPointIndex(startRole);
        const int endIndex = FindTorsoControlPointIndex(endRole);

        if (startIndex < 0 || endIndex < 0) {
            return false;
        }

        outStart = GetTorsoControlPointWorldPosition(startRole);
        outEnd = GetTorsoControlPointWorldPosition(endRole);

        const float startRadius =
            torsoControlPoints_[static_cast<size_t>(startIndex)].radius;
        const float endRadius =
            torsoControlPoints_[static_cast<size_t>(endIndex)].radius;

        outRadius = (std::max)(startRadius, endRadius) * 1.05f;
        outRadius = (std::max)(outRadius, 0.06f);
        return true;
    }

    // それ以外は owner の操作点から取る
    const int ownerId = ResolveControlOwnerPartId(assembly_, partId);
    if (ownerId < 0) {
        return false;
    }

    if (modBodies_.count(ownerId) == 0 || modObjects_.count(ownerId) == 0) {
        return false;
    }

    const ModBody& body = modBodies_.at(ownerId);
    const Object* object = modObjects_.at(ownerId).get();
    if (object == nullptr) {
        return false;
    }

    const int startIndex = body.FindControlPointIndex(startRole);
    const int endIndex = body.FindControlPointIndex(endRole);
    if (startIndex < 0 || endIndex < 0) {
        return false;
    }

    const std::vector<ModControlPoint>& points = body.GetControlPoints();

    outStart =
        body.GetControlPointWorldPosition(object, static_cast<size_t>(startIndex));
    outEnd =
        body.GetControlPointWorldPosition(object, static_cast<size_t>(endIndex));

    const float startRadius = points[static_cast<size_t>(startIndex)].radius;
    const float endRadius = points[static_cast<size_t>(endIndex)].radius;

    outRadius = (std::max)(startRadius, endRadius) * 1.05f;
    outRadius = (std::max)(outRadius, 0.05f);

    return true;
}

bool ModScene::IsMouseRayInsideSelectedControlMesh(const Ray& mouseRay) const {
    if (selectedControlPointIndex_ < 0) {
        return false;
    }

    // torso の共有点選択中は torso 全体のどちらかに当たっていれば有効
    if (selectedControlPartId_ == -2) {
        for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
            const int partId = orderedPartIds_[i];
            if (!IsTorsoVisiblePartId(partId)) {
                continue;
            }

            Vector3 capsuleStart{};
            Vector3 capsuleEnd{};
            float capsuleRadius = 0.0f;

            if (!BuildPartPickCapsule(partId, capsuleStart, capsuleEnd,
                capsuleRadius)) {
                continue;
            }

            float t = 0.0f;
            if (IntersectRayCapsule(mouseRay, capsuleStart, capsuleEnd, capsuleRadius,
                &t)) {
                return true;
            }
        }

        return false;
    }

    if (selectedPartId_ < 0) {
        return false;
    }

    Vector3 capsuleStart{};
    Vector3 capsuleEnd{};
    float capsuleRadius = 0.0f;

    if (!BuildPartPickCapsule(selectedPartId_, capsuleStart, capsuleEnd,
        capsuleRadius)) {
        return false;
    }

    float t = 0.0f;
    return IntersectRayCapsule(mouseRay, capsuleStart, capsuleEnd, capsuleRadius,
        &t);
}

int ModScene::ResolveFadeGroupId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  // 胴体は Chest / Stomach を同じグループとして扱う
  if (node->part == ModBodyPart::ChestBody ||
      node->part == ModBodyPart::StomachBody) {
    const int chestId = assembly_.GetBodyId();
    if (chestId >= 0) {
      return chestId;
    }
    return partId;
  }

  return ResolveControlOwnerPartId(assembly_, partId);
}

bool ModScene::IsTorsoPart(ModBodyPart part) const {
  return part == ModBodyPart::ChestBody || part == ModBodyPart::StomachBody;
}

bool ModScene::IsTorsoVisiblePartId(int partId) const {
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return false;
  }
  return IsTorsoPart(node->part);
}

Vector3
ModScene::GetTorsoControlPointWorldPosition(ModControlPointRole role) const {
  const int chestBodyId = assembly_.GetBodyId();
  if (chestBodyId < 0 || modObjects_.count(chestBodyId) == 0) {
    return ZeroV();
  }

  const int pointIndex = FindTorsoControlPointIndex(role);
  if (pointIndex < 0) {
    return ZeroV();
  }

  const Object *bodyObject = modObjects_.at(chestBodyId).get();
  if (bodyObject == nullptr) {
    return ZeroV();
  }

  return Add(
      bodyObject->mainPosition.transform.translate,
      torsoControlPoints_[static_cast<size_t>(pointIndex)].localPosition);
}

void ModScene::UpdateModObjects() {
  // AssemblyGraph の接続関係を Scene の Object 階層へ反映する
  ApplyAssemblyToSceneHierarchy();

  // torso 共有点バッファを更新する
  torsoSharedPointsBuffer_.clear();
  torsoSharedPointsBuffer_.reserve(torsoControlPoints_.size());

  for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
    ModControlPoint point{};
    point.role = torsoControlPoints_[i].role;
    point.localPosition = torsoControlPoints_[i].localPosition;
    point.radius = torsoControlPoints_[i].radius;
    point.movable = torsoControlPoints_[i].movable;
    point.isConnectionPoint = torsoControlPoints_[i].isConnectionPoint;
    point.acceptsParent = torsoControlPoints_[i].acceptsParent;
    point.acceptsChild = torsoControlPoints_[i].acceptsChild;
    torsoSharedPointsBuffer_.push_back(point);
  }

  // まず各部位の外部点列参照を設定する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    const PartNode *node = assembly_.FindNode(id);
    if (node == nullptr) {
      continue;
    }

    if (modBodies_.count(id) == 0) {
      continue;
    }

    ModBody &body = modBodies_[id];
    body.ClearExternalSegmentSource();

    switch (node->part) {
    case ModBodyPart::ChestBody: {
      body.SetExternalSegmentSource(&torsoSharedPointsBuffer_,
                                    ModControlPointRole::Chest,
                                    ModControlPointRole::Belly);
      break;
    }

    case ModBodyPart::StomachBody: {
      body.SetExternalSegmentSource(&torsoSharedPointsBuffer_,
                                    ModControlPointRole::Belly,
                                    ModControlPointRole::Waist);
      break;
    }

    case ModBodyPart::LeftForeArm:
    case ModBodyPart::RightForeArm: {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);
      if (ownerId >= 0 && ownerId != id && modBodies_.count(ownerId) > 0) {
        body.SetExternalSegmentSource(&modBodies_[ownerId].GetControlPoints(),
                                      ModControlPointRole::Bend,
                                      ModControlPointRole::End);
      }
      break;
    }

    case ModBodyPart::LeftShin:
    case ModBodyPart::RightShin: {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);
      if (ownerId >= 0 && ownerId != id && modBodies_.count(ownerId) > 0) {
        body.SetExternalSegmentSource(&modBodies_[ownerId].GetControlPoints(),
                                      ModControlPointRole::Bend,
                                      ModControlPointRole::End);
      }
      break;
    }

    case ModBodyPart::Neck: {
      const int headId = FindFirstChildPartId(assembly_, id, ModBodyPart::Head);
      if (headId >= 0 && modBodies_.count(headId) > 0) {
        body.SetExternalSegmentSource(&modBodies_[headId].GetControlPoints(),
                                      ModControlPointRole::LowerNeck,
                                      ModControlPointRole::UpperNeck);
      }
      break;
    }

    default:
      break;
    }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];

    if (modObjects_.count(id) == 0 || modBodies_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object != nullptr) {
      modBodies_[id].Apply(object);
    }
  }

  int fadedGroupId = -1;

  // カーソルが実際に部位へ当たっているときだけ hover フェードする
  if (hoveredPartId_ >= 0) {
      fadedGroupId = ResolveFadeGroupId(hoveredPartId_);
  }

  // 操作点を選択している、またはドラッグ中なら
  // その部位グループを維持してフェードする
  if (selectedControlPointIndex_ >= 0) {
      if (selectedControlPartId_ == -2) {
          const int chestId = assembly_.GetBodyId();
          if (chestId >= 0) {
              fadedGroupId = ResolveFadeGroupId(chestId);
          }
      } else if (selectedPartId_ >= 0) {
          fadedGroupId = ResolveFadeGroupId(selectedPartId_);
      }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object == nullptr || object->objectParts_.empty()) {
      continue;
    }

    float alpha = 1.0f;

    if (fadedGroupId >= 0) {
      const int groupId = ResolveFadeGroupId(id);
      if (groupId == fadedGroupId) {
        alpha = 0.35f;
      }
    }

    for (size_t partIndex = 0; partIndex < object->objectParts_.size();
         ++partIndex) {
      if (object->objectParts_[partIndex].materialConfig != nullptr) {
        object->objectParts_[partIndex].materialConfig->textureColor.w = alpha;
      }
    }
  }

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }

  UpdateControlPointGizmos();
}

void ModScene::DrawModObjects() {
  // orderedPartIds_ の順に各部位 Object を描画する
  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int id = orderedPartIds_[i];
    if (modObjects_.count(id) == 0) {
      continue;
    }

    Object *object = modObjects_[id].get();
    if (object != nullptr) {
      object->Draw();
    }
  }
}

#ifdef USE_IMGUI
void ModScene::DrawAssemblyGui() {
  // 部位一覧ツリーを開いているときだけ一覧を描画する
  if (ImGui::TreeNode("Assembly Parts")) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int id = orderedPartIds_[i];
      const PartNode *node = assembly_.FindNode(id);
      if (node == nullptr) {
        continue;
      }

      // 各部位を選択可能なリストとして表示する
      ImGui::PushID(id);

      const bool selected = (id == selectedPartId_);
      char label[128];
      sprintf_s(label, "%s (Id=%d)", PartName(node->part), id);

      if (ImGui::Selectable(label, selected)) {
        SelectPart(id);
      }

      ImGui::PopID();
    }

    ImGui::TreePop();
  }
}

void ModScene::DrawSelectedPartGui() {
  // 選択部位が無い場合は案内だけ表示する
  if (selectedPartId_ < 0) {
    ImGui::Text("No part selected.");
    return;
  }

  // 選択部位が不正なら編集を中断する
  const PartNode *node = assembly_.FindNode(selectedPartId_);
  if (node == nullptr || modBodies_.count(selectedPartId_) == 0 ||
      modObjects_.count(selectedPartId_) == 0) {
    ImGui::Text("Selected part is invalid.");
    return;
  }

  // 編集対象の Object とパラメータを取得する
  Object *object = modObjects_[selectedPartId_].get();
  ModBodyPartParam &param = modBodies_[selectedPartId_].GetParam();

  // 選択中部位の基本情報を表示する
  ImGui::Separator();
  ImGui::Text("Selected Part");
  ImGui::Text("PartId: %d", selectedPartId_);
  ImGui::Text("Type: %s", PartName(node->part));
  ImGui::Text("ParentId: %d", node->parentId);

  // ローカル位置を直接編集できるようにする
  Vector3 local = node->localTransform.translate;
  if (ImGui::SliderFloat3("Local Translate", &local.x, -5.0f, 5.0f)) {
    assembly_.SetPartLocalTranslate(selectedPartId_, local);
  }

  // 見た目の有効状態、スケール、長さを編集できるようにする
  ImGui::Checkbox("Enabled", &param.enabled);
  ImGui::SliderFloat3("Mesh Scale", &param.scale.x, 0.2f, 5.0f);
  ImGui::SliderFloat("Length", &param.length, 0.2f, 5.0f, "%.2f");

  // 現在の mesh transform を確認用に表示する
  if (!object->objectParts_.empty()) {
    const Transform &mesh = object->objectParts_[0].transform;
    ImGui::Text("Mesh Translate : %.2f %.2f %.2f", mesh.translate.x,
                mesh.translate.y, mesh.translate.z);
    ImGui::Text("Mesh Scale     : %.2f %.2f %.2f", mesh.scale.x, mesh.scale.y,
                mesh.scale.z);
  }

  // 付け替え先の親部位とコネクタを選択するUIを表示する
  ImGui::Separator();
  ImGui::Text("Reattach");
  if (ImGui::BeginCombo("Parent Part",
                        reattachParentId_ >= 0 ? "Selected" : "None")) {
    for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
      const int candidateId = orderedPartIds_[i];
      if (candidateId == selectedPartId_) {
        continue;
      }

      const PartNode *candidate = assembly_.FindNode(candidateId);
      if (candidate == nullptr) {
        continue;
      }

      char parentLabel[128];
      sprintf_s(parentLabel, "%s (Id=%d)", PartName(candidate->part),
                candidateId);

      const bool selected = (candidateId == reattachParentId_);
      if (ImGui::Selectable(parentLabel, selected)) {
        reattachParentId_ = candidateId;
        reattachConnectorId_ = -1;
      }
      if (selected) {
        ImGui::SetItemDefaultFocus();
      }
    }
    ImGui::EndCombo();
  }

  // 親部位が決まっている場合は、親側コネクタも選べるようにする
  if (reattachParentId_ >= 0) {
    const PartNode *parentNode = assembly_.FindNode(reattachParentId_);
    if (parentNode != nullptr) {
      if (ImGui::BeginCombo("Parent Connector",
                            reattachConnectorId_ >= 0 ? "Selected" : "Auto")) {
        if (ImGui::Selectable("Auto", reattachConnectorId_ < 0)) {
          reattachConnectorId_ = -1;
        }

        for (size_t i = 0; i < parentNode->connectors.size(); ++i) {
          const ConnectorNode &connector = parentNode->connectors[i];
          char connectorLabel[256];
          sprintf_s(connectorLabel, "Id=%d Role=%s Side=%s", connector.id,
                    ConnectorRoleName(connector.role),
                    SideName(connector.side));

          const bool selected = (connector.id == reattachConnectorId_);
          if (ImGui::Selectable(connectorLabel, selected)) {
            reattachConnectorId_ = connector.id;
          }
          if (selected) {
            ImGui::SetItemDefaultFocus();
          }
        }

        ImGui::EndCombo();
      }
    }
  }

  // 選択部位の付け替えと削除を実行できるようにする
  if (ImGui::Button("Apply Reattach")) {
    ReattachSelectedPart();
  }

  ImGui::SameLine();
  if (ImGui::Button("Delete Selected Part")) {
    DeleteSelectedPart();
  }

  // 選択部位だけパラメータを初期化するボタンを表示する
  ImGui::Separator();
  ImGui::Text("Reset");

  if (ImGui::Button("Reset Selected Part Params")) {
    ResetSelectedPartParams();
  }

  ImGui::Separator();
  ImGui::Text("Control Points");

  if (IsTorsoVisiblePartId(selectedPartId_)) {
    for (size_t i = 0; i < torsoControlPoints_.size(); ++i) {
      const bool isSelectedPoint =
          (selectedControlPartId_ == -2 &&
           selectedControlPointIndex_ == static_cast<int>(i));

      ImGui::PushID(static_cast<int>(i));

      if (isSelectedPoint) {
        ImGui::Text("-> Point %d", static_cast<int>(i));
      } else {
        ImGui::Text("Point %d", static_cast<int>(i));
      }

      Vector3 localPos = torsoControlPoints_[i].localPosition;
      if (ImGui::SliderFloat3("Local Pos", &localPos.x, -5.0f, 5.0f)) {
        MoveTorsoControlPoint(i, localPos);
      }

      float radius = torsoControlPoints_[i].radius;
      if (ImGui::SliderFloat("Radius", &radius, 0.06f, 0.30f, "%.2f")) {
        torsoControlPoints_[i].radius = radius;
      }

      ImGui::Text("Influence Radius : %.2f", torsoControlPoints_[i].radius);
      ImGui::Text("Movable: %s",
                  torsoControlPoints_[i].movable ? "true" : "false");

      ImGui::Separator();
      ImGui::PopID();
    }
  } else {
    const int controlOwnerId =
        ResolveControlOwnerPartId(assembly_, selectedPartId_);

    if (controlOwnerId >= 0 && modBodies_.count(controlOwnerId) > 0) {
      const std::vector<ModControlPoint> &points =
          modBodies_[controlOwnerId].GetControlPoints();

      for (size_t i = 0; i < points.size(); ++i) {
        const bool isSelectedPoint =
            (selectedControlPartId_ == controlOwnerId &&
             selectedControlPointIndex_ == static_cast<int>(i));

        ImGui::PushID(static_cast<int>(i));

        if (isSelectedPoint) {
          ImGui::Text("-> Point %d", static_cast<int>(i));
        } else {
          ImGui::Text("Point %d", static_cast<int>(i));
        }

        Vector3 localPos = points[i].localPosition;
        if (ImGui::SliderFloat3("Local Pos", &localPos.x, -5.0f, 5.0f)) {
          modBodies_[controlOwnerId].MoveControlPoint(i, localPos);
        }

        float radius = points[i].radius;
        if (ImGui::SliderFloat("Radius", &radius, 0.05f, 0.30f, "%.2f")) {
          const float current = (std::max)(points[i].radius, 0.0001f);
          modBodies_[controlOwnerId].ScaleControlPoint(i, radius / current);
        }

        ImGui::Text("Influence Radius : %.2f", points[i].radius);
        ImGui::Text("Movable: %s", points[i].movable ? "true" : "false");

        ImGui::Separator();
        ImGui::PopID();
      }
    }
  }
}

void ModScene::DrawModGui() {
  ImGui::Begin("ModScene");

  ImGui::Text("MouseMiddleDrag : Move Camera");
  ImGui::Text("MouseRightDrag  : Rotate Camera");
  ImGui::Text("MouseLeftDrag   : Move Control Point");
  ImGui::Text("DIK_0           : Toggle DebugCamera");

  ImGui::Separator();
  ImGui::Text("Add Parts");

  if (ImGui::Button("Add Left Arm")) {
    if (assembly_.AddArmAssembly(PartSide::Left)) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Right Arm")) {
    if (assembly_.AddArmAssembly(PartSide::Right)) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }

  if (ImGui::Button("Add Left Leg")) {
    if (assembly_.AddLegAssembly(PartSide::Left)) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Right Leg")) {
    if (assembly_.AddLegAssembly(PartSide::Right)) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }

  if (ImGui::Button("Add Neck")) {
    if (assembly_.AddNeckPart()) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Body")) {
    if (assembly_.AddBodyPart()) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Add Head")) {
    if (assembly_.AddHeadPart()) {
      SyncObjectsWithAssembly();
      EnsureValidSelection();
    }
  }

  DrawAssemblyGui();
  DrawSelectedPartGui();

  ImGui::Separator();
  ImGui::Text("Global Reset");
  if (ImGui::Button("Reset All Part Params")) {
    ResetModBodies();
  }

  if (ImGui::Button("Reset To Default Humanoid")) {
    ResetToDefaultHumanoid();
  }

  ImGui::End();
}

const char *ModScene::ConnectorRoleName(ConnectorRole role) const {
  // 接続点役割を表示名へ変換する
  switch (role) {
  case ConnectorRole::Generic:
    return "Generic";
  case ConnectorRole::Neck:
    return "Neck";
  case ConnectorRole::Shoulder:
    return "Shoulder";
  case ConnectorRole::ArmJoint:
    return "ArmJoint";
  case ConnectorRole::Hip:
    return "Hip";
  case ConnectorRole::LegJoint:
    return "LegJoint";
  default:
    return "Unknown";
  }
}

const char *ModScene::SideName(PartSide side) const {
  // 左右属性を表示名へ変換する
  switch (side) {
  case PartSide::Center:
    return "Center";
  case PartSide::Left:
    return "Left";
  case PartSide::Right:
    return "Right";
  default:
    return "Unknown";
  }
}
#endif

void ModScene::SaveControlPointsToCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  auto &cp = customizeData_->controlPoints;

  cp.leftShoulderPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftShoulder);
  cp.leftElbowPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftElbow);
  cp.leftWristPos =
      GetControlPointLocalPosition(ModControlPointRole::LeftWrist);

  cp.rightShoulderPos =
      GetControlPointLocalPosition(ModControlPointRole::RightShoulder);
  cp.rightElbowPos =
      GetControlPointLocalPosition(ModControlPointRole::RightElbow);
  cp.rightWristPos =
      GetControlPointLocalPosition(ModControlPointRole::RightWrist);

  cp.leftHipPos = GetControlPointLocalPosition(ModControlPointRole::LeftHip);
  cp.leftKneePos = GetControlPointLocalPosition(ModControlPointRole::LeftKnee);
  cp.leftAnklePos =
      GetControlPointLocalPosition(ModControlPointRole::LeftAnkle);

  cp.rightHipPos = GetControlPointLocalPosition(ModControlPointRole::RightHip);
  cp.rightKneePos =
      GetControlPointLocalPosition(ModControlPointRole::RightKnee);
  cp.rightAnklePos =
      GetControlPointLocalPosition(ModControlPointRole::RightAnkle);

  cp.chestPos = GetControlPointLocalPosition(ModControlPointRole::Chest);
  cp.bellyPos = GetControlPointLocalPosition(ModControlPointRole::Belly);
  cp.waistPos = GetControlPointLocalPosition(ModControlPointRole::Waist);

  cp.lowerNeckPos =
      GetControlPointLocalPosition(ModControlPointRole::LowerNeck);
  cp.upperNeckPos =
      GetControlPointLocalPosition(ModControlPointRole::UpperNeck);
  cp.headCenterPos =
      GetControlPointLocalPosition(ModControlPointRole::HeadCenter);

  Logger::Log("SEND LeftShoulderPos : %.2f %.2f %.2f\n", cp.leftShoulderPos.x,
              cp.leftShoulderPos.y, cp.leftShoulderPos.z);

  Logger::Log("SEND LeftElbowPos : %.2f %.2f %.2f\n", cp.leftElbowPos.x,
              cp.leftElbowPos.y, cp.leftElbowPos.z);
  Logger::Log("SEND LeftWristPos : %.2f %.2f %.2f\n", cp.leftWristPos.x,
              cp.leftWristPos.y, cp.leftWristPos.z);

  Logger::Log("SEND RightShoulderPos : %.2f %.2f %.2f\n", cp.rightShoulderPos.x,
              cp.rightShoulderPos.y, cp.rightShoulderPos.z);
  Logger::Log("SEND RightElbowPos : %.2f %.2f %.2f\n", cp.rightElbowPos.x,
              cp.rightElbowPos.y, cp.rightElbowPos.z);
  Logger::Log("SEND RightWristPos : %.2f %.2f %.2f\n", cp.rightWristPos.x,
              cp.rightWristPos.y, cp.rightWristPos.z);

  Logger::Log("SEND LeftHipPos : %.2f %.2f %.2f\n", cp.leftHipPos.x,
              cp.leftHipPos.y, cp.leftHipPos.z);
  Logger::Log("SEND LeftKneePos : %.2f %.2f %.2f\n", cp.leftKneePos.x,
              cp.leftKneePos.y, cp.leftKneePos.z);
  Logger::Log("SEND LeftAnklePos : %.2f %.2f %.2f\n", cp.leftAnklePos.x,
              cp.leftAnklePos.y, cp.leftAnklePos.z);

  Logger::Log("SEND RightHipPos : %.2f %.2f %.2f\n", cp.rightHipPos.x,
              cp.rightHipPos.y, cp.rightHipPos.z);
  Logger::Log("SEND RightKneePos : %.2f %.2f %.2f\n", cp.rightKneePos.x,
              cp.rightKneePos.y, cp.rightKneePos.z);
  Logger::Log("SEND RightAnklePos : %.2f %.2f %.2f\n", cp.rightAnklePos.x,
              cp.rightAnklePos.y, cp.rightAnklePos.z);

  Logger::Log("SEND ChestPos : %.2f %.2f %.2f\n", cp.chestPos.x, cp.chestPos.y,
              cp.chestPos.z);
  Logger::Log("SEND BellyPos : %.2f %.2f %.2f\n", cp.bellyPos.x, cp.bellyPos.y,
              cp.bellyPos.z);
  Logger::Log("SEND WaistPos : %.2f %.2f %.2f\n", cp.waistPos.x, cp.waistPos.y,
              cp.waistPos.z);

  Logger::Log("SEND LowerNeckPos : %.2f %.2f %.2f\n", cp.lowerNeckPos.x,
              cp.lowerNeckPos.y, cp.lowerNeckPos.z);
  Logger::Log("SEND UpperNeckPos : %.2f %.2f %.2f\n", cp.upperNeckPos.x,
              cp.upperNeckPos.y, cp.upperNeckPos.z);
  Logger::Log("SEND HeadCenterPos : %.2f %.2f %.2f\n", cp.headCenterPos.x,
              cp.headCenterPos.y, cp.headCenterPos.z);
}
Vector3 ModScene::GetControlPointLocalPosition(ModControlPointRole role) const {
  if (role == ModControlPointRole::LeftShoulder) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        return assembly_.ComputeWorldPosition(id);
      }
    }
  }

  if (role == ModControlPointRole::LeftElbow) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftWrist) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightShoulder) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        return assembly_.ComputeWorldPosition(id);
      }
    }
  }

  if (role == ModControlPointRole::RightElbow) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightWrist) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightUpperArm) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftHip) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        return assembly_.ComputeWorldPosition(id);
      }
    }
  }

  if (role == ModControlPointRole::LeftKnee) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::LeftAnkle) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::LeftThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightHip) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        return assembly_.ComputeWorldPosition(id);
      }
    }
  }

  if (role == ModControlPointRole::RightKnee) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int bendIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::Bend);
        if (bendIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(bendIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::RightAnkle) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::RightThigh) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }
        const int endIndex =
            modBodies_.at(id).FindControlPointIndex(ModControlPointRole::End);
        if (endIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(endIndex));
        }
      }
    }
  }

  if (role == ModControlPointRole::Chest) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Chest);
  }

  if (role == ModControlPointRole::Belly) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Belly);
  }

  if (role == ModControlPointRole::Waist) {
    return GetTorsoControlPointWorldPosition(ModControlPointRole::Waist);
  }

  if (role == ModControlPointRole::LowerNeck ||
      role == ModControlPointRole::UpperNeck ||
      role == ModControlPointRole::HeadCenter) {
    for (const auto &[id, node] : assembly_.GetNodes()) {
      if (node.part == ModBodyPart::Head) {
        if (modBodies_.count(id) == 0 || modObjects_.count(id) == 0) {
          return {0.0f, 0.0f, 0.0f};
        }
        Object *object = modObjects_.at(id).get();
        if (object == nullptr) {
          return {0.0f, 0.0f, 0.0f};
        }

        const int pointIndex = modBodies_.at(id).FindControlPointIndex(role);
        if (pointIndex >= 0) {
          return modBodies_.at(id).GetControlPointWorldPosition(
              object, static_cast<size_t>(pointIndex));
        }
      }
    }
  }

  return {0.0f, 0.0f, 0.0f};
}