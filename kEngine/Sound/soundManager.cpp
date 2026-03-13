#include "soundManager.h"
#include <algorithm>

void SoundManager::Initialize() {
	HRESULT result = XAudio2Create(&xAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	assert(SUCCEEDED(result));
	result = xAudio2->CreateMasteringVoice(&masterVoice);
	assert(SUCCEEDED(result));
	result = MFStartup(MF_VERSION,MFSTARTUP_NOSOCKET);
	assert(SUCCEEDED(result));
}

void SoundManager::Finalize() {
	for(auto& ptr:sounds_){
		ptr.reset();
	}
	sounds_.clear();
	resourceCounter = 0;

	if (masterVoice) { masterVoice->DestroyVoice(); masterVoice = nullptr; }
	xAudio2.Reset();
	
	HRESULT result = MFShutdown();
	assert(SUCCEEDED(result));
}

int SoundManager::SoundLoadFile(const std::string& filename) {
	// reuse existing sound if already loaded
	for (int i = 0; i < static_cast<int>(sounds_.size()); ++i) {
		if (sounds_[i]->GetFileName() == std::string(filename)) {
			return i; // 0-based handle
		}
	}

	auto sound = std::make_unique<SoundUnit>();
	sound->SoundLoad(filename);
	sounds_.push_back(std::move(sound));
	resourceCounter = static_cast<int>(sounds_.size());
	return resourceCounter - 1; // new 0-based handle
}

void SoundManager::SoundPlaySE(int Handle, float volume) {
	float nSeVolume = cMasterVolume_ * cSeVolume_;
	nSeVolume = std::clamp(nSeVolume, 0.0f, 2.0f);
	if(masterMute_ || seMute_){
		sounds_[Handle]->SoundPlaySE(xAudio2.Get(), 0.0f, volume);
	} else {
		sounds_[Handle]->SoundPlaySE(xAudio2.Get(), nSeVolume, volume);
	}
}

void SoundManager::SoundPlayBGM(int Handle, float volume) {
	if (isPlaying(Handle)) return;
	float nBGMVolume = cMasterVolume_ * cBgmVolume_;
	nBGMVolume = std::clamp(nBGMVolume, 0.0f, 2.0f);
	if (masterMute_ || bgmMute_) {
		sounds_[Handle]->SoundPlayBGM(xAudio2.Get(), 0.0f, volume);
	} else {
		sounds_[Handle]->SoundPlayBGM(xAudio2.Get(), nBGMVolume, volume);
	}
}



void SoundManager::SoundSetVolume(int Handle, float volume) {
	auto sound = sounds_[Handle].get();
	SoundUnit::Type type = sound->GetSoundType();

	float finalVolume = 0.0f;

	if (type == SoundUnit::Type::SE) {
		if (!masterMute_ && !seMute_ && !sound->SoundGetMute()) {
			finalVolume = std::clamp(cMasterVolume_ * cSeVolume_, 0.0f, 2.0f);
		}
		sound->SoundSetVolume(finalVolume, volume);
		return;
	}

	if (type == SoundUnit::Type::BGM) {
		if (!masterMute_ && !bgmMute_ && !sound->SoundGetMute()) {
			finalVolume = std::clamp(cMasterVolume_ * cBgmVolume_, 0.0f, 2.0f);
		}
		sound->SoundSetVolume(finalVolume, volume);
		return;
	}
}

void SoundManager::SoundPause(int Handle) {
	sounds_[Handle]->SoundPause();
};

void SoundManager::SoundContinue(int Handle) {
	sounds_[Handle]->SoundContinue();
}
void SoundManager::SoundStop(int Handle) {
	sounds_[Handle]->SoundStop();
}



void SoundManager::SoundSetMasterVolume(float volume) {
	if (volume == masterVolume_) return;

	masterVolume_ = volume;
	cMasterVolume_ = std::clamp(volume, 0.0f, 2.0f);

	nSeVolume_ = std::clamp(cMasterVolume_ * cSeVolume_, 0.0f, 2.0f);
	nBGMVolume_ = std::clamp(cMasterVolume_ * cBgmVolume_, 0.0f, 2.0f);

	if (masterMute_) return;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::SE) {
			if (!seMute_) {
				ptr->SoundSetMasterVolume(nSeVolume_);
			}
		}
		if (ptr->GetSoundType() == SoundUnit::Type::BGM) {
			if (!bgmMute_) {
				ptr->SoundSetMasterVolume(nBGMVolume_);
			}
		}
	}
}

void SoundManager::SoundSetMasterSEVolume(float volume) {
	if (volume == seVolume_) return;

	seVolume_ = volume;
	cSeVolume_ = std::clamp(volume, 0.0f, 2.0f);
	nSeVolume_ = std::clamp(cMasterVolume_ * cSeVolume_, 0.0f, 2.0f);

	if (masterMute_ || seMute_) return;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::SE) {
			ptr->SoundSetMasterVolume(nSeVolume_);
		}
	}
}

void SoundManager::SoundSetMasterBGMVolume(float volume) {
	if (volume == bgmVolume_) return;

	bgmVolume_ = volume;
	cBgmVolume_ = std::clamp(volume, 0.0f, 2.0f);
	nBGMVolume_ = std::clamp(cMasterVolume_ * cBgmVolume_, 0.0f, 2.0f);

	if (masterMute_ || bgmMute_) return;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::BGM) {
			ptr->SoundSetMasterVolume(nBGMVolume_);
		}
	}
}




bool SoundManager::isPlaying(int Handle) {
	return sounds_[Handle]->isPlaying();
}

float SoundManager::SoundGetVolume(int Handle) const {
	return sounds_[Handle]->GetfVolume();
}




void SoundManager::SoundSetMute(int Handle, bool isMute) {
	sounds_[Handle]->SoundSetMute(isMute);
}

void SoundManager::SoundSetMasterMute(bool isMute) {
	if (isMute == masterMute_) return;
	masterMute_ = isMute;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::SE) {
			if (isMute) {
				ptr->SoundSetMasterVolume(0.0f);
			} else if (!seMute_) {
				ptr->SoundSetMasterVolume(nSeVolume_);
			}
		}
		if (ptr->GetSoundType() == SoundUnit::Type::BGM) {
			if (isMute) {
				ptr->SoundSetMasterVolume(0.0f);
			} else if (!bgmMute_) {
				ptr->SoundSetMasterVolume(nBGMVolume_);
			}
		}
	}
}

void SoundManager::SoundSetBGMMute(bool isMute) {
	if (isMute == bgmMute_) return;
	bgmMute_ = isMute;
	if (masterMute_) return;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::BGM) {
			if (isMute) {
				ptr->SoundSetMasterVolume(0.0f);
			} else {
				ptr->SoundSetMasterVolume(nBGMVolume_);
			}
		}
	}
}

void SoundManager::SoundSetSEMute(bool isMute) {
	if (isMute == seMute_) return;
	seMute_ = isMute;
	if (masterMute_) return;
	for (auto& ptr : sounds_) {
		if (ptr->GetSoundType() == SoundUnit::Type::SE) {
			if (isMute) {
				ptr->SoundSetMasterVolume(0.0f);
			} else {
				ptr->SoundSetMasterVolume(nSeVolume_);
			}
		}
	}
}



bool SoundManager::SoundGetMute(int Handle) {
	return sounds_[Handle]->SoundGetMute();
}