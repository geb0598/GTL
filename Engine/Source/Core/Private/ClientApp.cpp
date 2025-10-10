#include "pch.h"
#include "Core/Public/ClientApp.h"

#include "Editor/Public/EditorEngine.h"
#include "Core/Public/AppWindow.h"
#include "Manager/Input/Public/InputManager.h"
#include "Manager/Asset/Public/AssetManager.h"
#include "Manager/Time/Public/TimeManager.h"

#include "Manager/UI/Public/UIManager.h"
#include "Manager/Config/Public/ConfigManager.h"
#include "Render/Renderer/Public/Renderer.h"

#include "Render/UI/Window/Public/ConsoleWindow.h"
#include "Render/UI/Overlay/Public/StatOverlay.h"

#include "Core/Public/ScopeCycleCounter.h"
#include "Core/Public/PlatformTime.h"

#ifdef IS_OBJ_VIEWER
#include "Utility/Public/FileDialog.h"
#endif

FClientApp::FClientApp() = default;

FClientApp::~FClientApp() = default;
/**
 * @brief Client Main Runtime Function
 * App 초기화, Main Loop 실행을 통한 전체 Cycle
 *
 * @param InInstanceHandle Process Instance Handle
 * @param InCmdShow Window Display Method
 *
 *
 * @return Program Termination Code
 */
int FClientApp::Run(HINSTANCE InInstanceHandle, int InCmdShow)
{
	TIME_PROFILE(Run)
	// Memory Leak Detection & Report
#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetBreakAlloc(0);
#endif

	// Window Object Initialize
	Window = new FAppWindow(this);
	if (!Window->Init(InInstanceHandle, InCmdShow))
	{
		assert(!"Window Creation Failed");
		return 0;
	}

	// Create Console
	// #ifdef _DEBUG
	// 	Window->InitializeConsole();
	// #endif

	// Keyboard Accelerator Table Setting
	// AcceleratorTable = LoadAccelerators(InInstanceHandle, MAKEINTRESOURCE(IDC_CLIENT));

	// Initialize Base System
	int InitResult = InitializeSystem();
	if (InitResult != S_OK)
	{
		assert(!"Initialization Failed");
		return 0;
	}

	// Execute Main Loop
	MainLoop();

	// Termination Process
	ShutdownSystem();

	return static_cast<int>(MainMessage.wParam);
}

/**
 * @brief Initialize System For Game Execution
 */
int FClientApp::InitializeSystem() const
{
	// 현재 시간을 랜덤 시드로 설정
	srand(static_cast<unsigned int>(time(NULL)));

	// Initialize By Get Instance
	UTimeManager::GetInstance();
	UInputManager::GetInstance().Initialize(Window);

	auto& Renderer = URenderer::GetInstance();
	Renderer.Init(Window->GetWindowHandle());

	// StatOverlay Initialize
	auto& StatOverlay = UStatOverlay::GetInstance();
	StatOverlay.Initialize();

	// UIManager Initialize
	auto& UIManger = UUIManager::GetInstance();
	UIManger.Initialize(Window->GetWindowHandle());
	UUIWindowFactory::CreateDefaultUILayout();

	UAssetManager::GetInstance().Initialize();

	// GEngine 생성 및 초기화 (내부에서 Editor 생성 및 마지막 레벨 로드)
	GEngine = NewObject<UEditorEngine>();
	GEngine->Initialize();

	return S_OK;
}

/**
 * @brief Update System While Game Processing
 */
void FClientApp::TickSystem(float DeltaSeconds) const
{
	auto& InputManager = UInputManager::GetInstance();
	auto& UIManager = UUIManager::GetInstance();
	auto& Renderer = URenderer::GetInstance();

	if (GEngine)
	{
		// PIE 실행 중이 아닐 때 F5 키로 PIE 시작
		if (!GEngine->IsPIEActive() && InputManager.IsKeyPressed(EKeyInput::F5))
		{
			GEngine->StartPIE();
		}

		// PIE 실행 중일 때 ESC로 종료
		if (GEngine->IsPIEActive() && InputManager.IsKeyPressed(EKeyInput::Esc))
		{
			GEngine->EndPIE();
		}

		GEngine->Tick(DeltaSeconds);
	}

	InputManager.Tick(DeltaSeconds);
	UIManager.Tick(DeltaSeconds);
	Renderer.Tick(DeltaSeconds);
}

/**
 * @brief Execute Main Message Loop
 * 윈도우 메시지 처리 및 게임 시스템 업데이트를 담당
 */
void FClientApp::MainLoop()
{
#ifdef IS_OBJ_VIEWER
	if (!OpenObjFromFileDialog())
	{
		MessageBoxW(
			nullptr,
			L"파일을 열 수 없습니다",
			L"오류",
			MB_OK | MB_ICONERROR
		);
		std::exit(1);
	}
#endif

	while (true)
	{
		// Async Message Process
		if (PeekMessage(&MainMessage, nullptr, 0, 0, PM_REMOVE))
		{
			// Process Termination
			if (MainMessage.message == WM_QUIT)
			{
				break;
			}
			// Shortcut Key Processing
			if (!TranslateAccelerator(MainMessage.hwnd, AcceleratorTable, &MainMessage))
			{
				TranslateMessage(&MainMessage);
				DispatchMessage(&MainMessage);
			}
		}
		// Game System Update
		else
		{
			auto& TimeManager = UTimeManager::GetInstance();
			TimeManager.UpdateDeltaSeconds();
			TickSystem(TimeManager.GetDeltaSeconds());
		}
	}
}

/**
 * @brief 시스템 종료 처리
 * 모든 리소스를 안전하게 해제하고 매니저들을 정리합니다.
 */
void FClientApp::ShutdownSystem() const
{
	UStatOverlay::GetInstance().Release();
	URenderer::GetInstance().Release();
	UUIManager::GetInstance().Shutdown();

	// GEngine 종료 및 삭제
	if (GEngine)
	{
		GEngine->Shutdown();
		delete GEngine;
		GEngine = nullptr;
	}

	UAssetManager::GetInstance().Release();

	// Release되지 않은 UObject의 메모리 할당 해제
	// 추후 GC가 처리할 것
	UClass::Shutdown();

	delete Window;
}
