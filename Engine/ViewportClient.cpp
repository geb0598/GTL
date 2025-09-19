#include "pch.h"
#include "Render/Renderer/Public/ViewportClient.h"
#include "Render/Renderer/Public/Viewport.h"
#include "Render/Renderer/Public/Renderer.h"

void FViewportClient::Init(const int InBackgroundHeight, const int InBackgroundWidth)
{
	BackgroundHeight = InBackgroundHeight;
	BackgroundWidth = InBackgroundWidth;
	Viewports.resize(4);
	UpdateLayout();
}

void FViewportClient::UpdateLayout()
{
	const float HalfWidth = BackgroundWidth * 0.5f;
	const float HalfHeight = BackgroundHeight * 0.5f;

	Viewports[0].SetViewport({ 0.0f, 0.0f, HalfWidth, HalfHeight, 0.0f, 1.0f });
	Viewports[1].SetViewport({ HalfWidth, 0.0f, HalfWidth, HalfHeight, 0.0f, 1.0f });
	Viewports[2].SetViewport({ 0.0f, HalfHeight, HalfWidth, HalfHeight, 0.0f, 1.0f });
	Viewports[0].SetViewport({ HalfWidth, HalfHeight, HalfWidth, HalfHeight, 0.0f, 1.0f });
}

void FViewportClient::Render(URenderer& Renderer, ID3D11DeviceContext* DeviceContext, ID3D11DepthStencilView* DepthStencilView)
{
	const int Size = Viewports.size();
	for (int Index = 0; Index < Size; ++Index)
	{
		const FViewport& Viewport = Viewports[Index];
		Viewport.Apply(DeviceContext);

		// 옵션: DepthStencilView를 사용한다면 뷰포트 초기화
		if (DepthStencilView) { Viewport.ClearDepth(DeviceContext, DepthStencilView); }

		Renderer.RenderLevel();
	}
}

void FViewportClient::Resize(const int InBackgroundHeight, const int InBackgroundWidth)
{
	BackgroundHeight = InBackgroundHeight;
	BackgroundWidth = InBackgroundWidth;
	UpdateLayout();
}
