#pragma once
#include <xaudio2.h>
#pragma comment(lib,"xaudio2.lib")

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#pragma comment(lib,"mfplat.lib")
#pragma comment(lib,"mf.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


#include <fstream>
#include <wrl.h>

#include <cassert>

#include <vector>
#include <memory>

#include <string.h>
#include <cstring> 

#pragma region wav Reader
/// チャンクヘッダ
struct ChunkHeader {
	char id[4];// チャンク每のID
	int32_t size;// チャンクサイズ
};
/// RIFFヘッダチャンク
struct RiffHeader {
	ChunkHeader chunk;// "RIFF"
	char type[4];// "WAVE"
};
/// FMTチャンク
struct FormatChunk {
	ChunkHeader chunk;// "fmt"
	WAVEFORMATEX fmt;// 波形フォーマット
};
#pragma endregion

/// 音声データ
struct SoundData {
	// 波形フォーマット
	WAVEFORMATEXTENSIBLE wfex{};
	// バッファ
	std::vector<BYTE> buffer;
};

class VoiceCallback : public IXAudio2VoiceCallback {
public:
	void STDMETHODCALLTYPE OnBufferEnd(void* pBufferContext) override {
		isBGMPlaying = false;
	}

	void STDMETHODCALLTYPE OnBufferStart(void* pBufferContext) override {
		isBGMPlaying = true;
	}

	// 其他 callback 可留空
	void STDMETHODCALLTYPE OnVoiceProcessingPassStart(UINT32) override {}
	void STDMETHODCALLTYPE OnVoiceProcessingPassEnd() override {}
	void STDMETHODCALLTYPE OnStreamEnd() override {}
	void STDMETHODCALLTYPE OnLoopEnd(void*) override {}
	void STDMETHODCALLTYPE OnVoiceError(void*, HRESULT) override {}

	bool isBGMPlaying = false;
};

class SoundUnit
{
public:

	enum class Type {
		NONE,
		SE,
		BGM,
	};

public:
	SoundUnit();
	~SoundUnit();

	SoundData SoundLoad(const std::string& filename);

	void SoundPlaySE(IXAudio2* xAudio2, float cVolume, float volume);
	void SoundPlayBGM(IXAudio2* xAudio2, float cVolume, float volume);

	void SoundSetVolume(float cVolume, float volume);
	void SoundSetMasterVolume(float cVolume);
	void SoundPause();
	void SoundContinue();
	void SoundStop();

	bool isPlaying();
	float GetVolume() const { return iVolume_; }
	float GetfVolume() const { return fVolume_; }
	Type GetSoundType() const { return soundType; }

	void SoundSetMute(bool isMute);
	bool SoundGetMute()const { return isMute_; }

	const std::string& GetFileName() const{ return filename_; }

	void SoundUnload();

private:
	std::unique_ptr<SoundData> soundData;
	std::unique_ptr <VoiceCallback> voiceCallBack_;
	std::string filename_{};

	/// やむ操作
	std::vector<IXAudio2SourceVoice*> pSourceVoiceGroup;
	IXAudio2SourceVoice* pSourceVoice_ = nullptr;
	bool isPause_ = false;
	float volume_ = 1.0f;
	float iVolume_ = 1.0f;
	float cVolume_ = 1.0f;
	float fVolume_ = 1.0f;

	bool isMute_ = false;

	/// type
	Type soundType = Type::NONE;

private:
	SoundData SoundLoadWave(const std::string& filename);
};

