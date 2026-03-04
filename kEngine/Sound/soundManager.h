#pragma once
#include "soundUnit.h"
#include <vector>
#include <memory>

/// 将来FFmpeg足した後全種類のやり方が統一するから気にしない

class SoundManager
{
public:
	void Initialize();
	void Finalize();

	int SoundLoadFile(const std::string& filename);
	void SoundPlaySE(int Handle, float volume);
	void SoundPlayBGM(int Handle, float volume);

	void SoundSetVolume(int Handle, float volume);
	void SoundPause(int Handle);
	void SoundContinue(int Handle);
	void SoundStop(int Handle);

	void SoundSetMasterVolume(float volume);
	void SoundSetMasterSEVolume(float volume);
	void SoundSetMasterBGMVolume(float volume);

	bool isPlaying(int Handle);
	float SoundGetVolume(int Handle) const;
	float SoundGetMasterVolume() const { return cMasterVolume_; }
	float SoundGetBGMVolume() const { return cBgmVolume_; }
	float SoundGetSEVolume() const { return cSeVolume_; }

	void SoundSetMute(int handle, bool isMute);
	void SoundSetMasterMute(bool isMute);
	void SoundSetBGMMute(bool isMute);
	void SoundSetSEMute(bool isMute);

	bool SoundGetMute(int Handle);
	bool SoundGetMasterMute() const { return masterMute_; };
	bool SoundGetBGMMute()const { return bgmMute_; };
	bool SoundGetSEMute()const { return seMute_; };


private:

	bool masterMute_ = false;
	bool bgmMute_ = false;
	bool seMute_ = false;

	float masterVolume_ = 1.0f;
	float bgmVolume_ = 1.0f;
	float seVolume_ = 1.0f;
	float cMasterVolume_ = 1.0f;
	float cBgmVolume_ = 1.0f;
	float cSeVolume_ = 1.0f;
	float nSeVolume_ = 1.0f;
	float nBGMVolume_ = 1.0f;


private:

	Microsoft::WRL::ComPtr<IXAudio2> xAudio2 = nullptr;
	IXAudio2MasteringVoice* masterVoice = nullptr;

	std::vector<SoundUnit*> sounds_;
	int resourceCounter = 0;
};

///エンさんは結構萎えたね