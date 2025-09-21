#include "pch.h"
#include "Render/UI/Window/Public/EditorWindow.h"
#include "Render/UI/Widget/Public/USplitterDebugWidget.h" 

/**
 * @brief Editor Window Constructor
 */
UEditorWindow::UEditorWindow()
{
	FUIWindowConfig Config;
	Config.WindowTitle = "Editor";
	Config.DefaultSize = ImVec2(Render::INIT_SCREEN_WIDTH, Render::INIT_SCREEN_HEIGHT);
	Config.DefaultPosition = ImVec2(0, 0);
	Config.DockDirection = EUIDockDirection::None; // 특정 위치에 고정하지 않음
	Config.Priority = 0; // 가장 뒤에 그려지도록 우선순위를 낮게 설정
	Config.bResizable = false;
	Config.bMovable = false;
	Config.bCollapsible = false;

	// 배경이 없고, 상호작용이 불가능한 투명 창으로 만들기 위한 플래그 설정
	Config.WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav |
		ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoInputs;

	SetConfig(Config);

	AddWidget(new USplitterDebugWidget("Splitter WIdget"));
}

/**
 * @brief Initializer
 */
void UEditorWindow::Initialize()
{
	UE_LOG("EditorWindow: Window가 성공적으로 생성되었습니다.");

	// 요청하신 대로, 기본 상태를 Hidden으로 설정합니다.
	SetWindowState(EUIWindowState::Visible);
}
