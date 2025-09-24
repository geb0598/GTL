// FViewportClient.cpp (구현 파일 전체)
#include "pch.h"
#include "Editor/Public/ViewportClient.h"
#include "Render/Renderer/Public/Renderer.h"
#include "Manager/Config/Public/ConfigManager.h"

FViewportClient::~FViewportClient()
{
	// 소멸 시에도 최신 상태를 저장하도록 함수 호출
	UpdateCameraSettingsToConfig();
}

// brief 현재 모든 뷰포트의 카메라 상태를 UConfigManager에 동기화하는 새로운 함수
void FViewportClient::UpdateCameraSettingsToConfig()
{
	auto& ConfigManager = UConfigManager::GetInstance();

	for (int32 Index = 0; Index < Viewports.size(); ++Index)
	{
		FViewportCameraData Data;
		FViewport& Viewport = Viewports[Index];
		UCamera& Camera = Viewport.Camera;

		// 현재 뷰포트와 카메라의 상태를 FViewportCameraData 구조체에 담습니다.
		Data.ViewportCameraType = Viewport.GetViewportCameraType();
		Data.Location = Camera.GetLocation();
		Data.Rotation = Camera.GetRotation();
		Data.FovY = Camera.GetFovY();
		Data.NearClip = Camera.GetNearZ();
		Data.FarClip = Camera.GetFarZ();
		Data.OrthoWidth = Camera.GetOrthoWidth();
		Data.FocusLocation = this->FocusPoint;

		// 완성된 데이터를 ConfigManager에 전달합니다.
		ConfigManager.SetViewportCameraData(Index, Data);
	}
}

void FViewportClient::InitializeLayout(const D3D11_VIEWPORT& InViewport)
{
	if (Viewports.size() < 4) { Viewports.resize(4); }
	const float BaseX = InViewport.TopLeftX;
	const float BaseY = InViewport.TopLeftY;
	const float HalfW = InViewport.Width * 0.5f;
	const float HalfH = InViewport.Height * 0.5f;

	Viewports[0].SetViewport({ BaseX + 0.0f,      BaseY + 0.0f,      HalfW, HalfH, 0.0f, 1.0f });
	Viewports[1].SetViewport({ BaseX + HalfW,     BaseY + 0.0f,      HalfW, HalfH, 0.0f, 1.0f });
	Viewports[2].SetViewport({ BaseX + 0.0f,      BaseY + HalfH,     HalfW, HalfH, 0.0f, 1.0f });
	Viewports[3].SetViewport({ BaseX + HalfW,     BaseY + HalfH,     HalfW, HalfH, 0.0f, 1.0f });

	// 모든 뷰포트에 저장된 카메라 데이터를 일괄 적용합니다.
	ApplyAllCameraDataToViewports();

	// 모든 직교 카메라가 새로운 FocusPoint를 바라보도록 위치를 즉시 갱신합니다.
	UpdateAllViewportCameras();
}

// 모든 뷰포트 설정을 순회하며 적용하는 함수
void FViewportClient::ApplyAllCameraDataToViewports()
{
	auto& ConfigManager = UConfigManager::GetInstance();

	for (int32 Index = 0; Index < Viewports.size(); ++Index)
	{
		FViewport& TargetViewport = Viewports[Index];
		UCamera& TargetCamera = TargetViewport.Camera;

		// ConfigManager로부터 저장된 뷰포트 데이터를 가져옵니다.
		const FViewportCameraData& CamData = ConfigManager.GetViewportCameraData(Index);

		// 뷰포트의 카메라 타입을 먼저 설정합니다.
		TargetViewport.SetViewportCameraType(CamData.ViewportCameraType);

		if (CamData.ViewportCameraType == EViewportCameraType::Perspective)
		{
			TargetCamera.SetLocation(CamData.Location);
			TargetCamera.SetRotation(CamData.Rotation);
			TargetCamera.SetFarZ(CamData.FarClip);
			TargetCamera.SetNearZ(CamData.NearClip);
			TargetCamera.SetFovY(CamData.FovY);
		}
		else // Orthographic
		{
			TargetCamera.SetLocation(CamData.Location);
			TargetCamera.SetRotation(CamData.Rotation);
			TargetCamera.SetFarZ(CamData.FarClip);
			TargetCamera.SetNearZ(CamData.NearClip);
			TargetCamera.SetOrthoWidth(CamData.OrthoWidth);
			FocusPoint = CamData.FocusLocation;
		}
	}
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
	}
}

void FViewportClient::UpdateAllViewportCameras()
{
	for (FViewport& Viewport : Viewports)
	{
		Viewport.SnapCameraToView(FocusPoint);
	}
}

void FViewportClient::UpdateOrthoFocusPointByDelta(const FVector& InDelta)
{
	FocusPoint += InDelta;
	UpdateAllViewportCameras();
}

void FViewportClient::SetFocusPoint(const FVector& NewFocusPoint)
{
	FocusPoint = NewFocusPoint;
	UpdateAllViewportCameras();
}
