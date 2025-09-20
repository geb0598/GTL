#include "pch.h"
#include "Editor/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"

void FViewportClient::UpdateLayout(const D3D11_VIEWPORT& InViewport)
{
	if (Viewports.size() < 4) { Viewports.resize(4); }
	const float BaseX = InViewport.TopLeftX;
	const float BaseY = InViewport.TopLeftY;
	const float HalfW = InViewport.Width * 0.5f;
	const float HalfH = InViewport.Height * 0.5f;

	Viewports[0].SetViewport({ BaseX + 0.0f,     BaseY + 0.0f,     HalfW, HalfH, 0.0f, 1.0f });
	Viewports[1].SetViewport({ BaseX + HalfW,    BaseY + 0.0f,     HalfW, HalfH, 0.0f, 1.0f });
	Viewports[2].SetViewport({ BaseX + 0.0f,     BaseY + HalfH,    HalfW, HalfH, 0.0f, 1.0f });
	Viewports[3].SetViewport({ BaseX + HalfW,    BaseY + HalfH,    HalfW, HalfH, 0.0f, 1.0f });
}

void FViewportClient::Render()
{
	URenderer& Renderer = URenderer::GetInstance();
	ID3D11DeviceContext* DeviceContext = Renderer.GetDeviceContext();
	ID3D11DepthStencilView* DepthStencilView = Renderer.GetDeviceResources()->GetDepthStencilView();

	// 각 뷰포트의 값을 갱신합니다.
	UpdateLayout(Renderer.GetDeviceResources()->GetViewportInfo());

	// 화면에 순차적으로 렌더합니다.
	for (int Index = 0, Size = Viewports.size(); Index < Size; ++Index)
	{
		const FViewport& Viewport = Viewports[Index];
		Viewport.Apply(DeviceContext);

		// 옵션: DepthStencilView를 사용한다면 뷰포트 초기화를 합니다.
		if (DepthStencilView) { Viewport.ClearDepth(DeviceContext, DepthStencilView); }

		Renderer.RenderLevel();
	}
}
