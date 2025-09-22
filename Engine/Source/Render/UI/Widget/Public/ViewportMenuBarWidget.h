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
	FViewportClient* ViewportClient = nullptr; // 참조할 뷰포트 클라이언트 대상
};

