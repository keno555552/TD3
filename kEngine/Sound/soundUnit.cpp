#include "soundUnit.h"
#include "StringManage\ConvertString.h"
#include <algorithm>

SoundUnit::SoundUnit() {
	soundData = std::make_unique<SoundData>();
	voiceCallBack_ = std::make_unique<VoiceCallback>();
}

SoundUnit::~SoundUnit() {
	SoundUnload();
}



SoundData SoundUnit::SoundLoadWave(const std::string& filename) {

	/// リソース作成
	// フルバスをワイド文字列に変換
	std::wstring filePathW = ConvertString::SwitchStdStringWstring(filename);
	HRESULT result;

	// SourceReaderを作成
	Microsoft::WRL::ComPtr<IMFSourceReader> pReader;
	result = MFCreateSourceReaderFromURL(filePathW.c_str(), nullptr, &pReader);
	assert(SUCCEEDED(result));

	/// フォーマット指定
	// PCM形式にフォーマット指定する
	Microsoft::WRL::ComPtr<IMFMediaType> pPCMType;
	MFCreateMediaType(&pPCMType);
	pPCMType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPCMType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	result = pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, pPCMType.Get());
	assert(SUCCEEDED(result));

	/// Waveフォーマットの取得
	// 実際にセットされたメディアタイプを取得する
	Microsoft::WRL::ComPtr<IMFMediaType> pOutype;
	pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pOutype);

	// Waveフォーマットを取得する
	WAVEFORMATEX* waveFormat = nullptr;
	result = MFCreateWaveFormatExFromMFMediaType(pOutype.Get(), &waveFormat, nullptr);
	assert(SUCCEEDED(result));

	/// 音声データに格納
	//　コンテナに格納する音声データ
	soundData = std::make_unique<SoundData>();

	memcpy(&soundData->wfex, waveFormat, waveFormat->cbSize + sizeof(WAVEFORMATEX));

	// 生成したWaveフォーマットを解放
	CoTaskMemFree(waveFormat);

	/// PCM形式の音声データを読み込む
	// PCMデータのバッファを構築
	while (true) {
		Microsoft::WRL::ComPtr<IMFSample> pSample;
		DWORD streamIndex = 0, flags = 0;
		LONGLONG llTimeStamp = 0;

		// サンプルを読み込む
		result = pReader->ReadSample(
			MF_SOURCE_READER_FIRST_AUDIO_STREAM,
			0,
			&streamIndex,
			&flags,
			&llTimeStamp,
			&pSample
		);

		assert(SUCCEEDED(result));

		// ストリームの末尾に達したら抜ける
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;

		if (pSample) {
			Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer;

			// サンプルに含まれるサウンドデータのバッファを一繋ぎにして取得
			result = pSample->ConvertToContiguousBuffer(&pBuffer);

			BYTE* pData = nullptr;              // データ読み取り用ポインタ
			DWORD maxLength = 0, currentLength = 0;



			// バッファ読み込み用にロック
			pBuffer->Lock(&pData, &maxLength, &currentLength);

			// バッファの末尾にデータを追加
			if (currentLength == 0) {
				pBuffer->Unlock();
				continue;
			}

			soundData->buffer.insert(soundData->buffer.end(), pData, pData + currentLength);
			pBuffer->Unlock();
		}
	}

	return *soundData;
}

SoundData SoundUnit::SoundLoad(const std::string& filename) {
	filename_ = filename; // 直接複製，不需要 new/delete
	return SoundLoadWave(filename_.c_str());
}

void SoundUnit::SoundUnload() {
	/// バッファのメモリを解放
	if (pSourceVoice_) {
		pSourceVoice_->Stop();
		pSourceVoice_->FlushSourceBuffers();
		pSourceVoice_->DestroyVoice();
		pSourceVoice_ = nullptr;
	}
	if (!pSourceVoiceGroup.empty()) {
		for (auto ptr : pSourceVoiceGroup) {
			if (ptr) {
				ptr->Stop();
				ptr->FlushSourceBuffers();
				ptr->DestroyVoice();
			}
		}
	}
	pSourceVoiceGroup.clear();

	soundData->buffer.clear();
	soundData->wfex = {};
}



void SoundUnit::SoundPlaySE(IXAudio2* xAudio2, float cVolume, float volume) {
	HRESULT result;

	/// 波形フォーマットを元にSourceVoiceの生成（SE は再生中フラグ不要なので callback なし）

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	result = xAudio2->CreateSourceVoice(&pSourceVoice,
		reinterpret_cast<WAVEFORMATEX*>(&soundData->wfex),
		0,
		XAUDIO2_DEFAULT_FREQ_RATIO,
		nullptr);
	pSourceVoiceGroup.push_back(pSourceVoice);

	assert(SUCCEEDED(result));

	soundType = Type::SE;

	/// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = reinterpret_cast<const BYTE*>(soundData->buffer.data());
	buf.AudioBytes = static_cast<UINT32>(soundData->buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;

	/// 波形データの再生
	result = pSourceVoice->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));

	result = pSourceVoice->Start();
	assert(SUCCEEDED(result));

	volume_ = volume;
	cVolume_ = cVolume;
	iVolume_ = std::clamp(volume, 0.0f, 2.0f);
	fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
	if (!isMute_) {
		pSourceVoice->SetVolume(fVolume_);
	} else {
		pSourceVoice->SetVolume(0.0f);
	}
}

void SoundUnit::SoundPlayBGM(IXAudio2* xAudio2, float cVolume, float volume) {
	HRESULT result;

	/// すでにSourceVoiceが存在する場合は停止して破棄
	if (pSourceVoice_) {
		pSourceVoice_->Stop();
		pSourceVoice_->FlushSourceBuffers();
		pSourceVoice_->DestroyVoice();
		pSourceVoice_ = nullptr;
	}

	/// 波形フォーマットを元にSourceVoiceの生成
	result = xAudio2->CreateSourceVoice(&pSourceVoice_,
		reinterpret_cast<WAVEFORMATEX*>(&soundData->wfex),
		0,
		XAUDIO2_DEFAULT_FREQ_RATIO,
		voiceCallBack_.get());
	assert(SUCCEEDED(result));

	soundType = Type::BGM;

	/// 再生する波形データの設定
	XAUDIO2_BUFFER buf{};
	buf.pAudioData = reinterpret_cast<const BYTE*>(soundData->buffer.data());
	buf.AudioBytes = static_cast<UINT32>(soundData->buffer.size());
	buf.Flags = XAUDIO2_END_OF_STREAM;
	buf.LoopCount = XAUDIO2_LOOP_INFINITE;

	/// 波形データの再生
	result = pSourceVoice_->SubmitSourceBuffer(&buf);
	assert(SUCCEEDED(result));
	result = pSourceVoice_->Start();
	assert(SUCCEEDED(result));

	volume_ = volume;
	cVolume_ = cVolume;
	iVolume_ = std::clamp(volume, 0.0f, 2.0f);
	fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
	if (!isMute_) {
		pSourceVoice_->SetVolume(fVolume_);
	} else {
		pSourceVoice_->SetVolume(0.0f);
	}
}

void SoundUnit::SoundSetVolume(float cVolume, float volume) {
	if (!isMute_) {
		if (pSourceVoice_) {
			if (volume == volume_ && cVolume_ == cVolume)return;
			volume_ = volume;
			cVolume_ = cVolume;
			iVolume_ = std::clamp(volume, 0.0f, 2.0f);
			fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
			pSourceVoice_->SetVolume(fVolume_);
		}
		if (!pSourceVoiceGroup.empty()) {
			for (auto ptr : pSourceVoiceGroup) {
				if (cVolume_ == cVolume)return;
				cVolume_ = cVolume;
				fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
				ptr->SetVolume(fVolume_);
			}
		}
	}
}

void SoundUnit::SoundSetMasterVolume(float cVolume) {
	if (!isMute_) {
		if (pSourceVoice_) {
			if (cVolume_ == cVolume)return;
			cVolume_ = cVolume;
			fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
			pSourceVoice_->SetVolume(fVolume_);
		}
		if (!pSourceVoiceGroup.empty()) {
			if (cVolume_ == cVolume)return;
			cVolume_ = cVolume;
			fVolume_ = std::clamp(iVolume_ * cVolume, 0.0f, 2.0f);
			for (auto ptr : pSourceVoiceGroup) {
				ptr->SetVolume(fVolume_);
			}
		}
	}
}

void SoundUnit::SoundPause() {
	if (pSourceVoice_) {
		pSourceVoice_->Stop();
		isPause_ = true;
		if (voiceCallBack_) { voiceCallBack_->isBGMPlaying = false; }
	}
	if (!pSourceVoiceGroup.empty()) {
		for (auto ptr : pSourceVoiceGroup) {
			if (ptr) { ptr->Stop(); }
			isPause_ = true;
		}
	}
}

void SoundUnit::SoundContinue() {
	if (pSourceVoice_) {
		pSourceVoice_->Start();
		isPause_ = false;
		if (voiceCallBack_) { voiceCallBack_->isBGMPlaying = true; }
	}
	if (!pSourceVoiceGroup.empty()) {
		for (auto ptr : pSourceVoiceGroup) {
			if (ptr) { ptr->Start(); }
			isPause_ = false;
		}
	}
}

void SoundUnit::SoundStop() {
	if (pSourceVoice_) {
		pSourceVoice_->Stop();
		pSourceVoice_->FlushSourceBuffers();
		isPause_ = false;
		if (voiceCallBack_) { voiceCallBack_->isBGMPlaying = false; }
	}
	if (!pSourceVoiceGroup.empty()) {
		for (auto ptr : pSourceVoiceGroup) {
			if (ptr) { ptr->Stop(); }
			isPause_ = false;
		}
	}
}

bool SoundUnit::isPlaying() {
	if (soundType == Type::BGM && voiceCallBack_ != nullptr) {
		if (voiceCallBack_->isBGMPlaying && !isPause_) {
			return true;
		}
	}
	return false;
}


void SoundUnit::SoundSetMute(bool isMute) {
	if (isMute == isMute_)return;
	isMute_ = isMute;
	float fVolume = 0.0f;
	if (!isMute)fVolume = fVolume_;

	if (pSourceVoice_) {
		pSourceVoice_->SetVolume(fVolume);
	}
	if (!pSourceVoiceGroup.empty()) {
		for (auto ptr : pSourceVoiceGroup) {
			if (ptr) { ptr->SetVolume(fVolume); }
		}
	}
}
