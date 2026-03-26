#include "ModScene.h"
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

    // torso は Graph 親子より solver を優先
    // NOTE: torso control points は body root ローカルとして扱うため
    // mainPosition を操作点に追従させない。
    // （追従させるとセグメントの座標系が崩れて腹→腰がズレる）
    /*
    if (node->part == ModBodyPart::ChestBody) {
      const int chestIndex =
          FindTorsoControlPointIndex(ModControlPointRole::Chest);
      if (chestIndex >= 0) {
        object->followObject_ = nullptr;
        object->mainPosition.parentPart = nullptr;
        object->mainPosition.transform.translate =
            torsoControlPoints_[static_cast<size_t>(chestIndex)].localPosition;
        object->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
        object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
        continue;
      }
    }

    if (node->part == ModBodyPart::StomachBody) {
      const int bellyIndex =
          FindTorsoControlPointIndex(ModControlPointRole::Belly);
      if (bellyIndex >= 0) {
        object->followObject_ = nullptr;
        object->mainPosition.parentPart = nullptr;
        object->mainPosition.transform.translate =
            torsoControlPoints_[static_cast<size_t>(bellyIndex)].localPosition;
        object->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
        object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
        continue;
      }
    }
    */

    if (node->parentId >= 0 && modObjects_.count(node->parentId) > 0) {
      Object *parentObject = modObjects_[node->parentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      bool handledByOwnerPoint = false;

      // 共有チェーンから位置をもらう部位は connector 基準ではなく
      // owner の制御点基準で root を置く
      switch (node->part) {
      case ModBodyPart::ChestBody: {
        const int chestIndex =
            FindTorsoControlPointIndex(ModControlPointRole::Chest);
        if (chestIndex >= 0) {
          localTranslate = torsoControlPoints_[static_cast<size_t>(chestIndex)]
                               .localPosition;
          handledByOwnerPoint = true;
        }
        break;
      }

      case ModBodyPart::StomachBody: {
        const int bellyIndex =
            FindTorsoControlPointIndex(ModControlPointRole::Belly);
        if (bellyIndex >= 0) {
          localTranslate = torsoControlPoints_[static_cast<size_t>(bellyIndex)]
                               .localPosition;
          handledByOwnerPoint = true;
        }
        break;
      }

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

      case ModBodyPart::Neck: {
        // Neck は Head owner の LowerNeck を root に合わせる
        const int ownerId = ResolveControlOwnerPartId(assembly_, id);
        if (ownerId >= 0 && ownerId != id && modBodies_.count(ownerId) > 0) {
          const int lowerIndex = modBodies_[ownerId].FindControlPointIndex(
              ModControlPointRole::LowerNeck);

          if (lowerIndex >= 0) {
            const std::vector<ModControlPoint> &ownerPoints =
                modBodies_[ownerId].GetControlPoints();

            localTranslate =
                ownerPoints[static_cast<size_t>(lowerIndex)].localPosition;
            handledByOwnerPoint = true;
          }
        }
        break;
      }

      case ModBodyPart::Head: {
        // Head owner の controlPoints は LowerNeck 基準で持つ
        // Scene 上の Head Object root も LowerNeck に合わせる
        if (modBodies_.count(id) > 0) {
          const ModBody &headBody = modBodies_[id];
          const int lowerIndex =
              headBody.FindControlPointIndex(ModControlPointRole::LowerNeck);

          if (lowerIndex >= 0) {
            const std::vector<ModControlPoint> &headPoints =
                headBody.GetControlPoints();

            localTranslate =
                headPoints[static_cast<size_t>(lowerIndex)].localPosition;
            handledByOwnerPoint = true;
          }
        }
        break;
      }

      default:
        break;
      }

      // 通常部位は connector 同士の差分で root を合わせる
      if (!handledByOwnerPoint) {
        Vector3 parentScale = {1.0f, 1.0f, 1.0f};
        if (modBodies_.count(node->parentId) > 0) {
          parentScale = modBodies_[node->parentId].GetVisualScaleRatio();
        }

        Vector3 selfScale = {1.0f, 1.0f, 1.0f};
        if (modBodies_.count(id) > 0) {
          selfScale = modBodies_[id].GetVisualScaleRatio();
        }

        const ConnectorNode *parentConn =
            assembly_.FindConnector(node->parentId, node->parentConnectorId);
        const ConnectorNode *selfConn =
            assembly_.FindConnector(id, node->selfConnectorId);

        Vector3 parentAnchor = {0.0f, 0.0f, 0.0f};
        Vector3 selfAnchor = {0.0f, 0.0f, 0.0f};

        if (parentConn != nullptr) {
          parentAnchor = Mul(parentConn->localPosition, parentScale);

          const PartNode *parentNode = assembly_.FindNode(node->parentId);
          if (parentNode != nullptr && IsTorsoPart(parentNode->part)) {
            const int chestIndex =
                FindTorsoControlPointIndex(ModControlPointRole::Chest);
            const int waistIndex =
                FindTorsoControlPointIndex(ModControlPointRole::Waist);

            const Vector3 torsoChest =
                (chestIndex >= 0)
                    ? torsoControlPoints_[static_cast<size_t>(chestIndex)]
                          .localPosition
                    : Vector3{0.0f, 0.45f, 0.0f};

            const Vector3 torsoWaist =
                (waistIndex >= 0)
                    ? torsoControlPoints_[static_cast<size_t>(waistIndex)]
                          .localPosition
                    : Vector3{0.0f, -0.45f, 0.0f};

            switch (parentConn->role) {
            case ConnectorRole::Neck:
              parentAnchor = torsoChest;
              break;

            case ConnectorRole::Shoulder:
              parentAnchor = {parentAnchor.x, torsoChest.y, parentAnchor.z};
              break;

            case ConnectorRole::Hip:
              parentAnchor = torsoWaist;
              break;

            case ConnectorRole::Generic:
            default:
              // 胸Bodyの下側汎用=腰として扱う（胸下の接続を追従させる）
              parentAnchor = torsoWaist;
              break;
            }
          }
        }

        if (selfConn != nullptr) {
          selfAnchor = Mul(selfConn->localPosition, selfScale);
        }

        localTranslate = Add(node->localTransform.translate,
                             Subtract(parentAnchor, selfAnchor));
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
    instance.param.count = 1;

    customizeData_->partInstances.push_back(instance);
  }

  // 旧方式配列も互換用に再構築する
  RebuildLegacyCustomizeDataFromInstances();
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
  // 付け替え元と付け替え先がそろっていなければ何もしない
  if (selectedPartId_ < 0 || reattachParentId_ < 0) {
    return;
  }

  // セット単位移動が必要な場合を考慮して対象部位IDを補正する
  const int targetId = ResolveAssemblyOperationPartId(selectedPartId_);
  if (targetId < 0) {
    return;
  }

  // 親コネクタ指定付きで Graph の親子関係を変更する
  int connectorId = reattachConnectorId_;
  if (!assembly_.MovePart(targetId, reattachParentId_, connectorId)) {
    return;
  }

  // 構造変更後の Object 一覧を同期する
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

    const Vector3 rootWorld = bodyObject->mainPosition.transform.translate;
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

  const Vector3 rootWorld = object->mainPosition.transform.translate;
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

void ModScene::UpdateHoveredPartFromMouseRay(const Ray &mouseRay) {
  // ドラッグ中は選択部位を優先して表示する
  if (isDraggingControlPoint_ && selectedControlPartId_ >= 0) {
    hoveredPartId_ = selectedControlPartId_;
    return;
  }

  float nearestT = FLT_MAX;
  int nearestPartId = -1;

  for (size_t i = 0; i < orderedPartIds_.size(); ++i) {
    const int partId = orderedPartIds_[i];

    if (modObjects_.count(partId) == 0 || modBodies_.count(partId) == 0) {
      continue;
    }

    Object *object = modObjects_[partId].get();
    if (object == nullptr || object->objectParts_.empty()) {
      continue;
    }

    // mesh の見た目中心とスケールから簡易球を作る
    const Transform &mesh = object->objectParts_[0].transform;

    Sphere pickSphere{};
    pickSphere.center =
        Add(object->mainPosition.transform.translate, mesh.translate);
    pickSphere.radius = Max3(mesh.scale.x, mesh.scale.y, mesh.scale.z) * 0.75f;

    float t = 0.0f;
    if (!crashDecision(pickSphere, mouseRay, &t)) {
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
      const float radius = torsoControlPoints_[i].radius;

      const Vector3 toCamera =
          NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
      const Vector3 drawPos = Add(worldPos, Multiply(radius * 1.5f, toCamera));

      gizmo->mainPosition.transform.translate = drawPos;
      gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
      gizmo->mainPosition.transform.scale = {radius * 2.0f, radius * 2.0f,
                                             radius * 2.0f};

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
    const float radius = points[i].radius;

    const Vector3 toCamera =
        NormalizeSafeV(Subtract(cameraPos, worldPos), {0.0f, 0.0f, -1.0f});
    const Vector3 drawPos = Add(worldPos, Multiply(radius * 1.5f, toCamera));

    gizmo->mainPosition.transform.translate = drawPos;
    gizmo->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
    gizmo->mainPosition.transform.scale = {radius * 2.0f, radius * 2.0f,
                                           radius * 2.0f};

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

  int fadedOwnerId = -1;
  if (hoveredPartId_ >= 0) {
    fadedOwnerId = ResolveControlOwnerPartId(assembly_, hoveredPartId_);
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

    if (fadedOwnerId >= 0) {
      const int ownerId = ResolveControlOwnerPartId(assembly_, id);
      if (ownerId == fadedOwnerId) {
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

      ImGui::Text("Radius : %.2f", torsoControlPoints_[i].radius);
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

        ImGui::Text("Radius : %.2f", points[i].radius);
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