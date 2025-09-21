#pragma once
#include "Render/UI/Widget/Public/Widget.h"

class SSplitter;

class USplitterDebugWidget : public UWidget
{
public:
	USplitterDebugWidget(const FString& InName) : UWidget(InName) {}
	virtual ~USplitterDebugWidget() override;

	virtual void Initialize() override {}
	virtual void Update() override {}
	virtual void RenderWidget() override;

	void SetSplitters(const SSplitter* InRoot, const SSplitter* InLeft, const SSplitter* InRight)
	{
		RootSplitter = InRoot;
		LeftSplitter = InLeft;
		RightSplitter = InRight;
	}

private:
	// 이름 없이 객체 생성 방지를 위해 추가
	USplitterDebugWidget() = delete;

	const SSplitter* RootSplitter = nullptr;
	const SSplitter* LeftSplitter = nullptr;
	const SSplitter* RightSplitter = nullptr;
};
