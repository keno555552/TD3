#include "ModScene.h"

namespace {

/*   GUI表示用の部位名テーブル   */
const char *kModPartNames[] = {
    "Body",
    "Neck",
    "Head",
    "LeftUpperArm",
    "LeftForeArm",
    "RightUpperArm",
    "RightForeArm",
    "LeftThigh",
    "LeftShin",
    "RightThigh",
    "RightShin",
};

/*   enum class を配列添字へ変換   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }

/*   成分ごとのスケール適用   */
Vector3 ScaleByRatio(const Vector3 &base, const Vector3 &ratio) {
  return {base.x * ratio.x, base.y * ratio.y, base.z * ratio.z};
}

} // namespace

/*   コンストラクタ   */
ModScene::ModScene(kEngine *system) {
  system_ = system;

  // ライト
  light1_ = new Light;
  light1_->direction = {-0.5f, -1.0f, -0.3f};
  light1_->color = {1.0f, 1.0f, 1.0f};
  light1_->intensity = 1.0f;
  system_->AddLight(light1_);

  // カメラ
  debugCamera_ = system_->CreateDebugCamera();
  camera_ = system_->CreateCamera();

  // デバッグカメラ初期位置
  debugCamera_->SetTranslate({0.0f, 0.0f, -8.0f});
  debugCamera_->SetDefaultTransform(debugCamera_->GetTransform());

  // 通常カメラ初期位置
  camera_->SetTranslate({0.0f, 0.0f, -8.0f});
  camera_->SetDefaultTransform(camera_->GetTransform());

  // 初期状態ではデバッグカメラを使用
  usingCamera_ = debugCamera_;
  system_->SetCamera(usingCamera_);

  // 各部位のハンドルと配列初期化
  modModelHandles_.fill(0);

  customizeData_ = ModBody::CopySharedCustomizeData();
  if (customizeData_ == nullptr) {
    customizeData_ = ModBody::CreateDefaultCustomizeData();
  }

  // 部位生成
  SetupModObjects();

  // 共有データから部位へ初期値を反映
  LoadCustomizeData();

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

/*   デストラクタ   */
ModScene::~ModScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  system_->RemoveLight(light1_);
  delete light1_;
}

/*   毎フレーム更新   */
void ModScene::Update() {
  CameraPart();
  UpdateModObjects();
  SyncCustomizeDataFromScene();

  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  fade_.Update(usingCamera_);

  if (isStartTransition_ && fade_.IsFinished()) {
    if (customizeData_ != nullptr) {
      ModBody::SetSharedCustomizeData(*customizeData_);
    }
    outcome_ = SceneOutcome::NEXT;
  }
}

/*   描画処理   */
void ModScene::Draw() {
  DrawModObjects();

#ifdef USE_IMGUI
  ImGui::Begin("Scene");
  ImGui::Text("ModScene");
  ImGui::End();

  DrawModGui();
#endif

  fade_.Draw();
}

/*   使用カメラの切り替えと更新   */
void ModScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
#ifdef USE_IMGUI
    if (!ImGui::GetIO().WantCaptureMouse) {
      debugCamera_->MouseControlUpdate();
    }
#else
    debugCamera_->MouseControlUpdate();
#endif
  } else {
    usingCamera_ = camera_;
  }

  system_->SetCamera(usingCamera_);
}

/*   改造用の各部位 Object を生成する   */
void ModScene::SetupModObjects() {
  SetupPartObject(ModBodyPart::Body, "GAME/resources/modBody/body/body.obj");
  SetupPartObject(ModBodyPart::Neck, "GAME/resources/modBody/neck/neck.obj");
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");

  SetupPartObject(ModBodyPart::LeftUpperArm,
                  "GAME/resources/modBody/leftUpperArm/leftUpperArm.obj");
  SetupPartObject(ModBodyPart::LeftForeArm,
                  "GAME/resources/modBody/leftForeArm/leftForeArm.obj");
  SetupPartObject(ModBodyPart::RightUpperArm,
                  "GAME/resources/modBody/rightUpperArm/rightUpperArm.obj");
  SetupPartObject(ModBodyPart::RightForeArm,
                  "GAME/resources/modBody/rightForeArm/rightForeArm.obj");

  SetupPartObject(ModBodyPart::LeftThigh,
                  "GAME/resources/modBody/leftThighs/leftThighs.obj");
  SetupPartObject(ModBodyPart::LeftShin,
                  "GAME/resources/modBody/leftShin/leftShin.obj");
  SetupPartObject(ModBodyPart::RightThigh,
                  "GAME/resources/modBody/rightThighs/rightThighs.obj");
  SetupPartObject(ModBodyPart::RightShin,
                  "GAME/resources/modBody/rightShin/rightShin.obj");

  SetupHierarchy();
  SetupInitialLayout();
  SetupBodyJointOffsets();
  ResetModBodies();
}

/*   指定部位の Object を1つ生成する   */
void ModScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  const size_t index = ToIndex(part);

  modModelHandles_[index] = system_->SetModelObj(path);

  modObjects_[index] = std::make_unique<Object>();
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();

  modBodies_[index].Initialize(modObjects_[index].get(), part);
}

/*   部位同士の親子関係設定   */
void ModScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)].get();
  Object *neck = modObjects_[ToIndex(ModBodyPart::Neck)].get();
  Object *leftUpperArm = modObjects_[ToIndex(ModBodyPart::LeftUpperArm)].get();
  Object *rightUpperArm =
      modObjects_[ToIndex(ModBodyPart::RightUpperArm)].get();
  Object *leftThigh = modObjects_[ToIndex(ModBodyPart::LeftThigh)].get();
  Object *rightThigh = modObjects_[ToIndex(ModBodyPart::RightThigh)].get();

  if (body == nullptr || neck == nullptr || leftUpperArm == nullptr ||
      rightUpperArm == nullptr || leftThigh == nullptr ||
      rightThigh == nullptr) {
    return;
  }

  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Neck)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightUpperArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftThigh)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightThigh)]->followObject_ = bodyRoot;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = &neck->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftForeArm)]->followObject_ =
      &leftUpperArm->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightForeArm)]->followObject_ =
      &rightUpperArm->mainPosition;
  modObjects_[ToIndex(ModBodyPart::LeftShin)]->followObject_ =
      &leftThigh->mainPosition;
  modObjects_[ToIndex(ModBodyPart::RightShin)]->followObject_ =
      &rightThigh->mainPosition;
}

/*   各部位の root 初期配置設定   */
void ModScene::SetupInitialLayout() {
  auto setRoot = [](Object *object, const Vector3 &translate) {
    if (object == nullptr) {
      return;
    }

    object->mainPosition.transform.translate = translate;
    object->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
    object->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
  };

  setRoot(modObjects_[ToIndex(ModBodyPart::Body)].get(), {0.0f, 0.0f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::Neck)].get(), {0.0f, 1.0f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::Head)].get(), {0.0f, 1.0f, 0.0f});

  setRoot(modObjects_[ToIndex(ModBodyPart::LeftUpperArm)].get(),
          {-1.25f, 1.0f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::LeftForeArm)].get(),
          {0.0f, -1.0f, 0.0f});

  setRoot(modObjects_[ToIndex(ModBodyPart::RightUpperArm)].get(),
          {1.25f, 1.0f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::RightForeArm)].get(),
          {0.0f, -1.0f, 0.0f});

  setRoot(modObjects_[ToIndex(ModBodyPart::LeftThigh)].get(),
          {-0.5f, -1.25f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::LeftShin)].get(),
          {0.0f, -1.0f, 0.0f});

  setRoot(modObjects_[ToIndex(ModBodyPart::RightThigh)].get(),
          {0.5f, -1.25f, 0.0f});
  setRoot(modObjects_[ToIndex(ModBodyPart::RightShin)].get(),
          {0.0f, -1.0f, 0.0f});
}

/*   Body root 基準の各 joint 初期位置を設定する   */
void ModScene::SetupBodyJointOffsets() {
  for (auto &offset : bodyJointOffsets_) {
    offset = {0.0f, 0.0f, 0.0f};
  }

  bodyJointOffsets_[ToIndex(ModBodyPart::Neck)] = {0.0f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)] = {-1.25f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)] = {1.25f, 1.0f, 0.0f};

  bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)] = {-0.5f, -1.25f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)] = {0.5f, -1.25f, 0.0f};
}

/*   共有改造データから部位へ値を反映する   */
void ModScene::LoadCustomizeData() {
  if (customizeData_ == nullptr) {
    return;
  }

  bodyJointOffsets_ = customizeData_->bodyJointOffsets;

  for (size_t i = 0; i < modBodies_.size(); ++i) {
    modBodies_[i].SetParam(customizeData_->partParams[i]);
  }
}

void ModScene::SyncCustomizeDataFromScene() {
  if (customizeData_ == nullptr) {
    return;
  }

  customizeData_->bodyJointOffsets = bodyJointOffsets_;

  for (size_t i = 0; i < modBodies_.size(); ++i) {
    customizeData_->partParams[i] = modBodies_[i].GetParam();
  }
}

/*   Body の現在サイズから joint を再計算して子 root を置く   */
void ModScene::UpdateChildRootsFromBody() {
  const Vector3 bodyScale =
      modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();

  auto setRootFromBody = [&](ModBodyPart childPart) {
    Object *child = modObjects_[ToIndex(childPart)].get();
    if (child == nullptr) {
      return;
    }

    const Vector3 &joint = bodyJointOffsets_[ToIndex(childPart)];
    child->mainPosition.transform.translate = ScaleByRatio(joint, bodyScale);
  };

  setRootFromBody(ModBodyPart::Neck);
  setRootFromBody(ModBodyPart::LeftUpperArm);
  setRootFromBody(ModBodyPart::RightUpperArm);
  setRootFromBody(ModBodyPart::LeftThigh);
  setRootFromBody(ModBodyPart::RightThigh);

  auto setRootFromParentUp = [&](ModBodyPart parentPart,
                                 ModBodyPart childPart) {
    Object *child = modObjects_[ToIndex(childPart)].get();
    if (child == nullptr) {
      return;
    }

    const Vector3 parentScale =
        modBodies_[ToIndex(parentPart)].GetVisualScaleRatio();

    child->mainPosition.transform.translate = {0.0f, parentScale.y, 0.0f};
  };

  auto setRootFromParentDown = [&](ModBodyPart parentPart,
                                   ModBodyPart childPart) {
    Object *child = modObjects_[ToIndex(childPart)].get();
    if (child == nullptr) {
      return;
    }

    const Vector3 parentScale =
        modBodies_[ToIndex(parentPart)].GetVisualScaleRatio();

    child->mainPosition.transform.translate = {0.0f, -parentScale.y, 0.0f};
  };

  setRootFromParentUp(ModBodyPart::Neck, ModBodyPart::Head);

  setRootFromParentDown(ModBodyPart::LeftUpperArm, ModBodyPart::LeftForeArm);
  setRootFromParentDown(ModBodyPart::RightUpperArm, ModBodyPart::RightForeArm);
  setRootFromParentDown(ModBodyPart::LeftThigh, ModBodyPart::LeftShin);
  setRootFromParentDown(ModBodyPart::RightThigh, ModBodyPart::RightShin);
}

/*   全部位へ ModBody を適用   */
void ModScene::ApplyModBodies() {
  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (modObjects_[i] != nullptr) {
      modBodies_[i].Apply(modObjects_[i].get());
    }
  }
}

/*   改造パラメータ初期化   */
void ModScene::ResetModBodies() {
  for (auto &modBody : modBodies_) {
    modBody.Reset();
  }
}

/*   各部位 Object 更新   */
void ModScene::UpdateModObjects() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)].get();
  if (body != nullptr) {
    modBodies_[ToIndex(ModBodyPart::Body)].Apply(body);
  }

  UpdateChildRootsFromBody();

  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (i == ToIndex(ModBodyPart::Body)) {
      continue;
    }

    if (modObjects_[i] != nullptr) {
      modBodies_[i].Apply(modObjects_[i].get());
    }
  }

  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }
}

/*   各部位 Object 描画   */
void ModScene::DrawModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Draw();
    }
  }
}

#ifdef USE_IMGUI
/*   デバッグ用 GUI   */
void ModScene::DrawModGui() {
  ImGui::Begin("ModScene");

  ImGui::Text("MouseMiddleDrag : Move Camera");
  ImGui::Text("MouseRightDrag  : Rotate Camera");
  ImGui::Text("DIK_0           : Toggle DebugCamera");
  ImGui::Separator();

  if (ImGui::TreeNode("Body Joints")) {
    ImGui::SliderFloat3("Neck Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::Neck)].x, -5.0f,
                        5.0f);
    ImGui::SliderFloat3(
        "LeftUpperArm Joint",
        &bodyJointOffsets_[ToIndex(ModBodyPart::LeftUpperArm)].x, -5.0f, 5.0f);
    ImGui::SliderFloat3(
        "RightUpperArm Joint",
        &bodyJointOffsets_[ToIndex(ModBodyPart::RightUpperArm)].x, -5.0f, 5.0f);
    ImGui::SliderFloat3("LeftThigh Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::LeftThigh)].x,
                        -5.0f, 5.0f);
    ImGui::SliderFloat3("RightThigh Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::RightThigh)].x,
                        -5.0f, 5.0f);
    ImGui::TreePop();
  }

  for (int i = 0; i < static_cast<int>(ModBodyPart::Count); ++i) {
    Object *object = modObjects_[static_cast<size_t>(i)].get();
    if (object == nullptr) {
      continue;
    }

    ModBody &modBody = modBodies_[static_cast<size_t>(i)];
    ModBodyPartParam &param = modBody.GetParam();

    if (ImGui::TreeNode(kModPartNames[i])) {
      ImGui::Text("Root Transform");
      ImGui::SliderFloat3("Root Translate",
                          &object->mainPosition.transform.translate.x, -5.0f,
                          5.0f);
      ImGui::SliderFloat3("Root Rotate",
                          &object->mainPosition.transform.rotate.x, -3.14f,
                          3.14f);

      ImGui::Separator();
      ImGui::Text("Mod Params");
      ImGui::Checkbox("Enabled", &param.enabled);
      ImGui::SliderFloat3("Mesh Scale", &param.scale.x, 0.1f, 3.0f);
      ImGui::SliderFloat("Length", &param.length, 0.1f, 3.0f, "%.2f");

      ImGui::Separator();
      ImGui::Text("Current Mesh Transform");

      if (!object->objectParts_.empty()) {
        const Transform &mesh = object->objectParts_[0].transform;
        ImGui::Text("Mesh Translate : %.2f %.2f %.2f", mesh.translate.x,
                    mesh.translate.y, mesh.translate.z);
        ImGui::Text("Mesh Rotate    : %.2f %.2f %.2f", mesh.rotate.x,
                    mesh.rotate.y, mesh.rotate.z);
        ImGui::Text("Mesh Scale     : %.2f %.2f %.2f", mesh.scale.x,
                    mesh.scale.y, mesh.scale.z);
      }

      ImGui::TreePop();
    }
  }

  if (ImGui::Button("Reset Layout")) {
    SetupInitialLayout();
    SetupBodyJointOffsets();
  }

  if (ImGui::Button("Reset Mod Params")) {
    ResetModBodies();
  }

  ImGui::End();
}
#endif