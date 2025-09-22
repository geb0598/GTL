#include "pch.h"
#include "Editor/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"

void FViewportClient::InitializeLayout(const D3D11_VIEWPORT& InViewport)
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

	Viewports[1].SetViewportCameraType(EViewportCameraType::Ortho_Back);
	Viewports[2].SetViewportCameraType(EViewportCameraType::Ortho_Right);
	Viewports[3].SetViewportCameraType(EViewportCameraType::Ortho_Top);
	UpdateAllViewportCameras();
}

void FViewportClient::UpdateActiveViewport(const FVector& InMousePosition)
{
	ActiveViewport = nullptr;
	for (auto& Viewport : Viewports)
	{
		Viewport.bIsActive = false;
		const D3D11_VIEWPORT& ViewportInfo = Viewport.GetViewport();

		// 마우스가 현재 뷰포트의 사각 영역 내에 있는지 확인합니다.
		if (InMousePosition.X >= ViewportInfo.TopLeftX && InMousePosition.X <= (ViewportInfo.TopLeftX + ViewportInfo.Width) &&
			InMousePosition.Y >= ViewportInfo.TopLeftY && InMousePosition.Y <= (ViewportInfo.TopLeftY + ViewportInfo.Height))
		{
			Viewport.bIsActive = true;
			ActiveViewport = &Viewport;
		}

		// 다른 뷰포트들의 bIsActive 플래그를 false로 만들기 위해, 전부 순회함
	}
}

void FViewportClient::UpdateAllViewportCameras()
{
	for (FViewport& Viewport : Viewports) { Viewport.SnapCameraToView(FocusPoint); }
}

void FViewportClient::UpdateOrthoFocusPointByDelta(const FVector& InDelta)
{
	FocusPoint += InDelta;

	UpdateAllViewportCameras();
}
