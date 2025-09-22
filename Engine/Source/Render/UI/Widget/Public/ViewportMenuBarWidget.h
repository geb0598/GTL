#pragma once
#include "Widget.h"

class FViewportClient;

class UViewportMenuBarWidget : public UWidget
{
public:
	UViewportMenuBarWidget() : UWidget("ViewportMenuBar Widget") {}
	virtual ~UViewportMenuBarWidget() override;

	void Initialize() override {}
	void Update() override {}
	void RenderWidget() override;

	void SetViewportClient(FViewportClient* InViewportClient) { ViewportClient = InViewportClient; }

private:
	void RenderCameraControls(UCamera& InCamera); // 특정 카메라의 제어 UI를 렌더링하는 헬퍼 함수

	FViewportClient* ViewportClient = nullptr; // 참조할 뷰포트 클라이언트 대상
};

