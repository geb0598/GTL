#pragma once
#include "Render/UI/Window/Public/UIWindow.h"

/**
 * @brief Editor의 전반적인 UI를 담당하는 Window
 * 스플리터 UI 렌더링, 기즈모 조작 등 Editor 관련 위젯을 포함합니다.
 */
class UEditorWindow : public UUIWindow
{
public:
	UEditorWindow();
	void Initialize() override;

	// UEditor 클래스 자체를 위젯처럼 다루기 위해 포인터를 저장합니다.
	// 추후 이 포인터를 통해 Editor의 기능을 호출할 수 있습니다.
};
