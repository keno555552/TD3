#include "ModScene.h"
#include "GAME/actor/prompt/PromptData.h"
#include <unordered_set>

namespace {

size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

Vector3 Mul(const Vector3 &a, const Vector3 &b) {
  return {a.x * b.x, a.y * b.y, a.z * b.z};
}

Vector3 AddV(const Vector3 &a, const Vector3 &b) {
  return {a.x + b.x, a.y + b.y, a.z + b.z};
}

Vector3 SubV(const Vector3 &a, const Vector3 &b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

const char *PartName(ModBodyPart part) {
  switch (part) {
  case ModBodyPart::Body:
    return "Body";
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
  case ModBodyPart::Body:
    return "GAME/resources/modBody/body/body.obj";
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
    return "GAME/resources/modBody/body/body.obj";
  }
}

bool IsSelectablePart(ModBodyPart part) { return part != ModBodyPart::Count; }

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
  }

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
  // まず全 Object の親参照をいったん外す
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

  // 現在の AssemblyGraph に従って親子関係と root 位置を再設定する
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

    // 基本は Graph が持っているローカル位置を使う
    Vector3 localTranslate = node->localTransform.translate;
    const Vector3 localRotate = {0.0f, 0.0f, 0.0f};

    // 親がいる場合は、親コネクタと自分のコネクタを合わせるように位置を補正する
    if (node->parentId >= 0 && modObjects_.count(node->parentId) > 0) {
      Object *parentObject = modObjects_[node->parentId].get();
      if (parentObject != nullptr) {
        object->followObject_ = &parentObject->mainPosition;
        object->mainPosition.parentPart = &parentObject->mainPosition;
      }

      // 親部位と自分の見た目スケールを取得する
      Vector3 parentScale = {1.0f, 1.0f, 1.0f};
      if (modBodies_.count(node->parentId) > 0) {
        parentScale = modBodies_[node->parentId].GetVisualScaleRatio();
      }

      Vector3 selfScale = {1.0f, 1.0f, 1.0f};
      if (modBodies_.count(id) > 0) {
        selfScale = modBodies_[id].GetVisualScaleRatio();
      }

      // 親側と自分側の接続点を取得する
      const ConnectorNode *parentConn =
          assembly_.FindConnector(node->parentId, node->parentConnectorId);
      const ConnectorNode *selfConn =
          assembly_.FindConnector(id, node->selfConnectorId);

      // 接続点のローカル位置をスケール込みで補正する
      Vector3 parentAnchor = {0.0f, 0.0f, 0.0f};
      Vector3 selfAnchor = {0.0f, 0.0f, 0.0f};

      if (parentConn != nullptr) {
        parentAnchor = Mul(parentConn->localPosition, parentScale);
      }
      if (selfConn != nullptr) {
        selfAnchor = Mul(selfConn->localPosition, selfScale);
      }

      // 親アンカーと自分アンカーが一致するよう root 位置を求める
      localTranslate =
          AddV(node->localTransform.translate, SubV(parentAnchor, selfAnchor));
    }

    // root transform へ反映する
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
                                  AddV(node->localTransform.translate, delta));
}

int ModScene::ResolveAssemblyOperationPartId(int partId) const {
  // 対象ノードを取得する
  const PartNode *node = assembly_.FindNode(partId);
  if (node == nullptr) {
    return -1;
  }

  // 前腕が選ばれている場合は上腕を操作対象にする
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

  // 脛が選ばれている場合は腿を操作対象にする
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

void ModScene::UpdateModObjects() {
  // 現在の AssemblyGraph を Object 階層へ反映する
  ApplyAssemblyToSceneHierarchy();

  // 各部位に対して見た目パラメータを反映する
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

  // 全 Object をカメラ付きで更新する
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
}

void ModScene::DrawModGui() {
  ImGui::Begin("ModScene");

  ImGui::Text("MouseMiddleDrag : Move Camera");
  ImGui::Text("MouseRightDrag  : Rotate Camera");
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