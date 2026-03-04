#include "DefaultMenu.h"

DefaultMenu::DefaultMenu(kEngine* system) {
	system_ = system;

	/// ============ タイマー初期化 ===========///
	animationTimer_ = new Timer;
	buttonTimer_ = new Timer;
	animationTimer_->Init0(animationDefaultTime_, system_->GetTimeManager());
	buttonTimer_->Init0(buttonAnimationDefaultTime_, system_->GetTimeManager());

	/// =========== リソース読み込み ==========///

	std::string menuPath = "resources/texture/menu/";
	TH_menuBG_ = system_->LoadTexture(menuPath + "menuBG.png");

	TH_menuPause = system_->LoadTexture(menuPath + "menuPause.png");
	TH_menuBGM = system_->LoadTexture(menuPath + "menuBGM.png");
	TH_menuBGM_NL = system_->LoadTexture(menuPath + "menuBGM_NL.png");
	TH_menuMASTER = system_->LoadTexture(menuPath + "menuMASTER.png");
	TH_menuMASTER_NL = system_->LoadTexture(menuPath + "menuMASTER_NL.png");
	TH_menuSE = system_->LoadTexture(menuPath + "menuSE.png");
	TH_menuSE_NL = system_->LoadTexture(menuPath + "menuSE_NL.png");

	TH_menuClose = system_->LoadTexture(menuPath + "menuClose.png");
	TH_menuClose_NL = system_->LoadTexture(menuPath + "menuClose_NL.png");
	TH_menuBack = system_->LoadTexture(menuPath + "menuBack.png");
	TH_menuBack_NL = system_->LoadTexture(menuPath + "menuBack_NL.png");
	TH_menuRetry = system_->LoadTexture(menuPath + "menuRetry.png");
	TH_menuRetry_NL = system_->LoadTexture(menuPath + "menuRetry_NL.png");

	TH_menuButton = system_->LoadTexture(menuPath + "menuButton.png");
	TH_menuButton_NL = system_->LoadTexture(menuPath + "menuButton_NL.png");
	TH_menuVolumeBar = system_->LoadTexture(menuPath + "menuVolumeBar.png");

	float middlePointX = (float)config::GetClientWidth() / 2;
	float middlePointY = (float)config::GetClientHeight() / 2;


	/// =========== 背景スプライト ==========///
	sMenuBG_->IntObject(system_);
	sMenuBG_->CreateDefaultData();
	sMenuBG_->objectParts_[0].materialConfig->textureHandle = TH_menuBG_;
	sMenuBG_->mainPosition.transform.translate = { menuStartPos_.x, menuStartPos_.y,99.0f };
	sMenuBG_->Update(nullptr);


	/// =========== 字スプライト ==========///
	sMenuPause->IntObject(system_);
	sMenuPause->CreateDefaultData();
	sMenuPause->objectParts_[0].materialConfig->textureHandle = TH_menuPause;
	sMenuPause->mainPosition.transform.translate = { 295.0f, 58.0f,100.0f };
	sMenuPause->followObject_ = sMenuBG_;
	sMenuPause->Update(nullptr);

	sMenuMASTER->IntObject(system_);
	sMenuMASTER->CreateDefaultData();
	sMenuMASTER->objectParts_[0].materialConfig->textureHandle = TH_menuMASTER;
	sMenuMASTER->mainPosition.transform.translate = { 68.0f, 140.0f, 103.0f };
	sMenuMASTER->followObject_ = sMenuBG_;
	sMenuMASTER->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuMASTER->Update(nullptr);
	buttonsOnColor_[(int)ButtonIndex::VolumeMASTER] = &sMenuMASTER->objectParts_[0].materialConfig->textureColor;

	sMenuMASTER_NL->IntObject(system_);
	sMenuMASTER_NL->CreateDefaultData();
	sMenuMASTER_NL->objectParts_[0].materialConfig->textureHandle = TH_menuMASTER_NL;
	sMenuMASTER_NL->mainPosition.transform.translate = { 68.0f, 140.0f, 102.0f };
	sMenuMASTER_NL->followObject_ = sMenuBG_;
	sMenuMASTER_NL->Update(nullptr);

	sMenuBGM->IntObject(system_);
	sMenuBGM->CreateDefaultData();
	sMenuBGM->objectParts_[0].materialConfig->textureHandle = TH_menuBGM;
	sMenuBGM->mainPosition.transform.translate = { 74.0f, 209.0f,105.0f };
	sMenuBGM->followObject_ = sMenuBG_;
	sMenuBGM->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuBGM->Update(nullptr);
	buttonsOnColor_[(int)ButtonIndex::VolumeBGM] = &sMenuBGM->objectParts_[0].materialConfig->textureColor;

	sMenuBGM_NL->IntObject(system_);
	sMenuBGM_NL->CreateDefaultData();
	sMenuBGM_NL->objectParts_[0].materialConfig->textureHandle = TH_menuBGM_NL;
	sMenuBGM_NL->mainPosition.transform.translate = { 74.0f, 209.0f,104.0f };
	sMenuBGM_NL->followObject_ = sMenuBG_;
	sMenuBGM_NL->Update(nullptr);

	sMenuSE->IntObject(system_);
	sMenuSE->CreateDefaultData();
	sMenuSE->objectParts_[0].materialConfig->textureHandle = TH_menuSE;
	sMenuSE->mainPosition.transform.translate = { 98.0f, 275.0f,107.0f };
	sMenuSE->followObject_ = sMenuBG_;
	sMenuSE->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuSE->Update(nullptr);
	buttonsOnColor_[(int)ButtonIndex::VolumeSE] = &sMenuSE->objectParts_[0].materialConfig->textureColor;

	sMenuSE_NL->IntObject(system_);
	sMenuSE_NL->CreateDefaultData();
	sMenuSE_NL->objectParts_[0].materialConfig->textureHandle = TH_menuSE_NL;
	sMenuSE_NL->mainPosition.transform.translate = { 98.0f, 275.0f,106.0f };
	sMenuSE_NL->followObject_ = sMenuBG_;
	sMenuSE_NL->Update(nullptr);

	sMenuClose->IntObject(system_);
	sMenuClose->CreateDefaultData();
	sMenuClose->objectParts_[0].materialConfig->textureHandle = TH_menuClose;
	sMenuClose->mainPosition.transform.translate = { 187.0f, 342.0f, 109.0f };
	sMenuClose->followObject_ = sMenuBG_;
	sMenuClose->Update(nullptr);
	buttonsOnColor_[(int)ButtonIndex::Close] = &sMenuClose->objectParts_[0].materialConfig->textureColor;

	sMenuClose_NL->IntObject(system_);
	sMenuClose_NL->CreateDefaultData();
	sMenuClose_NL->objectParts_[0].materialConfig->textureHandle = TH_menuClose_NL;
	sMenuClose_NL->mainPosition.transform.translate = { 187.0f, 342.0f, 108.0f };
	sMenuClose_NL->followObject_ = sMenuBG_;
	sMenuClose_NL->Update(nullptr);

	sMenuBack->IntObject(system_);
	sMenuBack->CreateDefaultData();
	sMenuBack->objectParts_[0].materialConfig->textureHandle = TH_menuBack;
	sMenuBack->mainPosition.transform.translate = { 523.0f, 342.0f, 111.0f };
	sMenuBack->followObject_ = sMenuBG_;
	sMenuBack->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuBack->Update(nullptr);
	sMenuBack->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	buttonsOnColor_[(int)ButtonIndex::Back] = &sMenuBack->objectParts_[0].materialConfig->textureColor;

	sMenuBack_NL->IntObject(system_);
	sMenuBack_NL->CreateDefaultData();
	sMenuBack_NL->objectParts_[0].materialConfig->textureHandle = TH_menuBack_NL;
	sMenuBack_NL->mainPosition.transform.translate = { 523.0f, 342.0f, 110.0f };
	sMenuBack_NL->followObject_ = sMenuBG_;
	sMenuBack_NL->Update(nullptr);

	sMenuRetry->IntObject(system_);
	sMenuRetry->CreateDefaultData();
	sMenuRetry->objectParts_[0].materialConfig->textureHandle = TH_menuRetry;
	sMenuRetry->mainPosition.transform.translate = { 355.0f, 342.0f, 113.0f };
	sMenuRetry->followObject_ = sMenuBG_;
	sMenuRetry->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuRetry->Update(nullptr);
	sMenuRetry->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	buttonsOnColor_[(int)ButtonIndex::Retry] = &sMenuRetry->objectParts_[0].materialConfig->textureColor;

	sMenuRetry_NL->IntObject(system_);
	sMenuRetry_NL->CreateDefaultData();
	sMenuRetry_NL->objectParts_[0].materialConfig->textureHandle = TH_menuRetry_NL;
	sMenuRetry_NL->mainPosition.transform.translate = { 355.0f, 342.0f, 112.0f };
	sMenuRetry_NL->followObject_ = sMenuBG_;
	sMenuRetry_NL->Update(nullptr);

	sMenuButtonM->IntObject(system_);
	sMenuButtonM->CreateDefaultData();
	sMenuButtonM->objectParts_[0].materialConfig->textureHandle = TH_menuButton;
	sMenuButtonM->mainPosition.transform.translate = { 195.0f, 143.0f, 115.0f };
	sMenuButtonM->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, masterVolume_);
	sMenuButtonM->followObject_ = sMenuBG_;
	sMenuButtonM->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuButtonM->Update(nullptr);

	sMenuButtonM_NL->IntObject(system_);
	sMenuButtonM_NL->CreateDefaultData();
	sMenuButtonM_NL->objectParts_[0].materialConfig->textureHandle = TH_menuButton_NL;
	sMenuButtonM_NL->mainPosition.transform.translate = { 0, 0, -1.0f };
	sMenuButtonM_NL->followObject_ = sMenuButtonM;
	sMenuButtonM_NL->Update(nullptr);

	sMenuButtonB->IntObject(system_);
	sMenuButtonB->CreateDefaultData();
	sMenuButtonB->objectParts_[0].materialConfig->textureHandle = TH_menuButton;
	sMenuButtonB->mainPosition.transform.translate = { 195.0f, 210.0f, 117.0f };
	sMenuButtonB->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, BGMVolume_);
	sMenuButtonB->followObject_ = sMenuBG_;
	sMenuButtonB->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuButtonB->Update(nullptr);

	sMenuButtonB_NL->IntObject(system_);
	sMenuButtonB_NL->CreateDefaultData();
	sMenuButtonB_NL->objectParts_[0].materialConfig->textureHandle = TH_menuButton_NL;
	sMenuButtonB_NL->mainPosition.transform.translate = { 0, 0, -1.0f };
	sMenuButtonB_NL->followObject_ = sMenuButtonB;
	sMenuButtonB_NL->Update(nullptr);

	sMenuButtonS->IntObject(system_);
	sMenuButtonS->CreateDefaultData();
	sMenuButtonS->objectParts_[0].materialConfig->textureHandle = TH_menuButton;
	sMenuButtonS->mainPosition.transform.translate = { 195.0f, 277.0f, 119.0f };
	sMenuButtonS->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, SEVolume_);
	sMenuButtonS->followObject_ = sMenuBG_;
	sMenuButtonS->objectParts_[0].materialConfig->textureColor.w = 0.0f;
	sMenuButtonS->Update(nullptr);

	sMenuButtonS_NL->IntObject(system_);
	sMenuButtonS_NL->CreateDefaultData();
	sMenuButtonS_NL->objectParts_[0].materialConfig->textureHandle = TH_menuButton_NL;
	sMenuButtonS_NL->mainPosition.transform.translate = { 0, 0, -1.0f };
	sMenuButtonS_NL->followObject_ = sMenuButtonS;
	sMenuButtonS_NL->Update(nullptr);

	sMenuVolumeBarM->IntObject(system_);
	sMenuVolumeBarM->CreateDefaultData();
	sMenuVolumeBarM->objectParts_[0].materialConfig->textureHandle = TH_menuVolumeBar;
	sMenuVolumeBarM->mainPosition.transform.translate = { 204.0f, 136.0f, 120.0f };
	sMenuVolumeBarM->followObject_ = sMenuBG_;
	sMenuVolumeBarM->Update(nullptr);

	sMenuVolumeBarB->IntObject(system_);
	sMenuVolumeBarB->CreateDefaultData();
	sMenuVolumeBarB->objectParts_[0].materialConfig->textureHandle = TH_menuVolumeBar;
	sMenuVolumeBarB->mainPosition.transform.translate = { 204.0f, 203.0f, 121.0f };
	sMenuVolumeBarB->followObject_ = sMenuBG_;
	sMenuVolumeBarB->Update(nullptr);

	sMenuVolumeBarS->IntObject(system_);
	sMenuVolumeBarS->CreateDefaultData();
	sMenuVolumeBarS->objectParts_[0].materialConfig->textureHandle = TH_menuVolumeBar;
	sMenuVolumeBarS->mainPosition.transform.translate = { 204.0f, 270.0f, 122.0f };
	sMenuVolumeBarS->followObject_ = sMenuBG_;
	sMenuVolumeBarS->Update(nullptr);

	/// ================ testSound ================///
	//SH_menuSE_ = system_->SoundLoadSE("resources/TemplateResource/sound/SE/game_start.wav");
	//SH_menuBGM_ = system_->SoundLoadSE("resources/TemplateResource/sound/BGM/Ordinary_Magic.wav");
	//
	//system_->SoundSetMasterVolume(masterVolume_);
	//system_->SoundSetBGMVolume(masterVolume_);
	//system_->SoundSetSEVolume(masterVolume_);

}

void DefaultMenu::Updata() {

	/// ============ メニュー開閉 ============///
	if (canOpen_) {
		if (system_->GetTriggerOn(DIK_ESCAPE)) {
			animationTimer_->Reset0();
			if (isOpened_) {
				phase_ = MenuPhase::TRANSITION;
				startTransform_ = sMenuBG_->mainPosition.transform;
				lastSelectedMenuIndex_ = selectedMenuIndex_;
				selectedMenuIndex_ = (int)ButtonIndex::Close;
			} else {
				isOpened_ = true;
				phase_ = MenuPhase::ENTRY;
				startTransform_ = sMenuBG_->mainPosition.transform;
				lastSelectedMenuIndex_ = selectedMenuIndex_;
				selectedMenuIndex_ = (int)ButtonIndex::Close;
			}
		}
	}

	if (!isOpened_) {
		if (animationTimer_->parameter_ == animationTimer_->maxTime_) {
			return;
		}
	}

	/// ============ メニュー更新 ============///
	float T;
	isConfirm_ = false;
	switch (phase_) {
	case MenuPhase::ENTRY:
		if (!isPause_)isPause_ = true;

		animationTimer_->ToMix();
		T = animationTimer_->easyIn(2.0f);
		sMenuBG_->mainPosition.transform.translate.y = easyIn(startTransform_.translate.y, menuNormalPos_.y, T, 2.0f);

		if (animationTimer_->parameter_ == animationTimer_->maxTime_) {
			phase_ = MenuPhase::PROGRESS;
		}
		ChangeSelect();
		break;
	case MenuPhase::PROGRESS:
		if (!isPause_)isPause_ = true;

		CheckClick();
		ChangeSelect();
		WorkChange();

		break;
	case MenuPhase::TRANSITION:
		if (isPause_)isPause_ = false;

		animationTimer_->ToMix();
		T = animationTimer_->easyOut(2.0f);
		sMenuBG_->mainPosition.transform.translate.y = easyOut(startTransform_.translate.y, menuStartPos_.y, T, 2.0f);

		if (animationTimer_->parameter_ == animationTimer_->maxTime_) {
			phase_ = MenuPhase::EXIT;
		}
		break;
	case MenuPhase::EXIT:
		if (isPause_)isPause_ = false;
		isOpened_ = false;
		break;
	}

	MenuObjectUpdate();
}


void DefaultMenu::Draw() {

	if (!isOpened_) {
		if (animationTimer_->parameter_ == animationTimer_->maxTime_)return;
	}
	/// ============ メニュー描画 ============///
	sMenuBG_->Draw();

	sMenuPause->Draw();
	sMenuBGM_NL->Draw();
	sMenuBGM->Draw();
	sMenuMASTER_NL->Draw();
	sMenuMASTER->Draw();
	sMenuSE_NL->Draw();
	sMenuSE->Draw();

	sMenuClose_NL->Draw();
	sMenuClose->Draw();
	sMenuBack_NL->Draw();
	sMenuBack->Draw();
	sMenuRetry_NL->Draw();
	sMenuRetry->Draw();

	sMenuVolumeBarM->Draw();
	sMenuVolumeBarB->Draw();
	sMenuVolumeBarS->Draw();
	sMenuButtonM_NL->Draw();
	sMenuButtonM->Draw();
	sMenuButtonB_NL->Draw();
	sMenuButtonB->Draw();
	sMenuButtonS_NL->Draw();
	sMenuButtonS->Draw();

#ifdef USE_IMGUI
	ImguiPart();
#endif
}

void DefaultMenu::MenuObjectUpdate() {
	if (isOpened_) {
		sMenuBG_->Update(nullptr);

		sMenuPause->Update(nullptr);
		sMenuBGM_NL->Update(nullptr);
		sMenuBGM->Update(nullptr);
		sMenuMASTER_NL->Update(nullptr);
		sMenuMASTER->Update(nullptr);
		sMenuSE_NL->Update(nullptr);
		sMenuSE->Update(nullptr);

		sMenuClose_NL->Update(nullptr);
		sMenuClose->Update(nullptr);
		sMenuBack_NL->Update(nullptr);
		sMenuBack->Update(nullptr);
		sMenuRetry_NL->Update(nullptr);
		sMenuRetry->Update(nullptr);

		sMenuButtonM_NL->Update(nullptr);
		sMenuButtonM->Update(nullptr);
		sMenuButtonB_NL->Update(nullptr);
		sMenuButtonB->Update(nullptr);
		sMenuButtonS_NL->Update(nullptr);
		sMenuButtonS->Update(nullptr);

		sMenuVolumeBarM->Update(nullptr);
		sMenuVolumeBarB->Update(nullptr);
		sMenuVolumeBarS->Update(nullptr);


		//system_->SoundPlayBGM(SH_menuBGM_, BGMVolume_);
	}
	//else {
	//	system_->SoundStop(SH_menuBGM_);
	//}
}

bool DefaultMenu::IsRetry() {
	if (isConfirm_ && selectedMenuIndex_ == (int)ButtonIndex::Retry) {
		return true;
	}
	return false;
}

bool DefaultMenu::IsBack() {
	if (isConfirm_ && selectedMenuIndex_ == (int)ButtonIndex::Back) {
		return true;
	}
	return false;
}


void DefaultMenu::ChangeSelect() {

	if (clickRight_) {
		if (selectedMenuIndex_ < 2) {
			lastSelectedMenuIndex_ = selectedMenuIndex_;
			selectedMenuIndex_++;
			buttonTimer_->Reset0();
		}
	}
	if (clickLeft_) {
		if (selectedMenuIndex_ > 0 && selectedMenuIndex_ <= 2) {
			lastSelectedMenuIndex_ = selectedMenuIndex_;
			selectedMenuIndex_--;
			buttonTimer_->Reset0();
		}
	}
	if (clickUp_) {
		lastSelectedMenuIndex_ = selectedMenuIndex_;
		if (selectedMenuIndex_ <= 2) {
			selectedMenuIndex_ = (int)ButtonIndex::VolumeSE;
		} else if (selectedMenuIndex_ > 2 && selectedMenuIndex_ < 5) {
			selectedMenuIndex_++;
		}
		buttonTimer_->Reset0();
	}
	if (clickDown_) {
		lastSelectedMenuIndex_ = selectedMenuIndex_;
		if (selectedMenuIndex_ == (int)ButtonIndex::VolumeSE) {
			selectedMenuIndex_ = (int)ButtonIndex::Close;
		} else if (selectedMenuIndex_ > 3 && selectedMenuIndex_ <= 5) {
			selectedMenuIndex_--;
		}
		buttonTimer_->Reset0();
	}
	ChangeButtonLight();
}

void DefaultMenu::CheckClick() {
	clickLeft_ = system_->GetTriggerOn(DIK_LEFT) || system_->GetTriggerOn(DIK_A);
	clickRight_ = system_->GetTriggerOn(DIK_RIGHT) || system_->GetTriggerOn(DIK_D);
	clickUp_ = system_->GetTriggerOn(DIK_UP) || system_->GetTriggerOn(DIK_W);
	clickDown_ = system_->GetTriggerOn(DIK_DOWN) || system_->GetTriggerOn(DIK_S);
	clickDecide_ = system_->GetTriggerOn(DIK_RETURN) || system_->GetTriggerOn(DIK_SPACE);
}

void DefaultMenu::WorkChange() {

	isConfirm_ = false;
	switch (selectedMenuIndex_) {
	case (int)ButtonIndex::Close:
		if (clickDecide_) {
			animationTimer_->Reset0();
			phase_ = MenuPhase::TRANSITION;
			startTransform_ = sMenuBG_->mainPosition.transform;
			selectedMenuIndex_ = (int)ButtonIndex::Close;
		}
		break;

	case (int)ButtonIndex::Retry:
		if (clickDecide_) {
			isConfirm_ = true;
			animationTimer_->Reset0();
			phase_ = MenuPhase::TRANSITION;
			startTransform_ = sMenuBG_->mainPosition.transform;
		}
		break;

	case (int)ButtonIndex::Back:
		if (clickDecide_) {
			isConfirm_ = true;
			animationTimer_->Reset0();
			phase_ = MenuPhase::TRANSITION;
			startTransform_ = sMenuBG_->mainPosition.transform;
		}
		break;

	case (int)ButtonIndex::VolumeSE:
		if (clickLeft_) SEVolume_ -= 0.1f;
		else if (clickRight_) SEVolume_ += 0.1f;
		if (clickLeft_ || clickRight_) {
			SEVolume_ = std::clamp(SEVolume_, 0.0f, 1.0f);
			system_->SoundSetSEVolume(SEVolume_);
			sMenuButtonS->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, SEVolume_);
		}
		break;

	case (int)ButtonIndex::VolumeBGM:
		if (clickLeft_) BGMVolume_ -= 0.1f;
		else if (clickRight_) BGMVolume_ += 0.1f;
		if (clickLeft_ || clickRight_) {
			BGMVolume_ = std::clamp(BGMVolume_, 0.0f, 1.0f);
			system_->SoundSetBGMVolume(BGMVolume_);
			sMenuButtonB->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, BGMVolume_);
		}
		break;

	case (int)ButtonIndex::VolumeMASTER:
		if (clickLeft_) masterVolume_ -= 0.1f;
		else if (clickRight_) masterVolume_ += 0.1f;
		if (clickLeft_ || clickRight_) {
			masterVolume_ = std::clamp(masterVolume_, 0.0f, 1.0f);
			system_->SoundSetMasterVolume(masterVolume_);
			sMenuButtonM->mainPosition.transform.translate.x = linearity(volumeButton0x_, volumeButton1x_, masterVolume_);
		}
		break;
	}

}


void DefaultMenu::ChangeButtonLight() {
	buttonTimer_->ToMix();
	float T = buttonTimer_->linearity();

	float onButtonAlpha = linearity(0.0f, 1.0f, T);
	float offButtonAlpha = linearity(1.0f, 0.0f, T);

	for (int i = 0; i < 6; i++) {
		if (i == selectedMenuIndex_) {
			buttonsOnColor_[i]->w = onButtonAlpha;
		} else if (i == lastSelectedMenuIndex_) {
			buttonsOnColor_[i]->w = offButtonAlpha;
		} else {
			if (buttonsOnColor_[i]->w != 0.0f) {
				buttonsOnColor_[i]->w = 0.0f;
			}
		}

		if (selectedMenuIndex_ == (int)ButtonIndex::VolumeMASTER) {
			sMenuButtonM->objectParts_[0].materialConfig->textureColor.w = onButtonAlpha;
		} else if (lastSelectedMenuIndex_ == (int)ButtonIndex::VolumeMASTER) {
			sMenuButtonM->objectParts_[0].materialConfig->textureColor.w = offButtonAlpha;
		} else {
			if (sMenuButtonM->objectParts_[0].materialConfig->textureColor.w != 0.0f) {
				sMenuButtonM->objectParts_[0].materialConfig->textureColor.w = 0.0f;
			}
		}

		if (selectedMenuIndex_ == (int)ButtonIndex::VolumeBGM) {
			sMenuButtonB->objectParts_[0].materialConfig->textureColor.w = onButtonAlpha;
		} else if (lastSelectedMenuIndex_ == (int)ButtonIndex::VolumeBGM) {
			sMenuButtonB->objectParts_[0].materialConfig->textureColor.w = offButtonAlpha;
		} else {
			if (sMenuButtonB->objectParts_[0].materialConfig->textureColor.w != 0.0f) {
				sMenuButtonB->objectParts_[0].materialConfig->textureColor.w = 0.0f;
			}
		}

		if (selectedMenuIndex_ == (int)ButtonIndex::VolumeSE) {
			sMenuButtonS->objectParts_[0].materialConfig->textureColor.w = onButtonAlpha;
		} else if (lastSelectedMenuIndex_ == (int)ButtonIndex::VolumeSE) {
			sMenuButtonS->objectParts_[0].materialConfig->textureColor.w = offButtonAlpha;
		} else {
			if (sMenuButtonS->objectParts_[0].materialConfig->textureColor.w != 0.0f) {
				sMenuButtonS->objectParts_[0].materialConfig->textureColor.w = 0.0f;
			}
		}
	}
}

#ifdef USE_IMGUI
void DefaultMenu::ImguiPart() {

	ImGui::Begin("MenuData");
	ImGui::SliderFloat3("MenuPosition", &sMenuBG_->mainPosition.transform.translate.x, -500.0f, 1920.0f);
	ImGui::Text("Timer: %.02f,%.02f", animationTimer_->parameter_, animationTimer_->maxTime_);
	ImGui::Text("selectedMenuIndex_: %d", GetSelectedIndex());
	ImGui::Checkbox("isOpened_", &isOpened_);



	ImGui::SliderFloat("masterVolume", &masterVolume_, -0.5f, 1.5f);
	ImGui::SliderFloat("BGMVolume", &BGMVolume_, -0.5f, 1.5f);
	ImGui::SliderFloat("SEVolume", &SEVolume_, -0.5f, 1.5f);


	if (IsRetry()) {
		ImGui::Text("IsRetry: True");
	} else {
		ImGui::Text("IsRetry: False");
	}

	if (IsBack()) {
		ImGui::Text("IsBack: True");
	} else {
		ImGui::Text("IsBack: False");
	}



	ImGui::End();

}
#endif
