#pragma once
#include "Editor/Public/Viewport.h"

class URenderer;

class FViewportClient
{
public:
	FViewportClient() = default;
	~FViewportClient() = default;

	/**
	* @brief ViewportClient가 보유한 Viewport들의 정보를 갱신합니다.
	* 이 함수는 현재 2x2 기준으로 작성되어 있습니다.
	*/
	void InitializeLayout(const D3D11_VIEWPORT& InViewport);

	/**
	* @brief 현재 활성화된 FViewport를 갱신합니다.
	*/
	void UpdateActiveViewport(const FVector& InMousePosition);

	/**
	* @brief 현재 활성화된 뷰포트를 반환합니다.
	* @return 활성 뷰포트의 포인터. 없으면 nullptr.
	*/
	FViewport* GetActiveViewport() const { return ActiveViewport; }

	/**
	* @brief 활성 뷰포트의 카메라를 반환하는 헬퍼 함수.
	* @return 활성 카메라의 포인터. 없으면 nullptr.
	*/
	UCamera* GetActiveCamera() const { return ActiveViewport ? &ActiveViewport->Camera : nullptr; }

	TArray<FViewport>& GetViewports() { return Viewports; }

private:
	TArray<FViewport> Viewports = {};
	FViewport* ActiveViewport = nullptr;
};

