#include "ModScene.h"

namespace {
/*   GUI表示用の部位名テーブル   */
const char *kModPartNames[] = {
    "Body", "Head", "LeftArm", "RightArm", "LeftLeg", "RightLeg",
};

/*    enum class を配列添字に変換するための補助関数   */
size_t ToIndex(ModBodyPart part) { return static_cast<size_t>(part); }
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

  // 各部位のハンドルとオブジェクト配列を初期化
  modModelHandles_.fill(0);
  modObjects_.fill(nullptr);

  // 改造用の各部位オブジェクトをセットアップ
  SetupModObjects();

  // フェード
  fade_.Initialize(system_);
  fade_.StartFadeIn();
}

/*   デストラクタ   */
ModScene::~ModScene() {
  system_->DestroyCamera(camera_);
  system_->DestroyCamera(debugCamera_);

  // 各部位オブジェクトを解放
  for (auto &object : modObjects_) {
    delete object;
    object = nullptr;
  }

  // delete modObject_;
  delete light1_;
}

/*   毎フレーム更新   */
void ModScene::Update() {
  // 使用するカメラを更新
  CameraPart();

  // 各部位オブジェクトを更新
  UpdateModObjects();

  /*if (modObject_) {
    modBody_.Apply(modObject_);
    modObject_->Update(usingCamera_);
  }*/

  // 0キーでデバッグカメラと通常カメラを切り替え
  if (system_->GetTriggerOn(DIK_0)) {
    useDebugCamera_ = !useDebugCamera_;
  }

  // スペースキーで移動シーンへ
  if (!fade_.IsBusy() && system_->GetTriggerOn(DIK_SPACE)) {
    fade_.StartFadeOut();
    isStartTransition_ = true;
  }

  // フェード更新
  fade_.Update(usingCamera_);

  // フェード終了後にシーン移行
  if (isStartTransition_ && fade_.IsFinished()) {
    outcome_ = SceneOutcome::NEXT;
  }
}

/*   描画処理   */
void ModScene::Draw() {
  // 各部位を描画
  DrawModObjects();

  /*if (modObject_) {
    modObject_->Draw();
  }*/

#ifdef USE_IMGUI
  // 現在シーン表示
  ImGui::Begin("Scene");
  ImGui::Text("ModScene");
  ImGui::End();

  // 部位ごとのTransform調整GUI
  DrawModGui();
#endif

  // フェード描画
  fade_.Draw();
}

/*   使用カメラの切り替えと更新   */
void ModScene::CameraPart() {
  if (useDebugCamera_) {
    usingCamera_ = debugCamera_;
#ifdef USE_IMGUI
    // ImGui操作中はカメラのマウス操作を無効化
    if (!ImGui::GetIO().WantCaptureMouse) {
      debugCamera_->MouseControlUpdate();
    }
#else
    debugCamera_->MouseControlUpdate();
#endif
  } else {
    usingCamera_ = camera_;
  }

  // エンジンへ現在使用中のカメラを設定
  system_->SetCamera(usingCamera_);
}

// void ModScene::SetupModObject() {
//   modModelHandle_ =
//       system_->SetModelObj("GAME/resources/modBody/body/body.obj");
//
//   modObject_ = new Object;
//   modObject_->IntObject(system_);
//   modObject_->CreateModelData(modModelHandle_);
//   modObject_->mainPosition.transform.scale = {1.0f, 1.0f, 1.0f};
//   modObject_->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};
//
//   if (!modObject_->objectParts_.empty()) {
//     ObjectPart *body = &modObject_->objectParts_[0];
//     for (size_t i = 1; i < modObject_->objectParts_.size(); ++i) {
//       modObject_->objectParts_[i].parentPart = body;
//     }
//   }
//
//   modBody_.Initialize(modObject_);
// }

/*   改造用の各部位オブジェクトを生成する   */
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
  // 親子関係設定
  SetupHierarchy();

  // 初期配置設定
  SetupInitialLayout();
}

/*   指定した部位のObjectを1つ生成する   */
void ModScene::SetupPartObject(ModBodyPart part, const std::string &path) {
  size_t index = ToIndex(part);

  // モデル読み込み
  modModelHandles_[index] = system_->SetModelObj(path);

  // Object生成
  modObjects_[index] = new Object;
  modObjects_[index]->IntObject(system_);
  modObjects_[index]->CreateModelData(modModelHandles_[index]);

  // 初期Transform設定
  modObjects_[index]->mainPosition.transform = CreateDefaultTransform();
}

/*   部位同士の親子関係を設定する   */
void ModScene::SetupHierarchy() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  if (body == nullptr) {
    return;
  }

  // BodyのmainPositionを親として使う
  ObjectPart *bodyRoot = &body->mainPosition;

  modObjects_[ToIndex(ModBodyPart::Head)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightArm)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::LeftLeg)]->followObject_ = bodyRoot;
  modObjects_[ToIndex(ModBodyPart::RightLeg)]->followObject_ = bodyRoot;
}

/*   各部位の初期配置を設定する   */
void ModScene::SetupInitialLayout() {
  Object *body = modObjects_[ToIndex(ModBodyPart::Body)];
  Object *head = modObjects_[ToIndex(ModBodyPart::Head)];
  Object *leftArm = modObjects_[ToIndex(ModBodyPart::LeftArm)];
  Object *rightArm = modObjects_[ToIndex(ModBodyPart::RightArm)];
  Object *leftLeg = modObjects_[ToIndex(ModBodyPart::LeftLeg)];
  Object *rightLeg = modObjects_[ToIndex(ModBodyPart::RightLeg)];

  // 胴体を基準位置へ配置
  body->mainPosition.transform.translate = {0.0f, 0.0f, 0.0f};

  // 頭を胴体の上に配置
  head->mainPosition.transform.translate = {0.0f, 1.5f, 0.0f};

  // 腕を左右に配置
  leftArm->mainPosition.transform.translate = {-1.25f, 0.5f, 0.0f};
  rightArm->mainPosition.transform.translate = {1.25f, 0.5f, 0.0f};

  // 脚を下に配置
  leftLeg->mainPosition.transform.translate = {-0.5f, -1.75f, 0.0f};
  rightLeg->mainPosition.transform.translate = {0.5f, -1.75f, 0.0f};
}

/*   各部位Objectの更新をまとめて行う   */
void ModScene::UpdateModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Update(usingCamera_);
    }
  }
}

/*   各部位Objectの描画をまとめて行う   */
void ModScene::DrawModObjects() {
  for (auto &object : modObjects_) {
    if (object != nullptr) {
      object->Draw();
    }
  }
}

/*   デバッグ用GUI   */
#ifdef USE_IMGUI
void ModScene::DrawModGui() {
  ImGui::Begin("ModScene");

  ImGui::Text("MouseMiddleDrag : Move Camera");
  ImGui::Text("MouseRightDrag  : Rotate Camera");
  ImGui::Text("DIK_0           : Toggle DebugCamera");
  ImGui::Separator();

  // 各部位ごとにGUIを表示
  for (int i = 0; i < static_cast<int>(ModBodyPart::Count); ++i) {
    Object *object = modObjects_[static_cast<size_t>(i)];
    if (object == nullptr) {
      continue;
    }

    if (ImGui::TreeNode(kModPartNames[i])) {
      // 位置調整
      ImGui::SliderFloat3("Translate",
                          &object->mainPosition.transform.translate.x, -5.0f,
                          5.0f);
      // 回転調整
      ImGui::SliderFloat3("Rotate", &object->mainPosition.transform.rotate.x,
                          -3.14f, 3.14f);
      // スケール調整
      ImGui::SliderFloat3("Scale", &object->mainPosition.transform.scale.x,
                          0.1f, 3.0f);
      ImGui::TreePop();
    }
  }

  // 初期配置へ戻す
  if (ImGui::Button("Reset Layout")) {
    SetupInitialLayout();
  }

  ImGui::End();
}
#endif