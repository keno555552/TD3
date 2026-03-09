#include "ModScene.h"

namespace {

/*   GUI表示用の部位名テーブル   */
const char *kModPartNames[] = {
    "Body", "Head", "LeftArm", "RightArm", "LeftLeg", "RightLeg",
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
  modObjects_.fill(nullptr);

  // 部位生成
  SetupModObjects();

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

/*   デストラクタ   */
ModScene::~ModScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  for (auto &object : modObjects_) {
    delete object;
    object = nullptr;
  }

  system_->RemoveLight(light1_);

  delete light1_;
}

/*   毎フレーム更新   */
void ModScene::Update() {
  CameraPart();
  UpdateModObjects();

  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  fade_.Update(usingCamera_);

  if (isStartTransition_ && fade_.IsFinished()) {
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
  SetupPartObject(ModBodyPart::Head, "GAME/resources/modBody/head/head.obj");
  SetupPartObject(ModBodyPart::LeftArm,
                  "GAME/resources/modBody/leftArm/leftArm.obj");
  SetupPartObject(ModBodyPart::RightArm,
                  "GAME/resources/modBody/rightArm/rightArm.obj");
  SetupPartObject(ModBodyPart::LeftLeg,
                  "GAME/resources/modBody/leftLeg/leftLeg.obj");
  SetupPartObject(ModBodyPart::RightLeg,
                  "GAME/resources/modBody/rightLeg/rightLeg.obj");

  SetupHierarchy();
  SetupInitialLayout();
  SetupBodyJointOffsets();
  ResetModBodies();
}

/*   指定部位の Object を1つ生成する   */
void ModScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  const size_t index = ToIndex(part);

  modModelHandles_[index] = system_->SetModelObj(path);

  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);

  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();

  modBodies_[index].Initialize(modObjects_[index], part);
}

/*   部位同士の親子関係設定   */
void ModScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftLeg)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightLeg)]->followObject_ = bodyRoot;
}

/*   各部位の root 初期配置設定   */
void ModScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *leftArm = modObjects_[ToIndex(ModBodyPart::LeftArm)];
  Object *rightArm = modObjects_[ToIndex(ModBodyPart::RightArm)];
  Object *leftLeg = modObjects_[ToIndex(ModBodyPart::LeftLeg)];
  Object *rightLeg = modObjects_[ToIndex(ModBodyPart::RightLeg)];

  if (body == nullptr || head == nullptr || leftArm == nullptr ||
      rightArm == nullptr || leftLeg == nullptr || rightLeg == nullptr) {
    return;
  }

  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  body->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  head->mainPosition.transform.translate = {0.0f, 1.0f, 0.0f};
  head->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  head->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  leftArm->mainPosition.transform.translate = {-1.25f, 1.0f, 0.0f};
  leftArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftArm->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  rightArm->mainPosition.transform.translate = {1.25f, 1.0f, 0.0f};
  rightArm->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightArm->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  leftLeg->mainPosition.transform.translate = {-0.5f, -1.25f, 0.0f};
  leftLeg->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  leftLeg->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};

  rightLeg->mainPosition.transform.translate = {0.5f, -1.25f, 0.0f};
  rightLeg->mainPosition.transform.rotate = {0.0f, 0.0f, 0.0f};
  rightLeg->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
}

/*   Body root 基準の各 joint 初期位置を設定する   */
void ModScene::SetupBodyJointOffsets() {
  bodyJointOffsets_[ToIndex(ModBodyPart::Body)] = {0.0f, 0.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::Head)] = {0.0f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::LeftArm)] = {-1.25f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightArm)] = {1.25f, 1.0f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::LeftLeg)] = {-0.5f, -1.25f, 0.0f};
  bodyJointOffsets_[ToIndex(ModBodyPart::RightLeg)] = {0.5f, -1.25f, 0.0f};
}

/*   Body の現在サイズから joint を再計算して子 root を置く   */
void ModScene::UpdateChildRootsFromBody() {
  const Vector3 bodyScale =
      modBodies_[ToIndex(ModBodyPart::Body)].GetVisualScaleRatio();

  auto updateChildRoot = [&](ModBodyPart part) {
    Object *child = modObjects_[ToIndex(part)];
    if (child == nullptr) {
      return;
    }

    const Vector3 &joint = bodyJointOffsets_[ToIndex(part)];
    child->mainPosition.transform.translate = ScaleByRatio(joint, bodyScale);
  };

  updateChildRoot(ModBodyPart::Head);
  updateChildRoot(ModBodyPart::LeftArm);
  updateChildRoot(ModBodyPart::RightArm);
  updateChildRoot(ModBodyPart::LeftLeg);
  updateChildRoot(ModBodyPart::RightLeg);
}

/*   全部位へ ModBody を適用   */
void ModScene::ApplyModBodies() {
  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (modObjects_[i] != nullptr) {
      modBodies_[i].Apply(modObjects_[i]);
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
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body != nullptr) {
    modBodies_[ToIndex(ModBodyPart::Body)].Apply(body);
  }

  UpdateChildRootsFromBody();

  for (size_t i = 0; i < modObjects_.size(); ++i) {
    if (i == ToIndex(ModBodyPart::Body)) {
      continue;
    }

    if (modObjects_[i] != nullptr) {
      modBodies_[i].Apply(modObjects_[i]);
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
    ImGui::SliderFloat3("Head Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::Head)].x, -5.0f,
                        5.0f);
    ImGui::SliderFloat3("LeftArm Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::LeftArm)].x,
                        -5.0f, 5.0f);
    ImGui::SliderFloat3("RightArm Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::RightArm)].x,
                        -5.0f, 5.0f);
    ImGui::SliderFloat3("LeftLeg Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::LeftLeg)].x,
                        -5.0f, 5.0f);
    ImGui::SliderFloat3("RightLeg Joint",
                        &bodyJointOffsets_[ToIndex(ModBodyPart::RightLeg)].x,
                        -5.0f, 5.0f);
    ImGui::TreePop();
  }

  for (int i = 0; i < static_cast<int>(ModBodyPart::Count); ++i) {
    Object *object = modObjects_[static_cast<size_t>(i)];
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