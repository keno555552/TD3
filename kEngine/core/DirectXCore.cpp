#include "DirectXCore.h"

#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

#include "StringManage/ConvertString.h"

#pragma region Input

bool CheckXInputDeviceConnected() {
	XINPUT_STATE state;
	ZeroMemory(&state, sizeof(XINPUT_STATE));

	// XInputのコントローラーをチェック
	for (DWORD i = 0; i < XUSER_MAX_COUNT; i++) {
		if (XInputGetState(i, &state) == ERROR_SUCCESS) {
			bool boool = true;
			config::SetGamePadType(XINPUT);
			config::SetConnectGamePad(&boool);
			config::SetXInputIp(i);
			return true; // 見つけたtrue
		}
	}
	config::SetXInputIp(-1);
	bool boool = false;
	return false; // 見つからないならfalse
}

#pragma endregion

#pragma region WindowMake

#pragma endregion

#pragma region DirectXCommon

DirectXCore::DirectXCore() {
}

uint32_t DirectXCore::GetDescriptorSizeRTV() {
	return device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

uint32_t DirectXCore::GetDescriptorSizeDSV() {
	return device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}


D3D12_CPU_DESCRIPTOR_HANDLE DirectXCore::GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += (descriptorSize * index);
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectXCore::GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += (descriptorSize * index);
	return handleGPU;
}

bool DirectXCore::isDirectInputCreated() {
	if (directInput != nullptr) { return true; }
	return false;
}


void DirectXCore::SetDirectXInputKeyBoard() {
	SetDirectXInput(Keyboard, keyBoardDevice);
	bool boool = true;
	config::SetConnectKeyboard(&boool);
}

void DirectXCore::ReleaseDirectXInputKeyBoard() {
	if (keyBoardDevice) {
		keyBoardDevice->Unacquire();
		keyBoardDevice->Release();
		keyBoardDevice = nullptr;
		bool boool = false;
		config::SetConnectKeyboard(&boool);
	}
}

void DirectXCore::SetDirectXInputMouse() {
	SetDirectXInput(Mouse, mouseDevice);
	bool boool = true;
	config::SetConnectMouse(&boool);
}

void DirectXCore::ReleaseDirectXInputMouse() {
	if (mouseDevice) {
		mouseDevice->Unacquire();
		mouseDevice->Release();
		mouseDevice = nullptr;
		bool boool = false;
		config::SetConnectMouse(&boool);
	}
}

void DirectXCore::SetDirectXInputGamepad() {
	SetDirectXInput(GamePad, gamepadDevice);
	bool boool = true;
	config::SetConnectGamePad(&boool);
}

void DirectXCore::ReleaseDirectXInputGamepad() {
	if (gamepadDevice) {
		gamepadDevice->Unacquire();
		gamepadDevice->Release();
		gamepadDevice = nullptr;
		bool boool = false;
		config::SetConnectGamePad(&boool);
	}
}


ID3D12Device* DirectXCore::CreateDevice(IDXGIAdapter4* adapter) {
	ID3D12Device* device = nullptr;
	// 機能レベルとログ出力用の文字列
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0
	};
	const char* featureLevelStrings[] = { "12.2", "12.1", "12.0" };
	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターでデバイスを生成
		HRESULT hr = D3D12CreateDevice(adapter, featureLevels[i], IID_PPV_ARGS(&device));
		// 指定した機能レベルでデバイスが生成できたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログ出力を行ってループを抜ける
			Logger::Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
	}
	// デバイスの生成がうまくいかなかったので起動できない
	assert(device != nullptr);
	Logger::Log("Complete create D3D12Device!!!\n"); // 初期化完了のログをだす

#ifdef _DEBUG
	ID3D12InfoQueue* infoQueue = nullptr;
	if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// ヤバイエラー時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバッグレイヤーとD3D12デバッグレイヤーの相互作用バグによるエラーメッセージ
			// https://stackoverflow.com/questions/6980245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_INFO_QUEUE_FILTER filter{};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);

		// 解除
		infoQueue->Release();
	}
#endif
	return device;
}

void DirectXCore::InitializeGameInput() {
}

//void DirectXBase::InitializeGameInput() {
//	HRESULT GetCurrentReading(
//		GameInputKind,
//		);
//	//assert(SUCCEEDED(result));
//}


void DirectXCore::InitializeDirectXInput() {
	HRESULT result = DirectInput8Create(
		winAPI_->GetHINSTANCE(),
		DIRECTINPUT_VERSION,
		IID_IDirectInput8,
		(void**)&directInput,
		nullptr);
	assert(SUCCEEDED(result));
}

void DirectXCore::SetDirectXInput(InputType type, Microsoft::WRL::ComPtr<IDirectInputDevice8>& drive) {
	HRESULT result;
	bool boool = false;
	bool skip = false;
	switch (type) {
	case Keyboard:
		/// キーボードデバイスの生成
		result = directInput->CreateDevice(GUID_SysKeyboard, &drive, NULL);
		if (SUCCEEDED(result)) {
			boool = true;
			config::SetConnectKeyboard(&boool);
		} else {
			config::SetConnectKeyboard(&boool);
			skip = true;
			break;
		}

		/// 入力データ形式のセット
		result = drive->SetDataFormat(&c_dfDIKeyboard);
		assert(SUCCEEDED(result));

		break;
	case Mouse:
		/// マウスデバイスの生成
		result = directInput->CreateDevice(GUID_SysMouse, &drive, NULL);
		if (SUCCEEDED(result)) {
			boool = true;
			config::SetConnectMouse(&boool);
		} else {
			config::SetConnectMouse(&boool);
			skip = true;
			break;
		}

		/// 入力データ形式のセット
		result = drive->SetDataFormat(&c_dfDIMouse);
		assert(SUCCEEDED(result));

		break;
	case GamePad:
		/// マウスデバイスの生成
		result = directInput->CreateDevice(GUID_Joystick, &drive, NULL);
		if (SUCCEEDED(result)) {
			boool = true;
			config::SetConnectGamePad(&boool);
			config::SetGamePadType(DIRECTINPUT);
		} else {
			config::SetConnectGamePad(&boool);
			config::SetGamePadType(NODEVICE);
			skip = true;
			break;
		}

		/// 入力データ形式のセット
		result = drive->SetDataFormat(&c_dfDIJoystick);
		assert(SUCCEEDED(result));
	}

	if (!skip) {
		/// 排他制御レベルのセット
		result = drive->SetCooperativeLevel(
			winAPI_->GetHWND(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE | DISCL_NOWINKEY);
		assert(SUCCEEDED(result));
	}
}

void DirectXCore::SetXInput() {
	CheckXInputDeviceConnected();
}


ID3D12CommandQueue* DirectXCore::CreateCommandQueue() {
	// コマンドキューを生成する
	ID3D12CommandQueue* commandQueue = nullptr;
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc{};
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	HRESULT hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	// コマンドキューの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
	return commandQueue;
}

ID3D12CommandAllocator* DirectXCore::CreateCommandAllocator() {
	// コマンドアロケータを生成する
	ID3D12CommandAllocator* commandAllocator = nullptr;
	HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	// コマンドアロケータの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	return commandAllocator;
}

ID3D12GraphicsCommandList* DirectXCore::CreateCommandList(ID3D12CommandAllocator* commandAllocator) {
	// コマンドリストを生成する
	ID3D12GraphicsCommandList* commandList = nullptr;
	HRESULT hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList));
	// コマンドリストの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));

	return commandList;
}

IDXGISwapChain4* DirectXCore::CreateSwapChain(IDXGIFactory7* dxgiFactory, ID3D12CommandQueue* commandQueue, HWND hwnd, UINT kClientWidth, UINT kClientHeight) {
	// スワップチェーンを生成する
	IDXGISwapChain4* swapChain = nullptr;
	swapChainDesc = {};
	swapChainDesc.Width = kClientWidth;    // 画面の幅。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Height = kClientHeight;  // 画面の高さ。ウィンドウのクライアント領域を同じものにしておく
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 色の形式
	swapChainDesc.SampleDesc.Count = 1;    // マルチサンプルしない
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;  // 描画のターゲットとして利用する
	swapChainDesc.BufferCount = 2;         // ダブルバッファ
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;  // モニタにうつしたら、中身を破棄
	// コマンドキュー、ウィンドウのハンドル、設定を渡して生成する
	HRESULT hr = dxgiFactory->CreateSwapChainForHwnd(commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, reinterpret_cast<IDXGISwapChain1**>(&swapChain));
	assert(SUCCEEDED(hr));

	return swapChain;
}

ID3D12DescriptorHeap* DirectXCore::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	ID3D12DescriptorHeap* descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	if (shaderVisible == true) { descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; } else { descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; }
	//descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

void DirectXCore::CreateRenderTargetViews(IDXGISwapChain4* swapChain, ID3D12DescriptorHeap* rtvDescriptorHeap) {
	// SwapChainからResourceを引っ張ってくる
	HRESULT hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	// うまく取得できなければ起動できない
	assert(SUCCEEDED(hr));
	hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	assert(SUCCEEDED(hr));
	////////////////////////////////////////////////////////////////////////////////////////////////////
	// RTVの設定
	rtvDesc = {};
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2dテクスチャとして書き込む

	// ディスクリプタの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	// RTVを2つ作るのでディスクリプタを2つ用意<<<<<<<<Classで使わない
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];

	// まず1つ目を作る。2つ目は最初のとこから作る。作る場所をこちらで指定してあげる必要がある
	rtvHandles[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandles[0]);

	// 2つ目のディスクリプタハンドルを得る（自力で）
	rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 2つ目を作る
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandles[1]);
}

void DirectXCore::CreateFence() {
	///初期値0でFenceを作る
	//ID3D12Fence* fence = nullptr;
	//uint64_t fenceValue = 0;
	HRESULT hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	assert(SUCCEEDED(hr));

	///FenceのSignalを待つためのイベントを作成する
	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent != nullptr);
}

void DirectXCore::InitializeDrive(const char* kClientTitle, int kClientWidth, int kClientHeight) {
#ifdef _DEBUG
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		// デバッグレイヤーを有効化する
		debugController->EnableDebugLayer();
		// さらにGPU側でもチェックを行うようにする
		debugController->SetEnableGPUBasedValidation(TRUE);
	}
#endif

	/// WindowAPI、windowを作る
	winAPI_ = std::make_unique<WinAPI>();
	winAPI_->Initialize(kClientTitle, kClientWidth, kClientHeight);

	/// DXGIファクトリーの生成
	// HRESULTはWindows系のエラーコードであり、
	// 関数が成功したかどうかをSUCCEEDEDマクロで判定できる
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	// 初期化の根本的な部分でエラーが出た場合はプログラムが間違っているか、どうにもできない場合が多いのでassertにしておく
	assert(SUCCEEDED(hr));


	// 使用するアダプタ用の変数。最初にnullptrを入れておく
	//IDXGIAdapter4* useAdapter = nullptr;
	// 良い順にアダプタを頼む
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i,
		DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) !=
		DXGI_ERROR_NOT_FOUND; ++i) {
		// アダプターの情報を取得する
		DXGI_ADAPTER_DESC3 adapterDesc{};
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr)); // 取得できないのは一大事
		// ソフトウェアアダプタでなければ採用！
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力。wstringの方なので注意
			//std::wstring description(adapterDesc.Description);
			Logger::Log(std::format(L"Use Adapter:{}\n", adapterDesc.Description));
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}
	// 適切なアダプタが見つからなかったので起動できない
	assert(useAdapter != nullptr);

	device = CreateDevice(useAdapter.Get());
	commandQueue = CreateCommandQueue();
	commandAllocator = CreateCommandAllocator();
	commandList = CreateCommandList(commandAllocator.Get());

	SwapChain = CreateSwapChain(dxgiFactory.Get(), commandQueue.Get(), winAPI_->GetHWND(), kClientWidth, kClientHeight);
	//DescriptorHeap = CreateDescriptorHeap(device);
	rtvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	dsvDescriptorHeap = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);

	CreateRenderTargetViews(SwapChain.Get(), rtvDescriptorHeap.Get());

	CreateFence();

	// DirectX入力装置の初期化
	InitializeDirectXInput();
	SetDirectXInput(Keyboard, keyBoardDevice);
	SetDirectXInput(Mouse, mouseDevice);
	// SetDirectXInput(GamePad, gamepadDevice); //XInput優先

}

bool DirectXCore::ProcessMessage() {
	return winAPI_->ProcessMessage();
}

void DirectXCore::Finalize() {

	// --- GPU objects (must release before device) ---
	commandList.Reset();
	commandAllocator.Reset();
	commandQueue.Reset();

	SwapChain.Reset();
	swapChainResources[0].Reset();
	swapChainResources[1].Reset();

	rtvDescriptorHeap.Reset();
	dsvDescriptorHeap.Reset();


	// --- DirectInput devices ---
	if (keyBoardDevice)keyBoardDevice->Unacquire();
	keyBoardDevice.Reset();

	if (mouseDevice)mouseDevice->Unacquire();
	mouseDevice.Reset();

	if (gamepadDevice)gamepadDevice->Unacquire();
	gamepadDevice.Reset();

	directInput.Reset();

	// --- Fence & event ---
	fence.Reset();

	CloseHandle(fenceEvent);
	fenceValue = 0;
	fenceEvent = nullptr;

	// --- Device & DXGI objects (must be last) ---

	device.Reset();
	useAdapter.Reset();
	dxgiFactory.Reset();

#ifdef _DEBUG

	Logger::Log("device = %p\n", device.Get());
	Logger::Log("commandQueue = %p\n", commandQueue.Get());
	Logger::Log("commandAllocator = %p\n", commandAllocator.Get());
	Logger::Log("commandList = %p\n", commandList.Get());
	Logger::Log("swapChain = %p\n", SwapChain.Get());
	Logger::Log("rtvDescriptorHeap = %p\n", rtvDescriptorHeap.Get());
	Logger::Log("dsvDescriptorHeap = %p\n", dsvDescriptorHeap.Get());
	Logger::Log("fence = %p\n", fence.Get());
	Logger::Log("fenceEvent = %p\n", fenceEvent);

	IDXGIDebug1* debug;
	//if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		//debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);///また未解放要素がある
		//debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		//debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);

		debug->Release();
	}
	/// デバッグレイヤーの解放
	if (debugController)		debugController.Reset();

	Logger::Log("139\n");
#endif

	/// WindowAPIを消す
	winAPI_->Finalize();
	winAPI_.reset();
}
#pragma endregion

//ULONG count = commandList->AddRef();
//commandList->Release();